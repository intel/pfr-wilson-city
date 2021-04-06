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


class RecoveryFlowTest : public testing::Test
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
 * @brief Corrupt static portion of the active firmware on PCH flash. This should trigger the FW recovery
 * mechanism in T-1 authentication step.
 */
TEST_F(RecoveryFlowTest, test_pch_fw_recovery_on_corrupted_static_regions_in_active_firmware)
{
    /*
     * Preparation
     */
    switch_spi_flash(SPI_FLASH_PCH);

    // Load the entire image locally for comparison purpose
    alt_u32 *full_image = new alt_u32[FULL_PFR_IMAGE_PCH_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(FULL_PFR_IMAGE_PCH_FILE, full_image);

    // Corrupt the first page in the first static region
    erase_spi_region(testdata_pch_static_regions_start_addr[0], PBC_EXPECTED_PAGE_SIZE);

    // flow preparation
    ut_prep_nios_gpi_signals();
    ut_send_block_complete_chkpt_msg();

    // Exit upon entry to T0
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS_END_AFTER_50_ITERS);

    /*
     * Run PFR Main. Always run with the timeout
     */
    ASSERT_DURATION_LE(40, pfr_main());

    /*
     * Verify recovered data
     */
    switch_spi_flash(SPI_FLASH_PCH);
    alt_u32* flash_x86_ptr = SYSTEM_MOCK::get()->get_x86_ptr_to_spi_flash(SPI_FLASH_PCH);

    // Check some mailbox output
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(1));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(LAST_RECOVERY_PCH_ACTIVE));
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(MAJOR_ERROR_PCH_AUTH_FAILED));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_AUTH_ACTIVE));

    // We should only enter T-1 once (because we ran Nios from beginning)
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(1));

    // Verify static regions
    for (alt_u32 region_i = 0; region_i < PCH_NUM_STATIC_REGIONS; region_i++)
    {
        for (alt_u32 word_i = testdata_pch_static_regions_start_addr[region_i] >> 2;
                word_i < testdata_pch_static_regions_end_addr[region_i] >> 2; word_i++)
        {
            ASSERT_EQ(full_image[word_i], flash_x86_ptr[word_i]);
        }
    }

    // Get PFM offsets
    alt_u32 pch_active_pfm_start = get_ufm_pfr_data()->pch_active_pfm;
    alt_u32 pch_active_pfm_end = pch_active_pfm_start + get_signed_payload_size(get_spi_active_pfm_ptr(SPI_FLASH_PCH));

    // Verify PFM
    EXPECT_TRUE(is_active_region_valid(get_spi_active_pfm_ptr(SPI_FLASH_PCH)));
    for (alt_u32 word_i = pch_active_pfm_start >> 2; word_i < pch_active_pfm_end >> 2; word_i++)
    {
        ASSERT_EQ(full_image[word_i], flash_x86_ptr[word_i]);
    }

    /*
     * Clean ups
     */
    delete[] full_image;
}

/**
 * @brief Corrupt static portion of the active firmware on BMC flash. This should trigger the FW recovery
 * mechanism in T-1 authentication step.
 */
