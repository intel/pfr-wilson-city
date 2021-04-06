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


/**
 * @brief This suite of tests send authentic signed binaries but with wrong update intent value.
 * These authentic binaries should fail capsule authentication, so that Nios does not accidentally
 * load wrong bitstream on platform flashes (e.g. SPI flashes or CFM)
 */
class UpdateFlowMixAndMatchAttackTest : public testing::Test
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
 * @brief This test sends BMC active update request, but has CPLD update capsule in BMC staging area.
 */
TEST_F(UpdateFlowMixAndMatchAttackTest, test_send_bmc_active_update_with_cpld_capsule)
{
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
    ut_send_in_update_intent(MB_BMC_UPDATE_INTENT, MB_UPDATE_INTENT_BMC_ACTIVE_MASK);

    // Load the fw update capsule to staging region in flash
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, SIGNED_CAPSULE_CPLD_FILE,
            SIGNED_CAPSULE_CPLD_FILE_SIZE, get_ufm_pfr_data()->bmc_staging_region);

    /*
     * Execute the flow. Active update flow will be triggered.
     * Since there's no update capsule in staging region, update would fail.
     */
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(1, pfr_main());

    /*
     * Check results.
     */
    EXPECT_TRUE(ut_is_blocking_update_from(SPI_FLASH_BMC));
    EXPECT_FALSE(ut_is_blocking_update_from(SPI_FLASH_PCH));

    // Check number of T-1 entry
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(1));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_bmc_only_counter(), alt_u32(MAX_FAILED_UPDATE_ATTEMPTS_FROM_BMC));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_pch_only_counter(), alt_u32(0));

    // Check Panic event count
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(MAX_FAILED_UPDATE_ATTEMPTS_FROM_BMC));

    // Check the error message
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(MAJOR_ERROR_UPDATE_FROM_BMC_FAILED));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_EXCEEDED_MAX_FAILED_ATTEMPTS));
}

/**
 * @brief This test sends BMC active update request, but has PCH update capsule in BMC staging area.
 */
TEST_F(UpdateFlowMixAndMatchAttackTest, test_send_bmc_active_update_with_pch_capsule)
{
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
    ut_send_in_update_intent(MB_BMC_UPDATE_INTENT, MB_UPDATE_INTENT_BMC_ACTIVE_MASK);

    // Load the fw update capsule to staging region in flash
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, SIGNED_CAPSULE_PCH_FILE,
            SIGNED_CAPSULE_PCH_FILE_SIZE, get_ufm_pfr_data()->bmc_staging_region);

    /*
     * Execute the flow. Active update flow will be triggered.
     * Since there's no update capsule in staging region, update would fail.
     */
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(20, pfr_main());

    /*
     * Check results.
     */
    EXPECT_TRUE(ut_is_blocking_update_from(SPI_FLASH_BMC));
    EXPECT_FALSE(ut_is_blocking_update_from(SPI_FLASH_PCH));

    // Check number of T-1 entry
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(1));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_bmc_only_counter(), alt_u32(MAX_FAILED_UPDATE_ATTEMPTS_FROM_BMC));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_pch_only_counter(), alt_u32(0));

    // Check Panic event count
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(MAX_FAILED_UPDATE_ATTEMPTS_FROM_BMC));

    // Check the error message
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(MAJOR_ERROR_UPDATE_FROM_BMC_FAILED));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_EXCEEDED_MAX_FAILED_ATTEMPTS));
}

/**
 * @brief This test sends BMC active update request, but has signed PCH PFM in BMC staging area.
 */
TEST_F(UpdateFlowMixAndMatchAttackTest, test_send_bmc_active_update_with_pch_pfm)
{
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
    ut_send_in_update_intent(MB_BMC_UPDATE_INTENT, MB_UPDATE_INTENT_BMC_ACTIVE_MASK);

    // Load the fw update capsule to staging region in flash
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, SIGNED_PFM_BIOS_FILE,
            SIGNED_PFM_BIOS_FILE_SIZE, get_ufm_pfr_data()->bmc_staging_region);

    /*
     * Execute the flow. Active update flow will be triggered.
     * Since there's no update capsule in staging region, update would fail.
     */
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(20, pfr_main());

    /*
     * Check results.
     */
    EXPECT_TRUE(ut_is_blocking_update_from(SPI_FLASH_BMC));
    EXPECT_FALSE(ut_is_blocking_update_from(SPI_FLASH_PCH));

    // Check number of T-1 entry
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(1));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_bmc_only_counter(), alt_u32(MAX_FAILED_UPDATE_ATTEMPTS_FROM_BMC));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_pch_only_counter(), alt_u32(0));

    // Check Panic event count
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(MAX_FAILED_UPDATE_ATTEMPTS_FROM_BMC));

    // Check the error message
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(MAJOR_ERROR_UPDATE_FROM_BMC_FAILED));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_EXCEEDED_MAX_FAILED_ATTEMPTS));
}

