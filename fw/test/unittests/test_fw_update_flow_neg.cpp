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


class FWUpdateFlowNegativeTest : public testing::Test
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
 * @brief This test sends in 5 update intents without having a valid capsule in the staging region.
 * This exceeds the maximum number of failed update attempt.
 */
TEST_F(FWUpdateFlowNegativeTest, test_exceed_max_failed_update_attempt_bmc)
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

    // This line checks for the number of times bmc only T-1 mode is entered
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_bmc_only_counter(), alt_u32(MAX_FAILED_UPDATE_ATTEMPTS_FROM_BMC));
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(MAX_FAILED_UPDATE_ATTEMPTS_FROM_BMC));
}

/**
 * @brief This test sends in 5 update intents without having a valid capsule in the staging region.
 * This exceeds the maximum number of failed update attempt.
 */
TEST_F(FWUpdateFlowNegativeTest, test_exceed_max_failed_update_attempt_pch)
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

    /*
     * Execute the flow. Active update flow will be triggered.
     * Since there's no update capsule in staging region, update would fail.
     */
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(1, pfr_main());

    /*
     * Check results.
     */
    EXPECT_TRUE(ut_is_blocking_update_from(SPI_FLASH_PCH));
    EXPECT_FALSE(ut_is_blocking_update_from(SPI_FLASH_BMC));

    // This line checks for the number of times T-1 mode is entered
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_pch_only_counter(), alt_u32(MAX_FAILED_UPDATE_ATTEMPTS_FROM_PCH));
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(MAX_FAILED_UPDATE_ATTEMPTS_FROM_PCH));
}

/**
 * @brief BMC active update with an update capsule that has a invalid SVN.
 */
TEST_F(FWUpdateFlowNegativeTest, test_bmc_active_update_with_invalid_svn)
{
    /*
     * Test Preparation
     */
    switch_spi_flash(SPI_FLASH_BMC);
    alt_u32* flash_x86_ptr = SYSTEM_MOCK::get()->get_x86_ptr_to_spi_flash(SPI_FLASH_BMC);

    // Load the fw update capsule to staging region in flash
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, SIGNED_CAPSULE_BMC_FILE,
            SIGNED_CAPSULE_BMC_FILE_SIZE, get_ufm_pfr_data()->bmc_staging_region);

    /*
     * Erase SPI regions and verify the erase
     */
    for (alt_u32 region_i = 0; region_i < BMC_NUM_STATIC_REGIONS; region_i++)
    {
        alt_u32 start_addr = testdata_bmc_static_regions_start_addr[region_i];
        alt_u32 end_addr = testdata_bmc_static_regions_end_addr[region_i] - start_addr;
        erase_spi_region(start_addr, end_addr);
    }

    // For sanity purpose, verify the erase on some of SPI regions
    for (alt_u32 region_i = 0; region_i < BMC_NUM_STATIC_REGIONS; region_i++)
    {
        for (alt_u32 word_i = testdata_bmc_static_regions_start_addr[region_i] >> 2;
                word_i < testdata_bmc_static_regions_end_addr[region_i] >> 2; word_i++)
        {
            ASSERT_EQ(alt_u32(0xffffffff), flash_x86_ptr[word_i]);
        }
    }

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

    // Set minimum SVN to higher than the SVN in the update capsule
    write_ufm_svn(BMC_UPDATE_CAPSULE_PFM_SVN + 1, UFM_SVN_POLICY_BMC);

    /*
     * Execute the flow. Active update flow will be triggered.
     */
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(40, pfr_main());

    /*
     * Expect to see no data in static regions, since update didn't take place
     */
    for (alt_u32 region_i = 0; region_i < BMC_NUM_STATIC_REGIONS; region_i++)
    {
        for (alt_u32 word_i = testdata_bmc_static_regions_start_addr[region_i] >> 2;
                word_i < testdata_bmc_static_regions_end_addr[region_i] >> 2; word_i++)
        {
            ASSERT_EQ(alt_u32(0xffffffff), flash_x86_ptr[word_i]);
        }
    }

    // Nios should transition from T0 to T-1 exactly once for authenticating this capsule
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(1));

    // Expect an error
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(MAJOR_ERROR_UPDATE_FROM_BMC_FAILED));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_INVALID_SVN));
}

/**
 * @brief PCH recovery update with an update capsule which is signed with a cancelled key
 */
