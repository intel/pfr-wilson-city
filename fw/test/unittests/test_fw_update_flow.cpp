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


class FWUpdateFlowTest : public testing::Test
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

        // Load some capsules to flash
        // If there're unexpected updates, unittest can catch them.
        SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_PCH, SIGNED_CAPSULE_PCH_V03P12_FILE,
                SIGNED_CAPSULE_PCH_V03P12_FILE_SIZE, get_ufm_pfr_data()->pch_staging_region);
        SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, SIGNED_CAPSULE_BMC_V14_FILE,
                SIGNED_CAPSULE_BMC_V14_FILE_SIZE, get_ufm_pfr_data()->bmc_staging_region);
    }

    virtual void TearDown() {}
};

/*
 * This test sends the update intent and runs pfr_main.
 * The active update flow is triggered.
 */
TEST_F(FWUpdateFlowTest, test_active_update_pch)
{
    /*
     * Test Preparation
     */
    switch_spi_flash(SPI_FLASH_PCH);
    alt_u32* flash_x86_ptr = SYSTEM_MOCK::get()->get_x86_ptr_to_spi_flash(SPI_FLASH_PCH);

    // Load the update capsule
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_PCH, SIGNED_CAPSULE_PCH_V03P12_FILE,
            SIGNED_CAPSULE_PCH_V03P12_FILE_SIZE, get_ufm_pfr_data()->pch_staging_region);

    // Load the entire images locally for comparison purpose
    alt_u32 *full_orig_image = new alt_u32[FULL_PFR_IMAGE_PCH_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(FULL_PFR_IMAGE_PCH_FILE, full_orig_image);

    alt_u32 *full_new_image = new alt_u32[FULL_PFR_IMAGE_PCH_V03P12_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(FULL_PFR_IMAGE_PCH_V03P12_FILE, full_new_image);

    /*
     * Flow preparation
     */
    ut_prep_nios_gpi_signals();
    ut_send_block_complete_chkpt_msg();

    // Exit after 50 iterations in the T0 loop
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS_END_AFTER_50_ITERS);

    // Send the active update intent
    ut_send_in_update_intent_once_upon_boot_complete(MB_PCH_UPDATE_INTENT, MB_UPDATE_INTENT_PCH_ACTIVE_MASK);

    /*
     * Execute the flow. Active update flow will be triggered.
     */
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(40, pfr_main());

    /*
     * Verify results
     */
    // Nios should transition from T0 to T-1 exactly once for active update flow
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_bmc_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_pch_only_counter(), alt_u32(1));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(1));

    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(1));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), LAST_PANIC_PCH_UPDATE_INTENT);
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(0));

    // Check PCH FW version
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_ACTIVE_MAJOR_VER), alt_u32(SIGNED_CAPSULE_PCH_V03P12_FILE_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_ACTIVE_MINOR_VER), alt_u32(SIGNED_CAPSULE_PCH_V03P12_FILE_PFM_MINOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_RECOVERY_MAJOR_VER), alt_u32(PCH_RECOVERY_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_RECOVERY_MINOR_VER), alt_u32(PCH_RECOVERY_PFM_MINOR_VER));

    // Check BMC FW version
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_ACTIVE_MAJOR_VER), alt_u32(BMC_ACTIVE_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_ACTIVE_MINOR_VER), alt_u32(BMC_ACTIVE_PFM_MINOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_RECOVERY_MAJOR_VER), alt_u32(BMC_RECOVERY_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_RECOVERY_MINOR_VER), alt_u32(BMC_RECOVERY_PFM_MINOR_VER));

    // Check for updated data in static regions
    for (alt_u32 region_i = 0; region_i < PCH_NUM_STATIC_REGIONS; region_i++)
    {
        for (alt_u32 word_i = testdata_pch_static_regions_start_addr[region_i] >> 2;
                word_i < testdata_pch_static_regions_end_addr[region_i] >> 2; word_i++)
        {
            ASSERT_EQ(full_new_image[word_i], flash_x86_ptr[word_i]);
        }
    }

    // Check that the dynamic regions are untouched.
    for (alt_u32 region_i = 0; region_i < PCH_NUM_DYNAMIC_REGIONS; region_i++)
    {
        for (alt_u32 word_i = testdata_pch_dynamic_regions_start_addr[region_i] >> 2;
                word_i < testdata_pch_dynamic_regions_end_addr[region_i] >> 2; word_i++)
        {
            ASSERT_EQ(full_orig_image[word_i], flash_x86_ptr[word_i]);
        }
    }

    // Verify updated PFM
    alt_u32 pch_active_pfm_start = get_ufm_pfr_data()->pch_active_pfm;
    switch_spi_flash(SPI_FLASH_PCH);
    alt_u32 pch_active_pfm_end = pch_active_pfm_start + get_signed_payload_size(get_spi_active_pfm_ptr(SPI_FLASH_PCH));
    for (alt_u32 word_i = pch_active_pfm_start >> 2; word_i < pch_active_pfm_end >> 2; word_i++)
    {
        ASSERT_EQ(full_new_image[word_i], flash_x86_ptr[word_i]);
    }

    /*
     * Clean ups
     */
    delete[] full_orig_image;
    delete[] full_new_image;
}


/*
 * This test sends the update intent and runs pfr_main.
 * The active update flow is triggered.
 */
TEST_F(FWUpdateFlowTest, test_active_update_pch_oob)
{
    /*
     * Test Preparation
     */
    switch_spi_flash(SPI_FLASH_PCH);
    alt_u32* flash_x86_ptr = SYSTEM_MOCK::get()->get_x86_ptr_to_spi_flash(SPI_FLASH_PCH);

    // Load the update capsule
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, SIGNED_CAPSULE_PCH_V03P12_FILE,
            SIGNED_CAPSULE_PCH_V03P12_FILE_SIZE, get_ufm_pfr_data()->bmc_staging_region + BMC_STAGING_REGION_PCH_UPDATE_CAPSULE_OFFSET);

    // Load the entire images locally for comparison purpose
    alt_u32 *full_orig_image = new alt_u32[FULL_PFR_IMAGE_PCH_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(FULL_PFR_IMAGE_PCH_FILE, full_orig_image);

    alt_u32 *full_new_image = new alt_u32[FULL_PFR_IMAGE_PCH_V03P12_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(FULL_PFR_IMAGE_PCH_V03P12_FILE, full_new_image);

    /*
     * Flow preparation
     */
    ut_prep_nios_gpi_signals();
    ut_send_block_complete_chkpt_msg();

    // Exit after 50 iterations in the T0 loop
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS_END_AFTER_50_ITERS);

    // Send the active update intent
    ut_send_in_update_intent_once_upon_boot_complete(MB_BMC_UPDATE_INTENT, MB_UPDATE_INTENT_PCH_ACTIVE_MASK);

    /*
     * Execute the flow. Active update flow will be triggered.
     */
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(40, pfr_main());

    /*
     * Verify results
     */
    // Nios should transition from T0 to T-1 exactly once for active update flow
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_bmc_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_pch_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(2));

    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(1));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), LAST_PANIC_BMC_UPDATE_INTENT);
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(0));

    // Check PCH FW version
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_ACTIVE_MAJOR_VER), alt_u32(SIGNED_CAPSULE_PCH_V03P12_FILE_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_ACTIVE_MINOR_VER), alt_u32(SIGNED_CAPSULE_PCH_V03P12_FILE_PFM_MINOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_RECOVERY_MAJOR_VER), alt_u32(PCH_RECOVERY_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_RECOVERY_MINOR_VER), alt_u32(PCH_RECOVERY_PFM_MINOR_VER));

    // Check BMC FW version
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_ACTIVE_MAJOR_VER), alt_u32(BMC_ACTIVE_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_ACTIVE_MINOR_VER), alt_u32(BMC_ACTIVE_PFM_MINOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_RECOVERY_MAJOR_VER), alt_u32(BMC_RECOVERY_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_RECOVERY_MINOR_VER), alt_u32(BMC_RECOVERY_PFM_MINOR_VER));

    // Check for updated data in static regions
    for (alt_u32 region_i = 0; region_i < PCH_NUM_STATIC_REGIONS; region_i++)
    {
        for (alt_u32 word_i = testdata_pch_static_regions_start_addr[region_i] >> 2;
                word_i < testdata_pch_static_regions_end_addr[region_i] >> 2; word_i++)
        {
            ASSERT_EQ(full_new_image[word_i], flash_x86_ptr[word_i]);
        }
    }

    // Check that the dynamic regions are untouched.
    for (alt_u32 region_i = 0; region_i < PCH_NUM_DYNAMIC_REGIONS; region_i++)
    {
        for (alt_u32 word_i = testdata_pch_dynamic_regions_start_addr[region_i] >> 2;
                word_i < testdata_pch_dynamic_regions_end_addr[region_i] >> 2; word_i++)
        {
            ASSERT_EQ(full_orig_image[word_i], flash_x86_ptr[word_i]);
        }
    }

    // Verify updated PFM
    alt_u32 pch_active_pfm_start = get_ufm_pfr_data()->pch_active_pfm;
    switch_spi_flash(SPI_FLASH_PCH);
    alt_u32 pch_active_pfm_end = pch_active_pfm_start + get_signed_payload_size(get_spi_active_pfm_ptr(SPI_FLASH_PCH));
    for (alt_u32 word_i = pch_active_pfm_start >> 2; word_i < pch_active_pfm_end >> 2; word_i++)
    {
        ASSERT_EQ(full_new_image[word_i], flash_x86_ptr[word_i]);
    }

    /*
     * Clean ups
     */
    delete[] full_orig_image;
    delete[] full_new_image;
}

