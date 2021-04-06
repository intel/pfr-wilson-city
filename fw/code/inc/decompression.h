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
 * @file decompression.h
 * @brief Decompression algorithm to extract BMC/PCH firmware from recovery/update capsule.
 */

#ifndef WHITLEY_INC_DECOMPRESSION_H
#define WHITLEY_INC_DECOMPRESSION_H

// Always include pfr_sys.h first
#include "pfr_sys.h"

#include "keychain_utils.h"
#include "pbc.h"
#include "pbc_utils.h"
#include "pfm_utils.h"
#include "pfr_pointers.h"
#include "spi_rw_utils.h"
#include "ufm_utils.h"

/**
 * Define type of decompression action. 
 * If it's a static region only decompression, Nios only decompress SPI region, that
 * is read allowed but not write allowed, in PFM definition.
 * If it's a dynamic region only decompression, Nios only decompress SPI region, that
 * is both read allowed and write allowed, in PFM definition.
 * The last option is to recover both static and dynamic regions. 
 */
typedef enum
{
    DECOMPRESSION_STATIC_REGIONS_MASK             = 0b1,
    DECOMPRESSION_DYNAMIC_REGIONS_MASK            = 0b10,
    DECOMPRESSION_STATIC_AND_DYNAMIC_REGIONS_MASK = 0b11,
} DECOMPRESSION_TYPE_MASK_ENUM;

/**
 * @brief Decompress a SPI region from the a signed firmware update capsule.
 * First, Nios firmware find the bit that represents the first page of this 
 * SPI region in the compression bitmap and active bitmap. A pointer is used 
 * to skip pages in the compressed payload along the process. 
 *
 * Once Nios firmware reaches the correct bit, it checks the value of that bit. 
 * If the bit is 1 in the active bitmap, then Nios would erase that page. If the 
 * bit is 1 in the compression bitmap, then Nios would copy that page from the 
 * compressed payload and overwrite the corresponding page of this SPI region. 
 * This process ends when Nios finishes with the last page of this SPI region.
 * 
 * For every 8 pages it processed in compressed payload, Nios firmware would
 * pet the hardware watchdog timer to prevent timer expiry.
 * 
 * @param region_start_addr Start address of the SPI region
 * @param region_end_addr End address of the SPI region
 * @param signed_capsule pointer to the start of a signed firmware update capsule
 */
static void decompress_spi_region_from_capsule(
        alt_u32 region_start_addr, alt_u32 region_end_addr, alt_u32* signed_capsule)
{
    alt_u32 region_start_bit = region_start_addr / PBC_EXPECTED_PAGE_SIZE;
    alt_u32 region_end_bit = region_end_addr / PBC_EXPECTED_PAGE_SIZE;

    // Destination address to be updated in SPI flash
    alt_u32 dest_addr = region_start_addr;

    // Pointers to various places in the capsule
    PBC_HEADER* pbc = get_pbc_ptr_from_signed_capsule(signed_capsule);
    alt_u32* src_ptr = get_compressed_payload(pbc);
    alt_u8* active_bitmap = (alt_u8*) get_active_bitmap(pbc);
    alt_u8* comp_bitmap = (alt_u8*) get_compression_bitmap(pbc);

    /*
     * Erase
     * Perform erase on the target SPI region first, according to the Active Bitmap.
     * Coalesce the pages where possible to erase them at once. This allows 64KB Erase to be used.
     */
    // This records the first bit in this chunk of pages to be erased
    alt_u32 erase_start_bit = 0xffffffff;
    for (alt_u32 bit_in_bitmap = region_start_bit; bit_in_bitmap < region_end_bit; bit_in_bitmap++)
    {
        alt_u8 active_bitmap_byte = active_bitmap[bit_in_bitmap >> 3];
        if (active_bitmap_byte & (1 << (7 - (bit_in_bitmap % 8))))
        {
            // Erase this page
            if (erase_start_bit == 0xffffffff)
            {
                erase_start_bit = bit_in_bitmap;
            }
        }
        else
        {
            // Don't erase this page
            if (erase_start_bit != 0xffffffff)
            {
                // Pages [erase_start_bit, bit_in_bitmap) needs to be erased
                alt_u32 erase_bits = bit_in_bitmap - erase_start_bit;
                if (erase_bits)
                {
                    erase_spi_region(erase_start_bit * PBC_EXPECTED_PAGE_SIZE, erase_bits * PBC_EXPECTED_PAGE_SIZE);
                    erase_start_bit = 0xffffffff;
                }
            }
        }
    }
    if (erase_start_bit != 0xffffffff)
    {
        // Erase to the end of the SPI region
        alt_u32 erase_start_addr = erase_start_bit * PBC_EXPECTED_PAGE_SIZE;
        erase_spi_region(erase_start_addr, region_end_addr - erase_start_addr);
    }

    /*
     * Copy
     * Perform copy from compressed payload to the target SPI region, according to the Compression Bitmap.
     * Nios go through the bitmap bit by bit and copy page by page.
     * Nios starts from bit 0 of the bitmap. If any bit, before the bit representing the start of
     * this SPI region, is set, Nios will skip that page in the compressed payload.
     */
    alt_u32 cur_bit = 0;
    while(cur_bit < region_end_bit)
    {
        // Read a byte in compression bitmap and active bitmap
        alt_u8 comp_bitmap_byte = *comp_bitmap;
        comp_bitmap++;

        // Process 8 bits (representing 8 pages)
        for (alt_u8 bit_mask = 0b10000000; bit_mask > 0; bit_mask >>= 1)
        {
            alt_u32 copy_this_page = comp_bitmap_byte & bit_mask;

            // The current page is part of the SPI region
            if ((region_start_bit <= cur_bit) && (cur_bit < region_end_bit))
            {
                if (copy_this_page)
                {
                    // Value of '1' indicates a copy operation is needed. Perform the copy.
                    alt_u32_memcpy(get_spi_flash_ptr_with_offset(dest_addr), src_ptr, PBC_EXPECTED_PAGE_SIZE);
                    // Wait for the writes to complete, before moving on to next page
                    poll_status_reg_done();
                }
                // Done with this page. Increment for updating the next page.
                dest_addr += PBC_EXPECTED_PAGE_SIZE;
            }

            if (copy_this_page)
            {
                // This page has been processed, moving to the next page in the compressed payload
                src_ptr = incr_alt_u32_ptr(src_ptr, PBC_EXPECTED_PAGE_SIZE);
            }

            // Move on to the next page (i.e. next bit in the bitmap)
            cur_bit++;
        }

        // Reset HW timer after 8 SPI pages have been processed
        reset_hw_watchdog();
    }
}