TEST_F(FWUpdateFlowNegativeTest, test_pch_recovery_update_with_cancelled_key_in_capsule)
{
    /*
     * Test Preparation
     */
    switch_spi_flash(SPI_FLASH_PCH);
    alt_u32* flash_x86_ptr = SYSTEM_MOCK::get()->get_x86_ptr_to_spi_flash(SPI_FLASH_PCH);

    // Load the fw update capsule to staging region in flash
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_PCH, SIGNED_CAPSULE_PCH_FILE,
            SIGNED_CAPSULE_PCH_FILE_SIZE, get_ufm_pfr_data()->pch_staging_region);

    /*
     * Erase SPI regions and verify the erase
     */
    for (alt_u32 region_i = 0; region_i < PCH_NUM_STATIC_REGIONS; region_i++)
    {
        alt_u32 start_addr = testdata_pch_static_regions_start_addr[region_i];
        alt_u32 end_addr = testdata_pch_static_regions_end_addr[region_i] - start_addr;
        erase_spi_region(start_addr, end_addr);
    }
    for (alt_u32 region_i = 0; region_i < PCH_NUM_DYNAMIC_REGIONS; region_i++)
    {
        alt_u32 start_addr = testdata_pch_dynamic_regions_start_addr[region_i];
        alt_u32 end_addr = testdata_pch_dynamic_regions_end_addr[region_i] - start_addr;
        erase_spi_region(start_addr, end_addr);
    }

    // For sanity purpose, verify the erase on some of SPI regions
    for (alt_u32 region_i = 0; region_i < PCH_NUM_STATIC_REGIONS; region_i++)
    {
        for (alt_u32 word_i = testdata_pch_static_regions_start_addr[region_i] >> 2;
                word_i < testdata_pch_static_regions_end_addr[region_i] >> 2; word_i++)
        {
            ASSERT_EQ(alt_u32(0xffffffff), flash_x86_ptr[word_i]);
        }
    }
    for (alt_u32 region_i = 0; region_i < PCH_NUM_DYNAMIC_REGIONS; region_i++)
    {
        for (alt_u32 word_i = testdata_pch_dynamic_regions_start_addr[region_i] >> 2;
                word_i < testdata_pch_dynamic_regions_end_addr[region_i] >> 2; word_i++)
        {
            ASSERT_EQ(alt_u32(0xffffffff), flash_x86_ptr[word_i]);
        }
    }

    /*
     * Flow preparation
     */
    ut_prep_nios_gpi_signals();
    ut_send_block_complete_chkpt_msg();

    // Exit after 10 iterations in the T0 loop
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS_END_AFTER_50_ITERS);
    // Skip authentication in T-1
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::SKIP_FLASH_AUTHENTICATION);

    // Send the recovery update intent
    ut_send_in_update_intent_once_upon_boot_complete(MB_PCH_UPDATE_INTENT, MB_UPDATE_INTENT_PCH_RECOVERY_MASK);

    // Cancel the key that was used to sign this update capsule.
    KCH_SIGNATURE* capsule_signature = (KCH_SIGNATURE*) get_spi_flash_ptr_with_offset(get_ufm_pfr_data()->pch_staging_region);
    cancel_key(KCH_PC_PFR_PCH_UPDATE_CAPSULE, capsule_signature->b1.csk_entry.key_id);

    /*
     * Execute the flow. Active update flow will be triggered.
     */
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(40, pfr_main());

    /*
     * Expect to see no data in static and dynamic regions, since update didn't take place
     */
    for (alt_u32 region_i = 0; region_i < PCH_NUM_STATIC_REGIONS; region_i++)
    {
        for (alt_u32 word_i = testdata_pch_static_regions_start_addr[region_i] >> 2;
                word_i < testdata_pch_static_regions_end_addr[region_i] >> 2; word_i++)
        {
            ASSERT_EQ(alt_u32(0xffffffff), flash_x86_ptr[word_i]);
        }
    }
    for (alt_u32 region_i = 0; region_i < PCH_NUM_DYNAMIC_REGIONS; region_i++)
    {
        for (alt_u32 word_i = testdata_pch_dynamic_regions_start_addr[region_i] >> 2;
                word_i < testdata_pch_dynamic_regions_end_addr[region_i] >> 2; word_i++)
        {
            ASSERT_EQ(alt_u32(0xffffffff), flash_x86_ptr[word_i]);
        }
    }

    // Nios should transition from T0 to T-1 exactly once for authenticating this capsule
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(1));

    // Expect an error
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(MAJOR_ERROR_UPDATE_FROM_PCH_FAILED));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_AUTHENTICATION_FAILED));
}

/**
 * @brief Out-of-band PCH active update with an update capsule. The PFM has been signed with a cancelled key.
 */
