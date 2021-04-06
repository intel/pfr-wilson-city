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


class DecommissionFlowTest : public testing::Test
{
public:

    // CFM0/CFM1 binary content used in this test suite
    alt_u32* m_cfm0_image;
    alt_u32* m_cfm1_image;

    // Pointer to start of CFM0/CFM1
    alt_u32* m_cfm0_ufm_ptr;
    alt_u32* m_cfm1_ufm_ptr;

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

        // Initialize CFM0
        m_cfm0_image = new alt_u32[UFM_CPLD_ROM_IMAGE_LENGTH/4];
        SYSTEM_MOCK::get()->init_x86_mem_from_file(CFM0_RECOVERY_IMAGE_FILE, m_cfm0_image);
        m_cfm0_ufm_ptr = get_ufm_ptr_with_offset(UFM_CPLD_ROM_IMAGE_OFFSET);
        for (alt_u32 i = 0; i < (UFM_CPLD_ROM_IMAGE_LENGTH/4); i++)
        {
            m_cfm0_ufm_ptr[i] = m_cfm0_image[i];
        }

        // Initialize CFM1
        m_cfm1_image = new alt_u32[UFM_CPLD_ACTIVE_IMAGE_LENGTH/4];
        SYSTEM_MOCK::get()->init_x86_mem_from_file(CFM1_ACTIVE_IMAGE_FILE, m_cfm1_image);
        m_cfm1_ufm_ptr = get_ufm_ptr_with_offset(UFM_CPLD_ACTIVE_IMAGE_OFFSET);
        for (alt_u32 i = 0; i < (UFM_CPLD_ACTIVE_IMAGE_LENGTH/4); i++)
        {
            m_cfm1_ufm_ptr[i] = m_cfm1_image[i];
        }
    }

    virtual void TearDown()
    {
        delete[] m_cfm0_image;
        delete[] m_cfm1_image;
    }
};

/*
 * @brief Execute the decommission flow in the manner that user would normally do.
 */
TEST_F(DecommissionFlowTest, test_happy_path)
{
    /*
     * Test Preparation
     */
    // Load the decommission capsule to BMC SPI flash
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, SIGNED_DECOMMISSION_CAPSULE_FILE,
            SIGNED_DECOMMISSION_CAPSULE_FILE_SIZE, get_ufm_pfr_data()->bmc_staging_region + BMC_STAGING_REGION_CPLD_UPDATE_CAPSULE_OFFSET);

    /*
     * Flow preparation
     */
    // Exit after 10 iterations in the T0 loop
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS_END_AFTER_50_ITERS);

    // Set asserts to throw as opposed to abort
    SYSTEM_MOCK::get()->set_assert_to_throw();
    // Throw after performing CFM switch
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::THROW_AFTER_CFM_SWITCH);

    /*
     * Run Nios FW through PFR/Recovery Main
     */
    ut_run_main(CPLD_CFM0, true);

    // Send the active update intent
    ut_send_in_update_intent_once_upon_boot_complete(
            MB_BMC_UPDATE_INTENT, MB_UPDATE_INTENT_CPLD_ACTIVE_MASK);
    ut_run_main(CPLD_CFM1, true);

    // Decommission flow would trigger reconfig into Active Image (in un-provisioned mode)
    ut_run_main(CPLD_CFM1, false);

    /*
     * Verify
     */
    // Make sure platform has been unprovisioned
    alt_u32* ufm_pfr_data_ptr = (alt_u32*) get_ufm_pfr_data();
    for (int i = 0; i < UFM_FLASH_PAGE_SIZE / 4; i++)
    {
        EXPECT_EQ(ufm_pfr_data_ptr[i], alt_u32(0xffffffff));
    }

    EXPECT_FALSE(ut_check_ufm_prov_status(MB_UFM_PROV_UFM_LOCKED_MASK));
    EXPECT_FALSE(ut_check_ufm_prov_status(MB_UFM_PROV_UFM_PROVISIONED_MASK));
    EXPECT_FALSE(ut_check_ufm_prov_status(MB_UFM_PROV_UFM_PIT_L1_ENABLED_MASK));
    EXPECT_FALSE(ut_check_ufm_prov_status(MB_UFM_PROV_UFM_PIT_L2_PASSED_MASK));

    // CPLD ROM image should be untouched
    for (int i = 0; i < (UFM_CPLD_ROM_IMAGE_LENGTH/4); i++)
    {
        ASSERT_EQ(m_cfm0_ufm_ptr[i], m_cfm0_image[i]);
    }
    // CPLD Active image should be untouched
    for (alt_u32 i = 0; i < (UFM_CPLD_ACTIVE_IMAGE_LENGTH/4); i++)
    {
        ASSERT_EQ(m_cfm1_ufm_ptr[i], m_cfm1_image[i]);
    }

    // Expect no error
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(0));

    // No info on PCH FW version since the platform has been unprovisioned
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_ACTIVE_MAJOR_VER), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_ACTIVE_MINOR_VER), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_RECOVERY_MAJOR_VER), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_RECOVERY_MINOR_VER), alt_u32(0));

    // No info on BMC FW version since the platform has been unprovisioned
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_ACTIVE_MAJOR_VER), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_ACTIVE_MINOR_VER), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_RECOVERY_MAJOR_VER), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_RECOVERY_MINOR_VER), alt_u32(0));
}