TEST_F(RecoveryFlowTest, test_bmc_fw_recovery_on_corrupted_static_regions_in_active_firmware)
{
    /*
     * Preparation
     */
    switch_spi_flash(SPI_FLASH_BMC);

    // Load the entire image locally for comparison purpose
    alt_u32 *full_image = new alt_u32[FULL_PFR_IMAGE_BMC_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(FULL_PFR_IMAGE_BMC_FILE, full_image);

    // Corrupt the first page in the first static region
    erase_spi_region(testdata_bmc_static_regions_start_addr[0], PBC_EXPECTED_PAGE_SIZE);

    // flow preparation
    ut_prep_nios_gpi_signals();
    ut_send_block_complete_chkpt_msg();

    // Exit upon entry to T0
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS_END_AFTER_50_ITERS);

    /*
     * Run PFR Main. Always run with the timeout
     */
    ASSERT_DURATION_LE(60, pfr_main());

    /*
     * Verify recovered data
     */
    switch_spi_flash(SPI_FLASH_BMC);
    alt_u32* flash_x86_ptr = SYSTEM_MOCK::get()->get_x86_ptr_to_spi_flash(SPI_FLASH_BMC);

    // Check some mailbox output
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(1));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(LAST_RECOVERY_BMC_ACTIVE));
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(MAJOR_ERROR_BMC_AUTH_FAILED));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_AUTH_ACTIVE));

    // We should only enter T-1 once (because we ran Nios from beginning)
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(1));

    // Verify static regions
    for (alt_u32 region_i = 0; region_i < BMC_NUM_STATIC_REGIONS; region_i++)
    {
        for (alt_u32 word_i = testdata_bmc_static_regions_start_addr[region_i] >> 2;
                word_i < testdata_bmc_static_regions_end_addr[region_i] >> 2; word_i++)
        {
            ASSERT_EQ(full_image[word_i], flash_x86_ptr[word_i]);
        }
    }

    // Get PFM offsets
    alt_u32 bmc_active_pfm_start = get_ufm_pfr_data()->bmc_active_pfm;
    alt_u32 bmc_active_pfm_end = bmc_active_pfm_start + get_signed_payload_size(get_spi_active_pfm_ptr(SPI_FLASH_BMC));

    // Verify PFM
    EXPECT_TRUE(is_active_region_valid(get_spi_active_pfm_ptr(SPI_FLASH_BMC)));
    for (alt_u32 word_i = bmc_active_pfm_start >> 2; word_i < bmc_active_pfm_end >> 2; word_i++)
    {
        ASSERT_EQ(full_image[word_i], flash_x86_ptr[word_i]);
    }

    /*
     * Clean ups
     */
    delete[] full_image;
}

/*
 *
 */
TEST_F(RecoveryFlowTest, test_static_recovery_on_corrupted_pfm_bmc)
{
    /*
     * Preparation
     */
    switch_spi_flash(SPI_FLASH_BMC);
    // Load the entire image to flash
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, FULL_PFR_IMAGE_BMC_FILE, FULL_PFR_IMAGE_BMC_FILE_SIZE);

    // Load the entire image locally for comparison purpose
    alt_u32 *full_image = new alt_u32[FULL_PFR_IMAGE_BMC_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(FULL_PFR_IMAGE_BMC_FILE, full_image);

    /*
     * Corrupt the signed active PFM
     */
    alt_u32* pfm_header = (alt_u32*) get_active_pfm(SPI_FLASH_BMC);
    *pfm_header = 0xdeadbeef;

    pfm_header = (pfm_header - 1);
    *pfm_header = 0xdeadbeef;

    /*
     * Perform the recovery
     */
    alt_u32* recovery_region_ptr = get_spi_recovery_region_ptr(SPI_FLASH_BMC);
    decompress_capsule(recovery_region_ptr, SPI_FLASH_BMC, DECOMPRESSION_STATIC_REGIONS_MASK);

    /*
     * Verify recovered data
     */
    alt_u32* signed_capsule_pfm = incr_alt_u32_ptr(get_spi_recovery_region_ptr(SPI_FLASH_BMC), SIGNATURE_SIZE);
    alt_u32* signed_active_pfm = get_spi_active_pfm_ptr(SPI_FLASH_BMC);
    alt_u32 nbytes = SIGNATURE_SIZE + ((KCH_BLOCK0*) signed_capsule_pfm)->pc_length;

    for (alt_u32 word_i = 0; word_i < nbytes / 4; word_i++)
    {
        alt_u32 expected_word = full_image[(get_ufm_pfr_data()->bmc_active_pfm/4) + word_i];
        ASSERT_EQ(expected_word, signed_active_pfm[word_i]);
    }

    /*
     * Clean ups
     */
    delete[] full_image;
}

