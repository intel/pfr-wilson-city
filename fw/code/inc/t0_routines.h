/******************************************************************************
 * Copyright (c) 2021 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 ******************************************************************************/

/**
 * @file t0_routines.h
 * @brief functions that are executed during normal mode (T0) of operation.
 */

#ifndef WHITLEY_INC_T0_ROUTINES_H_
#define WHITLEY_INC_T0_ROUTINES_H_

// Always include pfr_sys.h first
#include "pfr_sys.h"

#include "gen_gpo_controls.h"
#include "gen_gpi_signals.h"
#include "platform_log.h"
#include "t0_watchdog_handler.h"
#include "t0_provisioning.h"
#include "t0_update.h"
#include "transition.h"
#include "utils.h"

/**
 * @brief Perform a BMC-only reset to perform authentication on BMC firmware, when
 * a BMC reset has been detected by the SPI control IP block.
 *
 * A BMC reading from its IBB is interpreted by the CPLD RoT as BMC reset
 * and triggers an re-authentication procedure by the CPLD RoT.
 *
 * This monitoring routine is started after BMC has completed boot.
 */
static void bmc_reset_handler()
{
    // If BMC is still booting, exit
    // If a BMC reset has not been detected, exit
    if ((wdt_boot_status & WDT_BMC_BOOT_DONE_MASK)
            && check_bit(U_GPI_1_ADDR, GPI_1_BMC_SPI_IBB_ACCESS_DETECTED))
    {
        // Clear IBB detection since we are now in the process of reacting to the reset.
        set_bit(U_GPO_1_ADDR, GPO_1_BMC_SPI_CLEAR_IBB_DETECTED);

        // Perform BMC only reset to re-authenticate its flash
        log_panic(LAST_PANIC_BMC_RESET_DETECTED);
        perform_bmc_only_reset();
    }
}

/**
 * @brief Arm the ACM/BIOS watchdog timer when Nios firmware detects a platform reset
 * through PLTRST# GPI signal.
 */
static void platform_reset_handler()
{
    // When there's a platform reset, re-arm the ACM/BIOS watchdog timer
    if (check_bit(U_GPI_1_ADDR, GPI_1_PLTRST_DETECTED_REARM_ACM_TIMER))
    {
        // Clear the PLTRST detection flag
        set_bit(U_GPO_1_ADDR, GPO_1_CLEAR_PLTRST_DETECT_FLAG);
        clear_bit(U_GPO_1_ADDR, GPO_1_CLEAR_PLTRST_DETECT_FLAG);

        /*
         * Conditions to arm PCH ACM/BIOS watchdog:
         * 1. The ACM/BIOS watchdog is currently inactive
         * 2. PCH has valid recovery image. Otherwise, when there's a timeout, there's no image to recover to.
         */
        if(!check_bit(WDT_ACM_BIOS_TIMER_ADDR, U_TIMER_BANK_TIMER_ACTIVE_BIT)
                && !check_spi_flash_state(SPI_FLASH_PCH, SPI_FLASH_STATE_RECOVERY_FAILED_AUTH_MASK))
        {
            // Clear ACM/BIOS checkpoints
            write_to_mailbox(MB_ACM_CHECKPOINT, 0);
            write_to_mailbox(MB_BIOS_CHECKPOINT, 0);

            // Clear previous boot done status
            wdt_boot_status &= ~WDT_ACM_BIOS_BOOT_DONE_MASK;

#ifdef PLATFORM_MULTI_NODES_ENABLED
            if (!check_bit(U_GPI_1_ADDR, GPI_1_HPFR_ACTIVE)
                    || check_bit(U_GPI_1_ADDR, GPI_1_LEGACY))
            {
                // In 2s system, always arm the ACM/BIOS watchdog timer
                // In 4s/8s system, only arm the ACM/BIOS watchdog timer on the legacy board

                // Start the ACM/BIOS watchdog timer
                start_timer(WDT_ACM_BIOS_TIMER_ADDR, WD_ACM_TIMEOUT_VAL);
            }
            else
            {
                // Disable ACM/BIOS watchdog timer
                wdt_enable_status &= ~WDT_ENABLE_ACM_BIOS_TIMER_MASK;
            }
#else
            // Start the ACM/BIOS watchdog timer
            start_timer(WDT_ACM_BIOS_TIMER_ADDR, WD_ACM_TIMEOUT_VAL);
#endif
        }

        // When detected a platform reset, process the deferred updates
        // Simply clear the update at reset bit. The mb_update_intent_handler function will pick up the update.
        clear_mailbox_register_bit(MB_PCH_UPDATE_INTENT, MB_UPDATE_INTENT_UPDATE_AT_RESET_BIT_POS);
        clear_mailbox_register_bit(MB_BMC_UPDATE_INTENT, MB_UPDATE_INTENT_UPDATE_AT_RESET_BIT_POS);
    }
}

