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
 * @file tmin1_routines.h
 * @brief functions that are executed during pre-boot mode (T-1) of operation.
 */

#ifndef WHITLEY_INC_TMIN1_ROUTINES_H_
#define WHITLEY_INC_TMIN1_ROUTINES_H_

// Always include pfr_sys.h first
#include "pfr_sys.h"

#include "flash_validation.h"
#include "gen_gpo_controls.h"
#include "gen_gpi_signals.h"
#include "pfm_utils.h"
#include "pit_utils.h"
#include "smbus_relay_utils.h"
#include "spi_ctrl_utils.h"
#include "timer_utils.h"
#include "tmin1_update.h"
#include "watchdog_timers.h"


/**
 * @brief Release SPI control on BMC flash back to BMC and release BMC from reset.
 * If the system is provisioned, start the BMC watchdog timer. 
 *
 * Nios FW releases BMC SPI control by clearing FM_SPI_PFR_BMC_BT_MASTER_SEL mux. Prior to clearing the mux,
 * Nios sends "exit 4-byte addressing mode" command to BMC SPI flash. Nios also notifies the BMC SPI filter
 * that BMC SPI flash is in 3-byte addressing mode. This is only required in platform reset. BMC can boot if
 * its flash is in 3-byte addressing mode, but PCH won't.
 *
 * This function should only be used in platform reset. BMC-only reset should not run this function.
 */
static void tmin1_boot_bmc()
{
    // Boot BMC only if it has valid active image in its SPI flash
    if (!check_spi_flash_state(SPI_FLASH_BMC, SPI_FLASH_STATE_ALL_REGIONS_FAILED_AUTH_MASK))
    {
        /*
         * Release the BMC SPI control, with BMC flash in 3B addressing mode
         */
        switch_spi_flash(SPI_FLASH_BMC);

        // Exit 4-byte addressing mode before releasing SPI control of BMC flash
        // Write enable command is required prior to sending the enter/exit 4-byte mode commands
        execute_one_byte_spi_cmd(SPI_CMD_WRITE_ENABLE);
        execute_one_byte_spi_cmd(SPI_CMD_EXIT_4B_ADDR_MODE);

        // The BMC SPI flash device should now be in 3-byte addressing mode.
        // Notify the BMC SPI filter of this change.
        set_bit(U_GPO_1_ADDR, GPO_1_BMC_SPI_ADDR_MODE_SET_3B);
        clear_bit(U_GPO_1_ADDR, GPO_1_BMC_SPI_ADDR_MODE_SET_3B);

        // Flip the external mux
        release_spi_ctrl(SPI_FLASH_BMC);

        /*
         * Release BMC from reset and start the BMC watchdog timer
         */
        set_bit(U_GPO_1_ADDR, GPO_1_RST_SRST_BMC_PLD_R_N);

        /*
         * Conditions to arm BMC watchdog:
         * 1. System is currently provisioned
         * 2. BMC has valid recovery image. Otherwise, when there's a timeout, there's no image to recover to.
         * 3. Nios will boot PCH. If PCH doesn't have valid image, then Nios boots only BMC. In this scenario,
         * it's not recommended to arm BMC watchdog timer because BMC has undefined behavior when PCH is in reset.
         */
        if (is_ufm_provisioned()
                && !check_spi_flash_state(SPI_FLASH_BMC, SPI_FLASH_STATE_RECOVERY_FAILED_AUTH_MASK)
                && !check_spi_flash_state(SPI_FLASH_PCH, SPI_FLASH_STATE_ALL_REGIONS_FAILED_AUTH_MASK))
        {
            // Start the BMC watchdog timer
            start_timer(WDT_BMC_TIMER_ADDR, WD_BMC_TIMEOUT_VAL);
        }
    }
}