/*
 * This test loads a BMC image with version 0.11 PFM to BMC flash.
 * In BMC staging region, there's an update capsule with version 0.9 PFM.
 *
 * This test sends a BMC active firmware update intent to Nios firmware.
 * The results are compared against a BMC image with version 0.9 PFM.
 */
TEST_F(FWUpdateFlowTest, test_active_update_bmc)
{
    /*
     * Test Preparation
     */
    switch_spi_flash(SPI_FLASH_BMC);
    alt_u32* flash_x86_ptr = SYSTEM_MOCK::get()->get_x86_ptr_to_spi_flash(SPI_FLASH_BMC);

    // Load the update capsule
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, SIGNED_CAPSULE_BMC_V14_FILE,
            SIGNED_CAPSULE_BMC_V14_FILE_SIZE, get_ufm_pfr_data()->bmc_staging_region);

    // Load the entire images locally for comparison purpose
    alt_u32 *full_orig_image = new alt_u32[FULL_PFR_IMAGE_BMC_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(FULL_PFR_IMAGE_BMC_FILE, full_orig_image);

    alt_u32 *full_new_image = new alt_u32[FULL_PFR_IMAGE_BMC_V14_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(FULL_PFR_IMAGE_BMC_V14_FILE, full_new_image);

    /*
     * Flow preparation
     */
    ut_prep_nios_gpi_signals();
    ut_send_block_complete_chkpt_msg();

    // Exit after 50 iterations in the T0 loop
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS_END_AFTER_50_ITERS);

    // Send the active update intent
    ut_send_in_update_intent_once_upon_boot_complete(MB_BMC_UPDATE_INTENT, MB_UPDATE_INTENT_BMC_ACTIVE_MASK);

    /*
     * Execute the flow. Active update flow will be triggered.
     */
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(240, pfr_main());

    /*
     * Verify results
     */
    // Nios should transition from T0 to T-1 exactly once for active update flow
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_bmc_only_counter(), alt_u32(1));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_pch_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(1));

    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(1));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), LAST_PANIC_BMC_UPDATE_INTENT);
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(0));

    // Check PCH FW version
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_ACTIVE_MAJOR_VER), alt_u32(PCH_ACTIVE_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_ACTIVE_MINOR_VER), alt_u32(PCH_ACTIVE_PFM_MINOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_RECOVERY_MAJOR_VER), alt_u32(PCH_RECOVERY_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_RECOVERY_MINOR_VER), alt_u32(PCH_RECOVERY_PFM_MINOR_VER));

    // Check BMC FW version
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_ACTIVE_MAJOR_VER), alt_u32(SIGNED_CAPSULE_BMC_V14_FILE_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_ACTIVE_MINOR_VER), alt_u32(SIGNED_CAPSULE_BMC_V14_FILE_PFM_MINOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_RECOVERY_MAJOR_VER), alt_u32(BMC_RECOVERY_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_RECOVERY_MINOR_VER), alt_u32(BMC_RECOVERY_PFM_MINOR_VER));

    // In these BMC images, there are only two read-only area
    // Check for updated data in static regions
    for (alt_u32 region_i = 0; region_i < BMC_NUM_STATIC_REGIONS; region_i++)
    {
        for (alt_u32 word_i = testdata_bmc_static_regions_start_addr[region_i] >> 2;
                word_i < testdata_bmc_static_regions_end_addr[region_i] >> 2; word_i++)
        {
            ASSERT_EQ(full_new_image[word_i], flash_x86_ptr[word_i]);
        }
    }

    // Check that the dynamic regions are untouched.
    for (alt_u32 region_i = 0; region_i < BMC_NUM_DYNAMIC_REGIONS; region_i++)
    {
        for (alt_u32 word_i = testdata_bmc_dynamic_regions_start_addr[region_i] >> 2;
                word_i < testdata_bmc_dynamic_regions_end_addr[region_i] >> 2; word_i++)
        {
            ASSERT_EQ(full_orig_image[word_i], flash_x86_ptr[word_i]);
        }
    }

    // Verify updated PFM
    alt_u32 active_pfm_start = get_ufm_pfr_data()->bmc_active_pfm;
    switch_spi_flash(SPI_FLASH_BMC);
    alt_u32 active_pfm_end = active_pfm_start + get_signed_payload_size(get_spi_active_pfm_ptr(SPI_FLASH_BMC));
    for (alt_u32 word_i = active_pfm_start >> 2; word_i < active_pfm_end >> 2; word_i++)
    {
        ASSERT_EQ(full_new_image[word_i], flash_x86_ptr[word_i]);
    }

    /*
     * Clean ups
     */
    delete[] full_orig_image;
    delete[] full_new_image;
}

/*
 * This test sends the update intent and runs pfr_main.
 * The active update flow is triggered.
 */
TEST_F(FWUpdateFlowTest, test_active_update_with_dynamic_pch)
{
    /*
     * Test Preparation
     */
    switch_spi_flash(SPI_FLASH_PCH);
    alt_u32* flash_x86_ptr = SYSTEM_MOCK::get()->get_x86_ptr_to_spi_flash(SPI_FLASH_PCH);

    // Load the update capsule
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_PCH, SIGNED_CAPSULE_PCH_V03P12_FILE,
            SIGNED_CAPSULE_PCH_V03P12_FILE_SIZE, get_ufm_pfr_data()->pch_staging_region);

    // Load the entire image locally for comparison purpose
    alt_u32 *full_new_image = new alt_u32[FULL_PFR_IMAGE_PCH_V03P12_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(FULL_PFR_IMAGE_PCH_V03P12_FILE, full_new_image);

    /*
     * Flow preparation
     */
    ut_prep_nios_gpi_signals();
    ut_send_block_complete_chkpt_msg();

    // Exit after 50 iterations in the T0 loop
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS_END_AFTER_50_ITERS);

    // Send the active update intent
    ut_send_in_update_intent_once_upon_boot_complete(MB_PCH_UPDATE_INTENT,
            MB_UPDATE_INTENT_PCH_ACTIVE_MASK | MB_UPDATE_INTENT_UPDATE_DYNAMIC_MASK);

    /*
     * Execute the flow. Active update flow will be triggered.
     */
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(40, pfr_main());

    /*
     * Verify results
     */
    // Nios should transition from T0 to T-1 exactly once for active update flow
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_bmc_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_pch_only_counter(), alt_u32(1));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(1));

    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(1));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), LAST_PANIC_PCH_UPDATE_INTENT);
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(0));

    // Check PCH FW version
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_ACTIVE_MAJOR_VER), alt_u32(SIGNED_CAPSULE_PCH_V03P12_FILE_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_ACTIVE_MINOR_VER), alt_u32(SIGNED_CAPSULE_PCH_V03P12_FILE_PFM_MINOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_RECOVERY_MAJOR_VER), alt_u32(PCH_RECOVERY_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_RECOVERY_MINOR_VER), alt_u32(PCH_RECOVERY_PFM_MINOR_VER));

    // Check BMC FW version
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_ACTIVE_MAJOR_VER), alt_u32(BMC_ACTIVE_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_ACTIVE_MINOR_VER), alt_u32(BMC_ACTIVE_PFM_MINOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_RECOVERY_MAJOR_VER), alt_u32(BMC_RECOVERY_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_RECOVERY_MINOR_VER), alt_u32(BMC_RECOVERY_PFM_MINOR_VER));

    // Check for updated data in static regions
    for (alt_u32 region_i = 0; region_i < PCH_NUM_STATIC_REGIONS; region_i++)
    {
        for (alt_u32 word_i = testdata_pch_static_regions_start_addr[region_i] >> 2;
                word_i < testdata_pch_static_regions_end_addr[region_i] >> 2; word_i++)
        {
            ASSERT_EQ(full_new_image[word_i], flash_x86_ptr[word_i]);
        }
    }

    // Check for updated data in dynamic regions
    for (alt_u32 region_i = 0; region_i < PCH_NUM_DYNAMIC_REGIONS; region_i++)
    {
        for (alt_u32 word_i = testdata_pch_dynamic_regions_start_addr[region_i] >> 2;
                word_i < testdata_pch_dynamic_regions_end_addr[region_i] >> 2; word_i++)
        {
            ASSERT_EQ(full_new_image[word_i], flash_x86_ptr[word_i]);
        }
    }

    // Verify updated PFM
    alt_u32 pch_active_pfm_start = get_ufm_pfr_data()->pch_active_pfm;
    switch_spi_flash(SPI_FLASH_PCH);
    alt_u32 pch_active_pfm_end = pch_active_pfm_start + get_signed_payload_size(get_spi_active_pfm_ptr(SPI_FLASH_PCH));
    for (alt_u32 word_i = pch_active_pfm_start >> 2; word_i < pch_active_pfm_end >> 2; word_i++)
    {
        ASSERT_EQ(full_new_image[word_i], flash_x86_ptr[word_i]);
    }

    /*
     * Clean ups
     */
    delete[] full_new_image;
}