/**
 * @brief Check to see if there was a panic event in the other PFR CPLDs.
 * Should be called in main polling loop and can transition the platform into T-1 mode.
 * This function is only executed when the hierarchical PFR feaure is supported.
 */
static void check_for_hpfr_panic_event()
{
#ifdef PLATFORM_MULTI_NODES_ENABLED
    // Check if we are using hierarchical PFR
    if (check_bit(U_GPI_1_ADDR, GPI_1_HPFR_ACTIVE))
    {
        if (!check_bit(U_GPI_1_ADDR, GPI_1_HPFR_IN))
        {
            perform_platform_reset();
        }
    }
#endif
}

/**
 * @brief Perform all T0 operations.
 *
 * These T0 operations include:
 * - Monitor mailbox UFM provisioning commands.
 * - Monitor boot progress of the critical-to-boot components (e.g. BMC) and
 * ensure a successful boot is achieved within the expected time period.
 * - Monitor mailbox update intent registers.
 * - Detect BMC reset and react to it.
 * - Perform post-update flows (e.g. bumping SVN after a successful active update and
 * trigger recovery update).
 *
 * If hierarchical PFR is supported, Nios firmware also checks whether other CPLDs
 * have any panic event.
 *
 * @see mb_ufm_provisioning_handler()
 * @see check_for_hpfr_panic_event()
 * @see watchdog_routine()
 * @see mb_update_intent_handler()
 * @see bmc_reset_handler()
 * @see post_update_routine()
 */
static void perform_t0_operations()
{
    while (1)
    {
        // Pet the HW watchdog
        reset_hw_watchdog();

        // Check UFM provisioning request
        mb_ufm_provisioning_handler();

        // Monitor HPFR event if HPFR is supported
        check_for_hpfr_panic_event();

        // Activities for provisioned system
        if (is_ufm_provisioned())
        {
            // Detect PLTRST
            platform_reset_handler();

            // Monitor BMC/ME/ACM/BIOS boot progress
            watchdog_routine();

            // Check updates
            mb_update_intent_handler();

            // Monitor BMC reset
            bmc_reset_handler();

            // Finish up any firmware/CPLD update in progress
            post_update_routine();
        }

#ifdef USE_SYSTEM_MOCK
        if (SYSTEM_MOCK::get()->should_exec_code_block(
                SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS))
        {
            break;
        }
        if (SYSTEM_MOCK::get()->should_exec_code_block(
                SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_TIMED_BOOT))
        {
            if (wdt_boot_status == WDT_ALL_BOOT_DONE_MASK)
                break;
        }
        if (SYSTEM_MOCK::get()->should_exec_code_block(
                SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS_END_AFTER_50_ITERS))
        {
            if (SYSTEM_MOCK::get()->get_code_block_counter() > 50)
            {
                break;
            }
            SYSTEM_MOCK::get()->incr_code_block_counter();
        }
#endif
    }
}

#endif /* WHITLEY_INC_T0_ROUTINES_H_ */