TEST_F(RecoveryFlowTest, test_wdt_recovery_on_bmc)
{
    /*
     * Preparation
     */
    switch_spi_flash(SPI_FLASH_BMC);
    alt_u32* flash_x86_ptr = SYSTEM_MOCK::get()->get_x86_ptr_to_spi_flash(SPI_FLASH_BMC);

    // Load the entire image to flash
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, FULL_PFR_IMAGE_BMC_FILE, FULL_PFR_IMAGE_BMC_FILE_SIZE);

    // Load the entire image locally for comparison purpose
    alt_u32 *full_image = new alt_u32[FULL_PFR_IMAGE_BMC_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(FULL_PFR_IMAGE_BMC_FILE, full_image);

    /*
     * Erase read-only SPI regions and verify the erase
     */
    for (alt_u32 region_i = 0; region_i < BMC_NUM_STATIC_REGIONS; region_i++)
    {
        alt_u32 start_addr = testdata_bmc_static_regions_start_addr[region_i];
        alt_u32 end_addr = testdata_bmc_static_regions_end_addr[region_i] - start_addr;
        erase_spi_region(start_addr, end_addr);
    }
    for (alt_u32 region_i = 0; region_i < BMC_NUM_DYNAMIC_REGIONS; region_i++)
    {
        alt_u32 start_addr = testdata_bmc_dynamic_regions_start_addr[region_i];
        alt_u32 end_addr = testdata_bmc_dynamic_regions_end_addr[region_i] - start_addr;
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
    for (alt_u32 region_i = 0; region_i < BMC_NUM_DYNAMIC_REGIONS; region_i++)
    {
        for (alt_u32 word_i = testdata_bmc_dynamic_regions_start_addr[region_i] >> 2;
                word_i < testdata_bmc_dynamic_regions_end_addr[region_i] >> 2; word_i++)
        {
            ASSERT_EQ(alt_u32(0xffffffff), flash_x86_ptr[word_i]);
        }
    }

    /*
     * Perform the recovery
     */
    set_spi_flash_state(SPI_FLASH_BMC, SPI_FLASH_STATE_REQUIRE_WDT_RECOVERY_MASK);
    perform_wdt_recovery(SPI_FLASH_BMC);

    /*
     * Verify recovered data
     */
    for (alt_u32 region_i = 0; region_i < BMC_NUM_STATIC_REGIONS; region_i++)
    {
        for (alt_u32 word_i = testdata_bmc_static_regions_start_addr[region_i] >> 2;
                word_i < testdata_bmc_static_regions_end_addr[region_i] >> 2; word_i++)
        {
            ASSERT_EQ(full_image[word_i], flash_x86_ptr[word_i]);
        }
    }
    for (alt_u32 region_i = 0; region_i < BMC_NUM_DYNAMIC_REGIONS; region_i++)
    {
        for (alt_u32 word_i = testdata_bmc_dynamic_regions_start_addr[region_i] >> 2;
                word_i < testdata_bmc_dynamic_regions_end_addr[region_i] >> 2; word_i++)
        {
            ASSERT_EQ(full_image[word_i], flash_x86_ptr[word_i]);
        }
    }

    /*
     * Clean ups
     */
    delete[] full_image;
}

/**
 * @brief Simulate an attack where attacker can sneak in a staging capsule and attempt to exploit
 * Nios firmware recovery mechanism to promote staging capsule to recovery area.
 *
 * In this test, the staged image is different to the active image. Nios should not perform the recovery,
 * even if this staged image is authentic.
 */
TEST_F(RecoveryFlowTest, test_bmc_fw_recovery_on_corrupted_recovery_firmware_with_unexpected_staged_image)
{
    /*
     * Preparation
     */
    switch_spi_flash(SPI_FLASH_BMC);

    // Load the entire image locally for comparison purpose
    alt_u32 *full_image = new alt_u32[SIGNED_CAPSULE_BMC_V14_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(SIGNED_CAPSULE_BMC_V14_FILE, full_image);

    // Load the update capsule to staging area
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, SIGNED_CAPSULE_BMC_V14_FILE,
            SIGNED_CAPSULE_BMC_V14_FILE_SIZE, get_ufm_pfr_data()->bmc_staging_region);

    // Erase some page in the recovery firmware
    erase_spi_region(get_recovery_region_offset(SPI_FLASH_BMC) + PBC_EXPECTED_PAGE_SIZE, PBC_EXPECTED_PAGE_SIZE);
    // Verify that the recovery region has been corrupted
    EXPECT_FALSE(is_capsule_valid(get_spi_recovery_region_ptr(SPI_FLASH_BMC)));

    // flow preparation
    ut_prep_nios_gpi_signals();
    ut_send_block_complete_chkpt_msg();

    // Exit upon entry to T0
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS_END_AFTER_50_ITERS);

    /*
     * Run PFR Main. Always run with the timeout
     */
    ASSERT_DURATION_LE(60, pfr_main());

    /*
     * Verify recovered data
     */
    switch_spi_flash(SPI_FLASH_BMC);

    // Check some mailbox output
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(MAJOR_ERROR_BMC_AUTH_FAILED));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_AUTH_RECOVERY));

    // We should only enter T-1 once (because we ran Nios from beginning)
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(1));

    // Active firmware should be untouched
    EXPECT_TRUE(is_active_region_valid(get_spi_active_pfm_ptr(SPI_FLASH_BMC)));

    // Expect recovery image to still be corrupted
    EXPECT_FALSE(is_capsule_valid(get_spi_recovery_region_ptr(SPI_FLASH_BMC)));

    /*
     * Clean ups
     */
    delete[] full_image;
}

