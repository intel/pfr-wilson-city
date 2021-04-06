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

class PFRAntiRollbackWithSVNTest : public testing::Test
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
 * @brief This test sends in an PCH active firmware update request with a capsule that has invalid SVN in PFM.
 * The PFM has 0xFF as SVN in the update capsule.
 */
TEST_F(PFRAntiRollbackWithSVNTest, test_active_fw_update_with_invalid_svn)
{
    /*
     * Test Preparation
     */
    // Load the update capsule
    SYSTEM_MOCK::get()->load_to_flash(
            SPI_FLASH_PCH,
            SIGNED_CAPSULE_PCH_PFM_WITH_SVN255_FILE,
            SIGNED_CAPSULE_PCH_PFM_WITH_SVN255_FILE_SIZE,
            get_ufm_pfr_data()->pch_staging_region);

    /*
     * Flow Preparation
     */
    ut_prep_nios_gpi_signals();
    ut_send_block_complete_chkpt_msg();

    // Exit after 50 iterations in the T0 loop
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS_END_AFTER_50_ITERS);

    // Send the active update intent
    ut_send_in_update_intent_once_upon_entry_to_t0(MB_PCH_UPDATE_INTENT, MB_UPDATE_INTENT_PCH_ACTIVE_MASK);

    /*
     * Run Nios FW
     */
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(40, pfr_main());

    // Nios should transition from T0 to T-1 exactly once for firmware update
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(1));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), LAST_PANIC_PCH_UPDATE_INTENT);

    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(0));

    // Expecting error
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(MAJOR_ERROR_UPDATE_FROM_PCH_FAILED));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_INVALID_SVN));
}

/**
 * @brief This test sends in an PCH recovery firmware update request with a capsule that has invalid SVN in PFM.
 * The PFM has 0xFF as SVN in the update capsule.
 */
TEST_F(PFRAntiRollbackWithSVNTest, test_recovery_fw_update_with_invalid_svn)
{
    /*
     * Test Preparation
     */
    // Load the update capsule
    SYSTEM_MOCK::get()->load_to_flash(
            SPI_FLASH_PCH,
            SIGNED_CAPSULE_PCH_PFM_WITH_SVN255_FILE,
            SIGNED_CAPSULE_PCH_PFM_WITH_SVN255_FILE_SIZE,
            get_ufm_pfr_data()->pch_staging_region);

    /*
     * Flow Preparation
     */
    ut_prep_nios_gpi_signals();
    ut_send_block_complete_chkpt_msg();

    // Exit after 50 iterations in the T0 loop
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS_END_AFTER_50_ITERS);

    // Send the active update intent
    ut_send_in_update_intent_once_upon_entry_to_t0(MB_PCH_UPDATE_INTENT, MB_UPDATE_INTENT_PCH_RECOVERY_MASK);

    /*
     * Run Nios FW
     */
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(40, pfr_main());

    // Nios should transition from T0 to T-1 exactly once for firmware update
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(1));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), LAST_PANIC_PCH_UPDATE_INTENT);

    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(0));

    // Expecting error
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(MAJOR_ERROR_UPDATE_FROM_PCH_FAILED));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_INVALID_SVN));
}

/**
 * @brief This test sends in an PCH active firmware update request with a capsule that has banned SVN in PFM.
 * The PFM has 0x7 as SVN in the update capsule.
 * In this test, the update intent is sent from "BMC" (i.e. sent to the BMC update intent register). This is
 * for exercising more FW path.
 */
TEST_F(PFRAntiRollbackWithSVNTest, test_active_fw_update_with_banned_svn)
{
    /*
     * Test Preparation
     */
    // Load the update capsule
    SYSTEM_MOCK::get()->load_to_flash(
            SPI_FLASH_BMC,
            SIGNED_CAPSULE_PCH_PFM_WITH_SVN7_FILE,
            SIGNED_CAPSULE_PCH_PFM_WITH_SVN7_FILE_SIZE,
            get_ufm_pfr_data()->bmc_staging_region + BMC_STAGING_REGION_PCH_UPDATE_CAPSULE_OFFSET);

    /*
     * Flow Preparation
     */
    ut_prep_nios_gpi_signals();
    ut_send_block_complete_chkpt_msg();

    // Exit after 50 iterations in the T0 loop
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS_END_AFTER_50_ITERS);

    // Send the active update intent
    ut_send_in_update_intent_once_upon_entry_to_t0(MB_BMC_UPDATE_INTENT, MB_UPDATE_INTENT_PCH_ACTIVE_MASK);

    // Ban all SVNs up to 8 (excluding 8)
    write_ufm_svn(8, UFM_SVN_POLICY_PCH);

    /*
     * Run Nios FW
     */
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(40, pfr_main());

    // Nios should transition from T0 to T-1 exactly once for firmware update
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(1));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), LAST_PANIC_BMC_UPDATE_INTENT);

    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(0));

    // Expecting error
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(MAJOR_ERROR_UPDATE_FROM_BMC_FAILED));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_INVALID_SVN));
}

/**
 * @brief This test sends in an PCH recovery firmware update request with a capsule that has banned SVN in PFM.
 * The PFM has 0x7 as SVN in the update capsule.
 * In this test, the update intent is sent from "BMC" (i.e. sent to the BMC update intent register). This is
 * for exercising more FW path.
 */
TEST_F(PFRAntiRollbackWithSVNTest, test_recovery_fw_update_with_banned_svn)
{
    /*
     * Test Preparation
     */
    // Load the update capsule
    SYSTEM_MOCK::get()->load_to_flash(
            SPI_FLASH_BMC,
            SIGNED_CAPSULE_PCH_PFM_WITH_SVN7_FILE,
            SIGNED_CAPSULE_PCH_PFM_WITH_SVN7_FILE_SIZE,
            get_ufm_pfr_data()->bmc_staging_region + BMC_STAGING_REGION_PCH_UPDATE_CAPSULE_OFFSET);

    /*
     * Flow Preparation
     */
    ut_prep_nios_gpi_signals();
    ut_send_block_complete_chkpt_msg();

    // Exit after 50 iterations in the T0 loop
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS_END_AFTER_50_ITERS);

    // Send the active update intent
    ut_send_in_update_intent_once_upon_entry_to_t0(MB_BMC_UPDATE_INTENT, MB_UPDATE_INTENT_PCH_RECOVERY_MASK);

    // Ban all SVNs up to 8 (excluding 8)
    write_ufm_svn(8, UFM_SVN_POLICY_PCH);

    /*
     * Run Nios FW
     */
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(40, pfr_main());

    // Nios should transition from T0 to T-1 exactly once for firmware update
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(1));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), LAST_PANIC_BMC_UPDATE_INTENT);

    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(0));

    // Expecting error
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(MAJOR_ERROR_UPDATE_FROM_BMC_FAILED));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_INVALID_SVN));
}

/**
 * @brief This test attempts to bump the PCH SVN with an active firmware update, which is not allowed.
 * If user wishes to bump the SVN, a recovery update must be issued.
 */