/*
 * @brief Issue decommission request with a decommission capsule with invalid protected content.
 */
TEST_F(DecommissionFlowTest, test_against_decomm_capsule_with_bad_pc)
{
    /*
     * Test Preparation
     */
    // Load the decommission capsule to BMC SPI flash
    SYSTEM_MOCK::get()->load_to_flash(
            SPI_FLASH_BMC,
            SIGNED_DECOMMISSION_CAPSULE_WITH_INVALID_PC_FILE,
            SIGNED_DECOMMISSION_CAPSULE_WITH_INVALID_PC_FILE_SIZE,
            get_ufm_pfr_data()->bmc_staging_region + BMC_STAGING_REGION_CPLD_UPDATE_CAPSULE_OFFSET);

    /*
     * Flow preparation
     */
    // Exit after 10 iterations in the T0 loop
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS_END_AFTER_50_ITERS);

    // Set asserts to throw as opposed to abort
    SYSTEM_MOCK::get()->set_assert_to_throw();
    // Throw after performing CFM switch
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::THROW_AFTER_CFM_SWITCH);

    /*
     * Run Nios FW through PFR/Recovery Main
     */
    ut_run_main(CPLD_CFM0, true);

    // Send the active update intent
    ut_send_in_update_intent_once_upon_boot_complete(
            MB_BMC_UPDATE_INTENT, MB_UPDATE_INTENT_CPLD_ACTIVE_MASK);

    // This update should be rejected, so no reconfig is expected
    ut_run_main(CPLD_CFM1, false);

    /*
     * Verify
     */
    // Expect error about this update failure
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(MAJOR_ERROR_UPDATE_FROM_BMC_FAILED));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_AUTHENTICATION_FAILED));

    // Should see failed update attempts incremented
    EXPECT_EQ(num_failed_update_attempts_from_pch, alt_u32(0));
    EXPECT_EQ(num_failed_update_attempts_from_bmc, alt_u32(1));

    // Expect full platform T-1 entry
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_bmc_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_pch_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(2));

    // Check other status
    EXPECT_EQ(read_from_mailbox(MB_PLATFORM_STATE), alt_u32(PLATFORM_STATE_T0_BOOT_COMPLETE));
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(1));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), LAST_PANIC_BMC_UPDATE_INTENT);
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(0));

    // Make sure UFM has not been modified
    alt_u32* expected_ufm_data = new alt_u32[UFM_PFR_DATA_EXAMPLE_KEY_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(UFM_PFR_DATA_EXAMPLE_KEY_FILE, expected_ufm_data);

    alt_u32* ufm_pfr_data_ptr = (alt_u32*) get_ufm_pfr_data();
    for (int i = 0; i < UFM_PFR_DATA_EXAMPLE_KEY_FILE_SIZE / 4; i++)
    {
        EXPECT_EQ(ufm_pfr_data_ptr[i], expected_ufm_data[i]);
    }
    delete[] expected_ufm_data;

    EXPECT_FALSE(ut_check_ufm_prov_status(MB_UFM_PROV_UFM_LOCKED_MASK));
    EXPECT_TRUE(ut_check_ufm_prov_status(MB_UFM_PROV_UFM_PROVISIONED_MASK));
    EXPECT_FALSE(ut_check_ufm_prov_status(MB_UFM_PROV_UFM_PIT_L1_ENABLED_MASK));
    EXPECT_FALSE(ut_check_ufm_prov_status(MB_UFM_PROV_UFM_PIT_L2_PASSED_MASK));

    // CPLD ROM image should be untouched
    for (int i = 0; i < (UFM_CPLD_ROM_IMAGE_LENGTH/4); i++)
    {
        ASSERT_EQ(m_cfm0_ufm_ptr[i], m_cfm0_image[i]);
    }
    // CPLD Active image should be untouched
    for (alt_u32 i = 0; i < (UFM_CPLD_ACTIVE_IMAGE_LENGTH/4); i++)
    {
        ASSERT_EQ(m_cfm1_ufm_ptr[i], m_cfm1_image[i]);
    }
}