/**
 * @brief Simulate the recovery flow which happens after a power failure during update to recovery region.
 *
 * Recovery region is corrupted during the update. Now, after an AC cycle, Nios should detect that recovery
 * region is corrupted and attempt to restore it with the staging capsule.
 *
 * In this case, the staged image should be identical to the active image, because we are in the middle of
 * recovery update.
 */
TEST_F(RecoveryFlowTest, test_bmc_fw_recovery_on_corrupted_recovery_firmware_with_expected_staged_image)
{
    /*
     * Preparation
     */
    switch_spi_flash(SPI_FLASH_BMC);

    // Load the entire image locally for comparison purpose
    alt_u32 *full_image = new alt_u32[SIGNED_CAPSULE_BMC_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(SIGNED_CAPSULE_BMC_FILE, full_image);

    // Load the update capsule to staging area
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, SIGNED_CAPSULE_BMC_FILE,
            SIGNED_CAPSULE_BMC_FILE_SIZE, get_ufm_pfr_data()->bmc_staging_region);

    // Erase some page in the recovery firmware
    erase_spi_region(get_recovery_region_offset(SPI_FLASH_BMC) + PBC_EXPECTED_PAGE_SIZE, PBC_EXPECTED_PAGE_SIZE);
    // Verify that the recovery region has been corrupted
    EXPECT_FALSE(is_capsule_valid(get_spi_recovery_region_ptr(SPI_FLASH_BMC)));

    // flow preparation
    ut_prep_nios_gpi_signals();
    ut_send_block_complete_chkpt_msg();

    // Exit upon entry to T0
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS_END_AFTER_50_ITERS);

    /*
     * Run PFR Main. Always run with the timeout
     */
    ASSERT_DURATION_LE(60, pfr_main());

    /*
     * Verify recovered data
     */
    switch_spi_flash(SPI_FLASH_BMC);

    // Check some mailbox output
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(1));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(LAST_RECOVERY_BMC_RECOVERY));
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(MAJOR_ERROR_BMC_AUTH_FAILED));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_AUTH_RECOVERY));

    // We should only enter T-1 once (because we ran Nios from beginning)
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(1));

    // Active firmware should be untouched
    EXPECT_TRUE(is_active_region_valid(get_spi_active_pfm_ptr(SPI_FLASH_BMC)));

    // Expect recovery image to still be corrupted
    EXPECT_TRUE(is_capsule_valid(get_spi_recovery_region_ptr(SPI_FLASH_BMC)));

    /*
     * Clean ups
     */
    delete[] full_image;
}