/*
 * This test sends the update intent and runs pfr_main.
 * The active update flow is triggered.
 */
TEST_F(FWUpdateFlowTest, test_active_update_with_dynamic_pch_oob)
{
    /*
     * Test Preparation
     */
    switch_spi_flash(SPI_FLASH_PCH);
    alt_u32* flash_x86_ptr = SYSTEM_MOCK::get()->get_x86_ptr_to_spi_flash(SPI_FLASH_PCH);

    // Load the update capsule
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, SIGNED_CAPSULE_PCH_V03P12_FILE,
            SIGNED_CAPSULE_PCH_V03P12_FILE_SIZE, get_ufm_pfr_data()->bmc_staging_region + BMC_STAGING_REGION_PCH_UPDATE_CAPSULE_OFFSET);

    // Load the entire image locally for comparison purpose
    alt_u32 *full_new_image = new alt_u32[FULL_PFR_IMAGE_PCH_V03P12_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(FULL_PFR_IMAGE_PCH_V03P12_FILE, full_new_image);

    /*
     * Flow preparation
     */
    ut_prep_nios_gpi_signals();
    ut_send_block_complete_chkpt_msg();

    // Exit after 50 iterations in the T0 loop
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS_END_AFTER_50_ITERS);

    // Send the active update intent
    ut_send_in_update_intent_once_upon_boot_complete(MB_BMC_UPDATE_INTENT,
            MB_UPDATE_INTENT_PCH_ACTIVE_MASK | MB_UPDATE_INTENT_UPDATE_DYNAMIC_MASK);

    /*
     * Execute the flow. Active update flow will be triggered.
     */
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(40, pfr_main());

    /*
     * Verify results
     */
    // Nios should transition from T0 to T-1 exactly once for active update flow
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_bmc_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_pch_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(2));

    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(1));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), LAST_PANIC_BMC_UPDATE_INTENT);
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(0));

    // Check PCH FW version
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_ACTIVE_MAJOR_VER), alt_u32(SIGNED_CAPSULE_PCH_V03P12_FILE_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_ACTIVE_MINOR_VER), alt_u32(SIGNED_CAPSULE_PCH_V03P12_FILE_PFM_MINOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_RECOVERY_MAJOR_VER), alt_u32(PCH_RECOVERY_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_RECOVERY_MINOR_VER), alt_u32(PCH_RECOVERY_PFM_MINOR_VER));

    // Check BMC FW version
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_ACTIVE_MAJOR_VER), alt_u32(BMC_ACTIVE_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_ACTIVE_MINOR_VER), alt_u32(BMC_ACTIVE_PFM_MINOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_RECOVERY_MAJOR_VER), alt_u32(BMC_RECOVERY_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_RECOVERY_MINOR_VER), alt_u32(BMC_RECOVERY_PFM_MINOR_VER));

    // Check for updated data in static regions
    for (alt_u32 region_i = 0; region_i < PCH_NUM_STATIC_REGIONS; region_i++)
    {
        for (alt_u32 word_i = testdata_pch_static_regions_start_addr[region_i] >> 2;
                word_i < testdata_pch_static_regions_end_addr[region_i] >> 2; word_i++)
        {
            ASSERT_EQ(full_new_image[word_i], flash_x86_ptr[word_i]);
        }
    }

    // Check for updated data in dynamic regions
    for (alt_u32 region_i = 0; region_i < PCH_NUM_DYNAMIC_REGIONS; region_i++)
    {
        for (alt_u32 word_i = testdata_pch_dynamic_regions_start_addr[region_i] >> 2;
                word_i < testdata_pch_dynamic_regions_end_addr[region_i] >> 2; word_i++)
        {
            ASSERT_EQ(full_new_image[word_i], flash_x86_ptr[word_i]);
        }
    }

    // Verify updated PFM
    alt_u32 pch_active_pfm_start = get_ufm_pfr_data()->pch_active_pfm;
    switch_spi_flash(SPI_FLASH_PCH);
    alt_u32 pch_active_pfm_end = pch_active_pfm_start + get_signed_payload_size(get_spi_active_pfm_ptr(SPI_FLASH_PCH));
    for (alt_u32 word_i = pch_active_pfm_start >> 2; word_i < pch_active_pfm_end >> 2; word_i++)
    {
        ASSERT_EQ(full_new_image[word_i], flash_x86_ptr[word_i]);
    }

    /*
     * Clean ups
     */
    delete[] full_new_image;
}

/*
 * This test loads a BMC image with version 0.11 PFM to BMC flash.
 * In BMC staging region, there's an update capsule with version 0.9 PFM.
 *
 * This test sends a BMC active firmware update intent to Nios firmware.
 * The results are compared against a BMC image with version 0.9 PFM.
 */
