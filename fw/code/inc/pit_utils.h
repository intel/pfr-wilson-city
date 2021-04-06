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
 * @file pit_utils.h
 * @brief Responsible to support Protect-in-Transit feature.
 */

#ifndef WHITLEY_INC_PIT_UTILS_H_
#define WHITLEY_INC_PIT_UTILS_H_

// Always include pfr_sys.h first
#include "pfr_sys.h"

#include "crypto.h"
#include "pfr_pointers.h"
#include "rfnvram_utils.h"
#include "spi_rw_utils.h"
#include "timer_utils.h"
#include "ufm_utils.h"


/**
 * @brief Enforce the PIT L1 protection, if it's enabled. 
 * 
 * If PIT L1 is enabled, Nios fetches the PIT ID from RFNVRAM and compares
 * it against the stored PIT ID in UFM. If they do not match, Nios enters
 * a lockdown mode and platform stays in T-1. If the IDs match, exit
 * the function. 
 */
static void perform_pit_l1_check()
{
    if (check_ufm_status(UFM_STATUS_PIT_L1_ENABLE_BIT_MASK))
    {
        // PIT L1 protection is enabled
        // Check the PIT ID in UFM against the ID in RFNVRAM
        alt_u32 rfnvram_pit_id[RFNVRAM_PIT_ID_LENGTH / 4] = {};
        read_from_rfnvram((alt_u8*) rfnvram_pit_id, RFNVRAM_PIT_ID_OFFSET, RFNVRAM_PIT_ID_LENGTH);

        alt_u32* ufm_pit_id = get_ufm_pfr_data()->pit_id;
        for (alt_u32 word_i = 0; word_i < (RFNVRAM_PIT_ID_LENGTH / 4); word_i++)
        {
            if (ufm_pit_id[word_i] != rfnvram_pit_id[word_i])
            {
                // Stay in T-1 mode if there's a ID mismatch
                log_platform_state(PLATFORM_STATE_PIT_L1_LOCKDOWN);
                never_exit_loop();
            }
        }
    }
}

/**
 * @brief If the PIT L2 is enabled and firmware hashes are stored, enforce the Firmware Sealing
 * protection. If the PIT L2 is enabled but firmware hashes have not been stored, Nios calculates
 * firmware hashes for both PCH and BMC flashes. 
 * 
 * When Firmware Sealing (PIT L2) is enforced, if any of the calculated firmware hash does not match 
 * the expected firmware hash in UFM, Nios enters a lockdown mode and platform stays in T-1 mode. 
 */
static void perform_pit_l2_check()
{
    if (!check_ufm_status(UFM_STATUS_PIT_L2_PASSED_BIT_MASK))
    {
        if (check_ufm_status(UFM_STATUS_PIT_L2_ENABLE_BIT_MASK))
        {
            // L2 protection is enabled
            UFM_PFR_DATA* ufm_data = get_ufm_pfr_data();

            if (check_ufm_status(UFM_STATUS_PIT_HASH_STORED_BIT_MASK))
            {
                // Firmware hash has been stored.

                // Compute PCH firmware hash
                // If the hash doesn't match the stored hash, hang in T-1
                switch_spi_flash(SPI_FLASH_PCH);
                if (!verify_sha(ufm_data->pit_pch_fw_hash, get_spi_flash_ptr(), PCH_SPI_FLASH_SIZE))
                {
                    // Remain in T-1 mode if there's a hash mismatch
                    log_platform_state(PLATFORM_STATE_PIT_L2_PCH_HASH_MISMATCH_LOCKDOWN);
                    never_exit_loop();
                }

                // Compute BMC firmware hash
                // If the hash doesn't match the stored hash, hang in T-1
                switch_spi_flash(SPI_FLASH_BMC);
                if (!verify_sha(ufm_data->pit_bmc_fw_hash, get_spi_flash_ptr(), BMC_SPI_FLASH_SIZE))
                {
                    // Remain in T-1 mode if there's a hash mismatch
                    log_platform_state(PLATFORM_STATE_PIT_L2_BMC_HASH_MISMATCH_LOCKDOWN);
                    never_exit_loop();
                }

                // PIT L2 passed.
                // Save this result in UFM and share with other components through UFM status mailbox register.
                // In the subsequent power up, L2 checks will not be triggered
                set_ufm_status(UFM_STATUS_PIT_L2_PASSED_BIT_MASK);
                mb_set_ufm_provision_status(MB_UFM_PROV_UFM_PIT_L2_PASSED_MASK);
            }
            else
            {
                // Pending PIT hash to be stored
                // Compute and store PCH flash hash
                switch_spi_flash(SPI_FLASH_PCH);
                calculate_and_save_sha(ufm_data->pit_pch_fw_hash, get_spi_flash_ptr(), PCH_SPI_FLASH_SIZE);

                // Compute and store BMC flash hash
                switch_spi_flash(SPI_FLASH_BMC);
                calculate_and_save_sha(ufm_data->pit_bmc_fw_hash, get_spi_flash_ptr(), BMC_SPI_FLASH_SIZE);

                // Indicate that the firmware hashes have been stored
                set_ufm_status(UFM_STATUS_PIT_HASH_STORED_BIT_MASK);
                log_platform_state(PLATFORM_STATE_PIT_L2_FW_SEALED);

                // Remain in T-1 mode
                never_exit_loop();
            }
        }
    }
}

/**
 * @brief Apply the Protect-in-Transit Level 1 & 2 protections. 
 */ 
static void perform_pit_protection()
{
    perform_pit_l1_check();
    perform_pit_l2_check();
}

#endif /* WHITLEY_INC_PIT_UTILS_H_ */
