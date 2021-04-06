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

class AuthenticationNegativeTest : public testing::Test
{
public:
    virtual void SetUp()
    {
        SYSTEM_MOCK* sys = SYSTEM_MOCK::get();
        // Reset system mocks and SPI flash
        sys->reset();
        sys->reset_spi_flash_mock();

        // Provision the system (e.g. root key hash)
        sys->provision_ufm_data(UFM_PFR_DATA_EXAMPLE_KEY_FILE);

        // Always use PCH SPI flash
        switch_spi_flash(SPI_FLASH_BMC);
    }

    virtual void TearDown() {}
};


TEST_F(AuthenticationNegativeTest, test_authenticate_signed_can_cert_with_corrupted_payload)
{
    // Load the key cancellation certificate to SPI flash memory
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, KEY_CAN_CERT_PCH_PFM_KEY2, KEY_CAN_CERT_FILE_SIZE);

    // Corrupt the key id in protected content of key cancellation certificate
    alt_u32* signed_cert = get_spi_flash_ptr();
    signed_cert[SIGNATURE_SIZE/4] = 0xFF;

    // Authentication should fail
    EXPECT_FALSE(is_signature_valid((KCH_SIGNATURE*) signed_cert));
}

/**
 * @brief authenticate a key cancellation certificate with invalid key ID (e.g. 255).
 */
TEST_F(AuthenticationNegativeTest, test_authenticate_signed_can_cert_with_bad_key_id)
{
    // Load the key cancellation certificate to SPI flash memory
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, KEY_CAN_CERT_PCH_PFM_KEY255, KEY_CAN_CERT_FILE_SIZE);
    EXPECT_FALSE(is_signature_valid((KCH_SIGNATURE*)  get_spi_flash_ptr()));
}

/**
 * @brief authenticate a key cancellation certificate with invalid PC length.
 */
TEST_F(AuthenticationNegativeTest, test_authenticate_signed_can_cert_with_bad_pc_length)
{
    // Load the key cancellation certificate to SPI flash memory
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, KEY_CAN_CERT_BAD_PCH_LEGNTH, KEY_CAN_CERT_FILE_SIZE);
    EXPECT_FALSE(is_signature_valid((KCH_SIGNATURE*)  get_spi_flash_ptr()));
}

/**
 * @brief authenticate a key cancellation certificate with incorrect reserved area.
 */
TEST_F(AuthenticationNegativeTest, test_authenticate_signed_can_cert_with_modified_reserved_area)
{
    // Load the key cancellation certificate to SPI flash memory
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, KEY_CAN_CERT_MODIFIED_RESERVED_AREA, KEY_CAN_CERT_FILE_SIZE);
    EXPECT_FALSE(is_signature_valid((KCH_SIGNATURE*)  get_spi_flash_ptr()));
}

TEST_F(AuthenticationNegativeTest, test_authenticate_binary_with_bad_block0_magic_number)
{
    // Load a payload, which is signed by Blocksign tool, to SPI flash memory
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, SIGNED_BINARY_BLOCKSIGN_FILE, SIGNED_BINARY_BLOCKSIGN_FILE_SIZE);

    // Corrupt the magic number of Block 0
    alt_u32* signed_binary_blocksign = get_spi_flash_ptr();
    signed_binary_blocksign[0] = 0xdeadbeef;

    // Authentication should fail
    EXPECT_FALSE(is_signature_valid((KCH_SIGNATURE*) signed_binary_blocksign));
}

TEST_F(AuthenticationNegativeTest, test_authenticate_binary_with_bad_block1_magic_number)
{
    // Load a payload, which is signed by Blocksign tool, to SPI flash memory
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, SIGNED_BINARY_BLOCKSIGN_FILE, SIGNED_BINARY_BLOCKSIGN_FILE_SIZE);

    // Corrupt the magic number of Block 1
    alt_u32* signed_binary_blocksign = get_spi_flash_ptr();
    signed_binary_blocksign[BLOCK0_SIZE / 4] = 0xdeadbeef;

    // Authentication should fail
    EXPECT_FALSE(is_signature_valid((KCH_SIGNATURE*) signed_binary_blocksign));
}

TEST_F(AuthenticationNegativeTest, test_authenticate_binary_with_bad_block1_root_entry_magic_number)
{
    // Load a payload, which is signed by Blocksign tool, to SPI flash memory
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, SIGNED_BINARY_BLOCKSIGN_FILE, SIGNED_BINARY_BLOCKSIGN_FILE_SIZE);

    // Corrupt the magic number of Root entry in Block 1
    alt_u32* signed_binary_blocksign = get_spi_flash_ptr();
    signed_binary_blocksign[(BLOCK0_SIZE + BLOCK1_HEADER_SIZE) / 4] = 0xdeadbeef;

    // Authentication should fail
    EXPECT_FALSE(is_signature_valid((KCH_SIGNATURE*) signed_binary_blocksign));
}

