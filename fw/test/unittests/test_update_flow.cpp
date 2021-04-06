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


class UpdateFlowTest : public testing::Test
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

/*
 * This test sends a BMC update intent that requests CPLD, PCH, BMC active update at once
 */
TEST_F(UpdateFlowTest, test_oob_bmc_pch_cpld_active_update_at_once)
{
    /*
     * Test Preparation
     */
    switch_spi_flash(SPI_FLASH_PCH);
    alt_u32* pch_flash_x86_ptr = SYSTEM_MOCK::get()->get_x86_ptr_to_spi_flash(SPI_FLASH_PCH);
    switch_spi_flash(SPI_FLASH_BMC);
    alt_u32* bmc_flash_x86_ptr = SYSTEM_MOCK::get()->get_x86_ptr_to_spi_flash(SPI_FLASH_BMC);

    // Load the CPLD, BMC and PCH update capsule to BMC staging region in flash
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, SIGNED_CAPSULE_BMC_V14_FILE,
            SIGNED_CAPSULE_BMC_V14_FILE_SIZE, get_ufm_pfr_data()->bmc_staging_region);
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, SIGNED_CAPSULE_PCH_V03P12_FILE,
            SIGNED_CAPSULE_PCH_V03P12_FILE_SIZE, get_ufm_pfr_data()->bmc_staging_region + BMC_STAGING_REGION_PCH_UPDATE_CAPSULE_OFFSET);
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, SIGNED_CAPSULE_CPLD_FILE,
            SIGNED_CAPSULE_CPLD_FILE_SIZE, get_ufm_pfr_data()->bmc_staging_region + BMC_STAGING_REGION_CPLD_UPDATE_CAPSULE_OFFSET);

    // Load the entire image locally for comparison purpose
    alt_u32 *full_pch_orig_image = new alt_u32[FULL_PFR_IMAGE_PCH_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(FULL_PFR_IMAGE_PCH_FILE, full_pch_orig_image);

    alt_u32 *full_pch_new_image = new alt_u32[FULL_PFR_IMAGE_PCH_V03P12_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(FULL_PFR_IMAGE_PCH_V03P12_FILE, full_pch_new_image);

    alt_u32 *full_bmc_orig_image = new alt_u32[FULL_PFR_IMAGE_BMC_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(FULL_PFR_IMAGE_BMC_FILE, full_bmc_orig_image);

    alt_u32 *full_bmc_new_image = new alt_u32[FULL_PFR_IMAGE_BMC_V14_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(FULL_PFR_IMAGE_BMC_V14_FILE, full_bmc_new_image);

    alt_u32* expected_cpld_active_image = new alt_u32[CFM1_ACTIVE_IMAGE_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(CFM1_ACTIVE_IMAGE_FILE, expected_cpld_active_image);

    // CPLD Active image should be empty at this point
    alt_u32* cfm1_ufm_ptr = get_ufm_ptr_with_offset(UFM_CPLD_ACTIVE_IMAGE_OFFSET);
    for (alt_u32 i = 0; i < (UFM_CPLD_ACTIVE_IMAGE_LENGTH/4); i++)
    {
        ASSERT_EQ(cfm1_ufm_ptr[i], 0xFFFFFFFF);
    }
    // CPLD recovery image area should be empty in BMC flash by default
    alt_u32* cpld_recovery_capsule_ptr = get_spi_flash_ptr_with_offset(BMC_CPLD_RECOVERY_IMAGE_OFFSET);
    for (int i = 0; i < (SIGNED_CAPSULE_CPLD_FILE_SIZE/4); i++)
    {
        ASSERT_EQ(cpld_recovery_capsule_ptr[i], 0xFFFFFFFF);
    }

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
     * Run Nios FW through PFR/Recovery Main.
     * Active firmware updates should be done first, then CPLD update is executed.
     */
    ut_run_main(CPLD_CFM0, true);

    // Send the active update intent
    ut_send_in_update_intent_once_upon_boot_complete(
            MB_BMC_UPDATE_INTENT, MB_UPDATE_INTENT_PCH_ACTIVE_MASK | MB_UPDATE_INTENT_BMC_ACTIVE_MASK | MB_UPDATE_INTENT_CPLD_ACTIVE_MASK);
    ut_run_main(CPLD_CFM1, true);

    ut_run_main(CPLD_CFM0, true);
    ut_run_main(CPLD_CFM1, false);

    /*
     * Verify updated data
     * Both static and dynamic regions are updated
     */
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

    // CPLD Active image should be updated already
    for (alt_u32 i = 0; i < (UFM_CPLD_ACTIVE_IMAGE_LENGTH/4); i++)
    {
        ASSERT_EQ(cfm1_ufm_ptr[i], expected_cpld_active_image[i]);
    }
    // CPLD Recovery image should not be changed (i.e. still empty)
    for (int i = 0; i < (SIGNED_CAPSULE_CPLD_FILE_SIZE/4); i++)
    {
        ASSERT_EQ(cpld_recovery_capsule_ptr[i], 0xFFFFFFFF);
    }

    // Expect no error
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

    /*
     * Clean ups
     */
    delete[] full_pch_orig_image;
    delete[] full_pch_new_image;
    delete[] full_bmc_orig_image;
    delete[] full_bmc_new_image;
    delete[] expected_cpld_active_image;
}

/*
 * This test sends a BMC update intent that requests CPLD active update, BMC active update and
 * PCH recovery update at once.
 */
TEST_F(UpdateFlowTest, test_oob_bmc_active_pch_recovery_cpld_active_update_at_once)
{
    /*
     * Test Preparation
     */
    switch_spi_flash(SPI_FLASH_PCH);
    alt_u32* pch_flash_x86_ptr = SYSTEM_MOCK::get()->get_x86_ptr_to_spi_flash(SPI_FLASH_PCH);
    switch_spi_flash(SPI_FLASH_BMC);
    alt_u32* bmc_flash_x86_ptr = SYSTEM_MOCK::get()->get_x86_ptr_to_spi_flash(SPI_FLASH_BMC);

    // Load the CPLD, BMC and PCH update capsule to BMC staging region in flash
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, SIGNED_CAPSULE_BMC_V14_FILE,
            SIGNED_CAPSULE_BMC_V14_FILE_SIZE, get_ufm_pfr_data()->bmc_staging_region);
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, SIGNED_CAPSULE_PCH_V03P12_FILE,
            SIGNED_CAPSULE_PCH_V03P12_FILE_SIZE, get_ufm_pfr_data()->bmc_staging_region + BMC_STAGING_REGION_PCH_UPDATE_CAPSULE_OFFSET);
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, SIGNED_CAPSULE_CPLD_FILE,
            SIGNED_CAPSULE_CPLD_FILE_SIZE, get_ufm_pfr_data()->bmc_staging_region + BMC_STAGING_REGION_CPLD_UPDATE_CAPSULE_OFFSET);

    // Load the entire image locally for comparison purpose
    alt_u32 *full_pch_orig_image = new alt_u32[FULL_PFR_IMAGE_PCH_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(FULL_PFR_IMAGE_PCH_FILE, full_pch_orig_image);

    alt_u32 *full_pch_new_image = new alt_u32[FULL_PFR_IMAGE_PCH_V03P12_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(FULL_PFR_IMAGE_PCH_V03P12_FILE, full_pch_new_image);

    alt_u32 *pch_update_capsule = new alt_u32[SIGNED_CAPSULE_PCH_V03P12_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(SIGNED_CAPSULE_PCH_V03P12_FILE, pch_update_capsule);

    alt_u32 *full_bmc_orig_image = new alt_u32[FULL_PFR_IMAGE_BMC_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(FULL_PFR_IMAGE_BMC_FILE, full_bmc_orig_image);

    alt_u32 *full_bmc_new_image = new alt_u32[FULL_PFR_IMAGE_BMC_V14_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(FULL_PFR_IMAGE_BMC_V14_FILE, full_bmc_new_image);

    alt_u32* expected_cpld_active_image = new alt_u32[CFM1_ACTIVE_IMAGE_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(CFM1_ACTIVE_IMAGE_FILE, expected_cpld_active_image);

    // CPLD Active image should be empty at this point
    alt_u32* cfm1_ufm_ptr = get_ufm_ptr_with_offset(UFM_CPLD_ACTIVE_IMAGE_OFFSET);
    for (alt_u32 i = 0; i < (UFM_CPLD_ACTIVE_IMAGE_LENGTH/4); i++)
    {
        ASSERT_EQ(cfm1_ufm_ptr[i], 0xFFFFFFFF);
    }
    // CPLD recovery image area should be empty in BMC flash by default
    alt_u32* cpld_recovery_capsule_ptr = get_spi_flash_ptr_with_offset(BMC_CPLD_RECOVERY_IMAGE_OFFSET);
    for (int i = 0; i < (SIGNED_CAPSULE_CPLD_FILE_SIZE/4); i++)
    {
        ASSERT_EQ(cpld_recovery_capsule_ptr[i], 0xFFFFFFFF);
    }

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
     * Run Nios FW through PFR/Recovery Main.
     * Firmware updates should be done first, then CPLD update is executed.
     */
    ut_run_main(CPLD_CFM0, true);

    // Send the active update intent
    ut_send_in_update_intent_once_upon_boot_complete(
            MB_BMC_UPDATE_INTENT, MB_UPDATE_INTENT_PCH_RECOVERY_MASK | MB_UPDATE_INTENT_BMC_ACTIVE_MASK | MB_UPDATE_INTENT_CPLD_ACTIVE_MASK);
    ut_run_main(CPLD_CFM1, true);

    ut_run_main(CPLD_CFM0, true);
    ut_run_main(CPLD_CFM1, false);

    /*
     * Verify updated data
     * Both static and dynamic regions are updated
     */
    // PCH static regions should be updated
    for (alt_u32 region_i = 0; region_i < PCH_NUM_STATIC_REGIONS; region_i++)
    {
        for (alt_u32 word_i = testdata_pch_static_regions_start_addr[region_i] >> 2;
                word_i < testdata_pch_static_regions_end_addr[region_i] >> 2; word_i++)
        {
            ASSERT_EQ(full_pch_new_image[word_i], pch_flash_x86_ptr[word_i]);
        }
    }
    // PCH dynamic regions should be updated
    for (alt_u32 region_i = 0; region_i < PCH_NUM_DYNAMIC_REGIONS; region_i++)
    {
        for (alt_u32 word_i = testdata_pch_dynamic_regions_start_addr[region_i] >> 2;
                word_i < testdata_pch_dynamic_regions_end_addr[region_i] >> 2; word_i++)
        {
            ASSERT_EQ(full_pch_new_image[word_i], pch_flash_x86_ptr[word_i]);
        }
    }
    // PCH recovery region should have been updated
    alt_u32 pch_capsule_size = get_signed_payload_size(pch_update_capsule);
    alt_u32 start_i = get_recovery_region_offset(SPI_FLASH_PCH) >> 2;
    alt_u32 end_i = (get_recovery_region_offset(SPI_FLASH_PCH) + pch_capsule_size) >> 2;
    for (alt_u32 addr_i = 0; addr_i < (end_i - start_i); addr_i++)
    {
        ASSERT_EQ(pch_update_capsule[addr_i], pch_flash_x86_ptr[addr_i + start_i]);
    }
    // BMC static regions should be updated
    for (alt_u32 region_i = 0; region_i < BMC_NUM_STATIC_REGIONS; region_i++)
    {
        for (alt_u32 word_i = testdata_bmc_static_regions_start_addr[region_i] >> 2;
                word_i < testdata_bmc_static_regions_end_addr[region_i] >> 2; word_i++)
        {
            ASSERT_EQ(full_bmc_new_image[word_i], bmc_flash_x86_ptr[word_i]);
        }
    }
    // BMC dynamic regions should not be modified.
    for (alt_u32 region_i = 0; region_i < BMC_NUM_DYNAMIC_REGIONS; region_i++)
    {
        for (alt_u32 word_i = testdata_bmc_dynamic_regions_start_addr[region_i] >> 2;
                word_i < testdata_bmc_dynamic_regions_end_addr[region_i] >> 2; word_i++)
        {
            ASSERT_EQ(full_bmc_orig_image[word_i], bmc_flash_x86_ptr[word_i]);
        }
    }

    // CPLD Active image should be updated already
    for (alt_u32 i = 0; i < (UFM_CPLD_ACTIVE_IMAGE_LENGTH/4); i++)
    {
        ASSERT_EQ(cfm1_ufm_ptr[i], expected_cpld_active_image[i]);
    }
    // CPLD Recovery image should not be changed (i.e. still empty)
    for (int i = 0; i < (SIGNED_CAPSULE_CPLD_FILE_SIZE/4); i++)
    {
        ASSERT_EQ(cpld_recovery_capsule_ptr[i], 0xFFFFFFFF);
    }

    // Expect no error
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(0));

    // Check PCH FW version
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_ACTIVE_MAJOR_VER), alt_u32(SIGNED_CAPSULE_PCH_V03P12_FILE_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_ACTIVE_MINOR_VER), alt_u32(SIGNED_CAPSULE_PCH_V03P12_FILE_PFM_MINOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_RECOVERY_MAJOR_VER), alt_u32(SIGNED_CAPSULE_PCH_V03P12_FILE_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_RECOVERY_MINOR_VER), alt_u32(SIGNED_CAPSULE_PCH_V03P12_FILE_PFM_MINOR_VER));

    // Check BMC FW version
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_ACTIVE_MAJOR_VER), alt_u32(SIGNED_CAPSULE_BMC_V14_FILE_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_ACTIVE_MINOR_VER), alt_u32(SIGNED_CAPSULE_BMC_V14_FILE_PFM_MINOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_RECOVERY_MAJOR_VER), alt_u32(BMC_RECOVERY_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_RECOVERY_MINOR_VER), alt_u32(BMC_RECOVERY_PFM_MINOR_VER));

    /*
     * Clean ups
     */
    delete[] full_pch_orig_image;
    delete[] full_pch_new_image;
    delete[] pch_update_capsule;
    delete[] full_bmc_orig_image;
    delete[] full_bmc_new_image;
    delete[] expected_cpld_active_image;
}


/*
 * This test sends a BMC update intent that requests CPLD, BMC, PCH recovery updates at once.
 */
TEST_F(UpdateFlowTest, test_oob_bmc_pch_cpld_recovery_update_at_once)
{
    /*
     * Test Preparation
     */
    switch_spi_flash(SPI_FLASH_PCH);
    alt_u32* pch_flash_x86_ptr = SYSTEM_MOCK::get()->get_x86_ptr_to_spi_flash(SPI_FLASH_PCH);
    switch_spi_flash(SPI_FLASH_BMC);
    alt_u32* bmc_flash_x86_ptr = SYSTEM_MOCK::get()->get_x86_ptr_to_spi_flash(SPI_FLASH_BMC);

    // Load the CPLD, BMC and PCH update capsule to BMC staging region in flash
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, SIGNED_CAPSULE_BMC_V14_FILE,
            SIGNED_CAPSULE_BMC_V14_FILE_SIZE, get_ufm_pfr_data()->bmc_staging_region);
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, SIGNED_CAPSULE_PCH_V03P12_FILE,
            SIGNED_CAPSULE_PCH_V03P12_FILE_SIZE, get_ufm_pfr_data()->bmc_staging_region + BMC_STAGING_REGION_PCH_UPDATE_CAPSULE_OFFSET);
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, SIGNED_CAPSULE_CPLD_B393_6_FILE,
            SIGNED_CAPSULE_CPLD_B393_6_FILE_SIZE, get_ufm_pfr_data()->bmc_staging_region + BMC_STAGING_REGION_CPLD_UPDATE_CAPSULE_OFFSET);

    // Load the entire image locally for comparison purpose
    alt_u32 *full_pch_orig_image = new alt_u32[FULL_PFR_IMAGE_PCH_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(FULL_PFR_IMAGE_PCH_FILE, full_pch_orig_image);

    alt_u32 *full_pch_new_image = new alt_u32[FULL_PFR_IMAGE_PCH_V03P12_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(FULL_PFR_IMAGE_PCH_V03P12_FILE, full_pch_new_image);

    alt_u32 *pch_update_capsule = new alt_u32[SIGNED_CAPSULE_PCH_V03P12_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(SIGNED_CAPSULE_PCH_V03P12_FILE, pch_update_capsule);

    alt_u32 *full_bmc_orig_image = new alt_u32[FULL_PFR_IMAGE_BMC_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(FULL_PFR_IMAGE_BMC_FILE, full_bmc_orig_image);

    alt_u32 *full_bmc_new_image = new alt_u32[FULL_PFR_IMAGE_BMC_V14_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(FULL_PFR_IMAGE_BMC_V14_FILE, full_bmc_new_image);

    alt_u32 *bmc_update_capsule = new alt_u32[SIGNED_CAPSULE_BMC_V14_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(SIGNED_CAPSULE_BMC_V14_FILE, bmc_update_capsule);

    alt_u32* cpld_update_capsule = new alt_u32[SIGNED_CAPSULE_CPLD_B393_6_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(SIGNED_CAPSULE_CPLD_B393_6_FILE, cpld_update_capsule);

    CPLD_UPDATE_PC* cpld_update_capsule_pc = (CPLD_UPDATE_PC*) incr_alt_u32_ptr(cpld_update_capsule, SIGNATURE_SIZE);
    alt_u32* expected_cpld_active_image = cpld_update_capsule_pc->cpld_bitstream;

    // CPLD Active image should be empty at this point
    alt_u32* cfm1_ufm_ptr = get_ufm_ptr_with_offset(UFM_CPLD_ACTIVE_IMAGE_OFFSET);
    for (alt_u32 i = 0; i < (UFM_CPLD_ACTIVE_IMAGE_LENGTH/4); i++)
    {
        ASSERT_EQ(cfm1_ufm_ptr[i], 0xFFFFFFFF);
    }
    // CPLD recovery image area should be empty in BMC flash by default
    alt_u32* cpld_recovery_capsule_ptr = get_spi_flash_ptr_with_offset(BMC_CPLD_RECOVERY_IMAGE_OFFSET);
    for (int i = 0; i < (SIGNED_CAPSULE_CPLD_FILE_SIZE/4); i++)
    {
        ASSERT_EQ(cpld_recovery_capsule_ptr[i], 0xFFFFFFFF);
    }

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
     * Run Nios FW through PFR/Recovery Main.
     * Firmware updates should be done first, then CPLD update is executed.
     */
    ut_run_main(CPLD_CFM0, true);

    // Send the recovery fw and CPLD update intent
    ut_send_in_update_intent_once_upon_boot_complete(
            MB_BMC_UPDATE_INTENT,
            MB_UPDATE_INTENT_PCH_RECOVERY_MASK | MB_UPDATE_INTENT_BMC_RECOVERY_MASK | MB_UPDATE_INTENT_CPLD_RECOVERY_MASK);
    ut_run_main(CPLD_CFM1, true);

    ut_run_main(CPLD_CFM0, true);
    ut_run_main(CPLD_CFM1, false);

    /*
     * Verify updated data
     * Both static and dynamic regions are updated
     */
    // PCH static regions should be updated
    for (alt_u32 region_i = 0; region_i < PCH_NUM_STATIC_REGIONS; region_i++)
    {
        for (alt_u32 word_i = testdata_pch_static_regions_start_addr[region_i] >> 2;
                word_i < testdata_pch_static_regions_end_addr[region_i] >> 2; word_i++)
        {
            ASSERT_EQ(full_pch_new_image[word_i], pch_flash_x86_ptr[word_i]);
        }
    }
    // PCH dynamic regions should be updated
    for (alt_u32 region_i = 0; region_i < PCH_NUM_DYNAMIC_REGIONS; region_i++)
    {
        for (alt_u32 word_i = testdata_pch_dynamic_regions_start_addr[region_i] >> 2;
                word_i < testdata_pch_dynamic_regions_end_addr[region_i] >> 2; word_i++)
        {
            ASSERT_EQ(full_pch_new_image[word_i], pch_flash_x86_ptr[word_i]);
        }
    }
    // PCH recovery region should have been updated
    alt_u32 pch_capsule_size = get_signed_payload_size(pch_update_capsule);
    alt_u32 pch_start_i = get_recovery_region_offset(SPI_FLASH_PCH) >> 2;
    alt_u32 pch_end_i = (get_recovery_region_offset(SPI_FLASH_PCH) + pch_capsule_size) >> 2;
    for (alt_u32 addr_i = 0; addr_i < (pch_end_i - pch_start_i); addr_i++)
    {
        ASSERT_EQ(pch_update_capsule[addr_i], pch_flash_x86_ptr[addr_i + pch_start_i]);
    }
    // BMC static regions should be updated
    for (alt_u32 region_i = 0; region_i < BMC_NUM_STATIC_REGIONS; region_i++)
    {
        for (alt_u32 word_i = testdata_bmc_static_regions_start_addr[region_i] >> 2;
                word_i < testdata_bmc_static_regions_end_addr[region_i] >> 2; word_i++)
        {
            ASSERT_EQ(full_bmc_new_image[word_i], bmc_flash_x86_ptr[word_i]);
        }
    }
    // BMC dynamic regions should not be modified.
    for (alt_u32 region_i = 0; region_i < BMC_NUM_DYNAMIC_REGIONS; region_i++)
    {
        for (alt_u32 word_i = testdata_bmc_dynamic_regions_start_addr[region_i] >> 2;
                word_i < testdata_bmc_dynamic_regions_end_addr[region_i] >> 2; word_i++)
        {
            ASSERT_EQ(full_bmc_new_image[word_i], bmc_flash_x86_ptr[word_i]);
        }
    }
    // PCH recovery region should have been updated
    alt_u32 bmc_capsule_size = get_signed_payload_size(bmc_update_capsule);
    alt_u32 bmc_start_i = get_recovery_region_offset(SPI_FLASH_BMC) >> 2;
    alt_u32 bmc_end_i = (get_recovery_region_offset(SPI_FLASH_BMC) + bmc_capsule_size) >> 2;
    for (alt_u32 addr_i = 0; addr_i < (bmc_end_i - bmc_start_i); addr_i++)
    {
        ASSERT_EQ(bmc_update_capsule[addr_i], bmc_flash_x86_ptr[addr_i + bmc_start_i]);
    }

    // CPLD Active image should be updated already
    for (alt_u32 i = 0; i < (UFM_CPLD_ACTIVE_IMAGE_LENGTH/4); i++)
    {
        ASSERT_EQ(cfm1_ufm_ptr[i], expected_cpld_active_image[i]);
    }
    // CPLD Recovery image should be updated
    for (int i = 0; i < (SIGNED_CAPSULE_CPLD_FILE_SIZE/4); i++)
    {
        ASSERT_EQ(cpld_recovery_capsule_ptr[i], cpld_update_capsule[i]);
    }

    // Expect no error
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(0));

    // Check PCH FW version
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_ACTIVE_MAJOR_VER), alt_u32(SIGNED_CAPSULE_PCH_V03P12_FILE_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_ACTIVE_MINOR_VER), alt_u32(SIGNED_CAPSULE_PCH_V03P12_FILE_PFM_MINOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_RECOVERY_MAJOR_VER), alt_u32(SIGNED_CAPSULE_PCH_V03P12_FILE_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_RECOVERY_MINOR_VER), alt_u32(SIGNED_CAPSULE_PCH_V03P12_FILE_PFM_MINOR_VER));

    // Check BMC FW version
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_ACTIVE_MAJOR_VER), alt_u32(SIGNED_CAPSULE_BMC_V14_FILE_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_ACTIVE_MINOR_VER), alt_u32(SIGNED_CAPSULE_BMC_V14_FILE_PFM_MINOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_RECOVERY_MAJOR_VER), alt_u32(SIGNED_CAPSULE_BMC_V14_FILE_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_RECOVERY_MINOR_VER), alt_u32(SIGNED_CAPSULE_BMC_V14_FILE_PFM_MINOR_VER));

    /*
     * Clean ups
     */
    delete[] full_pch_orig_image;
    delete[] full_pch_new_image;
    delete[] pch_update_capsule;
    delete[] full_bmc_orig_image;
    delete[] full_bmc_new_image;
    delete[] bmc_update_capsule;
    delete[] cpld_update_capsule;
}