TEST_F(FWUpdateFlowNegativeTest, test_oob_pch_active_update_with_cancelled_key_in_capsule_pfm)
{
    /*
     * Test Preparation
     */
    switch_spi_flash(SPI_FLASH_BMC);
    alt_u32* flash_x86_ptr = SYSTEM_MOCK::get()->get_x86_ptr_to_spi_flash(SPI_FLASH_BMC);

    // Load the fw update capsule to staging region in flash
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, SIGNED_CAPSULE_PCH_FILE,
            SIGNED_CAPSULE_PCH_FILE_SIZE, get_ufm_pfr_data()->bmc_staging_region + BMC_STAGING_REGION_PCH_UPDATE_CAPSULE_OFFSET);

    /*
     * Erase SPI regions and verify the erase
     */
    for (alt_u32 region_i = 0; region_i < PCH_NUM_STATIC_REGIONS; region_i++)
    {
        alt_u32 start_addr = testdata_pch_static_regions_start_addr[region_i];
        alt_u32 end_addr = testdata_pch_static_regions_end_addr[region_i] - start_addr;
        erase_spi_region(start_addr, end_addr);
    }
    for (alt_u32 region_i = 0; region_i < PCH_NUM_DYNAMIC_REGIONS; region_i++)
    {
        alt_u32 start_addr = testdata_pch_dynamic_regions_start_addr[region_i];
        alt_u32 end_addr = testdata_pch_dynamic_regions_end_addr[region_i] - start_addr;
        erase_spi_region(start_addr, end_addr);
    }

    // For sanity purpose, verify the erase on some of SPI regions
    for (alt_u32 region_i = 0; region_i < PCH_NUM_STATIC_REGIONS; region_i++)
    {
        for (alt_u32 word_i = testdata_pch_static_regions_start_addr[region_i] >> 2;
                word_i < testdata_pch_static_regions_end_addr[region_i] >> 2; word_i++)
        {
            ASSERT_EQ(alt_u32(0xffffffff), flash_x86_ptr[word_i]);
        }
    }

    /*
     * Flow preparation
     */
    ut_prep_nios_gpi_signals();
    ut_send_block_complete_chkpt_msg();

    // Exit after 10 iterations in the T0 loop
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS_END_AFTER_50_ITERS);
    // Skip authentication in T-1
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::SKIP_FLASH_AUTHENTICATION);

    // Send the recovery update intent
    ut_send_in_update_intent_once_upon_boot_complete(MB_BMC_UPDATE_INTENT, MB_UPDATE_INTENT_PCH_ACTIVE_MASK);

    // Cancel the key that was used to sign this update capsule.
    alt_u32* signed_capsule = get_spi_flash_ptr_with_offset(
            get_ufm_pfr_data()->bmc_staging_region + BMC_STAGING_REGION_PCH_UPDATE_CAPSULE_OFFSET);
    KCH_SIGNATURE* pfm_signature = (KCH_SIGNATURE*) incr_alt_u32_ptr(signed_capsule, SIGNATURE_SIZE);
    cancel_key(KCH_PC_PFR_PCH_PFM, pfm_signature->b1.csk_entry.key_id);

    /*
     * Execute the flow. Active update flow will be triggered.
     */
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(40, pfr_main());

    /*
     * Expect to see no data in static regions, since update didn't take place
     */
    for (alt_u32 region_i = 0; region_i < PCH_NUM_STATIC_REGIONS; region_i++)
    {
        for (alt_u32 word_i = testdata_pch_static_regions_start_addr[region_i] >> 2;
                word_i < testdata_pch_static_regions_end_addr[region_i] >> 2; word_i++)
        {
            ASSERT_EQ(alt_u32(0xffffffff), flash_x86_ptr[word_i]);
        }
    }
    // Nios should transition from T0 to T-1 exactly once for authenticating this capsule
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(1));

    // Expect an error
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(MAJOR_ERROR_UPDATE_FROM_BMC_FAILED));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_AUTHENTICATION_FAILED));
}


/**
 * @brief This test sends some invalid update intent to PCH update intent register.
 * CPLD should ignores it.
 */
TEST_F(FWUpdateFlowNegativeTest, test_invalid_update_intent_update_for_pch)
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

    // Send an invalid update intent
    ut_send_in_update_intent(MB_PCH_UPDATE_INTENT, MB_UPDATE_INTENT_BMC_ACTIVE_MASK);

    /*
     * Execute the flow. Active update flow will be triggered.
     * Since there's no update capsule in staging region, update would fail.
     */
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(1, pfr_main());

    /*
     * Check results.
     */
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(MAJOR_ERROR_UPDATE_FROM_PCH_FAILED));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_INVALID_UPDATE_INTENT));
}

/**
 * @brief This test sends some invalid update intent to BMC update intent register.
 * CPLD should ignores it.
 */
TEST_F(FWUpdateFlowNegativeTest, test_invalid_update_intent_update_for_bmc)
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

    // Send an invalid update intent
    ut_send_in_update_intent(MB_BMC_UPDATE_INTENT, MB_UPDATE_INTENT_UPDATE_AT_RESET_MASK << 1);

    /*
     * Execute the flow. Active update flow will be triggered.
     * Since there's no update capsule in staging region, update would fail.
     */
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(1, pfr_main());

    /*
     * Check results.
     */
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(MAJOR_ERROR_UPDATE_FROM_BMC_FAILED));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_INVALID_UPDATE_INTENT));
}

/**
 * @brief This test sends some invalid update intent to PCH update intent register.
 * The update intent only has Update Dynamic bit set. That bit is meaningless without active firmware update bit set.
 * CPLD should ignores it.
 */
TEST_F(FWUpdateFlowNegativeTest, test_invalid_update_intent_update_dynamic_only_for_pch)
{
    /*
     * Flow preparation
     */
    ut_prep_nios_gpi_signals();
    ut_send_block_complete_chkpt_msg();

    // Exit after 50 iterations in the T0 loop
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS_END_AFTER_50_ITERS);
    // Skip authentication in T-1
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::SKIP_FLASH_AUTHENTICATION);

    // Send the active update intent
    ut_send_in_update_intent(MB_PCH_UPDATE_INTENT, MB_UPDATE_INTENT_UPDATE_DYNAMIC_MASK);

    /*
     * Execute the flow. Active update flow will be triggered.
     * Since there's no update capsule in staging region, update would fail.
     */
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(1, pfr_main());

    /*
     * Check results.
     */
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(MAJOR_ERROR_UPDATE_FROM_PCH_FAILED));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_INVALID_UPDATE_INTENT));
}