/**
 * @brief Perform all required operations to boot PCH. 
 * 
 * Steps: 
 * 1. Release the PCH SPI flash control back to PCH. The PCH SPI flash is in 4-byte addressing mode.
 * 2. Drive Z to DSWPWROK
 * 3. Wait for at least 100 ms to add some delays between setting DSW_PWROK and SLP_SUS_N. 
 * 4. Drive 0 to SLP_SUS_N
 * 5. Release PCH from reset (by driving 1 to RSMRST#)
 * 6. Start the ME watchdog timer, if the system is provisioned.
 *
 * ACM/BIOS watchdog timer is armed, when PLTRST falling edge is detected. This
 * is done in platform_reset_handler().
 *
 * @see platform_reset_handler
 */
static void tmin1_boot_pch()
{
    // Conditions to release PCH from reset:
    //   - PCH SPI flash must have authentic firmware
    //   - BMC SPI flash must have authentic firmware, because
    //     CPLD cannot release PCH from reset without releasing BMC from reset first.
    if (!(check_spi_flash_state(SPI_FLASH_PCH, SPI_FLASH_STATE_ALL_REGIONS_FAILED_AUTH_MASK) ||
            check_spi_flash_state(SPI_FLASH_BMC, SPI_FLASH_STATE_ALL_REGIONS_FAILED_AUTH_MASK)))
    {
        // Release SPI flash control to PCH
        release_spi_ctrl(SPI_FLASH_PCH);

#ifndef PLATFORM_WILSON_CITY_FAB2
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

#ifdef PLATFORM_WILSON_CITY_FAB2
		sleep_20ms(1);
#endif
        // Release PCH from reset (triggers SPS/ME boot)
        set_bit(U_GPO_1_ADDR, GPO_1_RST_RSMRST_PLD_R_N);

        /*
         * Conditions to arm PCH ME watchdog:
         * 1. System is currently provisioned
         * 2. PCH has valid recovery image. Otherwise, when there's a timeout, there's no image to recover to.
         */
        if (is_ufm_provisioned()
                && !check_spi_flash_state(SPI_FLASH_PCH, SPI_FLASH_STATE_RECOVERY_FAILED_AUTH_MASK))
        {
            // Start the ME watchdog timer
            start_timer(WDT_ME_TIMER_ADDR, WD_ME_TIMEOUT_VAL);
        }
    }
}

/**
 * @brief Boot BMC by calling tmin1_boot_bmc(), then boot PCH by calling tmin1_boot_pch().
 * The order is important because the power sequence requires BMC to be released from reset 11ms earlier
 * than PCH.
 * 
 * @see tmin1_boot_bmc()
 * @see tmin1_boot_pch()
 */
static void tmin1_boot_bmc_and_pch()
{
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

    tmin1_boot_bmc();
    tmin1_boot_pch();
}

/**
 * @brief Perform T-1 routines, such as authentication, update and recovery, on BMC flash.
 *
 * Process any BMC update according to the BMC update intent register.
 * Perform authentication and recovery of all critical regions
 * of BMC FW storage. If the active firmware passed authentication, 
 * enable BMC SPI write filtering and store SMBus command filtering rules.
 */
static void perform_tmin1_operations_for_bmc()
{
#ifdef USE_SYSTEM_MOCK
    if (SYSTEM_MOCK::get()->should_exec_code_block(
            SYSTEM_MOCK::CODE_BLOCK_TYPES::SKIP_TMIN1_OPERATIONS))
    {
        return;
    }
#endif
    switch_spi_flash(SPI_FLASH_BMC);

    // Clear BMC WDT and checkpoint
    IOWR(WDT_BMC_TIMER_ADDR, 0, 0);
    write_to_mailbox(MB_BMC_CHECKPOINT, 0);

    // Clear previous boot done status
    wdt_boot_status &= ~WDT_BMC_BOOT_DONE_MASK;

    // Perform WDT recovery if there was a watchdog timeout in the previous T0 mode.
    perform_wdt_recovery(SPI_FLASH_BMC);

    // Perform updates as requested in the BMC update intent register
    act_on_bmc_update_intent();

    // Perform authentication and possibly recovery on the BMC flash
    authenticate_and_recover_spi_flash(SPI_FLASH_BMC);

    // Manually write protect the cpld recovery region inside the BMC spi flash
    write_protect_cpld_recovery_region();

    // If there's an ongoing CPLD update, write protect the CPLD UPDATE staging region in BMC flash
    if (is_cpld_rc_update_in_progress())
    {
        write_protect_cpld_staging_region();
    }
}

