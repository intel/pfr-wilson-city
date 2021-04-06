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
 * @file firmware_recovery.h
 * @brief Responsible for firmware recovery in T-1 mode.
 */

#ifndef WHITLEY_INC_FIRMWARE_RECOVERY_H
#define WHITLEY_INC_FIRMWARE_RECOVERY_H

// Always include pfr_sys.h first
#include "pfr_sys.h"

#include "authentication.h"
#include "decompression.h"
#include "global_state.h"
#include "mailbox_utils.h"
#include "spi_ctrl_utils.h"
#include "spi_flash_state.h"
#include "ufm_utils.h"
#include "watchdog_timers.h"

// Keep track of the next recovery level
static alt_u8 current_recovery_level_mask_for_pch = SPI_REGION_PROTECT_MASK_RECOVER_ON_FIRST_RECOVERY;
static alt_u8 current_recovery_level_mask_for_bmc = SPI_REGION_PROTECT_MASK_RECOVER_ON_FIRST_RECOVERY;

/**
 * @brief Reset the current firmware recovery level for @p spi_flash_type SPI flash.
 *
 * @param spi_flash_type indicates BMC or PCH flash
 */
static void reset_fw_recovery_level(SPI_FLASH_TYPE_ENUM spi_flash_type)
{
    if (spi_flash_type == SPI_FLASH_PCH)
    {
        current_recovery_level_mask_for_pch = SPI_REGION_PROTECT_MASK_RECOVER_ON_FIRST_RECOVERY;
    }
    else // spi_flash_type == SPI_FLASH_BMC
    {
        current_recovery_level_mask_for_bmc = SPI_REGION_PROTECT_MASK_RECOVER_ON_FIRST_RECOVERY;
    }
}

/**
 * @brief Increment the current firmware recovery level for @p spi_flash_type SPI flash.
 *
 * @param spi_flash_type indicates BMC or PCH flash
 */
static void incr_fw_recovery_level(SPI_FLASH_TYPE_ENUM spi_flash_type)
{
    // Since current recovery level variables are bit mask,
    // increment the level by shifting 1 bit to the left.
    if (spi_flash_type == SPI_FLASH_PCH)
    {
        // Shift the recovery level bit mask by 1 bit
        current_recovery_level_mask_for_pch <<= 1;
    }
    else // spi_flash_type == SPI_FLASH_BMC
    {
        current_recovery_level_mask_for_bmc <<= 1;
    }
}

/**
 * @brief Return the current firmware recovery level for @p spi_flash_type SPI flash.
 *
 * @param spi_flash_type indicates BMC or PCH flash
 */
static alt_u8 get_fw_recovery_level(SPI_FLASH_TYPE_ENUM spi_flash_type)
{
    if (spi_flash_type == SPI_FLASH_PCH)
    {
        return current_recovery_level_mask_for_pch;
    }
    // spi_flash_type == SPI_FLASH_BMC
    return current_recovery_level_mask_for_bmc;
}

/**
 * @brief Clear any Top Swap configuration when performing the last level of WDT recovery for PCH image.
 * Top Swap reset is realized by driving RTCRST and SRTCRST pins low for 1s. The GPO control bit will drive
 * both of those pins low, if it's set.
 *
 * @param spi_flash_type indicates BMC or PCH flash
 * @param recovery_level the current recovery level
 */
static void perform_top_swap_for_pch_flash(SPI_FLASH_TYPE_ENUM spi_flash_type, alt_u32 recovery_level)
{
    if ((spi_flash_type == SPI_FLASH_PCH)
            && (recovery_level == SPI_REGION_PROTECT_MASK_RECOVER_ON_THIRD_RECOVERY))
    {
        set_bit(U_GPO_1_ADDR, GPO_1_TRIGGER_TOP_SWAP_RESET);

        // Wait 1 second
        sleep_20ms(50);

        clear_bit(U_GPO_1_ADDR, GPO_1_TRIGGER_TOP_SWAP_RESET);
    }
}