TEST_F(PFRAntiRollbackWithSVNTest, test_bump_svn_with_active_fw_update)
{
    /*
     * Test Preparation
     */
    // Load the update capsule
    SYSTEM_MOCK::get()->load_to_flash(
            SPI_FLASH_PCH,
            SIGNED_CAPSULE_PCH_PFM_WITH_SVN9_FILE,
            SIGNED_CAPSULE_PCH_PFM_WITH_SVN9_FILE_SIZE,
            get_ufm_pfr_data()->pch_staging_region);

    /*
     * Flow preparation
     */
    ut_prep_nios_gpi_signals();
    ut_send_block_complete_chkpt_msg();

    // Exit after 50 iterations in the T0 loop
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS_END_AFTER_50_ITERS);

    // Send the active update intent
    ut_send_in_update_intent_once_upon_entry_to_t0(MB_PCH_UPDATE_INTENT, MB_UPDATE_INTENT_PCH_ACTIVE_MASK);

    /*
     * Run Nios FW
     */
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(80, pfr_main());

    // Expect timed boot to be completed
    EXPECT_EQ(read_from_mailbox(MB_PLATFORM_STATE), alt_u32(PLATFORM_STATE_T0_BOOT_COMPLETE));
    // Have entered T-1 two times: Boot up + going T-1 for active update
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(1));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_bmc_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_pch_only_counter(), alt_u32(1));

    // Nios should transition from T0 to T-1 exactly once for active update flow
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(1));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), LAST_PANIC_PCH_UPDATE_INTENT);

    // Expecting no error
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(MAJOR_ERROR_UPDATE_FROM_PCH_FAILED));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_INVALID_SVN));
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(0));
}

/**
 * @brief This test bumps the SVN with a firmware update and issues another update request with a capsule
 * that has lower SVN.
 */
TEST_F(PFRAntiRollbackWithSVNTest, test_pch_active_update_with_lower_svn)
{
    /*
     * Test Preparation
     */
    // Load the update capsule
    SYSTEM_MOCK::get()->load_to_flash(
            SPI_FLASH_PCH,
            SIGNED_CAPSULE_PCH_PFM_WITH_SVN9_FILE,
            SIGNED_CAPSULE_PCH_PFM_WITH_SVN9_FILE_SIZE,
            get_ufm_pfr_data()->pch_staging_region);

    /*
     * Flow preparation
     */
    ut_prep_nios_gpi_signals();
    ut_send_block_complete_chkpt_msg();

    // Exit after 50 iterations in the T0 loop
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS_END_AFTER_50_ITERS);

    // Send the recovery update intent
    // Important: In order to bump the SVN, a recovery update must be issued.
    ut_send_in_update_intent_once_upon_entry_to_t0(MB_PCH_UPDATE_INTENT, MB_UPDATE_INTENT_PCH_RECOVERY_MASK);

    /*
     * Step 1: Perform recovery firmware update with capsule that has SVN 0x9
     */
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(80, pfr_main());

    // Expect timed boot to be completed
    EXPECT_EQ(read_from_mailbox(MB_PLATFORM_STATE), alt_u32(PLATFORM_STATE_T0_BOOT_COMPLETE));
    // Have entered T-1 three times: Boot up + going T-1 for active update + going T-1 for recovery update
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(3));

    // Nios should transition from T0 to T-1 twice for recovery update flow
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(2));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), LAST_PANIC_PCH_UPDATE_INTENT);

    // Expecting no error
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(0));

    /*
     * Step 2: Attempt another active firmware update with capsule that has SVN 0x7
     */
    // Load the update capsule
    SYSTEM_MOCK::get()->load_to_flash(
            SPI_FLASH_PCH,
            SIGNED_CAPSULE_PCH_PFM_WITH_SVN7_FILE,
            SIGNED_CAPSULE_PCH_PFM_WITH_SVN7_FILE_SIZE,
            get_ufm_pfr_data()->pch_staging_region);

    // Send the update intent
    write_to_mailbox(MB_PCH_UPDATE_INTENT, MB_UPDATE_INTENT_PCH_ACTIVE_MASK);

    // Process update intent in T0
    mb_update_intent_handler();

    // Nios should transition from T0 to T-1 once for active update flow
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(3));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), LAST_PANIC_PCH_UPDATE_INTENT);

    // Expecting errors since the minimal allowed SVN is now 0x9.
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(MAJOR_ERROR_UPDATE_FROM_PCH_FAILED));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_INVALID_SVN));
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(0));

    // Check the PCH SVN policy
    EXPECT_FALSE(is_svn_valid(UFM_SVN_POLICY_PCH, 0x8));
    EXPECT_TRUE(is_svn_valid(UFM_SVN_POLICY_PCH, 0x9));
    EXPECT_TRUE(is_svn_valid(UFM_SVN_POLICY_PCH, 0xA));

    // Make sure BMC/CPLD SVN policies are unaffected
    EXPECT_TRUE(is_svn_valid(UFM_SVN_POLICY_BMC, 0x0));
    EXPECT_TRUE(is_svn_valid(UFM_SVN_POLICY_BMC, 0x1));
    EXPECT_TRUE(is_svn_valid(UFM_SVN_POLICY_CPLD, 0x0));
    EXPECT_TRUE(is_svn_valid(UFM_SVN_POLICY_CPLD, 0x1));
}

/**
 * @brief This test ensures update request with lower SVN is rejected, when platform is initialized with firmware that has non-zero SVN.
 * If platform is initialized with firmware that has non-zero SVN and CPLD UFM has been locked,
 * then the CPLD internal SVN policies for these firmwares should be non-zero. This is checked.
 * This test also attempts to issue a firmware update with lower SVN. This request should be rejected.
 */
TEST_F(PFRAntiRollbackWithSVNTest, test_pch_update_with_lower_svn_with_pch_flash_initialized_with_non_zero_svn_fw)
{
    /*
     * Test Preparation
     */
    // Load a recovery image that has SVN 7 to PCH flash.
    SYSTEM_MOCK::get()->load_to_flash(
            SPI_FLASH_PCH,
            SIGNED_CAPSULE_PCH_PFM_WITH_SVN7_FILE,
            SIGNED_CAPSULE_PCH_PFM_WITH_SVN7_FILE_SIZE,
            get_ufm_pfr_data()->pch_recovery_region);
    // Load the update capsule with SVN 0 to staging region
    SYSTEM_MOCK::get()->load_to_flash(
            SPI_FLASH_PCH,
            SIGNED_CAPSULE_PCH_FILE,
            SIGNED_CAPSULE_PCH_FILE_SIZE,
            get_ufm_pfr_data()->pch_staging_region);

    /*
     * Flow preparation
     */
    ut_prep_nios_gpi_signals();
    ut_send_block_complete_chkpt_msg();

    // Assume UFM is not locked yet
    EXPECT_FALSE(is_ufm_locked());
    // Send UFM lock command (which should update the SVN policy)
    // Note that this will be send in before the update intent, because update intent needs to wait until boot completion.
    ut_send_in_ufm_command(MB_UFM_PROV_END);

    // Exit after 50 iterations in the T0 loop
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS_END_AFTER_50_ITERS);

    // Send the recovery update intent
    ut_send_in_update_intent_once_upon_boot_complete(MB_PCH_UPDATE_INTENT, MB_UPDATE_INTENT_PCH_RECOVERY_MASK);

    /*
     * Execute flow
     */
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(80, pfr_main());

    /*
     * Check results
     */
    // Nios should transition from T0 to T-1 once, in response to the update request
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(1));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), LAST_PANIC_PCH_UPDATE_INTENT);

    // Expecting errors since the minimal allowed SVN is now 0x7.
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(MAJOR_ERROR_UPDATE_FROM_PCH_FAILED));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_INVALID_SVN));
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(0));

    // Check the PCH SVN policy
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_PCH), alt_u32(0x7));
    EXPECT_FALSE(is_svn_valid(UFM_SVN_POLICY_PCH, 0x6));
    EXPECT_TRUE(is_svn_valid(UFM_SVN_POLICY_PCH, 0x7));
    EXPECT_TRUE(is_svn_valid(UFM_SVN_POLICY_PCH, 0x8));

    // Make sure BMC/CPLD SVN policies are unaffected
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_BMC), alt_u32(BMC_RECOVERY_PFM_SVN));
    EXPECT_FALSE(is_svn_valid(UFM_SVN_POLICY_BMC, 0x0));
    EXPECT_TRUE(is_svn_valid(UFM_SVN_POLICY_BMC, 0x1));

    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_CPLD), alt_u32(0));
    EXPECT_TRUE(is_svn_valid(UFM_SVN_POLICY_CPLD, 0x0));
    EXPECT_TRUE(is_svn_valid(UFM_SVN_POLICY_CPLD, 0x1));
}