/**
 * @brief Send decommission request with a capsule signed with CSK #3.
 * This should be allowed since CSK ID 3 is not cancelled.
 */
TEST_F(DecommissionFlowTest, test_against_decomm_capsule_with_new_key)
{
    /*
     * Test Preparation
     */
    // Load the decommission capsule to BMC SPI flash
    SYSTEM_MOCK::get()->load_to_flash(
            SPI_FLASH_BMC,
            SIGNED_DECOMMISSION_CAPSULE_WITH_CSK_ID3_FILE,
            SIGNED_DECOMMISSION_CAPSULE_WITH_CSK_ID3_FILE_SIZE,
            get_ufm_pfr_data()->bmc_staging_region + BMC_STAGING_REGION_CPLD_UPDATE_CAPSULE_OFFSET);

    /*
     * Flow preparation
     */
    // Exit after 10 iterations in the T0 loop
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS_END_AFTER_50_ITERS);

    // Set asserts to throw as opposed to abort
    SYSTEM_MOCK::get()->set_assert_to_throw();
    // Throw after performing CFM switch
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::THROW_AFTER_CFM_SWITCH);

    // In addition, lock UFM to see if decommission request can still go through
    set_ufm_status(UFM_STATUS_LOCK_BIT_MASK);

    /*
     * Run Nios FW through PFR/Recovery Main
     */
    ut_run_main(CPLD_CFM0, true);

    // Send the active update intent
    ut_send_in_update_intent_once_upon_boot_complete(
            MB_BMC_UPDATE_INTENT, MB_UPDATE_INTENT_CPLD_ACTIVE_MASK);
    ut_run_main(CPLD_CFM1, true);

    // Decommission flow would trigger reconfig into Active Image (in un-provisioned mode)
    ut_run_main(CPLD_CFM1, false);

    /*
     * Verify
     */
    // Make sure platform has been unprovisioned
    alt_u32* ufm_pfr_data_ptr = (alt_u32*) get_ufm_pfr_data();
    for (int i = 0; i < UFM_FLASH_PAGE_SIZE / 4; i++)
    {
        EXPECT_EQ(ufm_pfr_data_ptr[i], alt_u32(0xffffffff));
    }

    EXPECT_FALSE(ut_check_ufm_prov_status(MB_UFM_PROV_UFM_LOCKED_MASK));
    EXPECT_FALSE(ut_check_ufm_prov_status(MB_UFM_PROV_UFM_PROVISIONED_MASK));
    EXPECT_FALSE(ut_check_ufm_prov_status(MB_UFM_PROV_UFM_PIT_L1_ENABLED_MASK));
    EXPECT_FALSE(ut_check_ufm_prov_status(MB_UFM_PROV_UFM_PIT_L2_PASSED_MASK));

    // CPLD ROM image should be untouched
    for (int i = 0; i < (UFM_CPLD_ROM_IMAGE_LENGTH/4); i++)
    {
        ASSERT_EQ(m_cfm0_ufm_ptr[i], m_cfm0_image[i]);
    }
    // CPLD Active image should be untouched
    for (alt_u32 i = 0; i < (UFM_CPLD_ACTIVE_IMAGE_LENGTH/4); i++)
    {
        ASSERT_EQ(m_cfm1_ufm_ptr[i], m_cfm1_image[i]);
    }

    // Expect no error
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(0));

    // No info on PCH FW version since the platform has been unprovisioned
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_ACTIVE_MAJOR_VER), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_ACTIVE_MINOR_VER), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_RECOVERY_MAJOR_VER), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_RECOVERY_MINOR_VER), alt_u32(0));

    // No info on BMC FW version since the platform has been unprovisioned
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_ACTIVE_MAJOR_VER), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_ACTIVE_MINOR_VER), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_RECOVERY_MAJOR_VER), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_RECOVERY_MINOR_VER), alt_u32(0));
}