/**
 * @brief A recovery procedure performed after a watchdog timer timeouts.
 * In watchdog timeout recovery, a dynamic region is recovered, when RPLM bits 2-4 indicate that a
 * recovery is required for the current level of recovery. If any of the static regions has any of
 * RPLM bits 2-4 set, all static regions are recovered in this WDT recovery.
 *
 * A dynamic region is a SPI region that is read allowed and write allowed in PFM.
 * A static region is a SPI region that is read allowed but not write allowed in PFM.
 *
 * The current level of recovery is tracked by a global variable, which will be reset after a power
 * cycle or a successful timed boot.
 *
 * In T0 mode, Nios simply sets a bit in the global variable bmc_flash_state or pch_flash_state that
 * indicates a WDT recovery is needed. Then Nios performs a platform reset for PCH timer timeouts or
 * a BMC-only reset for BMC timer timeouts. In the subsequent T-1 cycle, this function is always run.
 *
 * It's assumed that Nios firmware is currently in T-1 mode and has control over the @p spi_flash_type
 * flash device.
 *
 * @param spi_flash_type indicates BMC or PCH flash
 *
 * @see watchdog_timeout_handler
 * @see perform_top_swap_for_pch_flash
 */
static void perform_wdt_recovery(SPI_FLASH_TYPE_ENUM spi_flash_type)
{
    if (check_spi_flash_state(spi_flash_type, SPI_FLASH_STATE_REQUIRE_WDT_RECOVERY_MASK))
    {
        // Log the platform state
        log_platform_state(PLATFORM_STATE_WDT_TIMEOUT_RECOVERY);

        // Recover the targeted flash
        switch_spi_flash(spi_flash_type);
        alt_u32* signed_recovery_capsule = get_spi_recovery_region_ptr(spi_flash_type);

        // Only perform recovery when the Recovery capsule is authentic
        if (is_signature_valid((KCH_SIGNATURE*) signed_recovery_capsule))
        {
            alt_u32 recovery_level = get_fw_recovery_level(spi_flash_type);
            perform_top_swap_for_pch_flash(spi_flash_type, recovery_level);

            alt_u32* pfm_body_ptr = get_active_pfm(spi_flash_type)->pfm_body;
            alt_u32 should_recover_static_region = 0;

            // Go through the PFM body
            // Recover the dynamic region, if RPLM indicates a recovery is needed for this recovery level.
            // Set the flag should_recover_static_region, if a static region definition, that has any RPLM bit 2-4 set, is found.
            while (1)
            {
                alt_u8 def_type = *((alt_u8*) pfm_body_ptr);
                if (def_type == SMBUS_RULE_DEF_TYPE)
                {
                    // Skip the rule definition
                    pfm_body_ptr = incr_alt_u32_ptr(pfm_body_ptr, SMBUS_RULE_DEF_SIZE);
                }
                else if (def_type == SPI_REGION_DEF_TYPE)
                {
                    PFM_SPI_REGION_DEF* region_def = (PFM_SPI_REGION_DEF*) pfm_body_ptr;

                    if (is_spi_region_static(region_def))
                    {
                        if (region_def->protection_mask & SPI_REGION_PROTECT_MASK_RECOVER_BITS)
                        {
                            should_recover_static_region = 1;
                        }
                    }
                    else if (is_spi_region_dynamic(region_def))
                    {
                        if (region_def->protection_mask & recovery_level)
                        {
                            decompress_spi_region_from_capsule(region_def->start_addr, region_def->end_addr, signed_recovery_capsule);
                        }
                    }

                    // Increment the pointer in PFM body appropriately
                    pfm_body_ptr = get_end_of_spi_region_def(region_def);
                }
                else
                {
                    // Break when there is no more region/rule definition in PFM body
                    break;
                }
            }

            if (should_recover_static_region)
            {
                // If a static PFM entry has any of RPLM.BIT[4:2] set,
                // WDT time out triggers T-1 static recovery (entire static portion of flashes).
                decompress_capsule(signed_recovery_capsule, spi_flash_type, DECOMPRESSION_STATIC_REGIONS_MASK);
            }

            // Increment recovery level after performing a firmware recovery
            incr_fw_recovery_level(spi_flash_type);

            // Clear the state
            clear_spi_flash_state(spi_flash_type, SPI_FLASH_STATE_REQUIRE_WDT_RECOVERY_MASK);
        }
    }
}

#endif /* WHITLEY_INC_FIRMWARE_RECOVERY_H */