/**
 * @brief This test sends some invalid update intent to BMC update intent register.
 * The update intent only has Update Dynamic bit set. That bit is meaningless without active firmware update bit set.
 * CPLD should ignores it.
 */
TEST_F(FWUpdateFlowNegativeTest, test_invalid_update_intent_update_dynamic_only_for_bmc)
{
    /*
     * Flow preparation
     */
    ut_prep_nios_gpi_signals();
    ut_send_block_complete_chkpt_msg();

    // Exit after 50 iterations in the T0 loop
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS_END_AFTER_50_ITERS);
    // Skip authentication in T-1
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::SKIP_FLASH_AUTHENTICATION);

    // Send the active update intent
    ut_send_in_update_intent(MB_BMC_UPDATE_INTENT, MB_UPDATE_INTENT_UPDATE_DYNAMIC_MASK);

    /*
     * Execute the flow. Active update flow will be triggered.
     * Since there's no update capsule in staging region, update would fail.
     */
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(1, pfr_main());

    /*
     * Check results.
     */
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(MAJOR_ERROR_UPDATE_FROM_BMC_FAILED));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_INVALID_UPDATE_INTENT));
}

/**
 * @brief PCH active update with an update capsule which has bad page size (16KB instead of 4KB) in
 * compression structure header.
 */
TEST_F(FWUpdateFlowNegativeTest, test_pch_active_update_with_capsule_that_has_bad_page_size_in_pbc_header)
{
    /*
     * Test Preparation
     */
    switch_spi_flash(SPI_FLASH_PCH);
    alt_u32* flash_x86_ptr = SYSTEM_MOCK::get()->get_x86_ptr_to_spi_flash(SPI_FLASH_PCH);

    // Load the fw update capsule to staging region in flash
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_PCH, SIGNED_CAPSULE_PCH_WITH_BAD_PAGE_SIZE_IN_PBC_HEADER_FILE,
            SIGNED_CAPSULE_PCH_WITH_BAD_PAGE_SIZE_IN_PBC_HEADER_FILE_SIZE, get_ufm_pfr_data()->pch_staging_region);

    /*
     * Erase SPI regions and verify the erase
     */
    for (alt_u32 region_i = 0; region_i < PCH_NUM_STATIC_REGIONS; region_i++)
    {
        alt_u32 start_addr = testdata_pch_static_regions_start_addr[region_i];
        alt_u32 end_addr = testdata_pch_static_regions_end_addr[region_i] - start_addr;
        erase_spi_region(start_addr, end_addr);
    }
    for (alt_u32 region_i = 0; region_i < PCH_NUM_DYNAMIC_REGIONS; region_i++)
    {
        alt_u32 start_addr = testdata_pch_dynamic_regions_start_addr[region_i];
        alt_u32 end_addr = testdata_pch_dynamic_regions_end_addr[region_i] - start_addr;
        erase_spi_region(start_addr, end_addr);
    }

    // For sanity purpose, verify the erase on some of SPI regions
    for (alt_u32 region_i = 0; region_i < PCH_NUM_STATIC_REGIONS; region_i++)
    {
        for (alt_u32 word_i = testdata_pch_static_regions_start_addr[region_i] >> 2;
                word_i < testdata_pch_static_regions_end_addr[region_i] >> 2; word_i++)
        {
            ASSERT_EQ(alt_u32(0xffffffff), flash_x86_ptr[word_i]);
        }
    }
    for (alt_u32 region_i = 0; region_i < PCH_NUM_DYNAMIC_REGIONS; region_i++)
    {
        for (alt_u32 word_i = testdata_pch_dynamic_regions_start_addr[region_i] >> 2;
                word_i < testdata_pch_dynamic_regions_end_addr[region_i] >> 2; word_i++)
        {
            ASSERT_EQ(alt_u32(0xffffffff), flash_x86_ptr[word_i]);
        }
    }

    /*
     * Flow preparation
     */
    ut_prep_nios_gpi_signals();
    ut_send_block_complete_chkpt_msg();

    // Exit after 50 iterations in the T0 loop
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS_END_AFTER_50_ITERS);
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::SKIP_FLASH_AUTHENTICATION);

    // Send the active update intent
    ut_send_in_update_intent_once_upon_boot_complete(MB_PCH_UPDATE_INTENT, MB_UPDATE_INTENT_PCH_ACTIVE_MASK);

    /*
     * Execute the flow. Active update flow will be triggered.
     */
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(60, pfr_main());

    // Nios should transition from T0 to T-1 exactly once for authenticating this capsule
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(1));

    // Expect an error
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(MAJOR_ERROR_UPDATE_FROM_PCH_FAILED));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_AUTHENTICATION_FAILED));

    /*
     * Expect to see no data in static and dynamic regions, since update didn't take place
     */
    for (alt_u32 region_i = 0; region_i < PCH_NUM_STATIC_REGIONS; region_i++)
    {
        for (alt_u32 word_i = testdata_pch_static_regions_start_addr[region_i] >> 2;
                word_i < testdata_pch_static_regions_end_addr[region_i] >> 2; word_i++)
        {
            ASSERT_EQ(alt_u32(0xffffffff), flash_x86_ptr[word_i]);
        }
    }
    for (alt_u32 region_i = 0; region_i < PCH_NUM_DYNAMIC_REGIONS; region_i++)
    {
        for (alt_u32 word_i = testdata_pch_dynamic_regions_start_addr[region_i] >> 2;
                word_i < testdata_pch_dynamic_regions_end_addr[region_i] >> 2; word_i++)
        {
            ASSERT_EQ(alt_u32(0xffffffff), flash_x86_ptr[word_i]);
        }
    }
}

