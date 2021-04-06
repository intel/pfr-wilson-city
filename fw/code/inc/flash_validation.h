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
 * @file flash_validation.h
 * @brief Perform validations on active/recovery/staging regions of the SPI flash memory in T-1 mode.
 */

#ifndef WHITLEY_INC_FLASH_VALIDATION_H
#define WHITLEY_INC_FLASH_VALIDATION_H

// Always include pfr_sys.h first
#include "pfr_sys.h"

#include "capsule_validation.h"
#include "firmware_update.h"
#include "firmware_recovery.h"
#include "keychain.h"
#include "keychain_utils.h"
#include "mailbox_utils.h"
#include "pfm_validation.h"
#include "pfr_pointers.h"
#include "spi_flash_state.h"
#include "ufm_utils.h"
#include "ufm.h"

/**
 * @brief Log any authentication failure in the mailbox major and minor error registers.
 *
 * @param spi_flash_type indicate BMC or PCH SPI flash device
 * @param is_active_invalid 1 if signed active PFM failed authentication
 * @param is_recovery_invalid 1 if signed recovery capsule failed authentication
 */
static void log_auth_results(SPI_FLASH_TYPE_ENUM spi_flash_type, alt_u32 is_active_invalid, alt_u32 is_recovery_invalid)
{
    if (is_active_invalid)
    {
        log_auth_failure(spi_flash_type, MINOR_ERROR_AUTH_ACTIVE);

        if (is_recovery_invalid)
        {
            log_auth_failure(spi_flash_type, MINOR_ERROR_AUTH_ACTIVE_AND_RECOVERY);
        }
    }
    else if (is_recovery_invalid)
    {
        log_auth_failure(spi_flash_type, MINOR_ERROR_AUTH_RECOVERY);
    }
}

/**
 * @brief Authenticate the active/recovery/staging region for @p spi_flash_type flash and 
 * perform recovery where appropriate. 
 * 
 * Nios authenticates the signed active PFM and signed recovery capsule. If any failure is seen, 
 * Nios proceeds to perform recovery according to the Recovery Matrix in the High-level Architecture Spec. 
 * 
 * Active | Recovery | Staging | Action
 * 0      | 0        | 0       | unrecoverable; do not allow the corresponding device to boot
 * 0      | 0        | 1       | Copy Staging capsule over to Recovery region; then perform an active recovery
 * 0      | 1        | 0       | Perform an active recovery
 * 0      | 1        | 1       | Perform an active recovery
 * 1      | 0        | 0       | Allow the device to boot but restrict active update; only allow recovery udpate
 * 1      | 0        | 1       | Copy the Staging capsule over to Recovery region
 * 1      | 1        | 0       | No action needed
 * 1      | 1        | 1       | No action needed
 *
 * Recovery of active image:
 * Nios performs a static recovery. Static recovery only recover the static regions (i.e. regions that allow read
 * but not write) of the active firmware. This recovery can be triggered by either authentication failure or forced
 * recovery.
 *
 * Recovery of recovery image:
 * If the staging image is valid, Nios copies it over to the recovery area. It's assumed that this scenario may occur
 * only after a power failure.
 *
 * If the active firmware is authentic, Nios applies the SPI and SMBus filtering rules. Only SPI filters are enabled
 * here. SMBus filters are enabled only when BMC has completed boot. 
 * 
 * Nios writes the PFMs' information to mailbox after all the above tasks are done. 
 *
 * @param spi_flash_type indicate BMC or PCH SPI flash device
 */