/**
 * @brief Decompress some types of SPI regions from a firmware update capsule.
 *
 * At this point, the capsule has been authenticated. Hence, Nios trusts the user settings in capsule. However,
 * if user has incorrect settings in PFM or Compression Structure Definition, unintended overwrite can
 * happen. For example, if a write-allowed SPI region includes a part of the recovery region, then
 * a dynamic firmware update would corrupt the recovery firmware. A more serious example would be: If
 * the staging region is not defined by just one SPI region definition, then a request to decompress
 * dynamic SPI regions would lead Nios to erase staging region while reading from it.
 *
 * Requirements on PFM and Compression Structure Definition are included in the MAS. When there's more
 * code space available, some of these requirements can be turned into checks in T-1 authentication.
 *
 * @param signed_capsule pointer to the start of a signed firmware update capsule
 * @param spi_flash_type indicate BMC or PCH SPI flash device
 * @param decomp_type indicate the type of this decompression action. This can be static region only
 * decompression, dynamic region only decompression, or decompression for both static and dynamic regions. 
 */
static void decompress_capsule(
        alt_u32* signed_capsule, SPI_FLASH_TYPE_ENUM spi_flash_type, DECOMPRESSION_TYPE_MASK_ENUM decomp_type)
{
    // Get addresses of active pfm and staging region
    alt_u32 active_pfm_addr = get_ufm_pfr_data()->bmc_active_pfm;
    alt_u32 staging_region_addr = get_ufm_pfr_data()->bmc_staging_region;
    if (spi_flash_type == SPI_FLASH_PCH)
    {
        active_pfm_addr = get_ufm_pfr_data()->pch_active_pfm;
        staging_region_addr = get_ufm_pfr_data()->pch_staging_region;
    }

    // Iterate through all the SPI region definitions in PFM body
    alt_u32* capsule_pfm_body = get_capsule_pfm(signed_capsule)->pfm_body;
    while (1)
    {
        alt_u8 def_type = *((alt_u8*) capsule_pfm_body);
        if (def_type == SMBUS_RULE_DEF_TYPE)
        {
            // Skip the SMBus rule definition
            capsule_pfm_body = incr_alt_u32_ptr(capsule_pfm_body, SMBUS_RULE_DEF_SIZE);
        }
        else if (def_type == SPI_REGION_DEF_TYPE)
        {
            PFM_SPI_REGION_DEF* region_def = (PFM_SPI_REGION_DEF*) capsule_pfm_body;

            if (is_spi_region_static(region_def))
            {
                if (decomp_type & DECOMPRESSION_STATIC_REGIONS_MASK)
                {
                    // Recover all regions that do not allow write
                    decompress_spi_region_from_capsule(region_def->start_addr, region_def->end_addr, signed_capsule);
                }
            }
            else if (is_spi_region_dynamic(region_def) && region_def->start_addr != staging_region_addr)
            {
                // This SPI region is a dynamic region and not staging region
                if (decomp_type & DECOMPRESSION_DYNAMIC_REGIONS_MASK)
                {
                    // Recover all regions that allows write
                    decompress_spi_region_from_capsule(region_def->start_addr, region_def->end_addr, signed_capsule);
                }
            }

            // Increment the pointer in PFM body appropriately
            capsule_pfm_body = get_end_of_spi_region_def(region_def);
        }
        else
        {
            // Break when there is no more region/rule definition in PFM body
            break;
        }
    }

    // If this decompression involves static region, also copy the PFM (in capsule) to replace the active PFM
    if (decomp_type & DECOMPRESSION_STATIC_REGIONS_MASK)
    {
        // Erase the current PFM SPI region first
        erase_spi_region(active_pfm_addr, SIGNED_PFM_MAX_SIZE);

        // Copy the capsule PFM over
        alt_u32* signed_capsule_pfm = incr_alt_u32_ptr(signed_capsule, SIGNATURE_SIZE);
        alt_u32 nbytes = get_signed_payload_size(signed_capsule_pfm);
        alt_u32_memcpy(get_spi_flash_ptr_with_offset(active_pfm_addr), signed_capsule_pfm, nbytes);
    }
}

#endif /* WHITLEY_INC_DECOMPRESSION_H */