TEST_F(FWUpdateFlowTest, test_active_update_with_dynamic_bmc)
{
    /*
     * Test Preparation
     */
    switch_spi_flash(SPI_FLASH_BMC);
    alt_u32* flash_x86_ptr = SYSTEM_MOCK::get()->get_x86_ptr_to_spi_flash(SPI_FLASH_BMC);

    // Load the update capsule
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, SIGNED_CAPSULE_BMC_V14_FILE,
            SIGNED_CAPSULE_BMC_V14_FILE_SIZE, get_ufm_pfr_data()->bmc_staging_region);

    // Load the entire image locally for comparison purpose
    alt_u32 *full_new_image = new alt_u32[FULL_PFR_IMAGE_BMC_V14_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(FULL_PFR_IMAGE_BMC_V14_FILE, full_new_image);

    /*
     * Flow preparation
     */
    ut_prep_nios_gpi_signals();
    ut_send_block_complete_chkpt_msg();

    // Exit after 50 iterations in the T0 loop
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS_END_AFTER_50_ITERS);

    // Send the active update intent
    ut_send_in_update_intent_once_upon_boot_complete(MB_BMC_UPDATE_INTENT,
            MB_UPDATE_INTENT_BMC_ACTIVE_MASK | MB_UPDATE_INTENT_UPDATE_DYNAMIC_MASK);

    /*
     * Execute the flow. Active update flow will be triggered.
     */
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(240, pfr_main());

    /*
     * Verify results
     */
    // Nios should transition from T0 to T-1 exactly once for active update flow
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_bmc_only_counter(), alt_u32(1));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_pch_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(1));

    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(1));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), LAST_PANIC_BMC_UPDATE_INTENT);
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(0));

    // Check PCH FW version
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_ACTIVE_MAJOR_VER), alt_u32(PCH_ACTIVE_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_ACTIVE_MINOR_VER), alt_u32(PCH_ACTIVE_PFM_MINOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_RECOVERY_MAJOR_VER), alt_u32(PCH_RECOVERY_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_RECOVERY_MINOR_VER), alt_u32(PCH_RECOVERY_PFM_MINOR_VER));

    // Check BMC FW version
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_ACTIVE_MAJOR_VER), alt_u32(SIGNED_CAPSULE_BMC_V14_FILE_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_ACTIVE_MINOR_VER), alt_u32(SIGNED_CAPSULE_BMC_V14_FILE_PFM_MINOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_RECOVERY_MAJOR_VER), alt_u32(BMC_RECOVERY_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_RECOVERY_MINOR_VER), alt_u32(BMC_RECOVERY_PFM_MINOR_VER));

    // In these BMC images, there are only two read-only area
    // Check for updated data in static regions
    for (alt_u32 region_i = 0; region_i < BMC_NUM_STATIC_REGIONS; region_i++)
    {
        for (alt_u32 word_i = testdata_bmc_static_regions_start_addr[region_i] >> 2;
                word_i < testdata_bmc_static_regions_end_addr[region_i] >> 2; word_i++)
        {
            ASSERT_EQ(full_new_image[word_i], flash_x86_ptr[word_i]);
        }
    }

    // Check that the dynamic regions are untouched.
    for (alt_u32 region_i = 0; region_i < BMC_NUM_DYNAMIC_REGIONS; region_i++)
    {
        for (alt_u32 word_i = testdata_bmc_dynamic_regions_start_addr[region_i] >> 2;
                word_i < testdata_bmc_dynamic_regions_end_addr[region_i] >> 2; word_i++)
        {
            ASSERT_EQ(full_new_image[word_i], flash_x86_ptr[word_i]);
        }
    }

    // Verify updated PFM
    alt_u32 active_pfm_start = get_ufm_pfr_data()->bmc_active_pfm;
    switch_spi_flash(SPI_FLASH_BMC);
    alt_u32 active_pfm_end = active_pfm_start + get_signed_payload_size(get_spi_active_pfm_ptr(SPI_FLASH_BMC));
    for (alt_u32 word_i = active_pfm_start >> 2; word_i < active_pfm_end >> 2; word_i++)
    {
        ASSERT_EQ(full_new_image[word_i], flash_x86_ptr[word_i]);
    }

    /*
     * Clean ups
     */
    delete[] full_new_image;
}

/*
 * This test sends a deferred update to the PCH update intent register
 */
TEST_F(FWUpdateFlowTest, test_deferred_active_update_triggered_by_pltrst_for_pch)
{
    /*
     * Test Preparation
     */
    switch_spi_flash(SPI_FLASH_PCH);
    alt_u32* flash_x86_ptr = SYSTEM_MOCK::get()->get_x86_ptr_to_spi_flash(SPI_FLASH_PCH);

    // Load the update capsule
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_PCH, SIGNED_CAPSULE_PCH_V03P12_FILE,
            SIGNED_CAPSULE_PCH_V03P12_FILE_SIZE, get_ufm_pfr_data()->pch_staging_region);

    // Load the entire image locally for comparison purpose
    alt_u32 *full_new_image = new alt_u32[FULL_PFR_IMAGE_PCH_V03P12_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(FULL_PFR_IMAGE_PCH_V03P12_FILE, full_new_image);

    /*
     * Flow preparation
     */
    ut_prep_nios_gpi_signals();
    ut_send_block_complete_chkpt_msg();

    // Toggle PLTRST once when platform gets to boot complete
    ut_toggle_pltrst_gpi_once_upon_boot_complete();

    // Exit after 50 iterations in the T0 loop
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS_END_AFTER_50_ITERS);
    // Send the active update intent
    ut_send_in_update_intent_once_upon_entry_to_t0(
            MB_PCH_UPDATE_INTENT, MB_UPDATE_INTENT_PCH_ACTIVE_MASK | MB_UPDATE_INTENT_UPDATE_AT_RESET_MASK);

    /*
     * Execute the flow.
     */
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(25, pfr_main());

    // There should be one panic event for the PCH update
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(1));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), alt_u32(LAST_PANIC_PCH_UPDATE_INTENT));

    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(0));

    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(0));

    // Check PCH FW version
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_ACTIVE_MAJOR_VER), alt_u32(SIGNED_CAPSULE_PCH_V03P12_FILE_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_ACTIVE_MINOR_VER), alt_u32(SIGNED_CAPSULE_PCH_V03P12_FILE_PFM_MINOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_RECOVERY_MAJOR_VER), alt_u32(PCH_RECOVERY_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_RECOVERY_MINOR_VER), alt_u32(PCH_RECOVERY_PFM_MINOR_VER));

    // Check BMC FW version
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_ACTIVE_MAJOR_VER), alt_u32(BMC_ACTIVE_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_ACTIVE_MINOR_VER), alt_u32(BMC_ACTIVE_PFM_MINOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_RECOVERY_MAJOR_VER), alt_u32(BMC_RECOVERY_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_RECOVERY_MINOR_VER), alt_u32(BMC_RECOVERY_PFM_MINOR_VER));

    // Check number of T-1 transitions
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_bmc_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_pch_only_counter(), alt_u32(1));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(1));

    // Update should be executed now. Check the result.
    for (alt_u32 region_i = 0; region_i < PCH_NUM_STATIC_REGIONS; region_i++)
    {
        for (alt_u32 word_i = testdata_pch_static_regions_start_addr[region_i] >> 2;
                word_i < testdata_pch_static_regions_end_addr[region_i] >> 2; word_i++)
        {
            ASSERT_EQ(full_new_image[word_i], flash_x86_ptr[word_i]);
        }
    }

    /*
     * Clean ups
     */
    delete[] full_new_image;
}

/*
 * This test sends a deferred update to the BMC update intent register
 */
TEST_F(FWUpdateFlowTest, test_deferred_active_update_triggered_by_pltrst_for_bmc)
{
    /*
     * Test Preparation
     */
    switch_spi_flash(SPI_FLASH_BMC);
    alt_u32* flash_x86_ptr = SYSTEM_MOCK::get()->get_x86_ptr_to_spi_flash(SPI_FLASH_BMC);

    // Load the update capsule
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, SIGNED_CAPSULE_BMC_V14_FILE,
            SIGNED_CAPSULE_BMC_V14_FILE_SIZE, get_ufm_pfr_data()->bmc_staging_region);

    alt_u32 *full_new_image = new alt_u32[FULL_PFR_IMAGE_BMC_V14_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(FULL_PFR_IMAGE_BMC_V14_FILE, full_new_image);

    /*
     * Flow preparation
     */
    ut_prep_nios_gpi_signals();
    ut_send_block_complete_chkpt_msg();

    // Toggle PLTRST once when platform gets to boot complete
    ut_toggle_pltrst_gpi_once_upon_boot_complete();

    // Exit after 50 iterations in the T0 loop
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS_END_AFTER_50_ITERS);

    // Send the active update intent
    ut_send_in_update_intent_once_upon_entry_to_t0(
            MB_BMC_UPDATE_INTENT, MB_UPDATE_INTENT_BMC_ACTIVE_MASK | MB_UPDATE_INTENT_UPDATE_AT_RESET_MASK);

    /*
     * Execute the flow.
     */
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(40, pfr_main());

    // There should be one panic event for the PCH update
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(1));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), alt_u32(LAST_PANIC_BMC_UPDATE_INTENT));

    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(0));

    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(0));

    // Check PCH FW version
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_ACTIVE_MAJOR_VER), alt_u32(PCH_ACTIVE_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_ACTIVE_MINOR_VER), alt_u32(PCH_ACTIVE_PFM_MINOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_RECOVERY_MAJOR_VER), alt_u32(PCH_RECOVERY_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_RECOVERY_MINOR_VER), alt_u32(PCH_RECOVERY_PFM_MINOR_VER));

    // Check BMC FW version
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_ACTIVE_MAJOR_VER), alt_u32(SIGNED_CAPSULE_BMC_V14_FILE_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_ACTIVE_MINOR_VER), alt_u32(SIGNED_CAPSULE_BMC_V14_FILE_PFM_MINOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_RECOVERY_MAJOR_VER), alt_u32(BMC_RECOVERY_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_RECOVERY_MINOR_VER), alt_u32(BMC_RECOVERY_PFM_MINOR_VER));

    // Check number of T-1 transitions
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_bmc_only_counter(), alt_u32(1));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_pch_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(1));

    /*
     * Verify updated data
     */
    for (alt_u32 region_i = 0; region_i < BMC_NUM_STATIC_REGIONS; region_i++)
    {
        for (alt_u32 word_i = testdata_bmc_static_regions_start_addr[region_i] >> 2;
                word_i < testdata_bmc_static_regions_end_addr[region_i] >> 2; word_i++)
        {
            ASSERT_EQ(full_new_image[word_i], flash_x86_ptr[word_i]);
        }
    }

    /*
     * Clean ups
     */
    delete[] full_new_image;
}