/**
 * @brief This test ensures update request with lower SVN is rejected, when platform is initialized with firmware that has non-zero SVN.
 * If platform is initialized with firmware that has non-zero SVN and CPLD UFM has been locked,
 * then the CPLD internal SVN policies for these firmwares should be non-zero. This is checked.
 * This test also attempts to issue a firmware update with lower SVN. This request should be rejected.
 */
TEST_F(PFRAntiRollbackWithSVNTest, test_bmc_update_with_lower_svn_with_bmc_flash_initialized_with_non_zero_svn_fw)
{
    /*
     * Test Preparation
     */
    // Load a recovery image that has SVN 2 to BMC flash.
    SYSTEM_MOCK::get()->load_to_flash(
            SPI_FLASH_BMC,
            SIGNED_CAPSULE_BMC_WITH_SVN2_FILE,
            SIGNED_CAPSULE_BMC_WITH_SVN2_FILE_SIZE,
            get_ufm_pfr_data()->bmc_recovery_region);
    // Load the update capsule with SVN 1 to staging region
    SYSTEM_MOCK::get()->load_to_flash(
            SPI_FLASH_BMC,
            SIGNED_CAPSULE_BMC_FILE,
            SIGNED_CAPSULE_BMC_FILE_SIZE,
            get_ufm_pfr_data()->bmc_staging_region);

    /*
     * Flow preparation
     */
    ut_prep_nios_gpi_signals();
    ut_send_block_complete_chkpt_msg();

    // Assume UFM is not locked yet
    EXPECT_FALSE(is_ufm_locked());
    // Send UFM lock command (which should update the SVN policy)
    // Note that this will be send in before the update intent, because update intent needs to wait until boot completion.
    ut_send_in_ufm_command(MB_UFM_PROV_END);

    // Exit after 50 iterations in the T0 loop
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS_END_AFTER_50_ITERS);

    // Send the recovery update intent
    ut_send_in_update_intent_once_upon_boot_complete(MB_BMC_UPDATE_INTENT, MB_UPDATE_INTENT_BMC_RECOVERY_MASK);

    /*
     * Execute flow
     */
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(80, pfr_main());

    /*
     * Check results
     */
    // Nios should transition from T0 to T-1 once, in response to the update request
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(1));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), LAST_PANIC_BMC_UPDATE_INTENT);

    // Expecting errors since the minimal allowed SVN is now 0x7.
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(MAJOR_ERROR_UPDATE_FROM_BMC_FAILED));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_INVALID_SVN));
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(0));

    // Check the PCH SVN policy
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_PCH), alt_u32(PCH_RECOVERY_PFM_SVN));
    EXPECT_TRUE(is_svn_valid(UFM_SVN_POLICY_PCH, 0));
    EXPECT_TRUE(is_svn_valid(UFM_SVN_POLICY_PCH, 2));
    EXPECT_TRUE(is_svn_valid(UFM_SVN_POLICY_PCH, 4));

    // Make sure BMC/CPLD SVN policies are unaffected
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_BMC), alt_u32(2));
    EXPECT_FALSE(is_svn_valid(UFM_SVN_POLICY_BMC, 0x0));
    EXPECT_FALSE(is_svn_valid(UFM_SVN_POLICY_BMC, 0x1));
    EXPECT_TRUE(is_svn_valid(UFM_SVN_POLICY_BMC, 0x2));

    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_CPLD), alt_u32(0));
    EXPECT_TRUE(is_svn_valid(UFM_SVN_POLICY_CPLD, 0x0));
    EXPECT_TRUE(is_svn_valid(UFM_SVN_POLICY_CPLD, 0x1));
}

/**
 * @brief This test bumps the SVN with a firmware update and issues another update request with a capsule
 * that has lower SVN.
 */
TEST_F(PFRAntiRollbackWithSVNTest, test_bmc_recovery_update_with_lower_svn)
{
    /*
     * Test Preparation
     */
    // Load the update capsule
    SYSTEM_MOCK::get()->load_to_flash(
            SPI_FLASH_BMC,
            SIGNED_CAPSULE_BMC_WITH_SVN2_FILE,
            SIGNED_CAPSULE_BMC_WITH_SVN2_FILE_SIZE,
            get_ufm_pfr_data()->bmc_staging_region);

    /*
     * Flow preparation
     */
    ut_prep_nios_gpi_signals();
    ut_send_block_complete_chkpt_msg();

    // Exit after 50 iterations in the T0 loop
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS_END_AFTER_50_ITERS);

    // Send the recovery update intent
    // Important: In order to bump the SVN, a recovery update must be issued.
    ut_send_in_update_intent_once_upon_entry_to_t0(MB_BMC_UPDATE_INTENT, MB_UPDATE_INTENT_BMC_RECOVERY_MASK);

    /*
     * Step 1: Perform recovery firmware update with capsule that has SVN 0x2
     */
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(80, pfr_main());

    // Expect timed boot to be completed
    EXPECT_EQ(read_from_mailbox(MB_PLATFORM_STATE), alt_u32(PLATFORM_STATE_T0_BOOT_COMPLETE));
    // Have entered T-1 three times: Boot up + going T-1 for active update + going T-1 for recovery update
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(3));

    // Nios should transition from T0 to T-1 twice for recovery update flow
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(2));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), LAST_PANIC_BMC_UPDATE_INTENT);

    // Expecting no error
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(0));

    // Check the BMC SVN policy
    EXPECT_FALSE(is_svn_valid(UFM_SVN_POLICY_BMC, 0));
    EXPECT_FALSE(is_svn_valid(UFM_SVN_POLICY_BMC, 1));
    EXPECT_TRUE(is_svn_valid(UFM_SVN_POLICY_BMC, 2));
    EXPECT_TRUE(is_svn_valid(UFM_SVN_POLICY_BMC, 3));

    /*
     * Step 2: Attempt another active firmware update with capsule that has SVN 0x1
     */
    // Load the update capsule
    SYSTEM_MOCK::get()->load_to_flash(
            SPI_FLASH_BMC,
            SIGNED_CAPSULE_BMC_FILE,
            SIGNED_CAPSULE_BMC_FILE_SIZE,
            get_ufm_pfr_data()->bmc_staging_region);

    // Send the update intent
    write_to_mailbox(MB_BMC_UPDATE_INTENT, MB_UPDATE_INTENT_BMC_RECOVERY_MASK);

    // Process update intent in T0
    mb_update_intent_handler();

    // Nios should transition from T0 to T-1 once for active update flow
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(3));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), LAST_PANIC_BMC_UPDATE_INTENT);

    // Expecting errors since the minimal allowed SVN is now 0x2.
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(MAJOR_ERROR_UPDATE_FROM_BMC_FAILED));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_INVALID_SVN));
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(0));

    // Check the BMC SVN policy
    EXPECT_FALSE(is_svn_valid(UFM_SVN_POLICY_BMC, 0));
    EXPECT_FALSE(is_svn_valid(UFM_SVN_POLICY_BMC, 1));
    EXPECT_TRUE(is_svn_valid(UFM_SVN_POLICY_BMC, 2));
    EXPECT_TRUE(is_svn_valid(UFM_SVN_POLICY_BMC, 3));

    // Make sure PCH/CPLD SVN policies are unaffected
    EXPECT_TRUE(is_svn_valid(UFM_SVN_POLICY_PCH, 0x0));
    EXPECT_TRUE(is_svn_valid(UFM_SVN_POLICY_PCH, 0x1));
    EXPECT_TRUE(is_svn_valid(UFM_SVN_POLICY_CPLD, 0x0));
    EXPECT_TRUE(is_svn_valid(UFM_SVN_POLICY_CPLD, 0x1));
}