/**
 * @brief Send decommission request with a capsule signed with a cancelled CS key.
 * This test cancels key #3 and then sends decommission capsule signed with key #3.
 */
TEST_F(DecommissionFlowTest, test_against_decomm_capsule_with_cancelled_key)
{
    /*
     * Test Preparation
     */
    // Load the decommission capsule to BMC SPI flash
    SYSTEM_MOCK::get()->load_to_flash(
            SPI_FLASH_BMC,
            SIGNED_DECOMMISSION_CAPSULE_WITH_CSK_ID3_FILE,
            SIGNED_DECOMMISSION_CAPSULE_WITH_CSK_ID3_FILE_SIZE,
            get_ufm_pfr_data()->bmc_staging_region + BMC_STAGING_REGION_CPLD_UPDATE_CAPSULE_OFFSET);

    /*
     * Flow preparation
     */
    // Exit after 10 iterations in the T0 loop
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS_END_AFTER_50_ITERS);

    // Set asserts to throw as opposed to abort
    SYSTEM_MOCK::get()->set_assert_to_throw();
    // Throw after performing CFM switch
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::THROW_AFTER_CFM_SWITCH);

    // Cancel CS key #3 for CPLD capsule
    cancel_key(KCH_PC_PFR_CPLD_UPDATE_CAPSULE, 3);

    /*
     * Run Nios FW through PFR/Recovery Main
     */
    ut_run_main(CPLD_CFM0, true);

    // Send the active update intent
    ut_send_in_update_intent_once_upon_boot_complete(
            MB_BMC_UPDATE_INTENT, MB_UPDATE_INTENT_CPLD_ACTIVE_MASK);

    // This update should be rejected, so no reconfig is expected
    ut_run_main(CPLD_CFM1, false);

    /*
     * Verify
     */
    // Expect error about this update failure
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(MAJOR_ERROR_UPDATE_FROM_BMC_FAILED));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_AUTHENTICATION_FAILED));

    // Should see failed update attempts incremented
    EXPECT_EQ(num_failed_update_attempts_from_pch, alt_u32(0));
    EXPECT_EQ(num_failed_update_attempts_from_bmc, alt_u32(1));

    // Expect full platform T-1 entry
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_bmc_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_pch_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(2));

    // Check other status
    EXPECT_EQ(read_from_mailbox(MB_PLATFORM_STATE), alt_u32(PLATFORM_STATE_T0_BOOT_COMPLETE));
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(1));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), LAST_PANIC_BMC_UPDATE_INTENT);
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(0));

    // Make sure UFM has not been modified
    alt_u32* expected_ufm_data_ptr = new alt_u32[UFM_PFR_DATA_EXAMPLE_KEY_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(UFM_PFR_DATA_EXAMPLE_KEY_FILE, expected_ufm_data_ptr);

    // Cancel Key #3 in the expected UFM data
    UFM_PFR_DATA* expected_ufm_data = (UFM_PFR_DATA*) expected_ufm_data_ptr;
    expected_ufm_data->csk_cancel_cpld_update_cap[0] = 0xEFFFFFFF;

    alt_u32* ufm_pfr_data_ptr = (alt_u32*) get_ufm_pfr_data();
    for (int i = 0; i < UFM_PFR_DATA_EXAMPLE_KEY_FILE_SIZE / 4; i++)
    {
        EXPECT_EQ(ufm_pfr_data_ptr[i], expected_ufm_data_ptr[i]);
    }
    delete[] expected_ufm_data_ptr;

    EXPECT_FALSE(ut_check_ufm_prov_status(MB_UFM_PROV_UFM_LOCKED_MASK));
    EXPECT_TRUE(ut_check_ufm_prov_status(MB_UFM_PROV_UFM_PROVISIONED_MASK));
    EXPECT_FALSE(ut_check_ufm_prov_status(MB_UFM_PROV_UFM_PIT_L1_ENABLED_MASK));
    EXPECT_FALSE(ut_check_ufm_prov_status(MB_UFM_PROV_UFM_PIT_L2_PASSED_MASK));

    // CPLD ROM image should be untouched
    for (int i = 0; i < (UFM_CPLD_ROM_IMAGE_LENGTH/4); i++)
    {
        ASSERT_EQ(m_cfm0_ufm_ptr[i], m_cfm0_image[i]);
    }
    // CPLD Active image should be untouched
    for (alt_u32 i = 0; i < (UFM_CPLD_ACTIVE_IMAGE_LENGTH/4); i++)
    {
        ASSERT_EQ(m_cfm1_ufm_ptr[i], m_cfm1_image[i]);
    }
}

