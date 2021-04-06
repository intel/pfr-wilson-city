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


class KeyCancellationFlowTest : public testing::Test
{
public:
    virtual void SetUp()
    {
        SYSTEM_MOCK* sys = SYSTEM_MOCK::get();
        // Reset system mocks and SPI flash
        sys->reset();
        sys->reset_spi_flash_mock();

        // Perform provisioning
        SYSTEM_MOCK::get()->provision_ufm_data(UFM_PFR_DATA_EXAMPLE_KEY_FILE);

        // Reset Nios firmware
        ut_reset_nios_fw();

        // Load the entire image to flash
        SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, FULL_PFR_IMAGE_BMC_FILE, FULL_PFR_IMAGE_BMC_FILE_SIZE);
        SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_PCH, FULL_PFR_IMAGE_PCH_FILE, FULL_PFR_IMAGE_PCH_FILE_SIZE);
    }

    virtual void TearDown() {}
};

/**
 * @brief Execute in-band key cancellation with pc type PCH_PFM and id 2.
 * Send key cancellation certificate to PCH flash and use PCH active update intent
 * to trigger the cancellation.
 */
TEST_F(KeyCancellationFlowTest, test_cancel_key2_for_pch_pfm_through_ib_pch_active_fw_update)
{
    /*
     * Test Preparation
     */
    // Load the key cancellation certificate to staging region in flash
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_PCH, KEY_CAN_CERT_PCH_PFM_KEY2,
            KEY_CAN_CERT_FILE_SIZE, get_ufm_pfr_data()->pch_staging_region);

    /*
     * Flow preparation
     */
    ut_prep_nios_gpi_signals();
    ut_send_block_complete_chkpt_msg();

    // Exit after 10 iterations in the T0 loop
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS_END_AFTER_50_ITERS);
    // Skip authentication in T-1
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::SKIP_FLASH_AUTHENTICATION);

    // Send the active update intent
    ut_send_in_update_intent_once_upon_boot_complete(MB_PCH_UPDATE_INTENT, MB_UPDATE_INTENT_PCH_ACTIVE_MASK);

    /*
     * Execute the flow.
     */
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(25, pfr_main());

    /*
     * Check results
     */
    // Nios should transition from T0 to T-1 once to process the key cancellation certificate
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(1));
    // Expect no error
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(0));

    // Key ID 2 for signing PCH PFM should be cancelled now.
    // Policies for other PC type should not be affected
    EXPECT_FALSE(is_csk_key_valid(KCH_PC_PFR_PCH_PFM, 2));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_PCH_UPDATE_CAPSULE, 2));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_CPLD_UPDATE_CAPSULE, 2));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_BMC_PFM, 2));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_BMC_UPDATE_CAPSULE, 2));
}

/**
 * @brief Execute in-band key cancellation with pc type BMC_UPDATE_CAPSULE and id 1.
 * Send key cancellation certificate to PCH flash and use PCH active update intent to
 * trigger the cancellation.
 */
TEST_F(KeyCancellationFlowTest, test_cancel_key1_for_bmc_capsule_through_ib_pch_active_fw_update)
{
    /*
     * Test Preparation
     */
    // Load the key cancellation certificate to staging region in flash
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_PCH, KEY_CAN_CERT_BMC_CAPSULE_KEY1,
            KEY_CAN_CERT_FILE_SIZE, get_ufm_pfr_data()->pch_staging_region);

    /*
     * Flow preparation
     */
    ut_prep_nios_gpi_signals();
    ut_send_block_complete_chkpt_msg();

    // Exit after 10 iterations in the T0 loop
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS_END_AFTER_50_ITERS);
    // Skip authentication in T-1
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::SKIP_FLASH_AUTHENTICATION);

    // Send the active update intent
    ut_send_in_update_intent_once_upon_boot_complete(MB_PCH_UPDATE_INTENT, MB_UPDATE_INTENT_PCH_ACTIVE_MASK);

    /*
     * Execute the flow.
     */
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(25, pfr_main());

    /*
     * Check results
     */
    // Nios should transition from T0 to T-1 once to process the key cancellation certificate
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(1));
    // Expect no error
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(0));

    // Key ID 1 for signing BMC update capsule should be cancelled now.
    // Policies for other PC type should not be affected
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_PCH_PFM, 1));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_PCH_UPDATE_CAPSULE, 1));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_CPLD_UPDATE_CAPSULE, 1));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_BMC_PFM, 1));
    EXPECT_FALSE(is_csk_key_valid(KCH_PC_PFR_BMC_UPDATE_CAPSULE, 1));
}