/**
 * @brief This test sends a CPLD active update with capsule that has higher SVN.
 * The active update should go through while the SVN remains unchanged.
 * If user wishes to bump the SVN, a recovery CPLD update must be issued.
 */
TEST_F(PFRAntiRollbackWithSVNTest, test_bump_svn_with_active_cpld_update)
{
    /*
     * Test Preparation
     */
    switch_spi_flash(SPI_FLASH_BMC);

    // Load the update capsule
    SYSTEM_MOCK::get()->load_to_flash(
            SPI_FLASH_BMC,
            SIGNED_CAPSULE_CPLD_WITH_SVN1_FILE,
            SIGNED_CAPSULE_CPLD_WITH_SVN1_FILE_SIZE,
            get_ufm_pfr_data()->bmc_staging_region + BMC_STAGING_REGION_CPLD_UPDATE_CAPSULE_OFFSET);

    // Load ROM image to CFM0
    alt_u32* cfm0_image = new alt_u32[CFM0_RECOVERY_IMAGE_FILE_SIZE/4];
    alt_u32* cfm0_ufm_ptr = get_ufm_ptr_with_offset(UFM_CPLD_ROM_IMAGE_OFFSET);

    SYSTEM_MOCK::get()->init_x86_mem_from_file(CFM0_RECOVERY_IMAGE_FILE, cfm0_image);
    SYSTEM_MOCK::get()->init_x86_mem_from_file(CFM0_RECOVERY_IMAGE_FILE, cfm0_ufm_ptr);

    // Load active image to CFM1
    alt_u32* cfm1_image = new alt_u32[CFM1_ACTIVE_IMAGE_FILE_SIZE/4];
    alt_u32* cfm1_ufm_ptr = get_ufm_ptr_with_offset(UFM_CPLD_ACTIVE_IMAGE_OFFSET);

    SYSTEM_MOCK::get()->init_x86_mem_from_file(CFM1_ACTIVE_IMAGE_FILE, cfm1_image);
    SYSTEM_MOCK::get()->init_x86_mem_from_file(CFM1_ACTIVE_IMAGE_FILE, cfm1_ufm_ptr);

    // Load active image to CPLD Recovery Region
    alt_u32* cpld_recovery_capsule_local = new alt_u32[SIGNED_CAPSULE_CPLD_FILE_SIZE/4];
    alt_u32* cpld_recovery_capsule_ptr = get_spi_flash_ptr_with_offset(BMC_CPLD_RECOVERY_IMAGE_OFFSET);

    SYSTEM_MOCK::get()->init_x86_mem_from_file(SIGNED_CAPSULE_CPLD_FILE, cpld_recovery_capsule_local);
    SYSTEM_MOCK::get()->init_x86_mem_from_file(SIGNED_CAPSULE_CPLD_FILE, cpld_recovery_capsule_ptr);

    /*
     * Flow preparation
     */
    // Exit after 50 iterations in the T0 loop
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS_END_AFTER_50_ITERS);

    // Set asserts to throw as opposed to abort
    SYSTEM_MOCK::get()->set_assert_to_throw();
    // Throw after performing CFM switch
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::THROW_AFTER_CFM_SWITCH);

    /*
     * Run Nios FW through PFR/Recovery Main.
     * Max10 is set up to boot with CFM0 (recovery_main). It will switch to CFM1 shortly after.
     * After receiving CPLD update intent, CPLD will switch from CFM1 to CFM0 to do update there.
     * Once update is complete in CFM0, CPLD will switch back to CFM1.
     */
    ut_run_main(CPLD_CFM0, true);

    // Send the active update intent
    ut_send_in_update_intent_once_upon_entry_to_t0(MB_BMC_UPDATE_INTENT, MB_UPDATE_INTENT_CPLD_ACTIVE_MASK);
    ut_run_main(CPLD_CFM1, true);

    ut_run_main(CPLD_CFM0, true);
    ut_run_main(CPLD_CFM1, false);

    /*
     * Verify results
     */
    // Expecting no error
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(0));

    // Update should have happened
    switch_spi_flash(SPI_FLASH_BMC);
    alt_u32* bmc_cpld_update_capsule_ptr =
            get_spi_flash_ptr_with_offset(get_ufm_pfr_data()->bmc_staging_region + BMC_STAGING_REGION_CPLD_UPDATE_CAPSULE_OFFSET);
    CPLD_UPDATE_PC* cpld_update_pc = (CPLD_UPDATE_PC*) incr_alt_u32_ptr(bmc_cpld_update_capsule_ptr, SIGNATURE_SIZE);
    alt_u32* cpld_update_bitstream = cpld_update_pc->cpld_bitstream;

    // Active image should be updated
    for (alt_u32 i = 0; i < (UFM_CPLD_ACTIVE_IMAGE_LENGTH/4); i++)
    {
        ASSERT_EQ(cfm1_ufm_ptr[i], cpld_update_bitstream[i]);
    }

    // CPLD ROM image should be untouched
    for (alt_u32 i = 0; i < (UFM_CPLD_ROM_IMAGE_LENGTH/4); i++)
    {
        ASSERT_EQ(cfm0_ufm_ptr[i], cfm0_image[i]);
    }

    // Recovery image should be untouched (i.e. match the previous active image)
    for (int i = 1; i < (SIGNED_CAPSULE_CPLD_FILE_SIZE/4); i++)
    {
        ASSERT_EQ(cpld_recovery_capsule_ptr[i], cpld_recovery_capsule_local[i]);
    }

    // SVN policy for CPLD update should remain unchanged
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_CPLD), alt_u32(0));
    EXPECT_TRUE(is_svn_valid(UFM_SVN_POLICY_CPLD, 0x0));
    EXPECT_EQ(read_from_mailbox(MB_CPLD_SVN), alt_u32(0));

    /*
     * Clean up
     */
    delete[] cfm0_image;
    delete[] cfm1_image;
    delete[] cpld_recovery_capsule_local;
}

/**
 * @brief This test issues a recovery CPLD update with higher SVN.
 * SVN policy should be updated afterwards.
 */