TEST_F(AuthenticationNegativeTest, test_authenticate_binary_with_bad_block1_root_entry_curve_magic_number)
{
    // Load a payload, which is signed by Blocksign tool, to SPI flash memory
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, SIGNED_BINARY_BLOCKSIGN_FILE, SIGNED_BINARY_BLOCKSIGN_FILE_SIZE);

    // Corrupt the curve magic number of Root entry in Block 1
    alt_u32* signed_binary_blocksign = get_spi_flash_ptr();
    // In Whitley design, CPLD only support 256-bit ECDSA algorithm.
    signed_binary_blocksign[(BLOCK0_SIZE + BLOCK1_HEADER_SIZE + 4) / 4] = CURVE_SECP384R1_MAGIC;

    // Authentication should fail
    EXPECT_FALSE(is_signature_valid((KCH_SIGNATURE*) signed_binary_blocksign));
}

TEST_F(AuthenticationNegativeTest, test_authenticate_binary_with_bad_block1_csk_entry_magic_number)
{
    // Load a payload, which is signed by Blocksign tool, to SPI flash memory
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, SIGNED_BINARY_BLOCKSIGN_FILE, SIGNED_BINARY_BLOCKSIGN_FILE_SIZE);

    // Corrupt the magic number of Root entry in Block 1
    alt_u32* signed_binary_blocksign = get_spi_flash_ptr();
    // In Whitley design, CPLD only support 256-bit ECDSA algorithm.
    signed_binary_blocksign[(BLOCK0_SIZE + BLOCK1_HEADER_SIZE + BLOCK1_ROOT_ENTRY_SIZE) / 4] = 0xdeadbeef;

    // Authentication should fail
    EXPECT_FALSE(is_signature_valid((KCH_SIGNATURE*) signed_binary_blocksign));
}

TEST_F(AuthenticationNegativeTest, test_authenticate_binary_with_bad_block1_csk_entry_curve_magic_number)
{
    // Load a payload, which is signed by Blocksign tool, to SPI flash memory
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, SIGNED_BINARY_BLOCKSIGN_FILE, SIGNED_BINARY_BLOCKSIGN_FILE_SIZE);

    // Corrupt the curve magic number of Root entry in Block 1
    alt_u32* signed_binary_blocksign = get_spi_flash_ptr();
    // In Whitley design, CPLD only support 256-bit ECDSA algorithm.
    signed_binary_blocksign[(BLOCK0_SIZE + BLOCK1_HEADER_SIZE + BLOCK1_ROOT_ENTRY_SIZE + 4) / 4] = CURVE_SECP384R1_MAGIC;

    // Authentication should fail
    EXPECT_FALSE(is_signature_valid((KCH_SIGNATURE*) signed_binary_blocksign));
}

TEST_F(AuthenticationNegativeTest, test_authenticate_block1_with_modified_block0_after_signing)
{
    // Load a payload, which is signed by Blocksign tool, to SPI flash memory
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, SIGNED_BINARY_BLOCKSIGN_FILE, SIGNED_BINARY_BLOCKSIGN_FILE_SIZE);

    // Modify an unimportant field in Block0
    alt_u32* signed_binary_blocksign = get_spi_flash_ptr();
    KCH_SIGNATURE* sig = (KCH_SIGNATURE*) signed_binary_blocksign;
    KCH_BLOCK0* b0 = &sig->b0;
    b0->reserved = 0x1;

    EXPECT_FALSE(is_block1_valid(b0, &sig->b1, 0));
}

TEST_F(AuthenticationNegativeTest, test_authenticate_binary_with_0_as_pc_length)
{
    // Load a payload, which is signed by Blocksign tool, to SPI flash memory
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, SIGNED_BINARY_BLOCKSIGN_FILE, SIGNED_BINARY_BLOCKSIGN_FILE_SIZE);

    // Corrupt the signature
    KCH_BLOCK0* b0 = (KCH_BLOCK0*) get_spi_flash_ptr();
    b0->pc_length = 0;

    EXPECT_FALSE(is_block0_valid(b0, incr_alt_u32_ptr((alt_u32*) b0, SIGNATURE_SIZE)));
}

TEST_F(AuthenticationNegativeTest, test_authenticate_fake_cpld_capsule_with_enormous_pc_length)
{
    // Load a CPLD capsule to SPI flash memory
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, SIGNED_CAPSULE_CPLD_FILE, SIGNED_CAPSULE_CPLD_FILE_SIZE);

    // Corrupt the PC length
    KCH_BLOCK0* b0 = (KCH_BLOCK0*) get_spi_flash_ptr();
    b0->pc_length = 0xFFFFFFFF;

    EXPECT_FALSE(is_block0_valid(b0, incr_alt_u32_ptr((alt_u32*) b0, SIGNATURE_SIZE)));
}