/**
 * @brief This test sends BMC active update request, but has signed BMC PFM in BMC staging area.
 */
TEST_F(UpdateFlowMixAndMatchAttackTest, test_send_bmc_active_update_with_bmc_pfm)
{
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
    ut_send_in_update_intent(MB_BMC_UPDATE_INTENT, MB_UPDATE_INTENT_BMC_ACTIVE_MASK);

    // Load the fw update capsule to staging region in flash
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, SIGNED_PFM_BMC_FILE,
            SIGNED_PFM_BMC_FILE_SIZE, get_ufm_pfr_data()->bmc_staging_region);

    /*
     * Execute the flow. Active update flow will be triggered.
     * Since there's no update capsule in staging region, update would fail.
     */
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(20, pfr_main());

    /*
     * Check results.
     */
    EXPECT_TRUE(ut_is_blocking_update_from(SPI_FLASH_BMC));
    EXPECT_FALSE(ut_is_blocking_update_from(SPI_FLASH_PCH));

    // Check number of T-1 entry
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(1));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_bmc_only_counter(), alt_u32(MAX_FAILED_UPDATE_ATTEMPTS_FROM_BMC));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_pch_only_counter(), alt_u32(0));

    // Check Panic event count
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(MAX_FAILED_UPDATE_ATTEMPTS_FROM_BMC));

    // Check the error message
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(MAJOR_ERROR_UPDATE_FROM_BMC_FAILED));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_EXCEEDED_MAX_FAILED_ATTEMPTS));
}

/**
 * @brief This test sends BMC recovery update request, but has CPLD update capsule in BMC staging area.
 */
TEST_F(UpdateFlowMixAndMatchAttackTest, test_send_bmc_recovery_update_with_cpld_capsule)
{
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
    ut_send_in_update_intent(MB_BMC_UPDATE_INTENT, MB_UPDATE_INTENT_BMC_RECOVERY_MASK);

    // Load the fw update capsule to staging region in flash
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, SIGNED_CAPSULE_CPLD_FILE,
            SIGNED_CAPSULE_CPLD_FILE_SIZE, get_ufm_pfr_data()->bmc_staging_region);

    /*
     * Execute the flow. Active update flow will be triggered.
     * Since there's no update capsule in staging region, update would fail.
     */
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(1, pfr_main());

    /*
     * Check results.
     */
    EXPECT_TRUE(ut_is_blocking_update_from(SPI_FLASH_BMC));
    EXPECT_FALSE(ut_is_blocking_update_from(SPI_FLASH_PCH));

    // Check number of T-1 entry
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(1 + MAX_FAILED_UPDATE_ATTEMPTS_FROM_BMC));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_bmc_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_pch_only_counter(), alt_u32(0));

    // Check Panic event count
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(MAX_FAILED_UPDATE_ATTEMPTS_FROM_BMC));

    // Check the error message
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(MAJOR_ERROR_UPDATE_FROM_BMC_FAILED));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_EXCEEDED_MAX_FAILED_ATTEMPTS));
}

/**
 * @brief This test sends BMC recovery update request, but has PCH update capsule in BMC staging area.
 */
TEST_F(UpdateFlowMixAndMatchAttackTest, test_send_bmc_recovery_update_with_pch_capsule)
{
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
    ut_send_in_update_intent(MB_BMC_UPDATE_INTENT, MB_UPDATE_INTENT_BMC_RECOVERY_MASK);

    // Load the fw update capsule to staging region in flash
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, SIGNED_CAPSULE_PCH_FILE,
            SIGNED_CAPSULE_PCH_FILE_SIZE, get_ufm_pfr_data()->bmc_staging_region);

    /*
     * Execute the flow. Active update flow will be triggered.
     * Since there's no update capsule in staging region, update would fail.
     */
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(20, pfr_main());

    /*
     * Check results.
     */
    EXPECT_TRUE(ut_is_blocking_update_from(SPI_FLASH_BMC));
    EXPECT_FALSE(ut_is_blocking_update_from(SPI_FLASH_PCH));

    // Check number of T-1 entry
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(1 + MAX_FAILED_UPDATE_ATTEMPTS_FROM_BMC));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_bmc_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_pch_only_counter(), alt_u32(0));

    // Check Panic event count
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(MAX_FAILED_UPDATE_ATTEMPTS_FROM_BMC));

    // Check the error message
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(MAJOR_ERROR_UPDATE_FROM_BMC_FAILED));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_EXCEEDED_MAX_FAILED_ATTEMPTS));
}