TEST_F(PFRAntiRollbackWithSVNTest, test_bump_svn_with_recovery_cpld_update)
{
    /*
     * Test Preparation
     */
    switch_spi_flash(SPI_FLASH_BMC);

    // Load the update capsule
    SYSTEM_MOCK::get()->load_to_flash(
            SPI_FLASH_BMC,
            SIGNED_CAPSULE_CPLD_WITH_SVN1_FILE,
            SIGNED_CAPSULE_CPLD_WITH_SVN1_FILE_SIZE,
            get_ufm_pfr_data()->bmc_staging_region + BMC_STAGING_REGION_CPLD_UPDATE_CAPSULE_OFFSET);

    // Load ROM image to CFM0
    alt_u32* cfm0_image = new alt_u32[CFM0_RECOVERY_IMAGE_FILE_SIZE/4];
    alt_u32* cfm0_ufm_ptr = get_ufm_ptr_with_offset(UFM_CPLD_ROM_IMAGE_OFFSET);

    SYSTEM_MOCK::get()->init_x86_mem_from_file(CFM0_RECOVERY_IMAGE_FILE, cfm0_image);
    SYSTEM_MOCK::get()->init_x86_mem_from_file(CFM0_RECOVERY_IMAGE_FILE, cfm0_ufm_ptr);

    // Load active image to CFM1
    alt_u32* cfm1_image = new alt_u32[CFM1_ACTIVE_IMAGE_FILE_SIZE/4];
    alt_u32* cfm1_ufm_ptr = get_ufm_ptr_with_offset(UFM_CPLD_ACTIVE_IMAGE_OFFSET);

    SYSTEM_MOCK::get()->init_x86_mem_from_file(CFM1_ACTIVE_IMAGE_FILE, cfm1_image);
    SYSTEM_MOCK::get()->init_x86_mem_from_file(CFM1_ACTIVE_IMAGE_FILE, cfm1_ufm_ptr);

    // Load active image to CPLD Recovery Region
    alt_u32* cpld_recovery_capsule_ptr = get_spi_flash_ptr_with_offset(BMC_CPLD_RECOVERY_IMAGE_OFFSET);
    SYSTEM_MOCK::get()->init_x86_mem_from_file(SIGNED_CAPSULE_CPLD_FILE, cpld_recovery_capsule_ptr);

    /*
     * Flow preparation
     */
    // Exit after 50 iterations in the T0 loop
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS_END_AFTER_50_ITERS);

    // Set asserts to throw as opposed to abort
    SYSTEM_MOCK::get()->set_assert_to_throw();
    // Throw after performing CFM switch
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::THROW_AFTER_CFM_SWITCH);

    /*
     * Run Nios FW through PFR/Recovery Main.
     * Max10 is set up to boot with CFM0 (recovery_main). It will switch to CFM1 shortly after.
     * After receiving CPLD update intent, CPLD will switch from CFM1 to CFM0 to do update there.
     * Once update is complete in CFM0, CPLD will switch back to CFM1.
     */
    ut_run_main(CPLD_CFM0, true);

    // Send the active update intent
    ut_send_in_update_intent_once_upon_entry_to_t0(MB_BMC_UPDATE_INTENT, MB_UPDATE_INTENT_CPLD_RECOVERY_MASK);
    ut_run_main(CPLD_CFM1, true);

    ut_run_main(CPLD_CFM0, true);
    ut_run_main(CPLD_CFM1, false);

    /*
     * Verify results
     */
    // Expecting no error
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(0));

    // Update should have happened
    switch_spi_flash(SPI_FLASH_BMC);
    alt_u32* bmc_cpld_update_capsule_ptr =
            get_spi_flash_ptr_with_offset(get_ufm_pfr_data()->bmc_staging_region + BMC_STAGING_REGION_CPLD_UPDATE_CAPSULE_OFFSET);
    CPLD_UPDATE_PC* cpld_update_pc = (CPLD_UPDATE_PC*) incr_alt_u32_ptr(bmc_cpld_update_capsule_ptr, SIGNATURE_SIZE);
    alt_u32* cpld_update_bitstream = cpld_update_pc->cpld_bitstream;

    // Active image should be updated
    for (alt_u32 i = 0; i < (UFM_CPLD_ACTIVE_IMAGE_LENGTH/4); i++)
    {
        ASSERT_EQ(cfm1_ufm_ptr[i], cpld_update_bitstream[i]);
    }

    // CPLD ROM image should be untouched
    for (alt_u32 i = 0; i < (UFM_CPLD_ROM_IMAGE_LENGTH/4); i++)
    {
        ASSERT_EQ(cfm0_ufm_ptr[i], cfm0_image[i]);
    }

    // Recovery image should be updated (i.e. match the update capsule)
    for (int i = 1; i < (SIGNED_CAPSULE_CPLD_FILE_SIZE/4); i++)
    {
        ASSERT_EQ(cpld_recovery_capsule_ptr[i], bmc_cpld_update_capsule_ptr[i]);
    }

    // SVN policy for CPLD update should have been updated
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_CPLD), alt_u32(1));
    EXPECT_FALSE(is_svn_valid(UFM_SVN_POLICY_CPLD, 0x0));
    EXPECT_TRUE(is_svn_valid(UFM_SVN_POLICY_CPLD, 0x1));
    EXPECT_EQ(read_from_mailbox(MB_CPLD_SVN), alt_u32(1));

    /*
     * Clean up
     */
    delete[] cfm0_image;
    delete[] cfm1_image;
}

/**
 * @brief This test issues a recovery CPLD update to bump the SVN to 1, then attempts to rollback CPLD image to SVN 0.
 * Any CPLD update with SVN 0 should be rejected.
 */
