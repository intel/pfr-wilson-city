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
 * @file pfm_utils.h
 * @brief Responsible for extracting and applying PFM rules.
 */

#ifndef WHITLEY_INC_PFM_UTILS_H_
#define WHITLEY_INC_PFM_UTILS_H_

// Always include pfr_sys.h first
#include "pfr_sys.h"

#include "keychain.h"
#include "pfm.h"
#include "smbus_relay_utils.h"
#include "spi_ctrl_utils.h"
#include "spi_flash_state.h"
#include "spi_rw_utils.h"
#include "ufm_utils.h"
#include "utils.h"


/**
 * @brief Return the pointer to the start of the PFM of the active firmware from a specific flash device.
 * This function first retrieves the offset of signed active PFM from UFM (assuming it has been provisioned)
 * Then, it creates a pointer to the flash AvMM location. After skipping over the signature (Block 0/1),
 * the pointer is returned.
 *
 * @param spi_flash_type indicates BMC or PCH flash
 */
static PFM* get_active_pfm(SPI_FLASH_TYPE_ENUM spi_flash_type)
{
    alt_u32* active_signed_pfm_ptr = get_spi_active_pfm_ptr(spi_flash_type);
    return (PFM*) incr_alt_u32_ptr(active_signed_pfm_ptr, SIGNATURE_SIZE);
}

/**
 * @brief Return a pointer to the start address of PFM structure in a signed firmware update capsule.
 */
static PFR_ALT_INLINE PFM* PFR_ALT_ALWAYS_INLINE get_capsule_pfm(alt_u32* signed_capsule)
{
    // Skip capsule signature and PFM signature
    return (PFM*) incr_alt_u32_ptr(signed_capsule, SIGNATURE_SIZE * 2);
}

/**
 * @brief Given a SPI region definition, return the pointer to the first word after the definition.
 * A SPI region definition has variable length. This function decides how many bytes to increment by
 * looking at whether there's a hash present.
 *
 * @param region_def a SPI region definition
 * @return the pointer to the first word after the SPI region definition
 */
static alt_u32* get_end_of_spi_region_def(PFM_SPI_REGION_DEF* region_def)
{
    if (region_def->hash_algorithm == 0)
    {
        return incr_alt_u32_ptr((alt_u32*) region_def, SPI_REGION_DEF_MIN_SIZE);
    }
    return (alt_u32*) (region_def + 1);
}

/**
 * @brief Check if the given SPI region is a static region.
 * A static region is a read-only region for PCH/BMC firmware.
 */
static alt_u32 is_spi_region_static(PFM_SPI_REGION_DEF* spi_region_def)
{
    return ((spi_region_def->protection_mask & SPI_REGION_PROTECT_MASK_READ_ALLOWED) &&
            ((spi_region_def->protection_mask & SPI_REGION_PROTECT_MASK_WRITE_ALLOWED) == 0));
}

/**
 * @brief Check if the given SPI region is a dynamic region.
 * A dynamic region is a Read & Write allowed region for PCH/BMC firmware.
 */
static alt_u32 is_spi_region_dynamic(PFM_SPI_REGION_DEF* spi_region_def)
{
    return ((spi_region_def->protection_mask & SPI_REGION_PROTECT_MASK_READ_ALLOWED) &&
            (spi_region_def->protection_mask & SPI_REGION_PROTECT_MASK_WRITE_ALLOWED));
}

/**
 * @brief Go through the PFM body to apply SPI write protection and SMBus rule.
 *
 * The entire SPI flash must be covered by the PFM, as required in architecture specification.
 * When validating PFM, Nios has already verified that all SPI regions does not overlap and addresses
 * are in ascending order. This is needed for correctness of this function.
 *
 * Writing 0s to appropriate location in the Write Enable Memory of the SPI control block would turn off write access.
 * Some SPI regions may share the same word in the write enable memory. Write enable memory is also read-only for Nios.
 * Therefore, Nios must process SPI regions definition in order to collect all the write protection rules in the same word,
 * before committing to the write enable memory.
 *
 * Nios currently only applies SMBus filtering rules from BMC PFM. SMBus filtering rules from PCH PFM are ignored.
 *
 * @see apply_smbus_rule
 */