/**
 * @brief This test sends BMC recovery update request, but has signed PCH PFM in BMC staging area.
 */
TEST_F(UpdateFlowMixAndMatchAttackTest, test_send_bmc_recovery_update_with_pch_pfm)
{
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
    ut_send_in_update_intent(MB_BMC_UPDATE_INTENT, MB_UPDATE_INTENT_BMC_RECOVERY_MASK);

    // Load the fw update capsule to staging region in flash
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, SIGNED_PFM_BIOS_FILE,
            SIGNED_PFM_BIOS_FILE_SIZE, get_ufm_pfr_data()->bmc_staging_region);

    /*
     * Execute the flow. Active update flow will be triggered.
     * Since there's no update capsule in staging region, update would fail.
     */
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(20, pfr_main());

    /*
     * Check results.
     */
    EXPECT_TRUE(ut_is_blocking_update_from(SPI_FLASH_BMC));
    EXPECT_FALSE(ut_is_blocking_update_from(SPI_FLASH_PCH));

    // Check number of T-1 entry
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(1 + MAX_FAILED_UPDATE_ATTEMPTS_FROM_BMC));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_bmc_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_pch_only_counter(), alt_u32(0));

    // Check Panic event count
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(MAX_FAILED_UPDATE_ATTEMPTS_FROM_BMC));

    // Check the error message
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(MAJOR_ERROR_UPDATE_FROM_BMC_FAILED));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_EXCEEDED_MAX_FAILED_ATTEMPTS));
}

/**
 * @brief This test sends OOB PCH active update request, but has CPLD update capsule in PCH staging area (of BMC flash).
 */
TEST_F(UpdateFlowMixAndMatchAttackTest, test_send_oob_pch_active_update_with_cpld_capsule)
{
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
    ut_send_in_update_intent(MB_BMC_UPDATE_INTENT, MB_UPDATE_INTENT_PCH_ACTIVE_MASK);

    // Load the fw update capsule to staging region in flash
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, SIGNED_CAPSULE_CPLD_FILE,
            SIGNED_CAPSULE_CPLD_FILE_SIZE, get_ufm_pfr_data()->bmc_staging_region + BMC_STAGING_REGION_PCH_UPDATE_CAPSULE_OFFSET);

    /*
     * Execute the flow. Active update flow will be triggered.
     * Since there's no update capsule in staging region, update would fail.
     */
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(1, pfr_main());

    /*
     * Check results.
     */
    EXPECT_TRUE(ut_is_blocking_update_from(SPI_FLASH_BMC));
    EXPECT_FALSE(ut_is_blocking_update_from(SPI_FLASH_PCH));

    // Check number of T-1 entry
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(1 + MAX_FAILED_UPDATE_ATTEMPTS_FROM_BMC));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_bmc_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_pch_only_counter(), alt_u32(0));

    // Check Panic event count
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(MAX_FAILED_UPDATE_ATTEMPTS_FROM_BMC));

    // Check the error message
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(MAJOR_ERROR_UPDATE_FROM_BMC_FAILED));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_EXCEEDED_MAX_FAILED_ATTEMPTS));
}

/**
 * @brief This test sends OOB PCH recovery update request, but has signed PCH PFM in PCH staging area (of BMC flash).
 */