/**
 * @brief When PCH recovery image is corrupted, in-band PCH active firmware update should be banned.
 */
TEST_F(FWUpdateFlowNegativeTest, test_banned_inband_pch_active_update_when_recovery_image_is_corrupted)
{
    /*
     * Flow preparation
     */
    ut_prep_nios_gpi_signals();

    // Insert the T0_OPERATIONS code block (break out of T0 loop upon entry)
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS_END_AFTER_50_ITERS);

    // Corrupt PCH flash
    switch_spi_flash(SPI_FLASH_PCH);
    alt_u32* signed_recovery_capsule = get_spi_recovery_region_ptr(SPI_FLASH_PCH);
    *signed_recovery_capsule = 0;
    EXPECT_FALSE(is_signature_valid((KCH_SIGNATURE*) signed_recovery_capsule));

    // Send active firwmare update
    ut_send_in_update_intent_once_upon_entry_to_t0(MB_PCH_UPDATE_INTENT, MB_UPDATE_INTENT_PCH_ACTIVE_MASK);

    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(60, pfr_main());

    /*
     * Check result
     */
    // Expect no recovery because staging region is empty
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(0));

    // Expect no panic event because active firmware update should be banned.
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(0));

    // Expect an error message about the banned active update
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(MAJOR_ERROR_UPDATE_FROM_PCH_FAILED));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_ACTIVE_FW_UPDATE_NOT_ALLOWED));
}

/**
 * @brief When BMC recovery image is corrupted, out-of-band BMC active firmware update should be banned.
 */
TEST_F(FWUpdateFlowNegativeTest, test_banned_oob_bmc_active_update_when_recovery_image_is_corrupted)
{
    /*
     * Flow preparation
     */
    ut_prep_nios_gpi_signals();

    // Insert the T0_OPERATIONS code block (break out of T0 loop upon entry)
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS_END_AFTER_50_ITERS);

    // Corrupt BMC flash
    switch_spi_flash(SPI_FLASH_BMC);
    alt_u32* signed_recovery_capsule = get_spi_recovery_region_ptr(SPI_FLASH_BMC);
    *signed_recovery_capsule = 0;
    EXPECT_FALSE(is_signature_valid((KCH_SIGNATURE*) signed_recovery_capsule));

    // Send active firwmare update
    ut_send_in_update_intent_once_upon_entry_to_t0(MB_BMC_UPDATE_INTENT, MB_UPDATE_INTENT_BMC_ACTIVE_MASK);

    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(60, pfr_main());

    /*
     * Check result
     */
    // Expect no recovery because staging region is empty
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(0));

    // Expect no panic event because active firmware update should be banned.
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(0));

    // Expect an error message about the banned active update
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(MAJOR_ERROR_UPDATE_FROM_BMC_FAILED));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_ACTIVE_FW_UPDATE_NOT_ALLOWED));
}

/**
 * @brief When PCH recovery image is corrupted, out-of-band PCH active firmware update should be banned.
 */
TEST_F(FWUpdateFlowNegativeTest, test_banned_oob_pch_active_update_when_recovery_image_is_corrupted)
{
    /*
     * Flow preparation
     */
    ut_prep_nios_gpi_signals();

    // Insert the T0_OPERATIONS code block (break out of T0 loop upon entry)
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS_END_AFTER_50_ITERS);

    // Corrupt PCH flash
    switch_spi_flash(SPI_FLASH_PCH);
    alt_u32* signed_recovery_capsule = get_spi_recovery_region_ptr(SPI_FLASH_PCH);
    *signed_recovery_capsule = 0;
    EXPECT_FALSE(is_signature_valid((KCH_SIGNATURE*) signed_recovery_capsule));

    // Send active firwmare update
    ut_send_in_update_intent_once_upon_entry_to_t0(MB_BMC_UPDATE_INTENT, MB_UPDATE_INTENT_PCH_ACTIVE_MASK);

    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(60, pfr_main());

    /*
     * Check result
     */
    // Expect no recovery because staging region is empty
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(0));

    // Expect no panic event because active firmware update should be banned.
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(0));

    // Expect an error message about the banned active update
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(MAJOR_ERROR_UPDATE_FROM_BMC_FAILED));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_ACTIVE_FW_UPDATE_NOT_ALLOWED));
}

/**
 * @brief During the recovery update, ACM/ME/BIOS may not be able to boot after applying update on
 * PCH active firmware. It's expected that the recovery update should be abandoned and a watchdog recovery
 * should be applied on PCH active firmware. This case performs the recovery update through in-band path.
 */