/*
 * This test sends a deferred BMC active fw update to the BMC update intent register.
 * This test also sends a bmc reset detected GPI signal.
 */
TEST_F(FWUpdateFlowTest, test_deferred_active_update_triggered_by_bmc_reset_for_bmc)
{
    /*
     * Test Preparation
     */
    switch_spi_flash(SPI_FLASH_BMC);
    alt_u32* flash_x86_ptr = SYSTEM_MOCK::get()->get_x86_ptr_to_spi_flash(SPI_FLASH_BMC);

    // Load the update capsule
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, SIGNED_CAPSULE_BMC_V14_FILE,
            SIGNED_CAPSULE_BMC_V14_FILE_SIZE, get_ufm_pfr_data()->bmc_staging_region);

    alt_u32 *full_new_image = new alt_u32[FULL_PFR_IMAGE_BMC_V14_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(FULL_PFR_IMAGE_BMC_V14_FILE, full_new_image);

    /*
     * Flow preparation
     */
    ut_prep_nios_gpi_signals();
    ut_send_block_complete_chkpt_msg();

    // Exit after 50 iterations in the T0 loop
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS_END_AFTER_50_ITERS);

    // Send the active update intent
    ut_send_in_update_intent_once_upon_boot_complete(
            MB_BMC_UPDATE_INTENT, MB_UPDATE_INTENT_BMC_ACTIVE_MASK | MB_UPDATE_INTENT_UPDATE_AT_RESET_MASK);

    // Send BMC reset detected GPI signal once when the platform has booted
    ut_send_bmc_reset_detected_gpi_once_upon_boot_complete();

    /*
     * Execute the flow.
     */
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(40, pfr_main());

    // There should be one panic event for the PCH update
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(1));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), alt_u32(LAST_PANIC_BMC_RESET_DETECTED));

    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(0));

    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(0));

    // Check PCH FW version
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_ACTIVE_MAJOR_VER), alt_u32(PCH_ACTIVE_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_ACTIVE_MINOR_VER), alt_u32(PCH_ACTIVE_PFM_MINOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_RECOVERY_MAJOR_VER), alt_u32(PCH_RECOVERY_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_RECOVERY_MINOR_VER), alt_u32(PCH_RECOVERY_PFM_MINOR_VER));

    // Check BMC FW version
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_ACTIVE_MAJOR_VER), alt_u32(SIGNED_CAPSULE_BMC_V14_FILE_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_ACTIVE_MINOR_VER), alt_u32(SIGNED_CAPSULE_BMC_V14_FILE_PFM_MINOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_RECOVERY_MAJOR_VER), alt_u32(BMC_RECOVERY_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_RECOVERY_MINOR_VER), alt_u32(BMC_RECOVERY_PFM_MINOR_VER));

    // Check number of T-1 transitions
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_bmc_only_counter(), alt_u32(1));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_pch_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(1));

    /*
     * Verify updated data
     */
    for (alt_u32 region_i = 0; region_i < BMC_NUM_STATIC_REGIONS; region_i++)
    {
        for (alt_u32 word_i = testdata_bmc_static_regions_start_addr[region_i] >> 2;
                word_i < testdata_bmc_static_regions_end_addr[region_i] >> 2; word_i++)
        {
            ASSERT_EQ(full_new_image[word_i], flash_x86_ptr[word_i]);
        }
    }

    /*
     * Clean ups
     */
    delete[] full_new_image;
}

/*
 * This test sends the BMC recovery update intent and runs pfr_main.
 * The recovery update flow is triggered.
 */
TEST_F(FWUpdateFlowTest, test_recovery_update_bmc)
{
    /*
     * Test Preparation
     */
    switch_spi_flash(SPI_FLASH_BMC);
    alt_u32* flash_x86_ptr = SYSTEM_MOCK::get()->get_x86_ptr_to_spi_flash(SPI_FLASH_BMC);

    // Load the update capsule
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, SIGNED_CAPSULE_BMC_V14_FILE,
            SIGNED_CAPSULE_BMC_V14_FILE_SIZE, get_ufm_pfr_data()->bmc_staging_region);

    alt_u32 *full_new_image = new alt_u32[FULL_PFR_IMAGE_BMC_V14_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(FULL_PFR_IMAGE_BMC_V14_FILE, full_new_image);

    alt_u32 *update_capsule = new alt_u32[SIGNED_CAPSULE_BMC_V14_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(SIGNED_CAPSULE_BMC_V14_FILE, update_capsule);

    /*
     * Flow preparation
     */
    ut_prep_nios_gpi_signals();
    ut_send_block_complete_chkpt_msg();

    // Exit after 50 iterations in the T0 loop
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS_END_AFTER_50_ITERS);
    // Send the recovery update intent
    ut_send_in_update_intent_once_upon_boot_complete(
            MB_BMC_UPDATE_INTENT, MB_UPDATE_INTENT_BMC_RECOVERY_MASK);

    /*
     * Execute the flow. Active update flow will be triggered.
     */
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(60, pfr_main());

    /*
     * Verify updated data
     */

    // Nios should transition from T0 to T-1 exactly twice for recovery update flow
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(2));

    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(0));

    // Check PCH FW version
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_ACTIVE_MAJOR_VER), alt_u32(PCH_ACTIVE_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_ACTIVE_MINOR_VER), alt_u32(PCH_ACTIVE_PFM_MINOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_RECOVERY_MAJOR_VER), alt_u32(PCH_RECOVERY_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_RECOVERY_MINOR_VER), alt_u32(PCH_RECOVERY_PFM_MINOR_VER));

    // Check BMC FW version
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_ACTIVE_MAJOR_VER), alt_u32(SIGNED_CAPSULE_BMC_V14_FILE_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_ACTIVE_MINOR_VER), alt_u32(SIGNED_CAPSULE_BMC_V14_FILE_PFM_MINOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_RECOVERY_MAJOR_VER), alt_u32(SIGNED_CAPSULE_BMC_V14_FILE_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_RECOVERY_MINOR_VER), alt_u32(SIGNED_CAPSULE_BMC_V14_FILE_PFM_MINOR_VER));

    // Check number of T-1 transitions (recovery update requires platform reset)
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_bmc_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_pch_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(3));

    // Both static and dynamic regions are updated
    for (alt_u32 region_i = 0; region_i < BMC_NUM_STATIC_REGIONS; region_i++)
    {
        for (alt_u32 word_i = testdata_bmc_static_regions_start_addr[region_i] >> 2;
                word_i < testdata_bmc_static_regions_end_addr[region_i] >> 2; word_i++)
        {
            ASSERT_EQ(full_new_image[word_i], flash_x86_ptr[word_i]);
        }
    }
    for (alt_u32 region_i = 0; region_i < BMC_NUM_DYNAMIC_REGIONS; region_i++)
    {
        for (alt_u32 word_i = testdata_bmc_dynamic_regions_start_addr[region_i] >> 2;
                word_i < testdata_bmc_dynamic_regions_end_addr[region_i] >> 2; word_i++)
        {
            ASSERT_EQ(full_new_image[word_i], flash_x86_ptr[word_i]);
        }
    }

    // Verify updated PFM
    alt_u32* capsule_pfm = incr_alt_u32_ptr(update_capsule, SIGNATURE_SIZE);
    alt_u32 capsule_pfm_size = get_signed_payload_size(capsule_pfm);
    alt_u32 start_i = get_ufm_pfr_data()->bmc_active_pfm  >> 2;
    alt_u32 end_i = (get_ufm_pfr_data()->bmc_active_pfm + capsule_pfm_size) >> 2;
    for (alt_u32 addr_i = 0; addr_i < (end_i - start_i); addr_i++)
    {
        ASSERT_EQ(capsule_pfm[addr_i], flash_x86_ptr[addr_i + start_i]);
    }

    // Verify updated data in recovery region
    alt_u32 capsule_size = get_signed_payload_size(update_capsule);
    start_i = get_recovery_region_offset(SPI_FLASH_BMC) >> 2;
    end_i = (get_recovery_region_offset(SPI_FLASH_BMC) + capsule_size) >> 2;
    for (alt_u32 addr_i = 0; addr_i < (end_i - start_i); addr_i++)
    {
        ASSERT_EQ(update_capsule[addr_i], flash_x86_ptr[addr_i + start_i]);
    }

    /*
     * Clean ups
     */
    delete[] full_new_image;
    delete[] update_capsule;
}

