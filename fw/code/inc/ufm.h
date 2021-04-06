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
 * @file ufm.h
 * @brief Define the UFM PFR data structure.
 */

#ifndef WHITLEY_INC_UFM_H_
#define WHITLEY_INC_UFM_H_

#include "pfr_sys.h"
#include "pfm.h"
#include "rfnvram.h"

#define UFM_PFR_DATA_SIZE 236
#define UFM_FLASH_PAGE_SIZE 0x1000

#define UFM_MAX_SVN 64

#define UFM_STATUS_LOCK_BIT_MASK                      0b1
#define UFM_STATUS_PROVISIONED_ROOT_KEY_HASH_BIT_MASK 0b10
#define UFM_STATUS_PROVISIONED_PCH_OFFSETS_BIT_MASK   0b100
#define UFM_STATUS_PROVISIONED_BMC_OFFSETS_BIT_MASK   0b1000
#define UFM_STATUS_PROVISIONED_PIT_ID_BIT_MASK  0b10000
#define UFM_STATUS_PIT_L1_ENABLE_BIT_MASK             0b100000
#define UFM_STATUS_PIT_L2_ENABLE_BIT_MASK             0b1000000
#define UFM_STATUS_PIT_HASH_STORED_BIT_MASK           0b10000000
#define UFM_STATUS_PIT_L2_PASSED_BIT_MASK             0b100000000

// If root key hash, pch and bmc offsets are provisioned, we say CPLD has been provisioned
#define UFM_STATUS_PROVISIONED_BIT_MASK               0b000001110

/*!
 * Structure of the data that is provisioned/stored onto on-chip UFM flash
 * 1. UFM Status.  Status bits used for the UFM provisioning and PIT enablement.
 *    - Bit0 = 0: UFM has been locked. Future modification is not allowed.
 *    - Bit1 = 0: UFM has been provisioned with root key hash and BMC/PCH offsets.
 *    - Bit2 = 0: UFM has been provisioned with PCH offsets.
 *    - Bit3 = 0: UFM has been provisioned with BMC offsets.
 *    - Bit4 = 0: UFM has been provisioned with PIT ID
 *    - Bit5 = 0: Enable PIT level 1 protection
 *    - Bit6 = 0: Enable PIT level 2 protection
 *    - Bit7 = 0: PIT Platform firmware (BMC/PCH) hash stored
 *    - Bit8 = 0: PIT L2 protection passed and disabled
 * 2. OEM PFM authentication root key
 * 3. Start address of Active region PFM, Recovery region and Staging region for PCH SPI flash
 * 4. Start address of Active region PFM, Recovery region and Staging region for BMC SPI flash
 * 5. PIT ID
 *    - Identification used in Protect-in-Transit level 1 verification.
 * 6. PIT PCH FW HASH. This is used for PIT Level 2 "FW Sealing" protection.
 * 7. PIT BMC FW HASH. This is used for PIT Level 2 "FW Sealing" protection.
 * 8. PFR SVN enforcement policies.
 *    - Track the minimum SVN that is acceptable for an update
 *    - This is a 64-bit bitfield. This bitfield translate to a number within 0-64, inclusive.
 *    - 0xFFFFFFFE 0xFFFFFFFF: min SVN is 1
 *    - 0xFFFFFFF0 0xFFFFFFFF: min SVN is 4
 * 9. CSK cancellation policies
 *    - Track the CSK keys that are cancelled.
 *    - This is a 128-bit bitmap. The least significant bit representing CSK Key ID 0,
 *      while the most significant bit represent CSK Key ID 127.
 *    - 0101_1111_ ... _1111: CSK Key ID 0 and 2 have been cancelled.
 *    - 1111_1111_ ... _1110: CSK Key ID 127 has been cancelled.
 */
typedef struct
{
    // UFM Status
    alt_u32 ufm_status;
    // OEM PFM authentication root key
    alt_u32 root_key_hash[PFR_CRYPTO_LENGTH / 4];
    // Start address of Active region PFM, Recovery region and Staging region for PCH SPI flash
    alt_u32 pch_active_pfm;
    alt_u32 pch_recovery_region;
    alt_u32 pch_staging_region;
    // Start address of Active region PFM, Recovery region and Staging region for BMC SPI flash
    alt_u32 bmc_active_pfm;
    alt_u32 bmc_recovery_region;
    alt_u32 bmc_staging_region;
    // PIT ID
    alt_u32 pit_id[RFNVRAM_PIT_ID_LENGTH / 4];
    // PIT PCH FW HASH
    alt_u32 pit_pch_fw_hash[PFR_CRYPTO_LENGTH / 4];
    // PIT BMC FW HASH
    alt_u32 pit_bmc_fw_hash[PFR_CRYPTO_LENGTH / 4];
    // PFR SVN enforcement policies
    alt_u32 svn_policy_cpld[2];
    alt_u32 svn_policy_pch[2];
    alt_u32 svn_policy_bmc[2];
    // CSK cancellation policies
    alt_u32 csk_cancel_pch_pfm[4];
    alt_u32 csk_cancel_pch_update_cap[4];
    alt_u32 csk_cancel_bmc_pfm[4];
    alt_u32 csk_cancel_bmc_update_cap[4];
    alt_u32 csk_cancel_cpld_update_cap[4];
} UFM_PFR_DATA;

/*!
Define SVN Policy type in UFM
 */
typedef enum
{
    UFM_SVN_POLICY_CPLD = 0x1,
    UFM_SVN_POLICY_PCH = 0x2,
    UFM_SVN_POLICY_BMC = 0x3,
} UFM_SVN_POLICY_TYPE_ENUM;

#endif /* WHITLEY_INC_UFM_H_ */