TEST_F(UpdateFlowMixAndMatchAttackTest, test_send_oob_pch_recovery_update_with_pch_pfm)
{
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
    ut_send_in_update_intent(MB_BMC_UPDATE_INTENT, MB_UPDATE_INTENT_PCH_RECOVERY_MASK);

    // Load the fw update capsule to staging region in flash
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, SIGNED_PFM_BIOS_FILE,
            SIGNED_PFM_BIOS_FILE_SIZE, get_ufm_pfr_data()->bmc_staging_region + BMC_STAGING_REGION_PCH_UPDATE_CAPSULE_OFFSET);

    /*
     * Execute the flow. Active update flow will be triggered.
     * Since there's no update capsule in staging region, update would fail.
     */
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(20, pfr_main());

    /*
     * Check results.
     */
    EXPECT_TRUE(ut_is_blocking_update_from(SPI_FLASH_BMC));
    EXPECT_FALSE(ut_is_blocking_update_from(SPI_FLASH_PCH));

    // Check number of T-1 entry
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(1 + MAX_FAILED_UPDATE_ATTEMPTS_FROM_BMC));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_bmc_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_pch_only_counter(), alt_u32(0));

    // Check Panic event count
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(MAX_FAILED_UPDATE_ATTEMPTS_FROM_BMC));

    // Check the error message
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(MAJOR_ERROR_UPDATE_FROM_BMC_FAILED));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_EXCEEDED_MAX_FAILED_ATTEMPTS));
}

/**
 * @brief This test sends OOB PCH recovery update request, but has signed BMC update capsule in PCH staging area (of BMC flash).
 */
TEST_F(UpdateFlowMixAndMatchAttackTest, test_send_oob_pch_recovery_update_with_bmc_capsule)
{
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
    ut_send_in_update_intent(MB_BMC_UPDATE_INTENT, MB_UPDATE_INTENT_PCH_RECOVERY_MASK);

    // Load the fw update capsule to staging region in flash
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, SIGNED_CAPSULE_BMC_FILE,
            SIGNED_CAPSULE_BMC_FILE_SIZE, get_ufm_pfr_data()->bmc_staging_region + BMC_STAGING_REGION_PCH_UPDATE_CAPSULE_OFFSET);

    /*
     * Execute the flow. Active update flow will be triggered.
     * Since there's no update capsule in staging region, update would fail.
     */
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(20, pfr_main());

    /*
     * Check results.
     */
    EXPECT_TRUE(ut_is_blocking_update_from(SPI_FLASH_BMC));
    EXPECT_FALSE(ut_is_blocking_update_from(SPI_FLASH_PCH));

    // Check number of T-1 entry
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(1 + MAX_FAILED_UPDATE_ATTEMPTS_FROM_BMC));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_bmc_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_pch_only_counter(), alt_u32(0));

    // Check Panic event count
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(MAX_FAILED_UPDATE_ATTEMPTS_FROM_BMC));

    // Check the error message
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(MAJOR_ERROR_UPDATE_FROM_BMC_FAILED));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_EXCEEDED_MAX_FAILED_ATTEMPTS));
}

/**
 * @brief This test sends OOB CPLD active update request, but has signature of BMC update capsule in
 * CPLD staging area (of BMC flash).
 */
TEST_F(UpdateFlowMixAndMatchAttackTest, test_send_oob_cpld_active_update_with_bmc_capsule_signature)
{
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
    ut_send_in_update_intent(MB_BMC_UPDATE_INTENT, MB_UPDATE_INTENT_CPLD_ACTIVE_MASK);

    // Load the BMC capsule's signature on CPLD staging region in BMC flash
    // If there's no protection in place, the pc_length in signature would force Nios to read from memory outside
    // of SPI flash AvMM range (or pre-allocated memory for BMC/PCH flash in SPI Flash mock).
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, SIGNED_CAPSULE_BMC_FILE,
            SIGNATURE_SIZE, get_ufm_pfr_data()->bmc_staging_region + BMC_STAGING_REGION_CPLD_UPDATE_CAPSULE_OFFSET);

    /*
     * Execute the flow. Active update flow will be triggered.
     * Since there's no update capsule in staging region, update would fail.
     */
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(20, pfr_main());

    /*
     * Check results.
     */
    EXPECT_TRUE(ut_is_blocking_update_from(SPI_FLASH_BMC));
    EXPECT_FALSE(ut_is_blocking_update_from(SPI_FLASH_PCH));

    // Check number of T-1 entry
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(1 + MAX_FAILED_UPDATE_ATTEMPTS_FROM_BMC));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_bmc_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_pch_only_counter(), alt_u32(0));

    // Check Panic event count
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(MAX_FAILED_UPDATE_ATTEMPTS_FROM_BMC));

    // Check the error message
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(MAJOR_ERROR_UPDATE_FROM_BMC_FAILED));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_EXCEEDED_MAX_FAILED_ATTEMPTS));
}

/**
 * @brief This test sends OOB CPLD recovery update request, but has signed PCH PFM in CPLD staging area (of BMC flash).
 */
