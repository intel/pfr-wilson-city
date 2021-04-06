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
 * @file t0_watchdog_handler.h
 * @brief Responsible for monitoring timed boot progress and handling timeouts of watchdog timers.
 */

#ifndef WHITLEY_INC_T0_WATCHDOG_HANDLER_H_
#define WHITLEY_INC_T0_WATCHDOG_HANDLER_H_

// Always include pfr_sys.h first
#include "pfr_sys.h"

#include "firmware_recovery.h"
#include "gen_gpo_controls.h"
#include "gen_gpi_signals.h"
#include "mailbox_utils.h"
#include "platform_log.h"
#include "spi_ctrl_utils.h"
#include "transition.h"
#include "utils.h"
#include "watchdog_timers.h"

// Forward declarations
static void bmc_watchdog_timer_handler();
static void me_watchdog_timer_handler();
static void acm_bios_watchdog_timer_handler();

/**
 * @brief Responsible for monitoring the state of all the SW watchdog timers.
 *
 * @see bmc_watchdog_timer_handler
 * @see me_watchdog_timer_handler
 * @see acm_bios_watchdog_timer_handler
 */
static void watchdog_routine()
{
    bmc_watchdog_timer_handler();
    me_watchdog_timer_handler();
    acm_bios_watchdog_timer_handler();
}

/**
 * @brief Trigger a WDT recovery on PCH SPI flash, when Nios FW observes a ME/ACM/BIOS watchdog timer timeout
 * or ME/ACM/BIOS internal authentication failure.
 *
 * If the current recovery level is still within 1-3 (inclusive), then proceed to T-1 mode and perform WDT recovery.
 * Otherwise, simply disable the appropriate watchdog timer and let the firmware hang.
 *
 * @param last_recovery_reason The specific recovery reason for the WDT recovery
 * @param panic_reason The specific panic reason for the transition to T-1 mode
 *
 * @see perform_wdt_recovery
 */
static void trigger_pch_wdt_recovery(
        STATUS_LAST_RECOVERY_ENUM last_recovery_reason, STATUS_LAST_PANIC_ENUM panic_reason)
{
    if (get_fw_recovery_level(SPI_FLASH_PCH) & SPI_REGION_PROTECT_MASK_RECOVER_BITS)
    {
        // Log the event
        log_wdt_recovery(last_recovery_reason, panic_reason);

        // Overwrite the pch_flash_state to indicate a watchdog recovery is needed.
        // Other states can be erased: recovery updates are abandoned and Nios will redo authentication. 
        pch_flash_state = SPI_FLASH_STATE_REQUIRE_WDT_RECOVERY_MASK;

        perform_platform_reset();
    }
    else
    {
        wdt_enable_status &= ~WDT_ENABLE_PCH_TIMERS_MASK;
    }
}

/**
 * @brief Trigger a WDT recovery on BMC SPI flash, when Nios FW observes a BMC watchdog timer timeout.
 *
 * If the current recovery level is still within 1-3 (inclusive), then do a platform reset and perform WDT recovery.
 * Otherwise, simply disable the BMC watchdog timer and let the firmware hang.
 *
 * @param last_recovery_reason The specific recovery reason for the WDT recovery
 * @param panic_reason The specific panic reason for the transition to T-1 mode
 *
 * @see perform_wdt_recovery
 */
static void trigger_bmc_wdt_recovery(
        STATUS_LAST_RECOVERY_ENUM last_recovery_reason, STATUS_LAST_PANIC_ENUM panic_reason)
{
    if (get_fw_recovery_level(SPI_FLASH_BMC) & SPI_REGION_PROTECT_MASK_RECOVER_BITS)
    {
        // Log the event
        log_wdt_recovery(last_recovery_reason, panic_reason);

        // Overwrite the bmc_flash_state to indicate a watchdog recovery is needed.
        // Other states can be erased: recovery updates are abandoned and Nios will redo authentication. 
        bmc_flash_state = SPI_FLASH_STATE_REQUIRE_WDT_RECOVERY_MASK;

        perform_platform_reset();
    }
    else
    {
        wdt_enable_status &= ~WDT_ENABLE_BMC_TIMER_MASK;
    }
}

