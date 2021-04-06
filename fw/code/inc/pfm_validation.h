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
 * @file pfm_validation.h
 * @brief Perform validation on PFM, including the PFM SPI region definition and SMBus rule definition.
 */

#ifndef WHITLEY_INC_PFM_VALIDATION_H_
#define WHITLEY_INC_PFM_VALIDATION_H_

// Always include pfr_sys.h first
#include "pfr_sys.h"

#include "authentication.h"
#include "crypto.h"
#include "pfm.h"
#include "pfm_utils.h"
#include "pfr_pointers.h"

/**
 * @brief Check if the SMBus device address in a rule definition is valid.
 * The address is valid when it matches the I2C address for the given bus ID
 * and rule ID based on gen_smbus_relay_config.h
 *
 * @return 1 if success else 0
 */
static PFR_ALT_INLINE alt_u32 PFR_ALT_ALWAYS_INLINE is_device_addr_valid(PFM_SMBUS_RULE_DEF* rule_def)
{
    return rule_def->device_addr == smbus_device_addr[rule_def->bus_id - 1][rule_def->rule_id - 1];
}

/**
 * @brief Perform validation on a PFM SMBus rule definition
 *
 * @return 1 if success else 0
 */
static alt_u32 is_smbus_rule_valid(PFM_SMBUS_RULE_DEF* rule_def)
{
    // Check rule ID and Bus ID
    if ((rule_def->rule_id > MAX_I2C_ADDRESSES_PER_RELAY) || (rule_def->bus_id > NUM_RELAYS))
    {
        return 0;
    }

    // Check device address
    return is_device_addr_valid(rule_def);
}

/**
 * @brief Perform validation on a PFM defined SPI region
 *
 * @return 1 if success else 0
 */
static alt_u32 is_spi_region_valid(PFM_SPI_REGION_DEF* region_def)
{
    // Only check hash for static region
    if ((region_def->hash_algorithm & PFM_HASH_ALGO_SHA256_MASK) &&
            is_spi_region_static(region_def))
    {
        return verify_sha(region_def->region_hash,
                get_spi_flash_ptr_with_offset(region_def->start_addr),
                region_def->end_addr - region_def->start_addr);
    }
    return 1;
}

/**
 * @brief Iterate through PFM body to validate SPI region definition and SMBus rule definition.
 *
 * @return 1 if success else 0
 */
static alt_u32 is_pfm_body_valid(alt_u32* pfm_body_ptr)
{
    // Go through the PFM Body
    while (1)
    {
        alt_u8 def_type = *((alt_u8*) pfm_body_ptr);
        if (def_type == SMBUS_RULE_DEF_TYPE)
        {
            PFM_SMBUS_RULE_DEF* rule_def = (PFM_SMBUS_RULE_DEF*) pfm_body_ptr;
            if (!is_smbus_rule_valid(rule_def))
            {
                return 0;
            }
            pfm_body_ptr = incr_alt_u32_ptr(pfm_body_ptr, SMBUS_RULE_DEF_SIZE);
        }
        else if (def_type == SPI_REGION_DEF_TYPE)
        {
            PFM_SPI_REGION_DEF* region_def = (PFM_SPI_REGION_DEF*) pfm_body_ptr;
            if (!is_spi_region_valid(region_def))
            {
                return 0;
            }
            pfm_body_ptr = get_end_of_spi_region_def(region_def);
        }
        else
        {
            // Break when there is no more region/rule definition in PFM body
            break;
        }
    }

    return 1;
}

/**
 * @brief Perform validation on a PFM data
 *
 * @param pfm_ptr a pointer to the start of a PFM data
 * @return 1 if success else 0
 */
static alt_u32 is_pfm_valid(PFM* pfm)
{
    if (pfm->tag == PFM_MAGIC)
    {
        // Iterate through PFM SPI region and SMBus rule definitions and validate them
        return is_pfm_body_valid(pfm->pfm_body);
    }

    return 0;
}

/**
 * @brief Perform validation on the active region PFM.
 * First, it verifies the signature of the PFM. Then, it verifies
 * the content (e.g. SPI region and SMBus rule definitions) of the PFM.
 *
 * @param active_addr start address of an active region
 * @return 1 if the active region is valid; 0, otherwise.
 */
static alt_u32 is_active_region_valid(alt_u32* active_addr)
{
    // Verify the signature of the PFM first, then SPI region definitions and other content in PFM.
    return is_signature_valid((KCH_SIGNATURE*) active_addr) &&
            is_pfm_valid((PFM*) incr_alt_u32_ptr(active_addr, SIGNATURE_SIZE));
}


#endif /* WHITLEY_INC_PFM_VALIDATION_H_ */
