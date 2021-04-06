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
 * @file transition.h
 * @brief Responsible for T0 / T-1 / T0 mode transition.
 */

#ifndef WHITLEY_INC_TRANSITION_H_
#define WHITLEY_INC_TRANSITION_H_

// Always include pfr_sys.h first
#include "pfr_sys.h"

#include "tmin1_routines.h"
#include "hierarchical_pfr.h"

/**
 * @brief Prepare the platform to enter T-1 mode.
 * 
 * Steps:
 * 1. Drive the SLP_SUS_N pin high prior to setting RSMRST# and DSWPWROK.
 * 2. Assert resets on BMC and PCH through SRST# and RSMRST# pin. 
 * 3. Take over the SPI flash devices controls through the SPI control block.
 * 4. Drive DSWPWROK to 0.
 * 
 * If hierarchical PFR is supported, Nios firmware also notifies other CPLDs that
 * it is entering T-1 mode.
 */
static void perform_entry_to_tmin1()
{
#ifdef USE_SYSTEM_MOCK
    SYSTEM_MOCK::get()->incr_t_minus_1_counter();
#endif

    // log that we are entering T-1 mode
    log_platform_state(PLATFORM_STATE_ENTER_TMIN1);

    // Notify other CPLDs if HPFR is supported
    perform_hpfr_entry_to_tmin1();

#ifndef PLATFORM_WILSON_CITY_FAB1
    // PFR should drive the new PFR_SLP_SUS_N pin high, during a T0 to T-1 transition.
    // It should drive that high before setting RSMRST# and DSWPWROK low.
    set_bit(U_GPO_1_ADDR, GPO_1_FM_PFR_SLP_SUS_N);
#endif

    // Assert reset on BMC and PCH
    clear_bit(U_GPO_1_ADDR, GPO_1_RST_SRST_BMC_PLD_R_N);
    clear_bit(U_GPO_1_ADDR, GPO_1_RST_RSMRST_PLD_R_N);

    // Take over controls on the SPI flash devices
    takeover_spi_ctrls();

#ifndef PLATFORM_WILSON_CITY_FAB1
    // Set Deep SX. This will drive 0 on PWRGD_DSW_PWROK_R
    set_bit(U_GPO_1_ADDR, GPO_1_PWRGD_DSW_PWROK_R);
#endif
}

/**
 * @brief Perform the platform to enter T0 mode. 
 *
 * The main tasks here are:
 *   - release controls of SPI flash devices
 *   - release BMC and PCH from resets
 *   - launch watchdog timers if the system is provisioned.
 * 
 * If a SPI flash device is found to have invalid images for active, recovery and staging regions, 
 * then Nios firmware will keep that associated component in reset. 
 * 
 * If only BMC SPI flash has valid image(s), then boot BMC in isolation with watchdog timer turned off. 
 * BMC's behavior is unknown when PCH is in reset. 
 * 
 * When both SPI flash device have invalid images, Nios firmware enters a lockdown mode in T-1. 
 * 
 * If hierarchical PFR is supported, Nios firmware also notifies other CPLDs that
 * it is entering T0 mode.
 */
static void perform_entry_to_t0()
{
    // Notify other CPLDs if HPFR is supported
    perform_hpfr_entry_to_t0();

    // If both BMC and PCH flashes failed authentication for all regions, stay in T-1 mode.
    if (check_spi_flash_state(SPI_FLASH_BMC, SPI_FLASH_STATE_ALL_REGIONS_FAILED_AUTH_MASK) &&
            check_spi_flash_state(SPI_FLASH_PCH, SPI_FLASH_STATE_ALL_REGIONS_FAILED_AUTH_MASK))
    {
        log_platform_state(PLATFORM_STATE_AUTHENTICATION_FAILED_LOCKDOWN);
        never_exit_loop();
    }

    // Boot BMC and PCH
    tmin1_boot_bmc_and_pch();

    log_platform_state(PLATFORM_STATE_ENTER_T0);
}

/**
 * @brief Transition the platform to T-1 mode, perform T-1 operations 
 * if the PFR system is provisioned, and then transition the platform back to T0 mode.
 */
static void perform_platform_reset()
{
    // Prepare for transition to T-1 mode
    perform_entry_to_tmin1();

    // In unprovisioned state, skip T-1 operations, filter enabling and boot monitoring.
    if (is_ufm_provisioned())
    {
        // Perform T-1 operations
        perform_tmin1_operations();
    }

    // Perform the entry to T0
    perform_entry_to_t0();
}

/********************************************
 *
 * BMC only reset
 *
 ********************************************/

/**
 * @brief This function transitions BMC to a reset state through EXTRST#. 
 *
 * When in T0 mode, Nios firmware may detect some panic events such as BMC 
 * watchdog timer timeout and BMC firmware update. In order to keep the host
 * up while recovering or updating BMC, Nios firmware performs a BMC-only reset. 
 */
static void perform_entry_to_tmin1_bmc_only()
{
#ifdef USE_SYSTEM_MOCK
    SYSTEM_MOCK::get()->incr_t_minus_1_bmc_only_counter();
#endif
    // Assert EXTRST# to reset only the BMC
    clear_bit(U_GPO_1_ADDR, GPO_1_RST_PFR_EXTRST_N);

    // Take over control on the BMC flash device
    takeover_spi_ctrl(SPI_FLASH_BMC);
}