/**
 * @brief Monitor the boot progress for ME firmware with the ME GPIOs.
 *
 * If the ME watchdog timer expires, indicate in the PCH spi flash state global
 * variable that WDT recovery is required for PCH flash. Then perform a platform
 * reset. The WDT recovery will be done in the T-1 stage of that reset.
 *
 * GPIO 1 (FM_ME_PFR_1): 1 means ME Authentication Failed
 * GPIO 2 (FM_ME_PFR_2): 1 means ME Boot Done
 *
 * If ME GPIO 2 is == 1 and ME GPIO 1 is == 0, then ME firmware has completed boot
 * successfully. ME watchdog timer becomes inactive.
 *
 * @see watchdog_timeout_handler
 */
static void me_watchdog_timer_handler()
{
    // Skip if this timer is disabled or reset.
    if ((wdt_enable_status & WDT_ENABLE_ME_TIMER_MASK) && IORD(WDT_ME_TIMER_ADDR, 0))
    {
        // If ME timer is enabled
        if (is_wd_timer_expired(WDT_ME_TIMER_ADDR))
        {
            trigger_pch_wdt_recovery(LAST_RECOVERY_ME_LAUNCH_FAIL, LAST_PANIC_ME_WDT_EXPIRED);
        }
        else
        {
            // GPIO 1: ME Authentication Failed
            // GPIO 2: ME Boot Done
            if (check_bit(U_GPI_1_ADDR, GPI_1_FM_ME_PFR_2) &&
                    !check_bit(U_GPI_1_ADDR, GPI_1_FM_ME_PFR_1))
            {
                // When ME firmware booted and authentication pass, stop the ME timer
                wdt_boot_status |= WDT_ME_BOOT_DONE_MASK;

                // Clear the ME timer
                IOWR(WDT_ME_TIMER_ADDR, 0, 0);

                // Clear the fw recovery level upon successful boot of BIOS and ME
                if (wdt_boot_status & WDT_OBB_BOOT_DONE_MASK)
                {
                    reset_fw_recovery_level(SPI_FLASH_PCH);
                }

                // Log boot progress
                log_t0_timed_boot_complete_if_ready(PLATFORM_STATE_T0_ME_BOOTED);
            }
        }
    }
}

/**
 * @brief Things for Nios to do after BMC has completed boot.
 *
 * SPI filter has a bit that is set if BMC accesses its uboot code. This is
 * used to detect BMC warm reset. But this bit is set during normal BMC boot. Hence,
 * this bit has to be cleared after BMC completing boot.
 *
 * SMBus filtering is enabled only after BMC has completed boot. During boot flow,
 * BMC configures some platform component (e.g. digital VR). Enabling SMBus filter would block that.
 */
static void bmc_boot_complete_actions()
{
    // BMC has completed boot
    wdt_boot_status |= WDT_BMC_BOOT_DONE_MASK;

    // Clear the BMC timer
    IOWR(WDT_BMC_TIMER_ADDR, 0, 0);

    // Clear the fw recovery level upon successful boot
    reset_fw_recovery_level(SPI_FLASH_BMC);

    // Enable SMBus filtering
    set_filter_disable_all(0);

    // Clear IBB detection. It was set during BMC boot up process.
    set_bit(U_GPO_1_ADDR, GPO_1_BMC_SPI_CLEAR_IBB_DETECTED);

    // Log boot progress
    log_t0_timed_boot_complete_if_ready(PLATFORM_STATE_T0_BMC_BOOTED);
}

/**
 * @brief Monitor the boot progress for BMC firmware with the BMC checkpoint message.
 *
 * If the BMC watchdog timer expires, indicate in the BMC spi flash state global
 * variable that WDT recovery is required for BMC flash. Then perform a BMC only
 * reset. The WDT recovery will be done in the T-1 stage of that reset.
 *
 * BMC boot flow contains two parts: Bootloader and kernel.
 * Currently, the bootloader sends the BLOCK_START checkpoint message and the kernel sends
 * BLOCK_COMPLETE checkpoint message. Hence, only 1 timeout value is used to track BMC
 * boot progress.
 *
 * SMBus filtering is enabled after BMC has completed boot. SMBus filtering rules were
 * stored during T-1 authentication flow.
 *
 * @see watchdog_timeout_handler
 */