/**
 * @brief Execute in-band key cancellation with pc type CPLD_UPDATE_CAPSULE and id 10.
 * Send key cancellation certificate to PCH flash and use PCH active update intent to
 * trigger the cancellation.
 */
TEST_F(KeyCancellationFlowTest, test_cancel_key10_for_cpld_capsule_through_ib_pch_active_fw_update)
{
    /*
     * Test Preparation
     */
    // Load the key cancellation certificate to staging region in flash
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_PCH, KEY_CAN_CERT_CPLD_CAPSULE_KEY10,
            KEY_CAN_CERT_FILE_SIZE, get_ufm_pfr_data()->pch_staging_region);

    /*
     * Flow preparation
     */
    ut_prep_nios_gpi_signals();
    ut_send_block_complete_chkpt_msg();

    // Exit after 10 iterations in the T0 loop
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS_END_AFTER_50_ITERS);
    // Skip authentication in T-1
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::SKIP_FLASH_AUTHENTICATION);

    // Send the active update intent
    ut_send_in_update_intent_once_upon_boot_complete(MB_PCH_UPDATE_INTENT, MB_UPDATE_INTENT_PCH_ACTIVE_MASK);

    /*
     * Execute the flow.
     */
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(25, pfr_main());

    /*
     * Check results
     */
    // Nios should transition from T0 to T-1 once to process the key cancellation certificate
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(1));
    // Expect no error
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(0));

    // Key ID 10 for signing CPLD update capsule should be cancelled now.
    // Policies for other PC type should not be affected
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_PCH_PFM, 10));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_PCH_UPDATE_CAPSULE, 10));
    EXPECT_FALSE(is_csk_key_valid(KCH_PC_PFR_CPLD_UPDATE_CAPSULE, 10));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_BMC_PFM, 10));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_BMC_UPDATE_CAPSULE, 10));
}

/**
 * @brief Execute oob-band key cancellation with pc type PCH_UPDATE_CAPSULE and id 2.
 * Send key cancellation certificate to BMC flash and use PCH recovery update intent
 * to trigger the cancellation.
 */
TEST_F(KeyCancellationFlowTest, test_cancel_key2_for_pch_capsule_through_oob_pch_recovery_fw_update)
{
    /*
     * Test Preparation
     */
    // Load the key cancellation certificate to staging region in flash
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, KEY_CAN_CERT_PCH_CAPSULE_KEY2,
            KEY_CAN_CERT_FILE_SIZE, get_ufm_pfr_data()->bmc_staging_region + BMC_STAGING_REGION_PCH_UPDATE_CAPSULE_OFFSET);

    /*
     * Flow preparation
     */
    ut_prep_nios_gpi_signals();
    ut_send_block_complete_chkpt_msg();

    // Exit after 10 iterations in the T0 loop
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS_END_AFTER_50_ITERS);
    // Skip authentication in T-1
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::SKIP_FLASH_AUTHENTICATION);

    // Send the active update intent
    ut_send_in_update_intent_once_upon_boot_complete(MB_BMC_UPDATE_INTENT, MB_UPDATE_INTENT_PCH_RECOVERY_MASK);

    /*
     * Execute the flow.
     */
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(25, pfr_main());

    /*
     * Check results
     */
    // Nios should transition from T0 to T-1 once to process the key cancellation certificate
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(1));
    // Expect no error
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(0));

    // Key ID 2 for signing PCH update capsule should be cancelled now.
    // Policies for other PC type should not be affected
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_PCH_PFM, 2));
    EXPECT_FALSE(is_csk_key_valid(KCH_PC_PFR_PCH_UPDATE_CAPSULE, 2));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_CPLD_UPDATE_CAPSULE, 2));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_BMC_PFM, 2));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_BMC_UPDATE_CAPSULE, 2));
}