static void authenticate_and_recover_spi_flash(SPI_FLASH_TYPE_ENUM spi_flash_type)
{
#ifdef USE_SYSTEM_MOCK
    if (SYSTEM_MOCK::get()->should_exec_code_block(
            SYSTEM_MOCK::CODE_BLOCK_TYPES::SKIP_FLASH_AUTHENTICATION))
    {
        return;
    }
#endif

    // Log platform state
    if (spi_flash_type == SPI_FLASH_BMC)
    {
        log_platform_state(PLATFORM_STATE_BMC_FLASH_AUTHENTICATION);
    }
    else // spi_flash_type == SPI_FLASH_PCH
    {
        log_platform_state(PLATFORM_STATE_PCH_FLASH_AUTHENTICATION);
    }

    // Clear previous SPI state on authentication result
    clear_spi_flash_state(spi_flash_type, SPI_FLASH_STATE_CLEAR_AUTH_RESULT);

    /*
     * Authentication of Active region and Recovery region
     */
    alt_u32* active_pfm_ptr = get_spi_active_pfm_ptr(spi_flash_type);
    alt_u32* recovery_region_ptr = get_spi_recovery_region_ptr(spi_flash_type);

    // Verify the signature and content of the active section PFM
    alt_u32 is_active_valid = is_active_region_valid(active_pfm_ptr);

    // Verify the signature of the recovery section capsule
    alt_u32 is_recovery_valid = is_capsule_valid(recovery_region_ptr);

    // Check for FORCE_RECOVERY GPI signal
    alt_u32 require_force_recovery = !check_bit(U_GPI_1_ADDR, GPI_1_FM_PFR_FORCE_RECOVERY_N);

    // Log the authentication results
    log_auth_results(spi_flash_type, !is_active_valid, !is_recovery_valid);

    /*
     * Perform recovery where appropriate
     * Please refer to Recovery Matrix in the High-level Architecture Spec
     *
     * Possible regions state (1 means valid; 0 means invalid)
     * Active | Recovery | Staging
     * 0      | 0        | 0
     * 0      | 0        | 1
     * 0      | 1        | 0
     * 0      | 1        | 1
     * 1      | 0        | 0
     * 1      | 0        | 1
     * 1      | 1        | 0 (no action required)
     * 1      | 1        | 1 (no action required)
     */
    // Deal with the cases where Recovery image is invalid first
    if (!is_recovery_valid)
    {
        if (check_capsule_before_fw_recovery_update(spi_flash_type, !is_active_valid))
        {
            // Log this recovery action
            log_tmin1_recovery_on_recovery_image(spi_flash_type);

            /* Scenarios
             * Active | Recovery | Staging
             * 0      | 0        | 1
             * 1      | 0        | 1
             *
             * Corrective action required: Copy Staging capsule to overwrite Recovery capsule
             */
            perform_firmware_recovery_update(spi_flash_type);
            is_recovery_valid = 1;
        }
        // Unable to recovery recovery image
        else if (is_active_valid)
        {
            /* Scenarios
             * Active | Recovery | Staging
             * 1      | 0        | 0
             *
             * Cannot recover the recovery image. Save the flash state.
             */
            set_spi_flash_state(spi_flash_type, SPI_FLASH_STATE_RECOVERY_FAILED_AUTH_MASK);
        }
        else
        {
            /* Scenarios
             * Active | Recovery | Staging
             * 0      | 0        | 0
             *
             * Hence, Active/Recovery/Staging images are all bad
             * Cannot recover the Active and Recovery images. Save the flash state.
             */
            set_spi_flash_state(spi_flash_type, SPI_FLASH_STATE_ALL_REGIONS_FAILED_AUTH_MASK);

            // Critical platform error
            log_auth_failure(spi_flash_type, MINOR_ERROR_AUTH_ALL_REGIONS);
        }
    }

    /* Scenarios
     * Active | Recovery | Staging
     * 0      | 1        | 0
     * 0      | 1        | 1
     *
     * Simply recover active image if needed.
     */
    if (is_recovery_valid)
    {
        if (require_force_recovery)
        {
            // Log event
            log_platform_state(PLATFORM_STATE_TMIN1_FORCED_ACTIVE_FW_RECOVERY);
            log_recovery(LAST_RECOVERY_FORCED_ACTIVE_FW_RECOVERY);

            // Recover the entire active firmware upon forced recovery request.
            decompress_capsule(recovery_region_ptr, spi_flash_type, DECOMPRESSION_STATIC_AND_DYNAMIC_REGIONS_MASK);
            is_active_valid = 1;
        }
        else if (!is_active_valid)
        {
            // Log event
            log_tmin1_recovery_on_active_image(spi_flash_type);

            // Recover the entire active firmware when it failed authentication
            decompress_capsule(recovery_region_ptr, spi_flash_type, DECOMPRESSION_STATIC_AND_DYNAMIC_REGIONS_MASK);
            is_active_valid = 1;
        }
    }

    if (is_active_valid)
    {
        // If the active firmware passed authentication, then apply the protection specified in the active PFM.
        apply_spi_write_protection_and_smbus_rules(spi_flash_type);
    }

    // Print the active & recovery PFM information to mailbox
    // Do this last in case there was some recovery action.
    mb_write_pfm_info(spi_flash_type);
}


#endif /* WHITLEY_INC_FLASH_VALIDATION_H */