/**
 * @brief This function transitions BMC back to T0 state through EXTRST#. 
 * 
 * After releasing SPI control of BMC flash back to BMC, Nios firmware releases
 * BMC from reset by setting EXTRST#. 
 * 
 * Since BMC-only reset only happens in a provisioned system, BMC watchdog timer 
 * is always ran after releasing the reset.
 */
static void perform_entry_to_t0_bmc_only()
{
    // Boot BMC only if it has valid image in its SPI flash
    if (!check_spi_flash_state(SPI_FLASH_BMC, SPI_FLASH_STATE_ALL_REGIONS_FAILED_AUTH_MASK))
    {
        // Release SPI flash control to BMC
        release_spi_ctrl(SPI_FLASH_BMC);

        // Release reset on BMC
        set_bit(U_GPO_1_ADDR, GPO_1_RST_PFR_EXTRST_N);

        // Don't turn on BMC WDT when PCH is in reset, because BMC may have undefined behavior.
        if (!check_spi_flash_state(SPI_FLASH_PCH, SPI_FLASH_STATE_ALL_REGIONS_FAILED_AUTH_MASK))
        {
            // Track the BMC boot progress
            // Clear any previous BMC boot done status
            wdt_boot_status &= ~WDT_BMC_BOOT_DONE_MASK;

            // Start the BMC watchdog timer
            start_timer(WDT_BMC_TIMER_ADDR, WD_BMC_TIMEOUT_VAL);
        }
    }
}

/**
 * @brief Transition the BMC to T-1 mode, perform T-1 operations, 
 * and then transition the BMC back to T0 mode.
 * 
 * Since BMC-only reset only happens in a provisioned system, Nios firmware always
 * perform the BMC specific T-1 operations.
 */
static void perform_bmc_only_reset()
{
    log_platform_state(PLATFORM_STATE_ENTER_TMIN1);

    // Enter T-1 for BMC
    perform_entry_to_tmin1_bmc_only();

    // Perform T-1 operations on BMC (assuming that Nios is provisioned at this point)
    perform_tmin1_operations_for_bmc();

    // Enter T0 for BMC
    perform_entry_to_t0_bmc_only();

    log_platform_state(PLATFORM_STATE_ENTER_T0);
}

/********************************************
 *
 * PCH only reset
 *
 ********************************************/

static void perform_entry_to_tmin1_pch_only()
{
#ifdef USE_SYSTEM_MOCK
    SYSTEM_MOCK::get()->incr_t_minus_1_pch_only_counter();
#endif
#ifndef PLATFORM_WILSON_CITY_FAB1
    // PFR should drive the new PFR_SLP_SUS_N pin high, during a T0 to T-1 transition.
    // It should drive that high before setting RSMRST# and DSWPWROK low.
    set_bit(U_GPO_1_ADDR, GPO_1_FM_PFR_SLP_SUS_N);
#endif

    // Assert reset on PCH
    clear_bit(U_GPO_1_ADDR, GPO_1_RST_RSMRST_PLD_R_N);

    // Take over PCH SPI flash control
    takeover_spi_ctrl(SPI_FLASH_PCH);

#ifndef PLATFORM_WILSON_CITY_FAB1
    // Set Deep SX. This will drive 0 on PWRGD_DSW_PWROK_R
    set_bit(U_GPO_1_ADDR, GPO_1_PWRGD_DSW_PWROK_R);
#endif
}

/**
 * @brief Transition the PCH to T-1 mode, perform T-1 operations,
 * and then transition the PCH back to T0 mode.
 *
 * Since PCH-only reset only happens in a provisioned system, Nios firmware always
 * perform the PCH specific T-1 operations.
 */
static void perform_pch_only_reset()
{
    log_platform_state(PLATFORM_STATE_ENTER_TMIN1);

    // Enter T-1 for PCH
    perform_entry_to_tmin1_pch_only();

    // Perform T-1 operations on PCH (assuming that Nios is provisioned at this point)
    perform_tmin1_operations_for_pch();

    // Wilson city specific change for HSD1507959205. This is a WC2 specific change because its not required in the other platforms and there are out of code space
#ifdef PLATFORM_WILSON_CITY_FAB2
    // Clear Deep SX. This will drive Z onto PWRGD_DSW_PWROK_R
    clear_bit(U_GPO_1_ADDR, GPO_1_PWRGD_DSW_PWROK_R);

    // Add some delays between DSW_PWROK and PFR_SLP_SUS
    // The max time for this transition is 100ms. Set a delay of 120ms to be safe.
    // If this delay is not added or is too small, platform would enter
    //   a power loop (constantly shutting down and then power up).
    sleep_20ms(6);

    // Set PFR_SLP_SUS_N (sleep suspend) low
    clear_bit(U_GPO_1_ADDR, GPO_1_FM_PFR_SLP_SUS_N);
#endif

    // Boot up PCH
    tmin1_boot_pch();

    log_platform_state(PLATFORM_STATE_ENTER_T0);
}

#endif /* WHITLEY_INC_TRANSITION_H_ */