TEST_F(FWUpdateFlowNegativeTest, test_boot_failure_during_ib_pch_recovery_update)
{
    /*
     * Test Preparation
     */
    switch_spi_flash(SPI_FLASH_PCH);
    alt_u32* pch_flash_x86_ptr = SYSTEM_MOCK::get()->get_x86_ptr_to_spi_flash(SPI_FLASH_PCH);

    // Load the PCH fw update capsule to BMC staging region in flash
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_PCH, SIGNED_CAPSULE_PCH_V03P12_FILE,
            SIGNED_CAPSULE_PCH_V03P12_FILE_SIZE, get_ufm_pfr_data()->pch_staging_region);

    // Load the entire image locally for comparison purpose
    alt_u32 *full_orig_pch_image = new alt_u32[FULL_PFR_IMAGE_PCH_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(FULL_PFR_IMAGE_PCH_FILE, full_orig_pch_image);
    alt_u32 *full_new_pch_image = new alt_u32[FULL_PFR_IMAGE_PCH_V03P12_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(FULL_PFR_IMAGE_PCH_V03P12_FILE, full_new_pch_image);

    /*
     * Flow preparation
     */
    ut_prep_nios_gpi_signals();

    // Exit after 1 iteration of the T0 loop
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS);
    // Send the recovery update intent
    ut_send_in_update_intent_once_upon_entry_to_t0(MB_PCH_UPDATE_INTENT, MB_UPDATE_INTENT_PCH_RECOVERY_MASK);

    /*
     * Run flow.
     * Active update flow will be triggered.
     */
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(80, pfr_main());

    /*
     * Verify the active update
     */
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(1));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), LAST_PANIC_PCH_UPDATE_INTENT);

    // Both static and dynamic regions are updated
    for (alt_u32 region_i = 0; region_i < PCH_NUM_STATIC_REGIONS; region_i++)
    {
        for (alt_u32 word_i = testdata_pch_static_regions_start_addr[region_i] >> 2;
                word_i < testdata_pch_static_regions_end_addr[region_i] >> 2; word_i++)
        {
            ASSERT_EQ(full_new_pch_image[word_i], pch_flash_x86_ptr[word_i]);
        }
    }
    for (alt_u32 region_i = 0; region_i < PCH_NUM_DYNAMIC_REGIONS; region_i++)
    {
        for (alt_u32 word_i = testdata_pch_dynamic_regions_start_addr[region_i] >> 2;
                word_i < testdata_pch_dynamic_regions_end_addr[region_i] >> 2; word_i++)
        {
            ASSERT_EQ(full_new_pch_image[word_i], pch_flash_x86_ptr[word_i]);
        }
    }

    /*
     * Run flow.
     * After entering T0, Nios was halted. Clear the ACM/BIOS timer count down value to trigger a watchdog recovery.
     */
    // Clear the count-down value and keep the HW tiemr in active mode.
    IOWR(WDT_ACM_BIOS_TIMER_ADDR, 0, U_TIMER_BANK_TIMER_ACTIVE_MASK);

    // Run the T0 loop again
    perform_t0_operations();

    /*
     * Check platform states and PCH/BMC flash content
     */
    // There should have been a watchdog recovery
    alt_u32 mb_platform_status = read_from_mailbox(MB_PLATFORM_STATE);
    EXPECT_TRUE((mb_platform_status == PLATFORM_STATE_ENTER_T0) || (mb_platform_status == PLATFORM_STATE_T0_ME_BOOTED));
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(2));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), LAST_PANIC_ACM_BIOS_WDT_EXPIRED);
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(1));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), LAST_RECOVERY_ACM_LAUNCH_FAIL);
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(0));

    // Check number of T-1 transitions (recovery update requires platform reset)
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_bmc_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_pch_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(3));

    // Both PCH and BMC should be out-of-reset
    EXPECT_TRUE(ut_is_bmc_out_of_reset());
    EXPECT_TRUE(ut_is_pch_out_of_reset());

    // Get the staging region offset
    alt_u32 pch_staging_start = get_staging_region_offset(SPI_FLASH_PCH);
    alt_u32 pch_staging_end = pch_staging_start + SPI_FLASH_PCH_STAGING_SIZE;
    alt_u32 pch_staging_start_word_i = pch_staging_start >> 2;
    alt_u32 pch_staging_end_word_i = pch_staging_end >> 2;

    // PCH flash should contain the original image
    for (alt_u32 word_i = 0; word_i < (FULL_PFR_IMAGE_PCH_FILE_SIZE >> 2); word_i++)
    {
        if ((pch_staging_start_word_i <= word_i) && (word_i < pch_staging_end_word_i))
        {
            // Skip the staging region
            continue;
        }
        ASSERT_EQ(full_orig_pch_image[word_i], pch_flash_x86_ptr[word_i]);
    }

    /*
     * Clean ups
     */
    delete[] full_orig_pch_image;
    delete[] full_new_pch_image;
}

