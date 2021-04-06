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
#include <iostream>

// Include the GTest headers
#include "gtest_headers.h"

// Include the SYSTEM MOCK and PFR headers
#include "ut_nios_wrapper.h"
#include "testdata_info.h"


class DecompressionFlowTest : public testing::Test
{
public:
    alt_u32* m_flash_x86_ptr = nullptr;

    // For simplicity, use PCH flash for all tests.
    SPI_FLASH_TYPE_ENUM m_spi_flash_in_use = SPI_FLASH_PCH;

    virtual void SetUp()
    {
        SYSTEM_MOCK* sys = SYSTEM_MOCK::get();
        sys->reset();

        // Prepare SPI flash
        sys->reset_spi_flash(m_spi_flash_in_use);
        m_flash_x86_ptr = sys->get_x86_ptr_to_spi_flash(m_spi_flash_in_use);
        switch_spi_flash(m_spi_flash_in_use);

        // Perform provisioning
        SYSTEM_MOCK::get()->provision_ufm_data(UFM_PFR_DATA_EXAMPLE_KEY_FILE);
    }

    virtual void TearDown() {}
};

TEST_F(DecompressionFlowTest, test_copy_region_from_capsule_pch)
{
    // Load the signed capsule to recovery region
    alt_u32 recovery_offset = PCH_RECOVERY_REGION_ADDR;
    SYSTEM_MOCK::get()->load_to_flash(m_spi_flash_in_use, SIGNED_CAPSULE_PCH_FILE,
            SIGNED_CAPSULE_PCH_FILE_SIZE, recovery_offset);
    alt_u32* signed_capsule = incr_alt_u32_ptr(m_flash_x86_ptr, recovery_offset);

    // Load the entire image locally
    alt_u32 *full_image = new alt_u32[FULL_PFR_IMAGE_PCH_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(FULL_PFR_IMAGE_PCH_FILE, full_image);

    // Make sure that this SPI region was cleared.
    for (alt_u32 word_i = (PCH_SPI_REGION2_START_ADDR >> 2); word_i < (PCH_SPI_REGION2_END_ADDR >> 2); word_i++)
    {
        EXPECT_EQ(m_flash_x86_ptr[word_i], (alt_u32) 0xffffffff);
    }

    // Perform the decompression
    decompress_spi_region_from_capsule(PCH_SPI_REGION2_START_ADDR, PCH_SPI_REGION2_END_ADDR, signed_capsule);

    // Compare against expected data
    for (alt_u32 word_i = (PCH_SPI_REGION2_START_ADDR >> 2); word_i < (PCH_SPI_REGION2_END_ADDR >> 2); word_i++)
    {
        EXPECT_EQ(full_image[word_i], m_flash_x86_ptr[word_i]);
    }

    delete[] full_image;
}

TEST_F(DecompressionFlowTest, test_authentication_after_decompressing_bmc_capsule)
{
    // Load the signed capsule to recovery region
    alt_u32 recovery_offset = BMC_RECOVERY_REGION_ADDR;
    SYSTEM_MOCK::get()->load_to_flash(m_spi_flash_in_use, SIGNED_CAPSULE_BMC_FILE,
            SIGNED_CAPSULE_BMC_FILE_SIZE, recovery_offset);

    // Perform the decompression
    alt_u32* active_pfm = get_spi_active_pfm_ptr(SPI_FLASH_BMC);
    alt_u32* signed_capsule = get_spi_recovery_region_ptr(SPI_FLASH_BMC);
    decompress_capsule(signed_capsule, SPI_FLASH_BMC, DECOMPRESSION_STATIC_AND_DYNAMIC_REGIONS_MASK);

    // Authenticate the active region after decompression
    alt_u32 is_active_valid = is_active_region_valid(active_pfm);
    EXPECT_TRUE(is_active_valid);
}

TEST_F(DecompressionFlowTest, test_authentication_after_decompressing_pch_capsule)
{
    // Load the signed capsule to recovery region
    alt_u32 recovery_offset = PCH_RECOVERY_REGION_ADDR;
    SYSTEM_MOCK::get()->load_to_flash(m_spi_flash_in_use, SIGNED_CAPSULE_PCH_FILE,
            SIGNED_CAPSULE_PCH_FILE_SIZE, recovery_offset);

    // Perform the decompression
    alt_u32* active_pfm = get_spi_active_pfm_ptr(SPI_FLASH_PCH);
    alt_u32* signed_capsule = get_spi_recovery_region_ptr(SPI_FLASH_PCH);
    decompress_capsule(signed_capsule, SPI_FLASH_PCH, DECOMPRESSION_STATIC_AND_DYNAMIC_REGIONS_MASK);

    // Authenticate the active region after decompression
    alt_u32 is_active_valid = is_active_region_valid(active_pfm);
    EXPECT_TRUE(is_active_valid);
}