TEST_F(PFRAntiRollbackWithSVNTest, test_cpld_updates_with_banned_svn)
{
    /*
     * Test Preparation
     */
    switch_spi_flash(SPI_FLASH_BMC);

    // Load ROM image to CFM0
    alt_u32* cfm0_image = new alt_u32[CFM0_RECOVERY_IMAGE_FILE_SIZE/4];
    alt_u32* cfm0_ufm_ptr = get_ufm_ptr_with_offset(UFM_CPLD_ROM_IMAGE_OFFSET);

    SYSTEM_MOCK::get()->init_x86_mem_from_file(CFM0_RECOVERY_IMAGE_FILE, cfm0_image);
    SYSTEM_MOCK::get()->init_x86_mem_from_file(CFM0_RECOVERY_IMAGE_FILE, cfm0_ufm_ptr);

    // Load active image to CFM1
    alt_u32* cfm1_image = new alt_u32[CFM1_ACTIVE_IMAGE_FILE_SIZE/4];
    alt_u32* cfm1_ufm_ptr = get_ufm_ptr_with_offset(UFM_CPLD_ACTIVE_IMAGE_OFFSET);

    SYSTEM_MOCK::get()->init_x86_mem_from_file(CFM1_ACTIVE_IMAGE_FILE, cfm1_image);
    SYSTEM_MOCK::get()->init_x86_mem_from_file(CFM1_ACTIVE_IMAGE_FILE, cfm1_ufm_ptr);

    // Load active image to CPLD Recovery Region
    alt_u32* cpld_recovery_capsule_ptr = get_spi_flash_ptr_with_offset(BMC_CPLD_RECOVERY_IMAGE_OFFSET);
    SYSTEM_MOCK::get()->init_x86_mem_from_file(SIGNED_CAPSULE_CPLD_FILE, cpld_recovery_capsule_ptr);

    // Load expected image for comparison purpose
    alt_u32* expected_cpld_recovery_capsule = new alt_u32[SIGNED_CAPSULE_CPLD_WITH_SVN1_FILE_SIZE/4];
    CPLD_UPDATE_PC* expected_cpld_recovery_capsule_pc = (CPLD_UPDATE_PC*) incr_alt_u32_ptr(expected_cpld_recovery_capsule, SIGNATURE_SIZE);
    alt_u32* expected_cpld_active_image = expected_cpld_recovery_capsule_pc->cpld_bitstream;
    SYSTEM_MOCK::get()->init_x86_mem_from_file(SIGNED_CAPSULE_CPLD_WITH_SVN1_FILE, expected_cpld_recovery_capsule);

    /***********************
     * Step 1: Send in CPLD recovery update to bump the SVN
     ***********************/
    /*
     * Flow preparation
     */
    // Exit after 50 iterations in the T0 loop
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS_END_AFTER_50_ITERS);

    // Set asserts to throw as opposed to abort
    SYSTEM_MOCK::get()->set_assert_to_throw();
    // Throw after performing CFM switch
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::THROW_AFTER_CFM_SWITCH);

    // Load the update capsule
    SYSTEM_MOCK::get()->load_to_flash(
            SPI_FLASH_BMC,
            SIGNED_CAPSULE_CPLD_WITH_SVN1_FILE,
            SIGNED_CAPSULE_CPLD_WITH_SVN1_FILE_SIZE,
            get_ufm_pfr_data()->bmc_staging_region + BMC_STAGING_REGION_CPLD_UPDATE_CAPSULE_OFFSET);

    /*
     * Run Nios FW through PFR/Recovery Main.
     * Max10 is set up to boot with CFM0 (recovery_main). It will switch to CFM1 shortly after.
     * After receiving CPLD update intent, CPLD will switch from CFM1 to CFM0 to do update there.
     * Once update is complete in CFM0, CPLD will switch back to CFM1.
     */
    ut_run_main(CPLD_CFM0, true);

    // Send the recovery update intent
    ut_send_in_update_intent_once_upon_entry_to_t0(MB_BMC_UPDATE_INTENT, MB_UPDATE_INTENT_CPLD_RECOVERY_MASK);
    ut_run_main(CPLD_CFM1, true);

    ut_run_main(CPLD_CFM0, true);
    ut_run_main(CPLD_CFM1, false);

    /*
     * Verify results
     */
    // Expecting no error
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(0));

    // SVN policy for CPLD update should have been updated
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_CPLD), alt_u32(1));
    EXPECT_FALSE(is_svn_valid(UFM_SVN_POLICY_CPLD, 0x0));
    EXPECT_TRUE(is_svn_valid(UFM_SVN_POLICY_CPLD, 0x1));

    // Active image should be updated
    for (alt_u32 i = 0; i < (UFM_CPLD_ACTIVE_IMAGE_LENGTH/4); i++)
    {
        ASSERT_EQ(cfm1_ufm_ptr[i], expected_cpld_active_image[i]);
    }

    // Recovery image should be updated (i.e. match the update capsule)
    for (int i = 1; i < (SIGNED_CAPSULE_CPLD_FILE_SIZE/4); i++)
    {
        ASSERT_EQ(cpld_recovery_capsule_ptr[i], expected_cpld_recovery_capsule[i]);
    }

    /***********************
     * Step 2: Send in CPLD active update with a banned SVN
     ***********************/
    // Load the update capsule (which has SVN 0)
    SYSTEM_MOCK::get()->load_to_flash(
            SPI_FLASH_BMC,
            SIGNED_CAPSULE_CPLD_FILE,
            SIGNED_CAPSULE_CPLD_FILE_SIZE,
            get_ufm_pfr_data()->bmc_staging_region + BMC_STAGING_REGION_CPLD_UPDATE_CAPSULE_OFFSET);

    /*
     * Run Nios FW through PFR/Recovery Main.
     * Max10 is set up to boot with CFM0 (recovery_main). It will switch to CFM1 shortly after.
     */
    SYSTEM_MOCK::get()->reset_ip_mocks();
    ut_run_main(CPLD_CFM0, true);

    // Send the active update intent
    ut_send_in_update_intent_once_upon_entry_to_t0(MB_BMC_UPDATE_INTENT, MB_UPDATE_INTENT_CPLD_ACTIVE_MASK);
    ut_run_main(CPLD_CFM1, false);

    /*
     * Verify results
     */
    // Expecting error about the banned SVN
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(MAJOR_ERROR_UPDATE_FROM_BMC_FAILED));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_INVALID_SVN));
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(0));

    // SVN policy for CPLD update should not be changed
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_CPLD), alt_u32(1));
    EXPECT_FALSE(is_svn_valid(UFM_SVN_POLICY_CPLD, 0x0));
    EXPECT_TRUE(is_svn_valid(UFM_SVN_POLICY_CPLD, 0x1));
    EXPECT_EQ(read_from_mailbox(MB_CPLD_SVN), alt_u32(1));

    // Active image should not be changed
    for (alt_u32 i = 0; i < (UFM_CPLD_ACTIVE_IMAGE_LENGTH/4); i++)
    {
        ASSERT_EQ(cfm1_ufm_ptr[i], expected_cpld_active_image[i]);
    }

    // Recovery image should not be changed
    for (int i = 1; i < (SIGNED_CAPSULE_CPLD_FILE_SIZE/4); i++)
    {
        ASSERT_EQ(cpld_recovery_capsule_ptr[i], expected_cpld_recovery_capsule[i]);
    }

    /***********************
     * Step 3: Send in CPLD recovery update with a banned SVN
     ***********************/
    // Load the update capsule (which has SVN 0)
    SYSTEM_MOCK::get()->load_to_flash(
            SPI_FLASH_BMC,
            SIGNED_CAPSULE_CPLD_FILE,
            SIGNED_CAPSULE_CPLD_FILE_SIZE,
            get_ufm_pfr_data()->bmc_staging_region + BMC_STAGING_REGION_CPLD_UPDATE_CAPSULE_OFFSET);

    /*
     * Run Nios FW through PFR/Recovery Main.
     * Max10 is set up to boot with CFM0 (recovery_main). It will switch to CFM1 shortly after.
     */
    SYSTEM_MOCK::get()->reset_ip_mocks();
    ut_run_main(CPLD_CFM0, true);

    // Send the recovery update intent
    ut_send_in_update_intent_once_upon_entry_to_t0(MB_BMC_UPDATE_INTENT, MB_UPDATE_INTENT_CPLD_RECOVERY_MASK);
    ut_run_main(CPLD_CFM1, false);

    /*
     * Verify results
     */
    // Expecting error about the banned SVN
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(MAJOR_ERROR_UPDATE_FROM_BMC_FAILED));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_INVALID_SVN));
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(0));

    // SVN policy for CPLD update should not be changed
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_CPLD), alt_u32(1));
    EXPECT_FALSE(is_svn_valid(UFM_SVN_POLICY_CPLD, 0x0));
    EXPECT_TRUE(is_svn_valid(UFM_SVN_POLICY_CPLD, 0x1));
    EXPECT_EQ(read_from_mailbox(MB_CPLD_SVN), alt_u32(1));

    // Active image should not be changed
    for (alt_u32 i = 0; i < (UFM_CPLD_ACTIVE_IMAGE_LENGTH/4); i++)
    {
        ASSERT_EQ(cfm1_ufm_ptr[i], expected_cpld_active_image[i]);
    }

    // Recovery image should not be changed
    for (int i = 1; i < (SIGNED_CAPSULE_CPLD_FILE_SIZE/4); i++)
    {
        ASSERT_EQ(cpld_recovery_capsule_ptr[i], expected_cpld_recovery_capsule[i]);
    }

    /*
     * Clean up
     */
    delete[] cfm0_image;
    delete[] cfm1_image;
    delete[] expected_cpld_recovery_capsule;
}

/**
 * @brief This test sends a CPLD recovery update with capsule that has the max SVN.
 * SVN policy should be updated afterwards.
 */