TEST_F(AuthenticationNegativeTest, test_authenticate_fake_pch_pfm_with_enormous_pc_length)
{
    // Load a PCH PFM to SPI flash memory
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, SIGNED_PFM_BIOS_FILE, SIGNED_PFM_BIOS_FILE_SIZE);

    // Corrupt the PC length
    KCH_BLOCK0* b0 = (KCH_BLOCK0*) get_spi_flash_ptr();
    b0->pc_length = 0xFF000000;

    EXPECT_FALSE(is_block0_valid(b0, incr_alt_u32_ptr((alt_u32*) b0, SIGNATURE_SIZE)));
}

TEST_F(AuthenticationNegativeTest, test_authenticate_fake_pch_capsule_with_enormous_pc_length)
{
    // Load a PCH capsule to SPI flash memory
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, SIGNED_CAPSULE_PCH_FILE, SIGNED_CAPSULE_PCH_FILE_SIZE);

    // Corrupt the PC length
    KCH_BLOCK0* b0 = (KCH_BLOCK0*) get_spi_flash_ptr();
    b0->pc_length = 0xFFFFFFFE;

    EXPECT_FALSE(is_block0_valid(b0, incr_alt_u32_ptr((alt_u32*) b0, SIGNATURE_SIZE)));
}

TEST_F(AuthenticationNegativeTest, test_authenticate_fake_bmc_pfm_with_enormous_pc_length)
{
    // Load a BMC PFM to SPI flash memory
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, SIGNED_PFM_BMC_FILE, SIGNED_PFM_BMC_FILE_SIZE);

    // Corrupt the PC length
    KCH_BLOCK0* b0 = (KCH_BLOCK0*) get_spi_flash_ptr();
    b0->pc_length = 0xAA000000;

    EXPECT_FALSE(is_block0_valid(b0, incr_alt_u32_ptr((alt_u32*) b0, SIGNATURE_SIZE)));
}

TEST_F(AuthenticationNegativeTest, test_authenticate_fake_bmc_capsule_with_enormous_pc_length)
{
    // Load a BMC capsule to SPI flash memory
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, SIGNED_CAPSULE_BMC_FILE, SIGNED_CAPSULE_BMC_FILE_SIZE);

    // Corrupt the PC length
    KCH_BLOCK0* b0 = (KCH_BLOCK0*) get_spi_flash_ptr();
    b0->pc_length = 0x08000000;

    EXPECT_FALSE(is_block0_valid(b0, incr_alt_u32_ptr((alt_u32*) b0, SIGNATURE_SIZE)));
}

/**
 * @brief Authenticate a PCH firmware update capsule that has 10 as the PC Type value. That
 * is invalid. Expecting to see an authentication failure.
 */
TEST_F(AuthenticationNegativeTest, test_authenticate_pch_fw_capsule_with_invalid_pc_type)
{
    // Load a PCH capsule to SPI flash memory
    SYSTEM_MOCK::get()->load_to_flash(
            SPI_FLASH_BMC,
			SIGNED_CAPSULE_PCH_WITH_PC_TYPE_10_FILE,
			SIGNED_CAPSULE_PCH_WITH_PC_TYPE_10_FILE_SIZE);

    KCH_BLOCK0* b0 = (KCH_BLOCK0*) get_spi_flash_ptr();

    EXPECT_EQ(get_kch_pc_type(b0), alt_u32(10));

    // PC Type 10 is invalid
    EXPECT_FALSE(is_block0_valid(b0, incr_alt_u32_ptr(get_spi_flash_ptr(), SIGNATURE_SIZE)));
    EXPECT_FALSE(is_signature_valid((KCH_SIGNATURE*) b0));
}

/**
 * @brief Authenticate a PCH firmware update capsule for which the CSK Key has been cancelled.
 */
TEST_F(AuthenticationNegativeTest, test_authenticate_pch_fw_capsule_with_cancelled_key)
{
    // Load a PCH capsule to SPI flash memory
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, SIGNED_CAPSULE_PCH_WITH_CSK_ID10_FILE, SIGNED_CAPSULE_PCH_WITH_CSK_ID10_FILE_SIZE);

    // Authentication should pass before key cancellation
    EXPECT_TRUE(is_signature_valid((KCH_SIGNATURE*) get_spi_flash_ptr()));

    // Cancel CSK Key #10 for signing PCH update capsule
    cancel_key(KCH_PC_PFR_PCH_UPDATE_CAPSULE, 10);

    // Authentication should fail after key cancellation
    EXPECT_FALSE(is_signature_valid((KCH_SIGNATURE*) get_spi_flash_ptr()));
}