TEST_F(UpdateFlowMixAndMatchAttackTest, test_send_oob_cpld_recovery_update_with_pch_pfm)
{
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
    ut_send_in_update_intent(MB_BMC_UPDATE_INTENT, MB_UPDATE_INTENT_PCH_RECOVERY_MASK);

    // Load the fw update capsule to staging region in flash
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, SIGNED_PFM_BIOS_FILE,
            SIGNED_PFM_BIOS_FILE_SIZE, get_ufm_pfr_data()->bmc_staging_region + BMC_STAGING_REGION_CPLD_UPDATE_CAPSULE_OFFSET);

    /*
     * Execute the flow. Active update flow will be triggered.
     * Since there's no update capsule in staging region, update would fail.
     */
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(20, pfr_main());

    /*
     * Check results.
     */
    EXPECT_TRUE(ut_is_blocking_update_from(SPI_FLASH_BMC));
    EXPECT_FALSE(ut_is_blocking_update_from(SPI_FLASH_PCH));

    // Check number of T-1 entry
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(1 + MAX_FAILED_UPDATE_ATTEMPTS_FROM_BMC));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_bmc_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_pch_only_counter(), alt_u32(0));

    // Check Panic event count
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(MAX_FAILED_UPDATE_ATTEMPTS_FROM_BMC));

    // Check the error message
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(MAJOR_ERROR_UPDATE_FROM_BMC_FAILED));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_EXCEEDED_MAX_FAILED_ATTEMPTS));
}

/**
 * @brief This test sends IB PCH active update request, but has CPLD update capsule in staging area of PCH flash.
 */
TEST_F(UpdateFlowMixAndMatchAttackTest, test_send_ib_pch_active_update_with_cpld_capsule)
{
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
    ut_send_in_update_intent(MB_PCH_UPDATE_INTENT, MB_UPDATE_INTENT_PCH_ACTIVE_MASK);

    // Load the fw update capsule to staging region in flash
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_PCH, SIGNED_CAPSULE_CPLD_FILE,
            SIGNED_CAPSULE_CPLD_FILE_SIZE, get_ufm_pfr_data()->pch_staging_region);

    /*
     * Execute the flow. Active update flow will be triggered.
     * Since there's no update capsule in staging region, update would fail.
     */
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(20, pfr_main());

    /*
     * Check results.
     */
    EXPECT_FALSE(ut_is_blocking_update_from(SPI_FLASH_BMC));
    EXPECT_TRUE(ut_is_blocking_update_from(SPI_FLASH_PCH));

    // Check number of T-1 entry
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(1));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_bmc_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_pch_only_counter(), alt_u32(MAX_FAILED_UPDATE_ATTEMPTS_FROM_PCH));

    // Check Panic event count
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(MAX_FAILED_UPDATE_ATTEMPTS_FROM_PCH));

    // Check the error message
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(MAJOR_ERROR_UPDATE_FROM_PCH_FAILED));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_EXCEEDED_MAX_FAILED_ATTEMPTS));
}

/**
 * @brief This test sends IB PCH recovery update request, but has BMC update capsule in staging area of PCH flash.
 */
TEST_F(UpdateFlowMixAndMatchAttackTest, test_send_ib_pch_recovery_update_with_bmc_capsule)
{
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
    ut_send_in_update_intent(MB_PCH_UPDATE_INTENT, MB_UPDATE_INTENT_PCH_RECOVERY_MASK);

    // Load the fw update capsule to staging region in flash
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_PCH, SIGNED_CAPSULE_BMC_FILE,
            SIGNED_CAPSULE_BMC_FILE_SIZE, get_ufm_pfr_data()->pch_staging_region);

    /*
     * Execute the flow. Active update flow will be triggered.
     * Since there's no update capsule in staging region, update would fail.
     */
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(20, pfr_main());

    /*
     * Check results.
     */
    EXPECT_FALSE(ut_is_blocking_update_from(SPI_FLASH_BMC));
    EXPECT_TRUE(ut_is_blocking_update_from(SPI_FLASH_PCH));

    // Check number of T-1 entry
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(1 + MAX_FAILED_UPDATE_ATTEMPTS_FROM_PCH));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_bmc_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_pch_only_counter(), alt_u32(0));

    // Check Panic event count
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(MAX_FAILED_UPDATE_ATTEMPTS_FROM_PCH));

    // Check the error message
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(MAJOR_ERROR_UPDATE_FROM_PCH_FAILED));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_EXCEEDED_MAX_FAILED_ATTEMPTS));
}
