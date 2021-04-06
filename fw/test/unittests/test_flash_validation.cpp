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

class FlashValidationTest : public testing::Test
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

TEST_F(FlashValidationTest, test_validate_full_pch_image)
{
    // Load the PCH PFR image to the SPI flash mock
    SYSTEM_MOCK::get()->load_to_flash(m_spi_flash_in_use, FULL_PFR_IMAGE_PCH_FILE, FULL_PFR_IMAGE_PCH_FILE_SIZE);

    alt_u32* active_pfm_ptr = get_spi_active_pfm_ptr(SPI_FLASH_PCH);
    alt_u32* recovery_region_ptr = get_spi_recovery_region_ptr(SPI_FLASH_PCH);

    // Verify the signature and content of the active section PFM
    alt_u32 is_active_valid = is_active_region_valid(active_pfm_ptr);

    // Verify the signature of the recovery section capsule
    alt_u32 is_recovery_valid = is_capsule_valid(recovery_region_ptr);

    EXPECT_TRUE(is_active_valid);
    EXPECT_TRUE(is_recovery_valid);
}

TEST_F(FlashValidationTest, test_validate_full_bmc_image)
{
    // Load the BMC PFR image to the SPI flash mock
    SYSTEM_MOCK::get()->load_to_flash(m_spi_flash_in_use, FULL_PFR_IMAGE_BMC_FILE, FULL_PFR_IMAGE_BMC_FILE_SIZE);

    alt_u32* active_pfm_ptr = get_spi_active_pfm_ptr(SPI_FLASH_BMC);
    alt_u32* recovery_region_ptr = get_spi_recovery_region_ptr(SPI_FLASH_BMC);

    // Verify the signature and content of the active section PFM
    alt_u32 is_active_valid = is_active_region_valid(active_pfm_ptr);

    // Verify the signature of the recovery section capsule
    alt_u32 is_recovery_valid = is_capsule_valid(recovery_region_ptr);

    EXPECT_TRUE(is_active_valid);
    EXPECT_TRUE(is_recovery_valid);
}