static void bmc_watchdog_timer_handler()
{
    // Skip if this timer is disabled or reset.
    if ((wdt_enable_status & WDT_ENABLE_BMC_TIMER_MASK) && IORD(WDT_BMC_TIMER_ADDR, 0))
    {
        if (is_wd_timer_expired(WDT_BMC_TIMER_ADDR))
        {
            trigger_bmc_wdt_recovery(LAST_RECOVERY_BMC_LAUNCH_FAIL, LAST_PANIC_BMC_WDT_EXPIRED);
        }
        else
        {
            // Read checkpoint command
            alt_u32 chkpt_cmd = read_from_mailbox(MB_BMC_CHECKPOINT);

            // Clear checkpoint messages after read
            if (chkpt_cmd)
            {
                write_to_mailbox(MB_BMC_CHECKPOINT, 0);
            }

            if (chkpt_cmd == MB_CHKPT_START)
            {
                // Start the watchdog timer
                // This would simply restart the watchdog timer if it's already started
                start_timer(WDT_BMC_TIMER_ADDR, WD_BMC_TIMEOUT_VAL);
            }
            else if (chkpt_cmd == MB_CHKPT_PAUSE)
            {
                pause_timer(WDT_BMC_TIMER_ADDR);
            }
            else if (chkpt_cmd == MB_CHKPT_RESUME)
            {
                resume_timer(WDT_BMC_TIMER_ADDR);
            }
            else if (chkpt_cmd == MB_CHKPT_COMPLETE)
            {
                bmc_boot_complete_actions();
            }
        }
    }
}

/**
 * @brief Monitor the boot progress of ACM and BIOS firmware with the ACM and BIOS checkpoint messages.
 *
 * A single watchdog timer is used for tracking both ACM and BIOS boot progress, because of the ACM ->
 * IBB -> OBB multi-level secure boot flow.
 *
 * Nios arms the ACM watchdog timer upon PLTRST# de-assertion (i.e. on a rising edge). ACM WDT is turned off
 * when BIOS IBB starts to boot (i.e. Nios receives START checkpoint in BIOS checkpoint register). Nios does
 * not process START/DONE message from ACM checkpoint register. ACM may not sends these checkpoint messages in some
 * BootGuard (BtG) profiles. When ACM WDT is actively counting down, ACM checkpoint register is monitored for
 * PAUSE/RESUME/AUTH_FAIL checkpoint messages.
 *
 * Once BIOS IBB sends START checkpoint message, Nios turns off ACM WDT and turns on BIOS IBB WDT. Then, when
 * IBB completes boot, Nios turns off BIOS IBB WDT and turns on BIOS OBB WDT. BIOS boot is considered complete after
 * Nios receives another boot DONE checkpoint message. Nios supports START/DONE/PAUSE/RESUME/AUTH_FAIL checkpoint messages
 * from BIOS.
 *
 * If the ACM BIOS watchdog timer expires, transition the platform to T-1 mode and perform WDT recovery on PCH SPI flash.
 *
 * @see platform_reset_handler
 * @see watchdog_timeout_handler
 * @see trigger_pch_wdt_recovery
 */