/*
 * This test sends the PCH recovery update intent and runs pfr_main.
 * The recovery update flow is triggered.
 */
TEST_F(FWUpdateFlowTest, test_recovery_update_pch)
{
    /*
     * Test Preparation
     */
    switch_spi_flash(SPI_FLASH_PCH);
    alt_u32* flash_x86_ptr = SYSTEM_MOCK::get()->get_x86_ptr_to_spi_flash(SPI_FLASH_PCH);

    // Load the update capsule
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_PCH, SIGNED_CAPSULE_PCH_V03P12_FILE,
            SIGNED_CAPSULE_PCH_V03P12_FILE_SIZE, get_ufm_pfr_data()->pch_staging_region);

    // Load the entire images locally for comparison purpose
    alt_u32 *full_new_image = new alt_u32[FULL_PFR_IMAGE_PCH_V03P12_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(FULL_PFR_IMAGE_PCH_V03P12_FILE, full_new_image);

    alt_u32 *update_capsule = new alt_u32[SIGNED_CAPSULE_PCH_V03P12_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(SIGNED_CAPSULE_PCH_V03P12_FILE, update_capsule);

    /*
     * Flow preparation
     */
    ut_prep_nios_gpi_signals();
    ut_send_block_complete_chkpt_msg();

    // Exit after 50 iterations in the T0 loop
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS_END_AFTER_50_ITERS);

    // Send the active update intent
    ut_send_in_update_intent_once_upon_boot_complete(MB_PCH_UPDATE_INTENT, MB_UPDATE_INTENT_PCH_RECOVERY_MASK);

    /*
     * Execute the flow. Active update flow will be triggered.
     */
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(70, pfr_main());

    /*
     * Verify results
     */
    // Nios should transition from T0 to T-1 exactly twice for recovery update flow
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(2));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), LAST_PANIC_PCH_UPDATE_INTENT);

    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(0));

    // Check PCH FW version
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_ACTIVE_MAJOR_VER), alt_u32(SIGNED_CAPSULE_PCH_V03P12_FILE_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_ACTIVE_MINOR_VER), alt_u32(SIGNED_CAPSULE_PCH_V03P12_FILE_PFM_MINOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_RECOVERY_MAJOR_VER), alt_u32(SIGNED_CAPSULE_PCH_V03P12_FILE_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_RECOVERY_MINOR_VER), alt_u32(SIGNED_CAPSULE_PCH_V03P12_FILE_PFM_MINOR_VER));

    // Check BMC FW version
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_ACTIVE_MAJOR_VER), alt_u32(BMC_ACTIVE_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_ACTIVE_MINOR_VER), alt_u32(BMC_ACTIVE_PFM_MINOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_RECOVERY_MAJOR_VER), alt_u32(BMC_RECOVERY_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_RECOVERY_MINOR_VER), alt_u32(BMC_RECOVERY_PFM_MINOR_VER));

    // Check number of T-1 transitions (recovery update requires platform reset)
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_bmc_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_pch_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(3));

    // Check for updated data in static regions
    for (alt_u32 region_i = 0; region_i < PCH_NUM_STATIC_REGIONS; region_i++)
    {
        for (alt_u32 word_i = testdata_pch_static_regions_start_addr[region_i] >> 2;
                word_i < testdata_pch_static_regions_end_addr[region_i] >> 2; word_i++)
        {
            ASSERT_EQ(full_new_image[word_i], flash_x86_ptr[word_i]);
        }
    }

    // Check that the dynamic regions are untouched.
    for (alt_u32 region_i = 0; region_i < PCH_NUM_DYNAMIC_REGIONS; region_i++)
    {
        for (alt_u32 word_i = testdata_pch_dynamic_regions_start_addr[region_i] >> 2;
                word_i < testdata_pch_dynamic_regions_end_addr[region_i] >> 2; word_i++)
        {
            ASSERT_EQ(full_new_image[word_i], flash_x86_ptr[word_i]);
        }
    }

    // Verify updated PFM
    alt_u32* capsule_pfm = incr_alt_u32_ptr(update_capsule, SIGNATURE_SIZE);
    alt_u32 capsule_pfm_size = get_signed_payload_size(capsule_pfm);
    alt_u32 start_i = get_ufm_pfr_data()->pch_active_pfm  >> 2;
    alt_u32 end_i = (get_ufm_pfr_data()->pch_active_pfm + capsule_pfm_size) >> 2;
    for (alt_u32 addr_i = 0; addr_i < (end_i - start_i); addr_i++)
    {
        ASSERT_EQ(capsule_pfm[addr_i], flash_x86_ptr[addr_i + start_i]);
    }

    // Verify updated data in recovery region
    alt_u32 capsule_size = get_signed_payload_size(update_capsule);
    start_i = get_recovery_region_offset(SPI_FLASH_PCH) >> 2;
    end_i = (get_recovery_region_offset(SPI_FLASH_PCH) + capsule_size) >> 2;
    for (alt_u32 addr_i = 0; addr_i < (end_i - start_i); addr_i++)
    {
        ASSERT_EQ(update_capsule[addr_i], flash_x86_ptr[addr_i + start_i]);
    }

    /*
     * Clean ups
     */
    delete[] full_new_image;
    delete[] update_capsule;
}

/*
 * Test out-of-bank PCH recovery update
 */
