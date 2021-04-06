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
 * @file firmware_update.h
 * @brief Responsible for firmware updates, as specified in the BMC/PCH update intent, in T-1 mode.
 */

#ifndef WHITLEY_INC_FIRMWARE_UPDATE_H
#define WHITLEY_INC_FIRMWARE_UPDATE_H

// Always include pfr_sys.h first
#include "pfr_sys.h"

#include "capsule_validation.h"
#include "decompression.h"
#include "gen_gpo_controls.h"
#include "key_cancellation.h"
#include "keychain_utils.h"
#include "platform_log.h"
#include "spi_ctrl_utils.h"
#include "spi_flash_state.h"
#include "utils.h"
#include "watchdog_timers.h"


/**
 * @brief Perform firmware update for the @p spi_flash_type flash.
 * If @p is_recovery_update is set to 1, do active update first and then set a flag
 * that will later trigger the recovery firmware update flow in T0 mode.
 *
 * Nios firmware needs to verify that the firmware from the update capsule passes authentication
 * and can boot up correctly, before promoting that firmware to the recovery region. That's why
 * active firmware update is required before performing recovery firmware update.
 *
 * Active firmware update is done by decompressing the static SPI regions from the update capsule. If
 * @p is_recovery_update is set to 1, then both static and dynamic SPI regions are being decompressed
 * from the update capsule. Active firmware PFM will also be overwritten by the PFM in the update capsule.
 *
 * If @p is_recovery_update is set to 1, Nios firmware will apply write protection on the
 * staging region. Hence, the staging update capsule won't be changed from active firmware update to
 * recovery firmware update.
 *
 * @param spi_flash_type indicate BMC or PCH SPI flash device
 * @param update_intent the value of the update intent register
 *
 * @see decompress_capsule
 * @see post_update_routine
 * @see perform_firmware_recovery_update
 */
static void perform_active_firmware_update(SPI_FLASH_TYPE_ENUM spi_flash_type, alt_u32 update_intent)
{
    log_firmware_update(spi_flash_type);

    // Make sure we are updating the right flash device
    switch_spi_flash(spi_flash_type);

    // Check if this is a recovery update
    alt_u32 is_recovery_update = update_intent & MB_UPDATE_INTENT_BMC_RECOVERY_MASK;
    if (spi_flash_type == SPI_FLASH_PCH)
    {
        is_recovery_update = update_intent & MB_UPDATE_INTENT_PCH_RECOVERY_MASK;
    }

    // Clear the number of failed attempts
    reset_failed_update_attempts(spi_flash_type);

    // Get pointers to staging capsule
    alt_u32* signed_staging_capsule = get_spi_staging_region_ptr(spi_flash_type);

    // Only overwrite static regions in active update
    DECOMPRESSION_TYPE_MASK_ENUM decomp_event = DECOMPRESSION_STATIC_REGIONS_MASK;

    if (is_recovery_update)
    {
        // The actual update to the recovery region will be executed, after timed boot of the
        // new active firmware is completed. This flag will remind Nios of the update to recovery region.
        set_spi_flash_state(spi_flash_type, SPI_FLASH_STATE_HAS_PENDING_RECOVERY_FW_UPDATE_MASK);

        // Overwrite both static and dynamic regions in recovery update
        decomp_event = DECOMPRESSION_STATIC_AND_DYNAMIC_REGIONS_MASK;
    }
    else if (update_intent & MB_UPDATE_INTENT_UPDATE_DYNAMIC_MASK)
    {
        // Overwrite both static and dynamic regions in recovery update
        decomp_event = DECOMPRESSION_STATIC_AND_DYNAMIC_REGIONS_MASK;
    }

    // Perform decompression
    decompress_capsule(signed_staging_capsule, spi_flash_type, decomp_event);

    // Re-enable watchdog timers in case they were disabled after 3 WDT timeouts.
    if (spi_flash_type == SPI_FLASH_PCH)
    {
        wdt_enable_status |= WDT_ENABLE_PCH_TIMERS_MASK;
    }
    else // spi_flash_type == SPI_FLASH_BMC
    {
        wdt_enable_status |= WDT_ENABLE_BMC_TIMER_MASK;
    }

}

/**
 * @brief Perform recovery firmware update for @p spi_flash_type flash.
 *
 * At this point, it is assumed that the staging capsule is authentic and matches the active
 * firmware. Nios simply copies the staging capsule and overwrite the recovery capsule. After
 * that, Nios update the SVN policy with the new SVN
 *
 * @param spi_flash_type indicate BMC or PCH SPI flash device
 *
 * @see perform_active_firmware_update
 * @see post_update_routine
 * @see process_pending_recovery_update
 */
static void perform_firmware_recovery_update(SPI_FLASH_TYPE_ENUM spi_flash_type)
{
    switch_spi_flash(spi_flash_type);

    // Copy staging capsule to overwrite recovery capsule
    alt_u32* staging_capsule = get_spi_staging_region_ptr(spi_flash_type);
    memcpy_signed_payload(get_recovery_region_offset(spi_flash_type), staging_capsule);

    // Update the SVN policy now that recovery update has completed
    PFM* staging_capsule_pfm = get_capsule_pfm(staging_capsule);
    UFM_SVN_POLICY_TYPE_ENUM svn_policy_type = UFM_SVN_POLICY_PCH;
    if (spi_flash_type == SPI_FLASH_BMC)
    {
        svn_policy_type = UFM_SVN_POLICY_BMC;
    }
    write_ufm_svn(staging_capsule_pfm->svn, svn_policy_type);
}

#endif /* WHITLEY_INC_FIRMWARE_UPDATE_H */