/**
 * @brief Issue BMC update intent but has decommission capsule in the staging area.
 * CPLD is supposed to reject this request.
 */
TEST_F(DecommissionFlowTest, test_decommission_with_bmc_update_intent)
{
    /*
     * Test Preparation
     */
    // Load the decommission capsule to BMC SPI flash
    SYSTEM_MOCK::get()->load_to_flash(
            SPI_FLASH_BMC,
            SIGNED_DECOMMISSION_CAPSULE_FILE,
            SIGNED_DECOMMISSION_CAPSULE_FILE_SIZE,
            get_ufm_pfr_data()->bmc_staging_region + BMC_STAGING_REGION_CPLD_UPDATE_CAPSULE_OFFSET);

    // Get pointers to BMC/PCH flash
    alt_u32* bmc_flash_x86_ptr = SYSTEM_MOCK::get()->get_x86_ptr_to_spi_flash(SPI_FLASH_BMC);
    alt_u32* pch_flash_x86_ptr = SYSTEM_MOCK::get()->get_x86_ptr_to_spi_flash(SPI_FLASH_PCH);

    // Load original bmc image for comparison purpose
    alt_u32 *full_orig_bmc_image = new alt_u32[FULL_PFR_IMAGE_BMC_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(FULL_PFR_IMAGE_BMC_FILE, full_orig_bmc_image);

    alt_u32 *full_orig_pch_image = new alt_u32[FULL_PFR_IMAGE_PCH_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(FULL_PFR_IMAGE_PCH_FILE, full_orig_pch_image);

    /*
     * Flow preparation
     */
    // Exit after 10 iterations in the T0 loop
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS_END_AFTER_50_ITERS);

    // Set asserts to throw as opposed to abort
    SYSTEM_MOCK::get()->set_assert_to_throw();
    // Throw after performing CFM switch
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::THROW_AFTER_CFM_SWITCH);

    /*
     * Run Nios FW through PFR/Recovery Main
     */
    ut_run_main(CPLD_CFM0, true);

    // Send the active update intent
    ut_send_in_update_intent_once_upon_boot_complete(
            MB_BMC_UPDATE_INTENT, MB_UPDATE_INTENT_BMC_ACTIVE_MASK);

    // This update should be rejected, so no reconfig is expected
    ut_run_main(CPLD_CFM1, false);

    /*
     * Verify
     */
    // Expect error about this update failure
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(MAJOR_ERROR_UPDATE_FROM_BMC_FAILED));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_AUTHENTICATION_FAILED));

    // Should see failed update attempts incremented
    EXPECT_EQ(num_failed_update_attempts_from_pch, alt_u32(0));
    EXPECT_EQ(num_failed_update_attempts_from_bmc, alt_u32(1));

    // Expect full platform T-1 entry
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_bmc_only_counter(), alt_u32(1));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_pch_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(1));

    // Check other status
    EXPECT_EQ(read_from_mailbox(MB_PLATFORM_STATE), alt_u32(PLATFORM_STATE_T0_BOOT_COMPLETE));
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(1));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), LAST_PANIC_BMC_UPDATE_INTENT);
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(0));

    // Make sure UFM has not been modified
    alt_u32* expected_ufm_data = new alt_u32[UFM_PFR_DATA_EXAMPLE_KEY_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(UFM_PFR_DATA_EXAMPLE_KEY_FILE, expected_ufm_data);

    alt_u32* ufm_pfr_data_ptr = (alt_u32*) get_ufm_pfr_data();
    for (int i = 0; i < UFM_PFR_DATA_EXAMPLE_KEY_FILE_SIZE / 4; i++)
    {
        EXPECT_EQ(ufm_pfr_data_ptr[i], expected_ufm_data[i]);
    }
    delete[] expected_ufm_data;

    EXPECT_FALSE(ut_check_ufm_prov_status(MB_UFM_PROV_UFM_LOCKED_MASK));
    EXPECT_TRUE(ut_check_ufm_prov_status(MB_UFM_PROV_UFM_PROVISIONED_MASK));
    EXPECT_FALSE(ut_check_ufm_prov_status(MB_UFM_PROV_UFM_PIT_L1_ENABLED_MASK));
    EXPECT_FALSE(ut_check_ufm_prov_status(MB_UFM_PROV_UFM_PIT_L2_PASSED_MASK));

    // CPLD ROM image should be untouched
    for (int i = 0; i < (UFM_CPLD_ROM_IMAGE_LENGTH/4); i++)
    {
        ASSERT_EQ(m_cfm0_ufm_ptr[i], m_cfm0_image[i]);
    }
    // CPLD Active image should be untouched
    for (alt_u32 i = 0; i < (UFM_CPLD_ACTIVE_IMAGE_LENGTH/4); i++)
    {
        ASSERT_EQ(m_cfm1_ufm_ptr[i], m_cfm1_image[i]);
    }

    // BMC flash should be untouched (check only the static regions for simplicity)
    for (alt_u32 region_i = 0; region_i < BMC_NUM_STATIC_REGIONS; region_i++)
    {
        for (alt_u32 word_i = testdata_bmc_static_regions_start_addr[region_i] >> 2;
                word_i < testdata_bmc_static_regions_end_addr[region_i] >> 2; word_i++)
        {
            ASSERT_EQ(full_orig_bmc_image[word_i], bmc_flash_x86_ptr[word_i]);
        }
    }

    // PCH flash should be untouched (check only the static regions for simplicity)
    for (alt_u32 region_i = 0; region_i < PCH_NUM_STATIC_REGIONS; region_i++)
    {
        for (alt_u32 word_i = testdata_pch_static_regions_start_addr[region_i] >> 2;
                word_i < testdata_pch_static_regions_end_addr[region_i] >> 2; word_i++)
        {
            ASSERT_EQ(full_orig_pch_image[word_i], pch_flash_x86_ptr[word_i]);
        }
    }

    // Clean up
    delete[] full_orig_bmc_image;
    delete[] full_orig_pch_image;
}