TEST_F(PFRAntiRollbackWithSVNTest, test_bump_svn_to_max_with_recovery_cpld_update)
{
    /*
     * Test Preparation
     */
    switch_spi_flash(SPI_FLASH_BMC);

    // Load ROM image to CFM0
    alt_u32* cfm0_image = new alt_u32[CFM0_RECOVERY_IMAGE_FILE_SIZE/4];
    alt_u32* cfm0_ufm_ptr = get_ufm_ptr_with_offset(UFM_CPLD_ROM_IMAGE_OFFSET);

    SYSTEM_MOCK::get()->init_x86_mem_from_file(CFM0_RECOVERY_IMAGE_FILE, cfm0_image);
    SYSTEM_MOCK::get()->init_x86_mem_from_file(CFM0_RECOVERY_IMAGE_FILE, cfm0_ufm_ptr);

    // Load active image to CFM1
    alt_u32* cfm1_image = new alt_u32[CFM1_ACTIVE_IMAGE_FILE_SIZE/4];
    alt_u32* cfm1_ufm_ptr = get_ufm_ptr_with_offset(UFM_CPLD_ACTIVE_IMAGE_OFFSET);

    SYSTEM_MOCK::get()->init_x86_mem_from_file(CFM1_ACTIVE_IMAGE_FILE, cfm1_image);
    SYSTEM_MOCK::get()->init_x86_mem_from_file(CFM1_ACTIVE_IMAGE_FILE, cfm1_ufm_ptr);

    // Load active image to CPLD Recovery Region
    alt_u32* cpld_recovery_capsule_ptr = get_spi_flash_ptr_with_offset(BMC_CPLD_RECOVERY_IMAGE_OFFSET);
    SYSTEM_MOCK::get()->init_x86_mem_from_file(SIGNED_CAPSULE_CPLD_FILE, cpld_recovery_capsule_ptr);

    // Load expected image for comparison purpose
    alt_u32* expected_cpld_recovery_capsule = new alt_u32[SIGNED_CAPSULE_CPLD_WITH_SVN64_FILE_SIZE/4];
    CPLD_UPDATE_PC* expected_cpld_recovery_capsule_pc = (CPLD_UPDATE_PC*) incr_alt_u32_ptr(expected_cpld_recovery_capsule, SIGNATURE_SIZE);
    alt_u32* expected_cpld_active_image = expected_cpld_recovery_capsule_pc->cpld_bitstream;
    SYSTEM_MOCK::get()->init_x86_mem_from_file(SIGNED_CAPSULE_CPLD_WITH_SVN64_FILE, expected_cpld_recovery_capsule);

    /***********************
     * Step 1: Send in CPLD recovery update to bump the SVN to 64
     ***********************/
    /*
     * Flow preparation
     */
    // Exit after 50 iterations in the T0 loop
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS_END_AFTER_50_ITERS);

    // Set asserts to throw as opposed to abort
    SYSTEM_MOCK::get()->set_assert_to_throw();
    // Throw after performing CFM switch
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::THROW_AFTER_CFM_SWITCH);

    // Load the update capsule
    SYSTEM_MOCK::get()->load_to_flash(
            SPI_FLASH_BMC,
            SIGNED_CAPSULE_CPLD_WITH_SVN64_FILE,
            SIGNED_CAPSULE_CPLD_WITH_SVN64_FILE_SIZE,
            get_ufm_pfr_data()->bmc_staging_region + BMC_STAGING_REGION_CPLD_UPDATE_CAPSULE_OFFSET);

    /*
     * Run Nios FW through PFR/Recovery Main.
     * Max10 is set up to boot with CFM0 (recovery_main). It will switch to CFM1 shortly after.
     * After receiving CPLD update intent, CPLD will switch from CFM1 to CFM0 to do update there.
     * Once update is complete in CFM0, CPLD will switch back to CFM1.
     */
    ut_run_main(CPLD_CFM0, true);

    // Send the recovery update intent
    ut_send_in_update_intent_once_upon_entry_to_t0(MB_BMC_UPDATE_INTENT, MB_UPDATE_INTENT_CPLD_RECOVERY_MASK);
    ut_run_main(CPLD_CFM1, true);

    ut_run_main(CPLD_CFM0, true);
    ut_run_main(CPLD_CFM1, false);

    /*
     * Verify results
     */
    // Expecting no error
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(0));

    // SVN policy for CPLD update should have been updated
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_CPLD), alt_u32(64));
    for (alt_u32 i = 0; i < 64; i++)
    {
        EXPECT_FALSE(is_svn_valid(UFM_SVN_POLICY_CPLD, i));
    }
    EXPECT_TRUE(is_svn_valid(UFM_SVN_POLICY_CPLD, 64));
    EXPECT_EQ(read_from_mailbox(MB_CPLD_SVN), alt_u32(64));

    // Active image should be updated
    for (alt_u32 i = 0; i < (UFM_CPLD_ACTIVE_IMAGE_LENGTH/4); i++)
    {
        ASSERT_EQ(cfm1_ufm_ptr[i], expected_cpld_active_image[i]);
    }

    // Recovery image should be updated (i.e. match the update capsule)
    for (int i = 1; i < (SIGNED_CAPSULE_CPLD_FILE_SIZE/4); i++)
    {
        ASSERT_EQ(cpld_recovery_capsule_ptr[i], expected_cpld_recovery_capsule[i]);
    }

    /***********************
     * Step 2: Send in CPLD active update with a banned SVN (1)
     ***********************/
    // Load the update capsule (which has SVN 0)
    SYSTEM_MOCK::get()->load_to_flash(
            SPI_FLASH_BMC,
            SIGNED_CAPSULE_CPLD_WITH_SVN1_FILE,
            SIGNED_CAPSULE_CPLD_WITH_SVN1_FILE_SIZE,
            get_ufm_pfr_data()->bmc_staging_region + BMC_STAGING_REGION_CPLD_UPDATE_CAPSULE_OFFSET);

    /*
     * Run Nios FW through PFR/Recovery Main.
     */
    SYSTEM_MOCK::get()->reset_ip_mocks();
    ut_run_main(CPLD_CFM0, true);

    // Send the active update intent
    ut_send_in_update_intent_once_upon_entry_to_t0(MB_BMC_UPDATE_INTENT, MB_UPDATE_INTENT_CPLD_ACTIVE_MASK);
    ut_run_main(CPLD_CFM1, false);

    /*
     * Verify results
     */
    // Expecting error about the banned SVN
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(MAJOR_ERROR_UPDATE_FROM_BMC_FAILED));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_INVALID_SVN));
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(0));

    // SVN policy for CPLD update should not be changed
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_CPLD), alt_u32(64));
    for (alt_u32 i = 0; i < 64; i++)
    {
        EXPECT_FALSE(is_svn_valid(UFM_SVN_POLICY_CPLD, i));
    }
    EXPECT_TRUE(is_svn_valid(UFM_SVN_POLICY_CPLD, 64));
    EXPECT_EQ(read_from_mailbox(MB_CPLD_SVN), alt_u32(64));

    // Active image should not be changed
    for (alt_u32 i = 0; i < (UFM_CPLD_ACTIVE_IMAGE_LENGTH/4); i++)
    {
        ASSERT_EQ(cfm1_ufm_ptr[i], expected_cpld_active_image[i]);
    }

    // Recovery image should not be changed
    for (int i = 1; i < (SIGNED_CAPSULE_CPLD_FILE_SIZE/4); i++)
    {
        ASSERT_EQ(cpld_recovery_capsule_ptr[i], expected_cpld_recovery_capsule[i]);
    }

    /***********************
     * Step 3: Send in CPLD active update with SVN 64 again, which is allowed.
     ***********************/
    // Load the update capsule (which has SVN 0)
    SYSTEM_MOCK::get()->load_to_flash(
            SPI_FLASH_BMC,
            SIGNED_CAPSULE_CPLD_WITH_SVN64_FILE,
            SIGNED_CAPSULE_CPLD_WITH_SVN64_FILE_SIZE,
            get_ufm_pfr_data()->bmc_staging_region + BMC_STAGING_REGION_CPLD_UPDATE_CAPSULE_OFFSET);

    /*
     * Run Nios FW through PFR/Recovery Main.
     * Max10 is set up to boot with CFM0 (recovery_main). It will switch to CFM1 shortly after.
     * After receiving CPLD update intent, CPLD will switch from CFM1 to CFM0 to do update there.
     * Once update is complete in CFM0, CPLD will switch back to CFM1.
     */
    SYSTEM_MOCK::get()->reset_ip_mocks();
    ut_run_main(CPLD_CFM0, true);

    // Send the active update intent
    ut_send_in_update_intent_once_upon_entry_to_t0(MB_BMC_UPDATE_INTENT, MB_UPDATE_INTENT_CPLD_ACTIVE_MASK);
    ut_run_main(CPLD_CFM1, true);

    ut_run_main(CPLD_CFM0, true);
    ut_run_main(CPLD_CFM1, false);

    /*
     * Verify results
     */
    // Expecting error about the banned SVN
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(0));

    // SVN policy for CPLD update should not be changed
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_CPLD), alt_u32(64));
    for (alt_u32 i = 0; i < 64; i++)
    {
        EXPECT_FALSE(is_svn_valid(UFM_SVN_POLICY_CPLD, i));
    }
    EXPECT_TRUE(is_svn_valid(UFM_SVN_POLICY_CPLD, 64));
    EXPECT_EQ(read_from_mailbox(MB_CPLD_SVN), alt_u32(64));

    // Active image should not be changed
    for (alt_u32 i = 0; i < (UFM_CPLD_ACTIVE_IMAGE_LENGTH/4); i++)
    {
        ASSERT_EQ(cfm1_ufm_ptr[i], expected_cpld_active_image[i]);
    }

    // Recovery image should not be changed
    for (int i = 1; i < (SIGNED_CAPSULE_CPLD_FILE_SIZE/4); i++)
    {
        ASSERT_EQ(cpld_recovery_capsule_ptr[i], expected_cpld_recovery_capsule[i]);
    }

    /*
     * Clean up
     */
    delete[] cfm0_image;
    delete[] cfm1_image;
    delete[] expected_cpld_recovery_capsule;
}