/**
 * @brief Execute oob-band key cancellation with pc type BMC_PFM and id 10.
 * Send key cancellation certificate to BMC flash and use BMC active update intent
 * to trigger the cancellation.
 */
TEST_F(KeyCancellationFlowTest, test_cancel_key10_for_bmc_pfm_through_oob_bmc_active_fw_update)
{
    /*
     * Test Preparation
     */
    // Load the key cancellation certificate to staging region in flash
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, KEY_CAN_CERT_BMC_PFM_KEY10,
            KEY_CAN_CERT_FILE_SIZE, get_ufm_pfr_data()->bmc_staging_region);

    /*
     * Flow preparation
     */
    ut_prep_nios_gpi_signals();
    ut_send_block_complete_chkpt_msg();

    // Exit after 10 iterations in the T0 loop
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS_END_AFTER_50_ITERS);
    // Skip authentication in T-1
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::SKIP_FLASH_AUTHENTICATION);

    // Send the active update intent
    ut_send_in_update_intent_once_upon_boot_complete(MB_BMC_UPDATE_INTENT, MB_UPDATE_INTENT_BMC_ACTIVE_MASK);

    /*
     * Execute the flow.
     */
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(25, pfr_main());

    /*
     * Check results
     */
    // Nios should transition from T0 to T-1 once to process the key cancellation certificate
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(1));
    // Expect no error
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(0));

    // Key ID 10 for signing BMC PFM should be cancelled now.
    // Policies for other PC type should not be affected
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_PCH_PFM, 10));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_PCH_UPDATE_CAPSULE, 10));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_CPLD_UPDATE_CAPSULE, 10));
    EXPECT_FALSE(is_csk_key_valid(KCH_PC_PFR_BMC_PFM, 10));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_BMC_UPDATE_CAPSULE, 10));
}

/**
 * @brief Execute oob-band key cancellation with pc type CPLD_UPDATE_CAPSULE and id 10.
 * Send key cancellation certificate to BMC flash and use BMC active update intent
 * to trigger the cancellation.
 */
TEST_F(KeyCancellationFlowTest, test_cancel_key10_for_cpld_capsule_through_oob_bmc_active_update)
{
    /*
     * Test Preparation
     */
    // Load the key cancellation certificate to staging region in flash
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, KEY_CAN_CERT_CPLD_CAPSULE_KEY10,
            KEY_CAN_CERT_FILE_SIZE, get_ufm_pfr_data()->bmc_staging_region);

    /*
     * Flow preparation
     */
    ut_prep_nios_gpi_signals();
    ut_send_block_complete_chkpt_msg();

    // Exit after 10 iterations in the T0 loop
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS_END_AFTER_50_ITERS);
    // Skip authentication in T-1
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::SKIP_FLASH_AUTHENTICATION);

    // Send the active update intent
    ut_send_in_update_intent_once_upon_boot_complete(MB_BMC_UPDATE_INTENT, MB_UPDATE_INTENT_BMC_ACTIVE_MASK);

    /*
     * Execute the flow.
     */
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(25, pfr_main());

    /*
     * Check results
     */
    // Nios should transition from T0 to T-1 once to process the key cancellation certificate
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(1));
    // Expect no error
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(0));

    // Key ID 10 for signing CPLD update capsule should be cancelled now.
    // Policies for other PC type should not be affected
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_PCH_PFM, 10));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_PCH_UPDATE_CAPSULE, 10));
    EXPECT_FALSE(is_csk_key_valid(KCH_PC_PFR_CPLD_UPDATE_CAPSULE, 10));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_BMC_PFM, 10));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_BMC_UPDATE_CAPSULE, 10));
}