/**
 * @brief Issue PCH update intent but has decommission capsule in the staging area.
 * CPLD is supposed to reject this request.
 */
TEST_F(DecommissionFlowTest, test_decommission_with_pch_update_intent)
{
    /*
     * Test Preparation
     */
    // Load the decommission capsule to BMC SPI flash
    SYSTEM_MOCK::get()->load_to_flash(
            SPI_FLASH_PCH,
            SIGNED_DECOMMISSION_CAPSULE_FILE,
            SIGNED_DECOMMISSION_CAPSULE_FILE_SIZE,
            get_ufm_pfr_data()->pch_staging_region);

    // Get pointers to BMC/PCH flash
    alt_u32* bmc_flash_x86_ptr = SYSTEM_MOCK::get()->get_x86_ptr_to_spi_flash(SPI_FLASH_BMC);
    alt_u32* pch_flash_x86_ptr = SYSTEM_MOCK::get()->get_x86_ptr_to_spi_flash(SPI_FLASH_PCH);

    // Load original bmc image for comparison purpose
    alt_u32 *full_orig_bmc_image = new alt_u32[FULL_PFR_IMAGE_BMC_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(FULL_PFR_IMAGE_BMC_FILE, full_orig_bmc_image);

    alt_u32 *full_orig_pch_image = new alt_u32[FULL_PFR_IMAGE_PCH_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(FULL_PFR_IMAGE_PCH_FILE, full_orig_pch_image);

    /*
     * Flow preparation
     */
    // Exit after 10 iterations in the T0 loop
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS_END_AFTER_50_ITERS);

    // Set asserts to throw as opposed to abort
    SYSTEM_MOCK::get()->set_assert_to_throw();
    // Throw after performing CFM switch
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::THROW_AFTER_CFM_SWITCH);

    /*
     * Run Nios FW through PFR/Recovery Main
     */
    ut_run_main(CPLD_CFM0, true);

    // Send the active update intent
    ut_send_in_update_intent_once_upon_boot_complete(
            MB_PCH_UPDATE_INTENT, MB_UPDATE_INTENT_PCH_ACTIVE_MASK);

    // This update should be rejected, so no reconfig is expected
    ut_run_main(CPLD_CFM1, false);

    /*
     * Verify
     */
    // Expect error about this update failure
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(MAJOR_ERROR_UPDATE_FROM_PCH_FAILED));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_AUTHENTICATION_FAILED));

    // Should see failed update attempts incremented
    EXPECT_EQ(num_failed_update_attempts_from_pch, alt_u32(1));
    EXPECT_EQ(num_failed_update_attempts_from_bmc, alt_u32(0));

    // Expect full platform T-1 entry
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_bmc_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_pch_only_counter(), alt_u32(1));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(1));

    // Check other status
    EXPECT_EQ(read_from_mailbox(MB_PLATFORM_STATE), alt_u32(PLATFORM_STATE_T0_BOOT_COMPLETE));
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(1));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), LAST_PANIC_PCH_UPDATE_INTENT);
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(0));

    // Make sure UFM has not been modified
    alt_u32* expected_ufm_data = new alt_u32[UFM_PFR_DATA_EXAMPLE_KEY_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(UFM_PFR_DATA_EXAMPLE_KEY_FILE, expected_ufm_data);

    alt_u32* ufm_pfr_data_ptr = (alt_u32*) get_ufm_pfr_data();
    for (int i = 0; i < UFM_PFR_DATA_EXAMPLE_KEY_FILE_SIZE / 4; i++)
    {
        EXPECT_EQ(ufm_pfr_data_ptr[i], expected_ufm_data[i]);
    }
    delete[] expected_ufm_data;

    EXPECT_FALSE(ut_check_ufm_prov_status(MB_UFM_PROV_UFM_LOCKED_MASK));
    EXPECT_TRUE(ut_check_ufm_prov_status(MB_UFM_PROV_UFM_PROVISIONED_MASK));
    EXPECT_FALSE(ut_check_ufm_prov_status(MB_UFM_PROV_UFM_PIT_L1_ENABLED_MASK));
    EXPECT_FALSE(ut_check_ufm_prov_status(MB_UFM_PROV_UFM_PIT_L2_PASSED_MASK));

    // CPLD ROM image should be untouched
    for (int i = 0; i < (UFM_CPLD_ROM_IMAGE_LENGTH/4); i++)
    {
        ASSERT_EQ(m_cfm0_ufm_ptr[i], m_cfm0_image[i]);
    }
    // CPLD Active image should be untouched
    for (alt_u32 i = 0; i < (UFM_CPLD_ACTIVE_IMAGE_LENGTH/4); i++)
    {
        ASSERT_EQ(m_cfm1_ufm_ptr[i], m_cfm1_image[i]);
    }

    // BMC flash should be untouched (check only the static regions for simplicity)
    for (alt_u32 region_i = 0; region_i < BMC_NUM_STATIC_REGIONS; region_i++)
    {
        for (alt_u32 word_i = testdata_bmc_static_regions_start_addr[region_i] >> 2;
                word_i < testdata_bmc_static_regions_end_addr[region_i] >> 2; word_i++)
        {
            ASSERT_EQ(full_orig_bmc_image[word_i], bmc_flash_x86_ptr[word_i]);
        }
    }

    // PCH flash should be untouched (check only the static regions for simplicity)
    for (alt_u32 region_i = 0; region_i < PCH_NUM_STATIC_REGIONS; region_i++)
    {
        for (alt_u32 word_i = testdata_pch_static_regions_start_addr[region_i] >> 2;
                word_i < testdata_pch_static_regions_end_addr[region_i] >> 2; word_i++)
        {
            ASSERT_EQ(full_orig_pch_image[word_i], pch_flash_x86_ptr[word_i]);
        }
    }

    // Clean up
    delete[] full_orig_bmc_image;
    delete[] full_orig_pch_image;
}