static void apply_spi_write_protection_and_smbus_rules(SPI_FLASH_TYPE_ENUM spi_flash_type)
{
    alt_u32* we_mem_ptr = __IO_CALC_ADDRESS_NATIVE_ALT_U32(U_SPI_FILTER_BMC_WE_AVMM_BRIDGE_BASE, 0);
    if (spi_flash_type == SPI_FLASH_PCH)
    {
        we_mem_ptr = __IO_CALC_ADDRESS_NATIVE_ALT_U32(U_SPI_FILTER_PCH_WE_AVMM_BRIDGE_BASE, 0);
    }

    // Two or more SPI regions may share the same word in the write enable memory
    // Use these variables to track where Nios is in the write enable memory and the content of a shared word there.
    alt_u32 write_spi_rule_word = 0;
    alt_u32 write_spi_rule_word_pos = 0;

    // Read the PFM
    PFM* active_pfm = get_active_pfm(spi_flash_type);
    alt_u32* pfm_body_ptr = active_pfm->pfm_body;

    // Go through the PFM Body
    while (1)
    {
        alt_u8 def_type = *((alt_u8*) pfm_body_ptr);
        if (def_type == SMBUS_RULE_DEF_TYPE)
        {
            PFM_SMBUS_RULE_DEF* rule_def = (PFM_SMBUS_RULE_DEF*) pfm_body_ptr;

            // Only Apply SMBus rule from BMC PFM
            if (spi_flash_type == SPI_FLASH_BMC)
            {
                apply_smbus_rule(rule_def);
            }

            // Move to next SMBus/SPI filter rule
            pfm_body_ptr = incr_alt_u32_ptr(pfm_body_ptr, SMBUS_RULE_DEF_SIZE);
        }
        else if (def_type == SPI_REGION_DEF_TYPE)
        {
            PFM_SPI_REGION_DEF* region_def = (PFM_SPI_REGION_DEF*) pfm_body_ptr;
            alt_u32 start_spi_addr = region_def->start_addr;
            alt_u32 end_spi_addr = region_def->end_addr;

            alt_u32 allow_write = 0;
            if (region_def->protection_mask & SPI_REGION_PROTECT_MASK_WRITE_ALLOWED)
            {
                allow_write = 1;
            }

            // 14 bits to get to 16kB chunks, 5 more bits because we have 32 (=2^5) bits in each word in the memory
            alt_u32 start_word_pos = start_spi_addr >> (14 + 5);
            // Grab the 5 address bits that tell us the starting bit position within the 32 bit word
            alt_u32 start_bit_pos = (start_spi_addr >> 14) & 0x0000001f;
            alt_u32 end_word_pos = end_spi_addr >> (14 + 5);
            alt_u32 end_bit_pos = (end_spi_addr >> 14) & 0x0000001f;

            for (alt_u32 iter = start_word_pos; iter <= end_word_pos; iter++)
            {
                alt_u32 word_pos_in_between_start_and_end = write_spi_rule_word_pos != start_word_pos && write_spi_rule_word_pos != end_word_pos;

                // Track if Nios should commit the collected rule to write enable memory
                // If the word is not fully constructed, Nios should not commit the rule.
                alt_u32 commit_to_we_mem = word_pos_in_between_start_and_end;

                if (word_pos_in_between_start_and_end)
                {
                    // Except for the first and last words, we can set/clear all bits in the word inside the region
                    write_spi_rule_word = 0;
                    if (allow_write)
                    {
                        write_spi_rule_word = 0xFFFFFFFF;
                    }
                }
                else
                {
                    // Special handling for first and last words (including the case where first word == last word)
                    alt_u32 first_bit_pos = start_bit_pos;
                    alt_u32 last_bit_pos = end_bit_pos;
                    if (write_spi_rule_word_pos != start_word_pos)
                    {
                        first_bit_pos = 0;

                        // Re-initalize to all 0s.
                        write_spi_rule_word = 0;
                    }
                    if (write_spi_rule_word_pos != end_word_pos)
                    {
                        // In this case, we want to write 0s till the end of the word (i.e. including bit[31])
                        last_bit_pos = 32;
                    }

                    if (allow_write)
                    {
                        for (alt_u32 bit_pos = first_bit_pos; bit_pos < last_bit_pos; bit_pos++)
                        {
                            // This 16KB page is not writable. Write a 0 to this bit position.
                            write_spi_rule_word |= (0b1 << bit_pos);
                        }
                    }

                    // Only write to write enable memory when the full word is collected.
                    commit_to_we_mem = last_bit_pos == 32;
                }

                // Save the collected rule and move on to the next 32 16KB pages
                if (commit_to_we_mem)
                {
                    // Write the collected rule to write enable memory
                    IOWR(we_mem_ptr, write_spi_rule_word_pos, write_spi_rule_word);

                    // Move to the next word in write enable memory
                    write_spi_rule_word_pos++;
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
}

#endif /* WHITLEY_INC_PFM_UTILS_H_ */
