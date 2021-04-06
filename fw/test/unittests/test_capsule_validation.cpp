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

class CapsuleValidationTest : public testing::Test
{
public:
    alt_u32* m_flash_x86_ptr = nullptr;

    // For simplicity, use PCH flash for all tests.
    SPI_FLASH_TYPE_ENUM m_spi_flash_in_use = SPI_FLASH_PCH;

    virtual void SetUp()
    {
        SYSTEM_MOCK* sys = SYSTEM_MOCK::get();
        sys->reset();

        sys->reset_spi_flash(m_spi_flash_in_use);
        m_flash_x86_ptr = sys->get_x86_ptr_to_spi_flash(m_spi_flash_in_use);
        switch_spi_flash(m_spi_flash_in_use);

        // Perform provisioning
        SYSTEM_MOCK::get()->provision_ufm_data(UFM_PFR_DATA_EXAMPLE_KEY_FILE);
    }

    virtual void TearDown() {}
};

TEST_F(CapsuleValidationTest, test_sanity)
{
    // Just need the first two signature blocks
    SYSTEM_MOCK::get()->load_to_flash(m_spi_flash_in_use, SIGNED_CAPSULE_BMC_FILE, SIGNATURE_SIZE * 2);

    // Check the Block 0 Magic number
    EXPECT_EQ(*m_flash_x86_ptr, 0xB6EAFD19);

    alt_u8* m_bmc_flash_x86_u8_ptr = (alt_u8*) m_flash_x86_ptr;
    EXPECT_EQ(*m_bmc_flash_x86_u8_ptr, (alt_u8) 0x19);
    EXPECT_EQ(*(m_bmc_flash_x86_u8_ptr + 1), (alt_u8) 0xfd);
    EXPECT_EQ(*(m_bmc_flash_x86_u8_ptr + 2), (alt_u8) 0xea);
    EXPECT_EQ(*(m_bmc_flash_x86_u8_ptr + 3), (alt_u8) 0xb6);

    m_bmc_flash_x86_u8_ptr += 0x400;
    EXPECT_EQ(*m_bmc_flash_x86_u8_ptr, (alt_u8) 0x19);
    EXPECT_EQ(*(m_bmc_flash_x86_u8_ptr + 1), (alt_u8) 0xfd);
    EXPECT_EQ(*(m_bmc_flash_x86_u8_ptr + 2), (alt_u8) 0xea);
    EXPECT_EQ(*(m_bmc_flash_x86_u8_ptr + 3), (alt_u8) 0xb6);
}

TEST_F(CapsuleValidationTest, test_is_pbc_valid)
{
    SYSTEM_MOCK::get()->load_to_flash(m_spi_flash_in_use, SIGNED_CAPSULE_BMC_FILE, SIGNED_CAPSULE_BMC_FILE_SIZE);

    PBC_HEADER* pbc = get_pbc_ptr_from_signed_capsule(m_flash_x86_ptr);
    EXPECT_TRUE(is_pbc_valid(pbc));
}

TEST_F(CapsuleValidationTest, test_authenticate_signed_capsule_bmc)
{
    SYSTEM_MOCK::get()->load_to_flash(m_spi_flash_in_use, SIGNED_CAPSULE_BMC_FILE, SIGNED_CAPSULE_BMC_FILE_SIZE);
    EXPECT_TRUE(are_signatures_in_capsule_valid(m_flash_x86_ptr));
}

TEST_F(CapsuleValidationTest, test_is_capsule_valid_with_bmc)
{
    SYSTEM_MOCK::get()->load_to_flash(m_spi_flash_in_use, SIGNED_CAPSULE_BMC_FILE, SIGNED_CAPSULE_BMC_FILE_SIZE);
    EXPECT_TRUE(is_capsule_valid(m_flash_x86_ptr));
}

TEST_F(CapsuleValidationTest, test_is_capsule_valid_with_pch)
{
    SYSTEM_MOCK::get()->load_to_flash(m_spi_flash_in_use, SIGNED_CAPSULE_PCH_FILE, SIGNED_CAPSULE_PCH_FILE_SIZE);
    EXPECT_TRUE(is_capsule_valid(m_flash_x86_ptr));
}

TEST_F(CapsuleValidationTest, test_authenticate_pch_fw_capsule_with_bad_tag_in_pbc_header)
{
    alt_u32* pch_update_capsule = new alt_u32[SIGNED_CAPSULE_PCH_WITH_BAD_TAG_IN_PBC_HEADER_FILE_SIZE / 4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(SIGNED_CAPSULE_PCH_WITH_BAD_TAG_IN_PBC_HEADER_FILE, pch_update_capsule);

    EXPECT_FALSE(is_capsule_valid(pch_update_capsule));

    delete[] pch_update_capsule;
}

TEST_F(CapsuleValidationTest, test_authenticate_pch_fw_capsule_with_bad_version_in_pbc_header)
{
    alt_u32* pch_update_capsule = new alt_u32[SIGNED_CAPSULE_PCH_WITH_BAD_VERSION_IN_PBC_HEADER_FILE_SIZE / 4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(SIGNED_CAPSULE_PCH_WITH_BAD_VERSION_IN_PBC_HEADER_FILE, pch_update_capsule);

    EXPECT_FALSE(is_capsule_valid(pch_update_capsule));

    delete[] pch_update_capsule;
}

TEST_F(CapsuleValidationTest, test_authenticate_pch_fw_capsule_with_bad_bitmap_nbit_in_pbc_header)
{
    alt_u32* pch_update_capsule = new alt_u32[SIGNED_CAPSULE_PCH_WITH_BAD_BITMAP_NBIT_IN_PBC_HEADER_FILE_SIZE / 4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(SIGNED_CAPSULE_PCH_WITH_BAD_BITMAP_NBIT_IN_PBC_HEADER_FILE, pch_update_capsule);

    EXPECT_FALSE(is_capsule_valid(pch_update_capsule));

    delete[] pch_update_capsule;
}

TEST_F(CapsuleValidationTest, test_authenticate_pch_fw_capsule_with_bad_page_size_in_pbc_header)
{
    alt_u32* pch_update_capsule = new alt_u32[SIGNED_CAPSULE_PCH_WITH_BAD_PAGE_SIZE_IN_PBC_HEADER_FILE_SIZE / 4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(SIGNED_CAPSULE_PCH_WITH_BAD_PAGE_SIZE_IN_PBC_HEADER_FILE, pch_update_capsule);

    EXPECT_FALSE(is_capsule_valid(pch_update_capsule));

    delete[] pch_update_capsule;
}

/**
 * @brief Authenticate a PCH firmware update capsule that has wrong pattern (0x0 instead of 0xFF) specified in
 * the compression structure header. This passes authentication right now, since Nios doesn't check this field.
 * Currently, the only pattern used in PFR is 0xFF.
 */
TEST_F(CapsuleValidationTest, test_authenticate_pch_fw_capsule_with_bad_pattern_in_pbc_header)
{
    alt_u32* pch_update_capsule = new alt_u32[SIGNED_CAPSULE_PCH_WITH_BAD_PATT_IN_PBC_HEADER_FILE_SIZE / 4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(SIGNED_CAPSULE_PCH_WITH_BAD_PATT_IN_PBC_HEADER_FILE, pch_update_capsule);

    EXPECT_TRUE(is_capsule_valid(pch_update_capsule));

    delete[] pch_update_capsule;
}