TEST_F(FWUpdateFlowTest, test_oob_pch_recovery_update)
{
    /*
     * Test Preparation
     */
    switch_spi_flash(SPI_FLASH_PCH);
    alt_u32* pch_flash_x86_ptr = SYSTEM_MOCK::get()->get_x86_ptr_to_spi_flash(SPI_FLASH_PCH);
    switch_spi_flash(SPI_FLASH_BMC);
    alt_u32* bmc_flash_x86_ptr = SYSTEM_MOCK::get()->get_x86_ptr_to_spi_flash(SPI_FLASH_BMC);

    // Load the PCH fw update capsule to BMC staging region in flash
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, SIGNED_CAPSULE_PCH_V03P12_FILE,
            SIGNED_CAPSULE_PCH_V03P12_FILE_SIZE, get_ufm_pfr_data()->bmc_staging_region + BMC_STAGING_REGION_PCH_UPDATE_CAPSULE_OFFSET);

    // Load the entire image locally for comparison purpose
    alt_u32 *full_new_pch_image = new alt_u32[FULL_PFR_IMAGE_PCH_V03P12_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(FULL_PFR_IMAGE_PCH_V03P12_FILE, full_new_pch_image);
    alt_u32 *full_bmc_image = new alt_u32[FULL_PFR_IMAGE_BMC_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(FULL_PFR_IMAGE_BMC_FILE, full_bmc_image);

    alt_u32 *update_capsule = new alt_u32[SIGNED_CAPSULE_PCH_V03P12_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(SIGNED_CAPSULE_PCH_V03P12_FILE, update_capsule);

    /*
     * Flow preparation
     */
    ut_prep_nios_gpi_signals();
    ut_send_block_complete_chkpt_msg();

    // Exit after 50 iterations in the T0 loop
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS_END_AFTER_50_ITERS);
    // Send the active update intent
    ut_send_in_update_intent_once_upon_boot_complete(MB_BMC_UPDATE_INTENT, MB_UPDATE_INTENT_PCH_RECOVERY_MASK);

    /*
     * Execute the flow. Active update flow will be triggered.
     */
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(80, pfr_main());

    /*
     * Verify updated data
     * Both static and dynamic regions are updated
     */
    // Nios should transition from T0 to T-1 exactly twice
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(2));

    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(0));

    // Check PCH FW version
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_ACTIVE_MAJOR_VER), alt_u32(SIGNED_CAPSULE_PCH_V03P12_FILE_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_ACTIVE_MINOR_VER), alt_u32(SIGNED_CAPSULE_PCH_V03P12_FILE_PFM_MINOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_RECOVERY_MAJOR_VER), alt_u32(SIGNED_CAPSULE_PCH_V03P12_FILE_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_RECOVERY_MINOR_VER), alt_u32(SIGNED_CAPSULE_PCH_V03P12_FILE_PFM_MINOR_VER));

    // Check BMC FW version
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_ACTIVE_MAJOR_VER), alt_u32(BMC_ACTIVE_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_ACTIVE_MINOR_VER), alt_u32(BMC_ACTIVE_PFM_MINOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_RECOVERY_MAJOR_VER), alt_u32(BMC_RECOVERY_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_RECOVERY_MINOR_VER), alt_u32(BMC_RECOVERY_PFM_MINOR_VER));

    // Check number of T-1 transitions (recovery update requires platform reset)
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_bmc_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_pch_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(3));

    // Verify the new PCH active firmware
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

    // Verify the new PCH recovery firmware
    alt_u32 capsule_size = get_signed_payload_size(update_capsule);
    alt_u32 start_i = get_recovery_region_offset(SPI_FLASH_PCH) >> 2;
    alt_u32 end_i = (get_recovery_region_offset(SPI_FLASH_PCH) + capsule_size) >> 2;
    for (alt_u32 addr_i = 0; addr_i < (end_i - start_i); addr_i++)
    {
        ASSERT_EQ(update_capsule[addr_i], pch_flash_x86_ptr[addr_i + start_i]);
    }

    // BMC SPI regions should be un-touched by this update
    for (alt_u32 region_i = 0; region_i < BMC_NUM_STATIC_REGIONS; region_i++)
    {
        for (alt_u32 word_i = testdata_bmc_static_regions_start_addr[region_i] >> 2;
                word_i < testdata_bmc_static_regions_end_addr[region_i] >> 2; word_i++)
        {
            ASSERT_EQ(full_bmc_image[word_i], bmc_flash_x86_ptr[word_i]);
        }
    }
    for (alt_u32 region_i = 0; region_i < BMC_NUM_DYNAMIC_REGIONS; region_i++)
    {
        for (alt_u32 word_i = testdata_bmc_dynamic_regions_start_addr[region_i] >> 2;
                word_i < testdata_bmc_dynamic_regions_end_addr[region_i] >> 2; word_i++)
        {
            ASSERT_EQ(full_bmc_image[word_i], bmc_flash_x86_ptr[word_i]);
        }
    }

    /*
     * Clean ups
     */
    delete[] full_new_pch_image;
    delete[] full_bmc_image;
    delete[] update_capsule;
}

/*
 * This test sends in BMC update intent that requests active update to both BMC and PCH flash.
 */
TEST_F(FWUpdateFlowTest, test_oob_bmc_and_pch_active_update)
{
    /*
     * Test Preparation
     */
    switch_spi_flash(SPI_FLASH_PCH);
    alt_u32* pch_flash_x86_ptr = SYSTEM_MOCK::get()->get_x86_ptr_to_spi_flash(SPI_FLASH_PCH);
    switch_spi_flash(SPI_FLASH_BMC);
    alt_u32* bmc_flash_x86_ptr = SYSTEM_MOCK::get()->get_x86_ptr_to_spi_flash(SPI_FLASH_BMC);

    // Load the BMC and PCH fw update capsule to BMC staging region in flash
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, SIGNED_CAPSULE_BMC_V14_FILE,
            SIGNED_CAPSULE_BMC_V14_FILE_SIZE, get_ufm_pfr_data()->bmc_staging_region);
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, SIGNED_CAPSULE_PCH_V03P12_FILE,
            SIGNED_CAPSULE_PCH_V03P12_FILE_SIZE, get_ufm_pfr_data()->bmc_staging_region + BMC_STAGING_REGION_PCH_UPDATE_CAPSULE_OFFSET);

    // Load the entire image locally for comparison purpose
    alt_u32 *full_pch_orig_image = new alt_u32[FULL_PFR_IMAGE_PCH_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(FULL_PFR_IMAGE_PCH_FILE, full_pch_orig_image);

    alt_u32 *full_pch_new_image = new alt_u32[FULL_PFR_IMAGE_PCH_V03P12_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(FULL_PFR_IMAGE_PCH_V03P12_FILE, full_pch_new_image);

    alt_u32 *full_bmc_orig_image = new alt_u32[FULL_PFR_IMAGE_BMC_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(FULL_PFR_IMAGE_BMC_FILE, full_bmc_orig_image);

    alt_u32 *full_bmc_new_image = new alt_u32[FULL_PFR_IMAGE_BMC_V14_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(FULL_PFR_IMAGE_BMC_V14_FILE, full_bmc_new_image);

    /*
     * Flow preparation
     */
    ut_prep_nios_gpi_signals();
    ut_send_block_complete_chkpt_msg();

    // Exit after 50 iterations in the T0 loop
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS_END_AFTER_50_ITERS);

    // Send the active update intent
    ut_send_in_update_intent_once_upon_boot_complete(
            MB_BMC_UPDATE_INTENT, MB_UPDATE_INTENT_PCH_ACTIVE_MASK | MB_UPDATE_INTENT_BMC_ACTIVE_MASK);

    /*
     * Execute the flow. Active update flow will be triggered.
     */
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(120, pfr_main());

    /*
     * Check results
     */
    // Nios should transition from T0 to T-1 exactly once
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(1));

    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(0));

    // Check PCH FW version
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_ACTIVE_MAJOR_VER), alt_u32(SIGNED_CAPSULE_PCH_V03P12_FILE_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_ACTIVE_MINOR_VER), alt_u32(SIGNED_CAPSULE_PCH_V03P12_FILE_PFM_MINOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_RECOVERY_MAJOR_VER), alt_u32(PCH_RECOVERY_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_RECOVERY_MINOR_VER), alt_u32(PCH_RECOVERY_PFM_MINOR_VER));

    // Check BMC FW version
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_ACTIVE_MAJOR_VER), alt_u32(SIGNED_CAPSULE_BMC_V14_FILE_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_ACTIVE_MINOR_VER), alt_u32(SIGNED_CAPSULE_BMC_V14_FILE_PFM_MINOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_RECOVERY_MAJOR_VER), alt_u32(BMC_RECOVERY_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_RECOVERY_MINOR_VER), alt_u32(BMC_RECOVERY_PFM_MINOR_VER));

    // Check number of T-1 transitions
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_bmc_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_pch_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(2));

    // Both static and dynamic regions are updated
    for (alt_u32 region_i = 0; region_i < PCH_NUM_STATIC_REGIONS; region_i++)
    {
        for (alt_u32 word_i = testdata_pch_static_regions_start_addr[region_i] >> 2;
                word_i < testdata_pch_static_regions_end_addr[region_i] >> 2; word_i++)
        {
            ASSERT_EQ(full_pch_new_image[word_i], pch_flash_x86_ptr[word_i]);
        }
    }
    // Check that the PCH dynamic regions are untouched.
    for (alt_u32 region_i = 0; region_i < PCH_NUM_DYNAMIC_REGIONS; region_i++)
    {
        for (alt_u32 word_i = testdata_pch_dynamic_regions_start_addr[region_i] >> 2;
                word_i < testdata_pch_dynamic_regions_end_addr[region_i] >> 2; word_i++)
        {
            ASSERT_EQ(full_pch_orig_image[word_i], pch_flash_x86_ptr[word_i]);
        }
    }

    for (alt_u32 region_i = 0; region_i < BMC_NUM_STATIC_REGIONS; region_i++)
    {
        for (alt_u32 word_i = testdata_bmc_static_regions_start_addr[region_i] >> 2;
                word_i < testdata_bmc_static_regions_end_addr[region_i] >> 2; word_i++)
        {
            ASSERT_EQ(full_bmc_new_image[word_i], bmc_flash_x86_ptr[word_i]);
        }
    }
    // Check that the dynamic regions are untouched.
    for (alt_u32 region_i = 0; region_i < BMC_NUM_DYNAMIC_REGIONS; region_i++)
    {
        for (alt_u32 word_i = testdata_bmc_dynamic_regions_start_addr[region_i] >> 2;
                word_i < testdata_bmc_dynamic_regions_end_addr[region_i] >> 2; word_i++)
        {
            ASSERT_EQ(full_bmc_orig_image[word_i], bmc_flash_x86_ptr[word_i]);
        }
    }

    /*
     * Clean ups
     */
    delete[] full_pch_orig_image;
    delete[] full_pch_new_image;
    delete[] full_bmc_orig_image;
    delete[] full_bmc_new_image;
}