/**
 * @brief Execute oob-band key cancellation with pc type CPLD_UPDATE_CAPSULE and id 10.
 * Send key cancellation certificate to BMC flash and use CPLD active update intent
 * to trigger the cancellation.
 */
TEST_F(KeyCancellationFlowTest, test_cancel_key10_for_cpld_capsule_through_oob_cpld_active_update)
{
    /*
     * Test Preparation
     */
    // Load the key cancellation certificate to staging region in flash
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, KEY_CAN_CERT_CPLD_CAPSULE_KEY10,
            KEY_CAN_CERT_FILE_SIZE, get_ufm_pfr_data()->bmc_staging_region + BMC_STAGING_REGION_CPLD_UPDATE_CAPSULE_OFFSET);

    /*
     * Flow preparation
     */
    ut_prep_nios_gpi_signals();
    ut_send_block_complete_chkpt_msg();

    // Exit after 10 iterations in the T0 loop
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS_END_AFTER_50_ITERS);
    // Skip authentication in T-1
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::SKIP_FLASH_AUTHENTICATION);

    // Send the active update intent
    ut_send_in_update_intent_once_upon_boot_complete(MB_BMC_UPDATE_INTENT, MB_UPDATE_INTENT_CPLD_ACTIVE_MASK);

    /*
     * Execute the flow.
     */
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(25, pfr_main());

    /*
     * Check results
     */
    // Nios should transition from T0 to T-1 once to process the key cancellation certificate
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(1));
    // Expect no error
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(0));

    // Key ID 10 for signing CPLD update capsule should be cancelled now.
    // Policies for other PC type should not be affected
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_PCH_PFM, 10));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_PCH_UPDATE_CAPSULE, 10));
    EXPECT_FALSE(is_csk_key_valid(KCH_PC_PFR_CPLD_UPDATE_CAPSULE, 10));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_BMC_PFM, 10));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_BMC_UPDATE_CAPSULE, 10));
}

/**
 * @brief Execute oob-band key cancellation with pc type CPLD_UPDATE_CAPSULE and id 10.
 * Send key cancellation certificate to BMC flash and use CPLD recover update intent
 * to trigger the cancellation.
 */
TEST_F(KeyCancellationFlowTest, test_cancel_key10_for_cpld_capsule_through_oob_cpld_recovery_update)
{
    /*
     * Test Preparation
     */
    // Load the key cancellation certificate to staging region in flash
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, KEY_CAN_CERT_CPLD_CAPSULE_KEY10,
            KEY_CAN_CERT_FILE_SIZE, get_ufm_pfr_data()->bmc_staging_region + BMC_STAGING_REGION_CPLD_UPDATE_CAPSULE_OFFSET);

    /*
     * Flow preparation
     */
    ut_prep_nios_gpi_signals();
    ut_send_block_complete_chkpt_msg();

    // Exit after 10 iterations in the T0 loop
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS_END_AFTER_50_ITERS);
    // Skip authentication in T-1
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::SKIP_FLASH_AUTHENTICATION);

    // Send the active update intent
    ut_send_in_update_intent_once_upon_boot_complete(MB_BMC_UPDATE_INTENT, MB_UPDATE_INTENT_CPLD_RECOVERY_MASK);

    /*
     * Execute the flow.
     */
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(25, pfr_main());

    /*
     * Check results
     */
    // Nios should transition from T0 to T-1 once to process the key cancellation certificate
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(1));
    // Expect no error
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(0));

    // Key ID 10 for signing CPLD update capsule should be cancelled now.
    // Policies for other PC type should not be affected
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_PCH_PFM, 10));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_PCH_UPDATE_CAPSULE, 10));
    EXPECT_FALSE(is_csk_key_valid(KCH_PC_PFR_CPLD_UPDATE_CAPSULE, 10));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_BMC_PFM, 10));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_BMC_UPDATE_CAPSULE, 10));
}