/**
 * @brief This test sends a CPLD active update with capsule that has an invalid SVN (65).
 * This update request should be rejected.
 */
TEST_F(PFRAntiRollbackWithSVNTest, test_active_cpld_update_with_invalid_svn)
{
    /*
     * Test Preparation
     */
    switch_spi_flash(SPI_FLASH_BMC);

    // Load the update capsule
    SYSTEM_MOCK::get()->load_to_flash(
            SPI_FLASH_BMC,
            SIGNED_CAPSULE_CPLD_WITH_SVN65_FILE,
            SIGNED_CAPSULE_CPLD_WITH_SVN65_FILE_SIZE,
            get_ufm_pfr_data()->bmc_staging_region + BMC_STAGING_REGION_CPLD_UPDATE_CAPSULE_OFFSET);

    // Load ROM image to CFM0
    alt_u32* cfm0_image = new alt_u32[CFM0_RECOVERY_IMAGE_FILE_SIZE/4];
    alt_u32* cfm0_ufm_ptr = get_ufm_ptr_with_offset(UFM_CPLD_ROM_IMAGE_OFFSET);

    SYSTEM_MOCK::get()->init_x86_mem_from_file(CFM0_RECOVERY_IMAGE_FILE, cfm0_image);
    SYSTEM_MOCK::get()->init_x86_mem_from_file(CFM0_RECOVERY_IMAGE_FILE, cfm0_ufm_ptr);

    // Load active image to CFM1
    alt_u32* cfm1_image = new alt_u32[CFM1_ACTIVE_IMAGE_FILE_SIZE/4];
    alt_u32* cfm1_ufm_ptr = get_ufm_ptr_with_offset(UFM_CPLD_ACTIVE_IMAGE_OFFSET);

    SYSTEM_MOCK::get()->init_x86_mem_from_file(CFM1_ACTIVE_IMAGE_FILE, cfm1_image);
    SYSTEM_MOCK::get()->init_x86_mem_from_file(CFM1_ACTIVE_IMAGE_FILE, cfm1_ufm_ptr);

    // Load active image to CPLD Recovery Region
    alt_u32* cpld_recovery_capsule_local = new alt_u32[SIGNED_CAPSULE_CPLD_FILE_SIZE/4];
    alt_u32* cpld_recovery_capsule_ptr = get_spi_flash_ptr_with_offset(BMC_CPLD_RECOVERY_IMAGE_OFFSET);

    SYSTEM_MOCK::get()->init_x86_mem_from_file(SIGNED_CAPSULE_CPLD_FILE, cpld_recovery_capsule_local);
    SYSTEM_MOCK::get()->init_x86_mem_from_file(SIGNED_CAPSULE_CPLD_FILE, cpld_recovery_capsule_ptr);

    /*
     * Flow preparation
     */
    // Exit after 50 iterations in the T0 loop
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS_END_AFTER_50_ITERS);

    // Set asserts to throw as opposed to abort
    SYSTEM_MOCK::get()->set_assert_to_throw();
    // Throw after performing CFM switch
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::THROW_AFTER_CFM_SWITCH);

    /*
     * Run Nios FW through PFR/Recovery Main.
     * Max10 is set up to boot with CFM0 (recovery_main). It will switch to CFM1 shortly after.
     */
    ut_run_main(CPLD_CFM0, true);

    // Send the recovery update intent
    ut_send_in_update_intent_once_upon_entry_to_t0(MB_BMC_UPDATE_INTENT, MB_UPDATE_INTENT_CPLD_ACTIVE_MASK);
    ut_run_main(CPLD_CFM1, false);

    /*
     * Verify results
     */
    // Expecting no error
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(MAJOR_ERROR_UPDATE_FROM_BMC_FAILED));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_INVALID_SVN));
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(0));


    // Active image should be untouched
    for (alt_u32 i = 0; i < (UFM_CPLD_ACTIVE_IMAGE_LENGTH/4); i++)
    {
        ASSERT_EQ(cfm1_ufm_ptr[i], cfm1_image[i]);
    }

    // CPLD ROM image should be untouched
    for (alt_u32 i = 0; i < (UFM_CPLD_ROM_IMAGE_LENGTH/4); i++)
    {
        ASSERT_EQ(cfm0_ufm_ptr[i], cfm0_image[i]);
    }

    // Recovery image should be untouched (i.e. match the previous active image)
    for (int i = 1; i < (SIGNED_CAPSULE_CPLD_FILE_SIZE/4); i++)
    {
        ASSERT_EQ(cpld_recovery_capsule_ptr[i], cpld_recovery_capsule_local[i]);
    }

    // SVN policy for CPLD update should remain unchanged
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_CPLD), alt_u32(0));
    EXPECT_TRUE(is_svn_valid(UFM_SVN_POLICY_CPLD, 0x0));
    EXPECT_EQ(read_from_mailbox(MB_CPLD_SVN), alt_u32(0));

    /*
     * Clean up
     */
    delete[] cfm0_image;
    delete[] cfm1_image;
    delete[] cpld_recovery_capsule_local;
}