/*
 * This test sends in BMC update intent that requests a BMC recovery update and PCH active update
 */
TEST_F(FWUpdateFlowTest, test_oob_bmc_recovery_update_and_pch_active_update)
{
    /*
     * Test Preparation
     */
    // Load the BMC and PCH fw update capsule to BMC staging region in flash
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, SIGNED_CAPSULE_BMC_V14_FILE,
            SIGNED_CAPSULE_BMC_V14_FILE_SIZE, get_ufm_pfr_data()->bmc_staging_region);
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, SIGNED_CAPSULE_PCH_V03P12_FILE,
            SIGNED_CAPSULE_PCH_V03P12_FILE_SIZE, get_ufm_pfr_data()->bmc_staging_region + BMC_STAGING_REGION_PCH_UPDATE_CAPSULE_OFFSET);

    switch_spi_flash(SPI_FLASH_PCH);
    alt_u32* pch_flash_x86_ptr = SYSTEM_MOCK::get()->get_x86_ptr_to_spi_flash(SPI_FLASH_PCH);
    switch_spi_flash(SPI_FLASH_BMC);
    alt_u32* bmc_flash_x86_ptr = SYSTEM_MOCK::get()->get_x86_ptr_to_spi_flash(SPI_FLASH_BMC);

    // Load the entire image locally for comparison purpose
    alt_u32 *full_pch_orig_image = new alt_u32[FULL_PFR_IMAGE_PCH_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(FULL_PFR_IMAGE_PCH_FILE, full_pch_orig_image);

    alt_u32 *full_pch_new_image = new alt_u32[FULL_PFR_IMAGE_PCH_V03P12_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(FULL_PFR_IMAGE_PCH_V03P12_FILE, full_pch_new_image);

    alt_u32 *full_bmc_orig_image = new alt_u32[FULL_PFR_IMAGE_BMC_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(FULL_PFR_IMAGE_BMC_FILE, full_bmc_orig_image);

    alt_u32 *full_bmc_new_image = new alt_u32[FULL_PFR_IMAGE_BMC_V14_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(FULL_PFR_IMAGE_BMC_V14_FILE, full_bmc_new_image);

    alt_u32 *bmc_update_capsule = new alt_u32[SIGNED_CAPSULE_BMC_V14_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(SIGNED_CAPSULE_BMC_V14_FILE, bmc_update_capsule);

    /*
     * Flow preparation
     */
    ut_prep_nios_gpi_signals();
    ut_send_block_complete_chkpt_msg();

    // Exit after 50 iterations in the T0 loop
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS_END_AFTER_50_ITERS);

    // Send the active update intent
    ut_send_in_update_intent_once_upon_boot_complete(
            MB_BMC_UPDATE_INTENT, MB_UPDATE_INTENT_PCH_ACTIVE_MASK | MB_UPDATE_INTENT_BMC_RECOVERY_MASK);

    /*
     * Execute the flow. Active update flow will be triggered.
     */
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(200, pfr_main());

    /*
     * Check results
     */
    // Nios should transition from T0 to T-1 exactly once
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(2));

    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(0));

    // Check PCH FW version
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_ACTIVE_MAJOR_VER), alt_u32(SIGNED_CAPSULE_PCH_V03P12_FILE_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_ACTIVE_MINOR_VER), alt_u32(SIGNED_CAPSULE_PCH_V03P12_FILE_PFM_MINOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_RECOVERY_MAJOR_VER), alt_u32(PCH_RECOVERY_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_RECOVERY_MINOR_VER), alt_u32(PCH_RECOVERY_PFM_MINOR_VER));

    // Check BMC FW version
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_ACTIVE_MAJOR_VER), alt_u32(SIGNED_CAPSULE_BMC_V14_FILE_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_ACTIVE_MINOR_VER), alt_u32(SIGNED_CAPSULE_BMC_V14_FILE_PFM_MINOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_RECOVERY_MAJOR_VER), alt_u32(SIGNED_CAPSULE_BMC_V14_FILE_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_RECOVERY_MINOR_VER), alt_u32(SIGNED_CAPSULE_BMC_V14_FILE_PFM_MINOR_VER));

    // Check number of T-1 transitions
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_bmc_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_pch_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(3));

    // Both static and dynamic regions are updated
    for (alt_u32 region_i = 0; region_i < PCH_NUM_STATIC_REGIONS; region_i++)
    {
        for (alt_u32 word_i = testdata_pch_static_regions_start_addr[region_i] >> 2;
                word_i < testdata_pch_static_regions_end_addr[region_i] >> 2; word_i++)
        {
            ASSERT_EQ(full_pch_new_image[word_i], pch_flash_x86_ptr[word_i]);
        }
    }
    // Check that the PCH dynamic regions are untouched.
    for (alt_u32 region_i = 0; region_i < PCH_NUM_DYNAMIC_REGIONS; region_i++)
    {
        for (alt_u32 word_i = testdata_pch_dynamic_regions_start_addr[region_i] >> 2;
                word_i < testdata_pch_dynamic_regions_end_addr[region_i] >> 2; word_i++)
        {
            ASSERT_EQ(full_pch_orig_image[word_i], pch_flash_x86_ptr[word_i]);
        }
    }

    for (alt_u32 region_i = 0; region_i < BMC_NUM_STATIC_REGIONS; region_i++)
    {
        for (alt_u32 word_i = testdata_bmc_static_regions_start_addr[region_i] >> 2;
                word_i < testdata_bmc_static_regions_end_addr[region_i] >> 2; word_i++)
        {
            ASSERT_EQ(full_bmc_new_image[word_i], bmc_flash_x86_ptr[word_i]);
        }
    }
    // Check that the dynamic regions are untouched.
    for (alt_u32 region_i = 0; region_i < BMC_NUM_DYNAMIC_REGIONS; region_i++)
    {
        for (alt_u32 word_i = testdata_bmc_dynamic_regions_start_addr[region_i] >> 2;
                word_i < testdata_bmc_dynamic_regions_end_addr[region_i] >> 2; word_i++)
        {
            ASSERT_EQ(full_bmc_new_image[word_i], bmc_flash_x86_ptr[word_i]);
        }
    }

    // Verify updated PFM in BMC flash
    alt_u32* capsule_pfm = incr_alt_u32_ptr(bmc_update_capsule, SIGNATURE_SIZE);
    alt_u32 capsule_pfm_size = get_signed_payload_size(capsule_pfm);
    alt_u32 start_i = get_ufm_pfr_data()->bmc_active_pfm  >> 2;
    alt_u32 end_i = (get_ufm_pfr_data()->bmc_active_pfm + capsule_pfm_size) >> 2;
    for (alt_u32 addr_i = 0; addr_i < (end_i - start_i); addr_i++)
    {
        ASSERT_EQ(capsule_pfm[addr_i], bmc_flash_x86_ptr[addr_i + start_i]);
    }

    // Verify updated data in BMC recovery region
    alt_u32 capsule_size = get_signed_payload_size(bmc_update_capsule);
    start_i = get_recovery_region_offset(SPI_FLASH_BMC) >> 2;
    end_i = (get_recovery_region_offset(SPI_FLASH_BMC) + capsule_size) >> 2;
    for (alt_u32 addr_i = 0; addr_i < (end_i - start_i); addr_i++)
    {
        ASSERT_EQ(bmc_update_capsule[addr_i], bmc_flash_x86_ptr[addr_i + start_i]);
    }

    /*
     * Clean ups
     */
    delete[] full_pch_orig_image;
    delete[] full_pch_new_image;
    delete[] full_bmc_orig_image;
    delete[] full_bmc_new_image;
    delete[] bmc_update_capsule;
}