/**
 * @brief During the recovery update, ACM/ME/BIOS may not be able to boot after applying update on
 * PCH active firmware. It's expected that the recovery update should be abandoned and a watchdog recovery
 * should be applied on PCH active firmware. This case performs the recovery update through out-of-band path.
 */
TEST_F(FWUpdateFlowNegativeTest, test_boot_failure_during_oob_pch_recovery_update)
{
    /*
     * Test Preparation
     */
    switch_spi_flash(SPI_FLASH_PCH);
    alt_u32* pch_flash_x86_ptr = SYSTEM_MOCK::get()->get_x86_ptr_to_spi_flash(SPI_FLASH_PCH);

    // Load the PCH fw update capsule to BMC staging region in flash
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, SIGNED_CAPSULE_PCH_V03P12_FILE,
            SIGNED_CAPSULE_PCH_V03P12_FILE_SIZE, get_ufm_pfr_data()->bmc_staging_region + BMC_STAGING_REGION_PCH_UPDATE_CAPSULE_OFFSET);

    // Load the entire image locally for comparison purpose
    alt_u32 *full_orig_pch_image = new alt_u32[FULL_PFR_IMAGE_PCH_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(FULL_PFR_IMAGE_PCH_FILE, full_orig_pch_image);

    alt_u32 *full_new_pch_image = new alt_u32[FULL_PFR_IMAGE_PCH_V03P12_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(FULL_PFR_IMAGE_PCH_V03P12_FILE, full_new_pch_image);

    /*
     * Flow preparation
     */
    ut_prep_nios_gpi_signals();

    // Exit after 1 iteration of the T0 loop
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS);
    // Send the recovery update intent
    ut_send_in_update_intent_once_upon_entry_to_t0(MB_BMC_UPDATE_INTENT, MB_UPDATE_INTENT_PCH_RECOVERY_MASK);

    /*
     * Run flow.
     * Active update flow will be triggered.
     */
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(80, pfr_main());

    /*
     * Verify the active update
     */
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(1));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), LAST_PANIC_BMC_UPDATE_INTENT);

    // Both static and dynamic regions are updated
    for (alt_u32 region_i = 0; region_i < PCH_NUM_STATIC_REGIONS; region_i++)
    {
        for (alt_u32 word_i = testdata_pch_static_regions_start_addr[region_i] >> 2;
                word_i < testdata_pch_static_regions_end_addr[region_i] >> 2; word_i++)
        {
            ASSERT_EQ(full_new_pch_image[word_i], pch_flash_x86_ptr[word_i]);
        }
    }
    for (alt_u32 region_i = 0; region_i < PCH_NUM_DYNAMIC_REGIONS; region_i++)
    {
        for (alt_u32 word_i = testdata_pch_dynamic_regions_start_addr[region_i] >> 2;
                word_i < testdata_pch_dynamic_regions_end_addr[region_i] >> 2; word_i++)
        {
            ASSERT_EQ(full_new_pch_image[word_i], pch_flash_x86_ptr[word_i]);
        }
    }

    /*
     * Run flow.
     * After entering T0, Nios was halted. Clear the ACM/BIOS timer count down value to trigger a watchdog recovery.
     */
    // Clear the count-down value and keep the HW tiemr in active mode.
    IOWR(WDT_ACM_BIOS_TIMER_ADDR, 0, U_TIMER_BANK_TIMER_ACTIVE_MASK);

    // Run the T0 loop again
    perform_t0_operations();

    /*
     * Check platform states and PCH/BMC flash content
     */
    // There should have been a watchdog recovery
    alt_u32 mb_platform_status = read_from_mailbox(MB_PLATFORM_STATE);
    EXPECT_TRUE((mb_platform_status == PLATFORM_STATE_ENTER_T0) || (mb_platform_status == PLATFORM_STATE_T0_ME_BOOTED));
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(2));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), LAST_PANIC_ACM_BIOS_WDT_EXPIRED);
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(1));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), LAST_RECOVERY_ACM_LAUNCH_FAIL);
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(0));

    // Check number of T-1 transitions (recovery update requires platform reset)
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_bmc_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_pch_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(3));

    // Both PCH and BMC should be out-of-reset
    EXPECT_TRUE(ut_is_bmc_out_of_reset());
    EXPECT_TRUE(ut_is_pch_out_of_reset());

    // Get the staging region offset
    alt_u32 pch_staging_start = get_staging_region_offset(SPI_FLASH_PCH);
    alt_u32 pch_staging_end = pch_staging_start + SPI_FLASH_PCH_STAGING_SIZE;
    alt_u32 pch_staging_start_word_i = pch_staging_start >> 2;
    alt_u32 pch_staging_end_word_i = pch_staging_end >> 2;

    // PCH flash should contain the original image
    for (alt_u32 word_i = 0; word_i < (FULL_PFR_IMAGE_PCH_FILE_SIZE >> 2); word_i++)
    {
        if ((pch_staging_start_word_i <= word_i) && (word_i < pch_staging_end_word_i))
        {
            // Skip the staging region
            continue;
        }
        ASSERT_EQ(full_orig_pch_image[word_i], pch_flash_x86_ptr[word_i]);
    }

    /*
     * Clean ups
     */
    delete[] full_orig_pch_image;
    delete[] full_new_pch_image;
}