static void acm_bios_watchdog_timer_handler()
{
    // Skip if this timer is disabled or reset.
    if ((wdt_enable_status & WDT_ENABLE_ACM_BIOS_TIMER_MASK) && IORD(WDT_ACM_BIOS_TIMER_ADDR, 0))
    {
        STATUS_LAST_PANIC_ENUM panic_reason = LAST_PANIC_DEFAULT;
        STATUS_LAST_RECOVERY_ENUM recovery_reason = LAST_RECOVERY_ACM_LAUNCH_FAIL;

        if (is_wd_timer_expired(WDT_ACM_BIOS_TIMER_ADDR))
        {
            panic_reason = LAST_PANIC_ACM_BIOS_WDT_EXPIRED;
        }

        // Read BIOS checkpoint
        alt_u32 chkpt_cmd = read_from_mailbox(MB_BIOS_CHECKPOINT);
        if (chkpt_cmd)
        {
            // Clear checkpoint messages after read
            write_to_mailbox(MB_BIOS_CHECKPOINT, 0);
        }

        // Three-stage boot: ACM -> IBB -> OBB
        if (wdt_boot_status & WDT_IBB_BOOT_DONE_MASK)
        {
            // Both ACM and IBB have booted. Tracking OBB boot progress now
            if (chkpt_cmd == MB_CHKPT_START)
            {
                // Restart OBB timer (initially started after IBB booted)
                start_timer(WDT_ACM_BIOS_TIMER_ADDR, WD_OBB_TIMEOUT_VAL);
            }
            else if (chkpt_cmd == MB_CHKPT_COMPLETE)
            {
                // BIOS OBB boot has completed
                wdt_boot_status |= WDT_OBB_BOOT_DONE_MASK;

                // Clear the ACM/BIOS timer
                IOWR(WDT_ACM_BIOS_TIMER_ADDR, 0, 0);

                // Clear the fw recovery level upon successful boot of BIOS and ME
                if (wdt_boot_status & WDT_ME_BOOT_DONE_MASK)
                {
                    reset_fw_recovery_level(SPI_FLASH_PCH);
                }

                // Log boot progress
                log_t0_timed_boot_complete_if_ready(PLATFORM_STATE_T0_BIOS_BOOTED);
            }

            // If Nios was to recover PCH FW, the reason would be a boot failure in Bios OBB.
            recovery_reason = LAST_RECOVERY_OBB_LAUNCH_FAIL;
        }
        else
        {
            // Booting ACM or BIOS IBB
            // Check BIOS checkpoint messages first
            if (chkpt_cmd == MB_CHKPT_START)
            {
                // ACM has completed booting
                wdt_boot_status |= WDT_ACM_BOOT_DONE_MASK;

                // Log boot progress
                log_platform_state(PLATFORM_STATE_T0_ACM_BOOTED);

                // Start the BIOS IBB timer.
                start_timer(WDT_ACM_BIOS_TIMER_ADDR, WD_IBB_TIMEOUT_VAL);
            }
            else if (chkpt_cmd == MB_CHKPT_COMPLETE)
            {
                // BIOS IBB boot has completed
                wdt_boot_status |= WDT_IBB_BOOT_DONE_MASK;
                // Start the BIOS OBB timer.
                start_timer(WDT_ACM_BIOS_TIMER_ADDR, WD_OBB_TIMEOUT_VAL);
            }

            if (wdt_boot_status & WDT_ACM_BOOT_DONE_MASK)
            {
                // If Nios was to recover PCH FW, the reason would be a boot failure in Bios OBB.
                recovery_reason = LAST_RECOVERY_IBB_LAUNCH_FAIL;
            }
            else
            {
                // If Nios was to recover PCH FW, the reason would be a boot failure in ACM.
                recovery_reason = LAST_RECOVERY_ACM_LAUNCH_FAIL;

                // Read ACM checkpoint
                // Nios only read ACM checkpoint when ACM is booting.
                chkpt_cmd = read_from_mailbox(MB_ACM_CHECKPOINT);
                if (chkpt_cmd)
                {
                    // Clear checkpoint messages after read
                    write_to_mailbox(MB_ACM_CHECKPOINT, 0);
                }
            }
        }

        // Process PAUSE/RESUME/AUTH_FAIL checkpoint message from ACM/BIOS
        if (chkpt_cmd == MB_CHKPT_PAUSE)
        {
            pause_timer(WDT_ACM_BIOS_TIMER_ADDR);
        }
        else if (chkpt_cmd == MB_CHKPT_RESUME)
        {
            resume_timer(WDT_ACM_BIOS_TIMER_ADDR);
        }
        else if (chkpt_cmd == MB_CHKPT_AUTH_FAIL)
        {
            panic_reason = LAST_PANIC_ACM_BIOS_AUTH_FAILED;

            // When there's ACM BtG authentication failure, ACM will pass that information to ME firmware.
            // Wait for ME firmware to clean up and shutdown system.
            sleep_20ms(WD_ACM_AUTH_FAILURE_WAIT_TIME_VAL);
        }

        // If WDT has expired or AUTH_FAIL checkpoint msg is received, perform WDT recovery on PCH flash
        if (panic_reason != LAST_PANIC_DEFAULT)
        {
            trigger_pch_wdt_recovery(recovery_reason, panic_reason);
        }
    }
}

#endif /* WHITLEY_INC_T0_WATCHDOG_HANDLER_H_ */