/**
 * @brief Perform T-1 routines, such as authentication, update and recovery, on PCH flash.
 *
 * Process any PCH update according to the PCH update intent register.
 * Perform authentication and recovery of all critical regions
 * of PCH FW storage. If the active firmware passed authentication,
 * enable PCH SPI write filtering rules. Note that Nios only processes SMBus filtering
 * rules from BMC PFM. SMBus filtering rules from PCH PFM are ignored.
 */
static void perform_tmin1_operations_for_pch()
{
#ifdef USE_SYSTEM_MOCK
    if (SYSTEM_MOCK::get()->should_exec_code_block(
            SYSTEM_MOCK::CODE_BLOCK_TYPES::SKIP_TMIN1_OPERATIONS))
    {
        return;
    }
#endif
    switch_spi_flash(SPI_FLASH_PCH);

    // Clear PCH WDTs
    IOWR(WDT_ME_TIMER_ADDR, 0, 0);
    IOWR(WDT_ACM_BIOS_TIMER_ADDR, 0, 0);

    // Clear previous boot done status
    wdt_boot_status &= ~WDT_PCH_BOOT_DONE_MASK;

    // Perform WDT recovery if there was a watchdog timeout in the previous T0 mode.
    perform_wdt_recovery(SPI_FLASH_PCH);

    // Perform updates as requested in the PCH update intent register
    act_on_pch_update_intent();

    // Perform authentication and possibly recovery on the PCH flash
    authenticate_and_recover_spi_flash(SPI_FLASH_PCH);
}

/**
 * @brief Perform T-1 operations on both BMC and PCH SPI flashes. 
 *
 * Nios firmware performs these operations in order:
 * 1. Check protect-in-transit feature. If the L1 protection is enabled, Nios firmware
 * attempts to get the PIT password from the RFNVRAM and check it against the provisioned
 * PIT password in UFM. If the L2 protection is enabled, Nios firmware will compute the 
 * firmware hashes for both BMC and PCH flashes. The calculated hashes are compared against
 * the stored hashes in UFM. If either L1 or L2 check failed, Nios firmware enters a lockdown 
 * mode in T-1. 
 * 2. If there was a watchdog timer timeout in the past T0 cycle, Nios firmware performs WDT 
 * recovery for that component. @perform_wdt_recovery for more details on WDT recovery. 
 * 3. Perform any update as indicated in PCH and BMC update intent registers.
 * 4. Perform authentication and recovery of all critical regions of platform FW storage (PCH flash and BMC flash). 
 * If the active firmware is valid, Nios firmware enables the SPI filtering and store SMBus command filtering rules
 * according to the active PFM.
 */
static void perform_tmin1_operations()
{
#ifdef USE_SYSTEM_MOCK
    if (SYSTEM_MOCK::get()->should_exec_code_block(
            SYSTEM_MOCK::CODE_BLOCK_TYPES::SKIP_TMIN1_OPERATIONS))
    {
        return;
    }
#endif

    // If a level of Protect-in-Transit is enabled, perform appropriate protection
    perform_pit_protection();

    // Preparation for OOB PCH FW update (e.g. authenticate capsule and move it to PCH flash)
    // OOB PCH FW update requires both BMC and PCH in reset.
    prep_for_oob_pch_fw_update();

    // Recovery update is supposed to happen only when both BMC and PCH are in reset.
    // Process any pending recovery update
    process_pending_recovery_update(SPI_FLASH_BMC);
    process_pending_recovery_update(SPI_FLASH_PCH);

    // Do BMC T-1 routine first. If there's any OOB PCH update request, the updated firmware version will be reflected in mailbox.
    perform_tmin1_operations_for_bmc();
    perform_tmin1_operations_for_pch();
}

#endif /* WHITLEY_INC_TMIN1_ROUTINES_H_ */