/**
 * @brief During the BMC recovery update, BMC may not be able to boot after applying update on
 * its active firmware. It's expected that the recovery update should be abandoned and a watchdog recovery
 * should be applied on BMC active firmware.
 */
TEST_F(FWUpdateFlowNegativeTest, test_boot_failure_during_bmc_recovery_update)
{
    /*
     * Test Preparation
     */
    switch_spi_flash(SPI_FLASH_BMC);
    alt_u32* bmc_flash_x86_ptr = SYSTEM_MOCK::get()->get_x86_ptr_to_spi_flash(SPI_FLASH_BMC);

    // Load the PCH fw update capsule to BMC staging region in flash
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, SIGNED_CAPSULE_BMC_V14_FILE,
            SIGNED_CAPSULE_BMC_V14_FILE_SIZE, get_ufm_pfr_data()->bmc_staging_region);

    // Load the entire image locally for comparison purpose
    alt_u32 *full_orig_bmc_image = new alt_u32[FULL_PFR_IMAGE_BMC_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(FULL_PFR_IMAGE_BMC_FILE, full_orig_bmc_image);

    alt_u32 *full_new_bmc_image = new alt_u32[FULL_PFR_IMAGE_BMC_V14_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(FULL_PFR_IMAGE_BMC_V14_FILE, full_new_bmc_image);

    /*
     * Flow preparation
     */
    ut_prep_nios_gpi_signals();

    // Exit after 1 iteration of the T0 loop
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS);
    // Send the recovery update intent
    ut_send_in_update_intent_once_upon_entry_to_t0(MB_BMC_UPDATE_INTENT, MB_UPDATE_INTENT_BMC_RECOVERY_MASK);

    /*
     * Run flow.
     * Active update flow will be triggered.
     */
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(80, pfr_main());

    /*
     * Verify the active update
     */
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(1));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), LAST_PANIC_BMC_UPDATE_INTENT);

    // Both static and dynamic regions are updated
    for (alt_u32 region_i = 0; region_i < BMC_NUM_STATIC_REGIONS; region_i++)
    {
        for (alt_u32 word_i = testdata_bmc_static_regions_start_addr[region_i] >> 2;
                word_i < testdata_bmc_static_regions_end_addr[region_i] >> 2; word_i++)
        {
            ASSERT_EQ(full_new_bmc_image[word_i], bmc_flash_x86_ptr[word_i]);
        }
    }
    for (alt_u32 region_i = 0; region_i < BMC_NUM_DYNAMIC_REGIONS; region_i++)
    {
        for (alt_u32 word_i = testdata_bmc_dynamic_regions_start_addr[region_i] >> 2;
                word_i < testdata_bmc_dynamic_regions_end_addr[region_i] >> 2; word_i++)
        {
            ASSERT_EQ(full_new_bmc_image[word_i], bmc_flash_x86_ptr[word_i]);
        }
    }

    /*
     * Run flow.
     * After entering T0, Nios was halted. Clear the BMC timer count down value to trigger a watchdog recovery.
     */
    // Clear the count-down value and keep the HW tiemr in active mode.
    IOWR(WDT_BMC_TIMER_ADDR, 0, U_TIMER_BANK_TIMER_ACTIVE_MASK);

    // Run the T0 loop again
    perform_t0_operations();

    /*
     * Check platform states and PCH/BMC flash content
     */
    // There should have been a watchdog recovery
    alt_u32 mb_platform_status = read_from_mailbox(MB_PLATFORM_STATE);
    EXPECT_TRUE((mb_platform_status == PLATFORM_STATE_ENTER_T0) || (mb_platform_status == PLATFORM_STATE_T0_ME_BOOTED));
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(2));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), LAST_PANIC_BMC_WDT_EXPIRED);
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(1));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), LAST_RECOVERY_BMC_LAUNCH_FAIL);
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(0));

    // Check number of T-1 transitions (recovery update requires platform reset)
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_bmc_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_pch_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(3));

    // Both PCH and BMC should be out-of-reset
    EXPECT_TRUE(ut_is_bmc_out_of_reset());
    EXPECT_TRUE(ut_is_pch_out_of_reset());

    // Get the staging region offset
    alt_u32 bmc_staging_start = get_staging_region_offset(SPI_FLASH_BMC);
    alt_u32 bmc_staging_end = bmc_staging_start + SPI_FLASH_BMC_STAGING_SIZE;
    alt_u32 bmc_staging_start_word_i = bmc_staging_start >> 2;
    alt_u32 bmc_staging_end_word_i = bmc_staging_end >> 2;

    // BMC flash should contain the original image
    for (alt_u32 word_i = 0; word_i < (FULL_PFR_IMAGE_BMC_FILE_SIZE >> 2); word_i++)
    {
        if ((bmc_staging_start_word_i <= word_i) && (word_i < bmc_staging_end_word_i))
        {
            // Skip the staging region
            continue;
        }
        ASSERT_EQ(full_orig_bmc_image[word_i], bmc_flash_x86_ptr[word_i]);
    }

    /*
     * Clean ups
     */
    delete[] full_orig_bmc_image;
    delete[] full_new_bmc_image;
}
