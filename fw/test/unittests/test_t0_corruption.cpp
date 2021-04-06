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
 * @brief This test suite cover the scenarios where BMC/PCH flash gets random byte corruption in T0
 * and user triggers a warm reset from either BMC or PCH side.
 *
 * Please refer to High-level Architecture Spec (Recovery Matrix) for expected behaviors.
 */
class T0CorruptionTest : public testing::Test
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
 * @brief This test case corrupts the PCH flash (active and recovery regions) during T0.
 * In the PCH flash, the staging region is initialized to be empty.
 */
TEST_F(T0CorruptionTest, test_corrupt_pch_active_and_recovery_in_t0_with_empty_staging)
{
    /*
     * Flow preparation
     */
    ut_prep_nios_gpi_signals();
    ut_send_block_complete_chkpt_msg();

    // Exit after 50 iterations in the T0 loop
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS_END_AFTER_50_ITERS);

    /*
     * Execute the flow.
     */
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(40, pfr_main());

    /*
     * Verify that Nios is currently in T0
     */
    EXPECT_EQ(read_from_mailbox(MB_PLATFORM_STATE), alt_u32(PLATFORM_STATE_T0_BOOT_COMPLETE));
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), alt_u32(0));
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
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_ACTIVE_MAJOR_VER), alt_u32(BMC_ACTIVE_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_ACTIVE_MINOR_VER), alt_u32(BMC_ACTIVE_PFM_MINOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_RECOVERY_MAJOR_VER), alt_u32(BMC_RECOVERY_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_RECOVERY_MINOR_VER), alt_u32(BMC_RECOVERY_PFM_MINOR_VER));

    /*
     * Corrupt PCH flash
     */
    switch_spi_flash(SPI_FLASH_PCH);
    alt_u32* signed_recovery_capsule = get_spi_recovery_region_ptr(SPI_FLASH_PCH);
    alt_u32* signed_active_pfm = get_spi_active_pfm_ptr(SPI_FLASH_PCH);

    // Corrupt recovery region
    signed_recovery_capsule[3] = 0xDEADBEEF;

    // Corrupt active PFM
    signed_active_pfm[10] = 0xDEADBEEF;

    /*
     * Assume there was a warm reset from BIOS during T0 and CPU hangs.
     * This should trigger a WDT recovery.
     * Make sure no recovery was performed and PCH/CPU remains in reset.
     */
    trigger_pch_wdt_recovery(LAST_RECOVERY_ACM_LAUNCH_FAIL, LAST_PANIC_ACM_BIOS_WDT_EXPIRED);

    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(1));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), alt_u32(LAST_PANIC_ACM_BIOS_WDT_EXPIRED));
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(1));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(LAST_RECOVERY_ACM_LAUNCH_FAIL));
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(MAJOR_ERROR_PCH_AUTH_FAILED));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_AUTH_ALL_REGIONS));

    // BMC should be out of reset
    EXPECT_TRUE(ut_is_bmc_out_of_reset());

    // PCH should still be in reset
    EXPECT_FALSE(ut_is_pch_out_of_reset());

    // Both BMC and PCH timers should be inactive
    EXPECT_FALSE(check_bit(WDT_BMC_TIMER_ADDR, U_TIMER_BANK_TIMER_ACTIVE_BIT));
    EXPECT_FALSE(check_bit(WDT_ACM_BIOS_TIMER_ADDR, U_TIMER_BANK_TIMER_ACTIVE_BIT));
}

/**
 * @brief This test case corrupts the PCH flash (active and recovery regions) during T0.
 * In the PCH flash, the staging region is initialized to have a good image.
 *
 * In the active region, this test case corrupts the PFM content.
 */
TEST_F(T0CorruptionTest, test_corrupt_pch_active_and_recovery_in_t0_with_good_image_in_staging_case1)
{
    /*
     * Test Preparation
     */
    alt_u32* flash_x86_ptr = SYSTEM_MOCK::get()->get_x86_ptr_to_spi_flash(SPI_FLASH_PCH);

    // Load an update capsule to Staging region
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_PCH, SIGNED_CAPSULE_PCH_V03P12_FILE,
            SIGNED_CAPSULE_PCH_V03P12_FILE_SIZE, get_ufm_pfr_data()->pch_staging_region);

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

    /*
     * Execute the flow.
     */
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(40, pfr_main());

    /*
     * Verify that Nios is currently in T0
     */
    EXPECT_EQ(read_from_mailbox(MB_PLATFORM_STATE), alt_u32(PLATFORM_STATE_T0_BOOT_COMPLETE));
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), alt_u32(0));
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
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_ACTIVE_MAJOR_VER), alt_u32(BMC_ACTIVE_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_ACTIVE_MINOR_VER), alt_u32(BMC_ACTIVE_PFM_MINOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_RECOVERY_MAJOR_VER), alt_u32(BMC_RECOVERY_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_RECOVERY_MINOR_VER), alt_u32(BMC_RECOVERY_PFM_MINOR_VER));

    /*
     * Corrupt PCH flash
     */
    switch_spi_flash(SPI_FLASH_PCH);
    alt_u32* signed_recovery_capsule = get_spi_recovery_region_ptr(SPI_FLASH_PCH);
    alt_u32* signed_active_pfm = get_spi_active_pfm_ptr(SPI_FLASH_PCH);

    // Corrupt recovery region
    signed_recovery_capsule[3] = 0xDEADBEEF;

    // Corrupt active PFM
    signed_active_pfm[10] = 0xDEADBEEF;

    /*
     * Assume there was a warm reset from BIOS during T0 and CPU hangs.
     * This should trigger a WDT recovery.
     * Make sure Nios recovered the active and recovery region and allowed platform to boot.
     */
    trigger_pch_wdt_recovery(LAST_RECOVERY_ACM_LAUNCH_FAIL, LAST_PANIC_ACM_BIOS_WDT_EXPIRED);

    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(1));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), alt_u32(LAST_PANIC_ACM_BIOS_WDT_EXPIRED));
    // Three recovery events: ACM WDT recovery / Recovery region recovery / Active region recovery
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(3));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(LAST_RECOVERY_PCH_ACTIVE));
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(MAJOR_ERROR_PCH_AUTH_FAILED));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_AUTH_ACTIVE_AND_RECOVERY));

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

    // Both BMC and PCH should be out of reset
    EXPECT_TRUE(ut_is_bmc_out_of_reset());
    EXPECT_TRUE(ut_is_pch_out_of_reset());

    // Both BMC and PCH (ME) timers should be active
    EXPECT_TRUE(check_bit(WDT_BMC_TIMER_ADDR, U_TIMER_BANK_TIMER_ACTIVE_BIT));
    EXPECT_TRUE(check_bit(WDT_ME_TIMER_ADDR, U_TIMER_BANK_TIMER_ACTIVE_BIT));
    // ACM timer is inactive because there was no PLTRST event
    EXPECT_FALSE(check_bit(WDT_ACM_BIOS_TIMER_ADDR, U_TIMER_BANK_TIMER_ACTIVE_BIT));

    /*
     * In PCH flash, the static fw, dynamic fw, PFM and Recovery regions should be recovered.
     */
    // Static firmware regions
    for (alt_u32 region_i = 0; region_i < PCH_NUM_STATIC_REGIONS; region_i++)
    {
        for (alt_u32 word_i = testdata_pch_static_regions_start_addr[region_i] >> 2;
                word_i < testdata_pch_static_regions_end_addr[region_i] >> 2; word_i++)
        {
            ASSERT_EQ(full_new_image[word_i], flash_x86_ptr[word_i]);
        }
    }
    // Dynamic firmware regions
    for (alt_u32 region_i = 0; region_i < PCH_NUM_DYNAMIC_REGIONS; region_i++)
    {
        for (alt_u32 word_i = testdata_pch_dynamic_regions_start_addr[region_i] >> 2;
                word_i < testdata_pch_dynamic_regions_end_addr[region_i] >> 2; word_i++)
        {
            ASSERT_EQ(full_new_image[word_i], flash_x86_ptr[word_i]);
        }
    }
    // PFM
    alt_u32 pch_active_pfm_start = get_ufm_pfr_data()->pch_active_pfm;
    switch_spi_flash(SPI_FLASH_PCH);
    alt_u32 pch_active_pfm_end = pch_active_pfm_start + get_signed_payload_size(get_spi_active_pfm_ptr(SPI_FLASH_PCH));
    for (alt_u32 word_i = pch_active_pfm_start >> 2; word_i < pch_active_pfm_end >> 2; word_i++)
    {
        ASSERT_EQ(full_new_image[word_i], flash_x86_ptr[word_i]);
    }
    // Recovery region
    alt_u32 capsule_size = get_signed_payload_size(update_capsule);
    alt_u32 start_i = get_recovery_region_offset(SPI_FLASH_PCH) >> 2;
    alt_u32 end_i = (get_recovery_region_offset(SPI_FLASH_PCH) + capsule_size) >> 2;
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


/**
 * @brief This test case corrupts the PCH flash (active and recovery regions) during T0.
 * In the PCH flash, the staging region is initialized to have a good image.
 *
 * In the active region, this test case corrupts a piece of firmware.
 */
TEST_F(T0CorruptionTest, test_corrupt_pch_active_and_recovery_in_t0_with_good_image_in_staging_case2)
{
    /*
     * Test Preparation
     */
    alt_u32* flash_x86_ptr = SYSTEM_MOCK::get()->get_x86_ptr_to_spi_flash(SPI_FLASH_PCH);

    // Load an update capsule to Staging region
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_PCH, SIGNED_CAPSULE_PCH_V03P12_FILE,
            SIGNED_CAPSULE_PCH_V03P12_FILE_SIZE, get_ufm_pfr_data()->pch_staging_region);

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

    /*
     * Execute the flow.
     */
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(40, pfr_main());

    /*
     * Verify that Nios is currently in T0
     */
    EXPECT_EQ(read_from_mailbox(MB_PLATFORM_STATE), alt_u32(PLATFORM_STATE_T0_BOOT_COMPLETE));
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), alt_u32(0));
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
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_ACTIVE_MAJOR_VER), alt_u32(BMC_ACTIVE_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_ACTIVE_MINOR_VER), alt_u32(BMC_ACTIVE_PFM_MINOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_RECOVERY_MAJOR_VER), alt_u32(BMC_RECOVERY_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_RECOVERY_MINOR_VER), alt_u32(BMC_RECOVERY_PFM_MINOR_VER));

    /*
     * Corrupt PCH flash
     */
    switch_spi_flash(SPI_FLASH_PCH);
    alt_u32* signed_recovery_capsule = get_spi_recovery_region_ptr(SPI_FLASH_PCH);

    // Corrupt recovery region
    signed_recovery_capsule[3] = 0xDEADBEEF;

    // Corrupt active firmware
    alt_u32* spi_ptr = get_spi_flash_ptr_with_offset(testdata_pch_static_regions_start_addr[1]);
    spi_ptr[0] = 0xDEADBEEF;
    spi_ptr[1] = 0x12345678;
    spi_ptr[2] = 0xDECAF000;

    /*
     * Assume there was a warm reset from BIOS during T0 and CPU hangs.
     * This should trigger a WDT recovery.
     * Make sure Nios recovered the active and recovery region and allowed platform to boot.
     */
    trigger_pch_wdt_recovery(LAST_RECOVERY_ACM_LAUNCH_FAIL, LAST_PANIC_ACM_BIOS_WDT_EXPIRED);

    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(1));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), alt_u32(LAST_PANIC_ACM_BIOS_WDT_EXPIRED));
    // Three recovery events: ACM WDT recovery / Recovery region recovery / Active region recovery
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(3));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(LAST_RECOVERY_PCH_ACTIVE));
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(MAJOR_ERROR_PCH_AUTH_FAILED));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_AUTH_ACTIVE_AND_RECOVERY));

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

    // Both BMC and PCH should be out of reset
    EXPECT_TRUE(ut_is_bmc_out_of_reset());
    EXPECT_TRUE(ut_is_pch_out_of_reset());

    // Both BMC and PCH (ME) timers should be active
    EXPECT_TRUE(check_bit(WDT_BMC_TIMER_ADDR, U_TIMER_BANK_TIMER_ACTIVE_BIT));
    EXPECT_TRUE(check_bit(WDT_ME_TIMER_ADDR, U_TIMER_BANK_TIMER_ACTIVE_BIT));
    // ACM timer is inactive because there was no PLTRST event
    EXPECT_FALSE(check_bit(WDT_ACM_BIOS_TIMER_ADDR, U_TIMER_BANK_TIMER_ACTIVE_BIT));

    /*
     * In PCH flash, the static fw, dynamic fw, PFM and Recovery regions should be recovered.
     */
    // Static firmware regions
    for (alt_u32 region_i = 0; region_i < PCH_NUM_STATIC_REGIONS; region_i++)
    {
        for (alt_u32 word_i = testdata_pch_static_regions_start_addr[region_i] >> 2;
                word_i < testdata_pch_static_regions_end_addr[region_i] >> 2; word_i++)
        {
            ASSERT_EQ(full_new_image[word_i], flash_x86_ptr[word_i]);
        }
    }
    // Dynamic firmware regions
    for (alt_u32 region_i = 0; region_i < PCH_NUM_DYNAMIC_REGIONS; region_i++)
    {
        for (alt_u32 word_i = testdata_pch_dynamic_regions_start_addr[region_i] >> 2;
                word_i < testdata_pch_dynamic_regions_end_addr[region_i] >> 2; word_i++)
        {
            ASSERT_EQ(full_new_image[word_i], flash_x86_ptr[word_i]);
        }
    }
    // PFM
    alt_u32 pch_active_pfm_start = get_ufm_pfr_data()->pch_active_pfm;
    switch_spi_flash(SPI_FLASH_PCH);
    alt_u32 pch_active_pfm_end = pch_active_pfm_start + get_signed_payload_size(get_spi_active_pfm_ptr(SPI_FLASH_PCH));
    for (alt_u32 word_i = pch_active_pfm_start >> 2; word_i < pch_active_pfm_end >> 2; word_i++)
    {
        ASSERT_EQ(full_new_image[word_i], flash_x86_ptr[word_i]);
    }
    // Recovery region
    alt_u32 capsule_size = get_signed_payload_size(update_capsule);
    alt_u32 start_i = get_recovery_region_offset(SPI_FLASH_PCH) >> 2;
    alt_u32 end_i = (get_recovery_region_offset(SPI_FLASH_PCH) + capsule_size) >> 2;
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

/**
 * @brief This test case corrupts the BMC flash (active and recovery regions) during T0.
 * In the BMC flash, the staging region is initialized to be empty.
 */
TEST_F(T0CorruptionTest, test_corrupt_bmc_active_and_recovery_in_t0_with_empty_staging)
{
    /*
     * Flow preparation
     */
    ut_prep_nios_gpi_signals();
    ut_send_block_complete_chkpt_msg();

    // Exit after 50 iterations in the T0 loop
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS_END_AFTER_50_ITERS);

    /*
     * Execute the flow.
     */
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(40, pfr_main());

    /*
     * Verify that Nios is currently in T0
     */
    EXPECT_EQ(read_from_mailbox(MB_PLATFORM_STATE), alt_u32(PLATFORM_STATE_T0_BOOT_COMPLETE));
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), alt_u32(0));
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
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_ACTIVE_MAJOR_VER), alt_u32(BMC_ACTIVE_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_ACTIVE_MINOR_VER), alt_u32(BMC_ACTIVE_PFM_MINOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_RECOVERY_MAJOR_VER), alt_u32(BMC_RECOVERY_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_RECOVERY_MINOR_VER), alt_u32(BMC_RECOVERY_PFM_MINOR_VER));

    /*
     * Corrupt BMC flash
     */
    switch_spi_flash(SPI_FLASH_BMC);
    alt_u32* signed_recovery_capsule = get_spi_recovery_region_ptr(SPI_FLASH_BMC);
    alt_u32* signed_active_pfm = get_spi_active_pfm_ptr(SPI_FLASH_BMC);

    // Corrupt recovery region
    signed_recovery_capsule[3] = 0xDEADBEEF;

    // Corrupt active PFM
    signed_active_pfm[1] = 0xDEADBEEF;

    /*
     * Simulate a BMC reset
     * Make sure no recovery was performed and BMC remains in reset.
     */
    ut_send_bmc_reset_detected_gpi_once_upon_boot_complete();
    bmc_reset_handler();

    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(1));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), alt_u32(LAST_PANIC_BMC_RESET_DETECTED));
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(MAJOR_ERROR_BMC_AUTH_FAILED));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_AUTH_ALL_REGIONS));

    // BMC should be in reset (EXTRST)
    EXPECT_FALSE(check_bit(U_GPO_1_ADDR, GPO_1_RST_PFR_EXTRST_N));

    // PCH should be out of reset
    EXPECT_TRUE(ut_is_pch_out_of_reset());

    // Both BMC and PCH timers should be inactive
    EXPECT_FALSE(check_bit(WDT_BMC_TIMER_ADDR, U_TIMER_BANK_TIMER_ACTIVE_BIT));
    EXPECT_FALSE(check_bit(WDT_ACM_BIOS_TIMER_ADDR, U_TIMER_BANK_TIMER_ACTIVE_BIT));
    EXPECT_FALSE(check_bit(WDT_ME_TIMER_ADDR, U_TIMER_BANK_TIMER_ACTIVE_BIT));
}

/**
 * @brief This test case corrupts the BMC flash (active and recovery regions) during T0.
 * In the BMC flash, the staging region is initialized to have a good image.
 *
 * In the active region, this test case corrupts the PFM content.
 */
TEST_F(T0CorruptionTest, test_corrupt_bmc_active_and_recovery_in_t0_with_good_image_in_staging_case1)
{
    /*
     * Test Preparation
     */
    alt_u32* flash_x86_ptr = SYSTEM_MOCK::get()->get_x86_ptr_to_spi_flash(SPI_FLASH_BMC);

    // Load an update capsule to Staging region
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

    /*
     * Execute the flow.
     */
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(40, pfr_main());

    /*
     * Verify that Nios is currently in T0
     */
    EXPECT_EQ(read_from_mailbox(MB_PLATFORM_STATE), alt_u32(PLATFORM_STATE_T0_BOOT_COMPLETE));
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), alt_u32(0));
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
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_ACTIVE_MAJOR_VER), alt_u32(BMC_ACTIVE_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_ACTIVE_MINOR_VER), alt_u32(BMC_ACTIVE_PFM_MINOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_RECOVERY_MAJOR_VER), alt_u32(BMC_RECOVERY_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_RECOVERY_MINOR_VER), alt_u32(BMC_RECOVERY_PFM_MINOR_VER));

    /*
     * Corrupt BMC flash
     */
    switch_spi_flash(SPI_FLASH_BMC);
    alt_u32* signed_recovery_capsule = get_spi_recovery_region_ptr(SPI_FLASH_BMC);
    alt_u32* signed_active_pfm = get_spi_active_pfm_ptr(SPI_FLASH_BMC);

    // Corrupt recovery region
    signed_recovery_capsule[1000] = 0xDEADBEEF;

    // Corrupt active PFM
    signed_active_pfm[4] = 0xDEADBEEF;

    /*
     * Simulate a BMC reset
     * Make sure Nios recovered the active and recovery region and allowed platform to boot.
     */
    ut_send_bmc_reset_detected_gpi_once_upon_boot_complete();
    bmc_reset_handler();

    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(1));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), alt_u32(LAST_PANIC_BMC_RESET_DETECTED));
    // Three recovery events: Recovery region recovery / Active region recovery
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(2));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(LAST_RECOVERY_BMC_ACTIVE));
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(MAJOR_ERROR_BMC_AUTH_FAILED));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_AUTH_ACTIVE_AND_RECOVERY));

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

    // Both BMC and PCH should be out of reset
    EXPECT_TRUE(check_bit(U_GPO_1_ADDR, GPO_1_RST_PFR_EXTRST_N));
    EXPECT_TRUE(ut_is_pch_out_of_reset());

    // BMC timer should be active
    EXPECT_TRUE(check_bit(WDT_BMC_TIMER_ADDR, U_TIMER_BANK_TIMER_ACTIVE_BIT));
    // PCH timers should already be turned off since this is a BMC only reset
    EXPECT_FALSE(check_bit(WDT_ME_TIMER_ADDR, U_TIMER_BANK_TIMER_ACTIVE_BIT));
    EXPECT_FALSE(check_bit(WDT_ACM_BIOS_TIMER_ADDR, U_TIMER_BANK_TIMER_ACTIVE_BIT));

    /*
     * In BMC flash, the static fw, dynamic fw, PFM and Recovery regions should be recovered.
     */
    // Static firmware regions
    for (alt_u32 region_i = 0; region_i < BMC_NUM_STATIC_REGIONS; region_i++)
    {
        for (alt_u32 word_i = testdata_bmc_static_regions_start_addr[region_i] >> 2;
                word_i < testdata_bmc_static_regions_end_addr[region_i] >> 2; word_i++)
        {
            ASSERT_EQ(full_new_image[word_i], flash_x86_ptr[word_i]);
        }
    }
    // Dynamic firmware regions
    for (alt_u32 region_i = 0; region_i < BMC_NUM_DYNAMIC_REGIONS; region_i++)
    {
        for (alt_u32 word_i = testdata_bmc_dynamic_regions_start_addr[region_i] >> 2;
                word_i < testdata_bmc_dynamic_regions_end_addr[region_i] >> 2; word_i++)
        {
            ASSERT_EQ(full_new_image[word_i], flash_x86_ptr[word_i]);
        }
    }
    // PFM
    alt_u32 bmc_active_pfm_start = get_ufm_pfr_data()->bmc_active_pfm;
    switch_spi_flash(SPI_FLASH_BMC);
    alt_u32 bmc_active_pfm_end = bmc_active_pfm_start + get_signed_payload_size(get_spi_active_pfm_ptr(SPI_FLASH_BMC));
    for (alt_u32 word_i = bmc_active_pfm_start >> 2; word_i < bmc_active_pfm_end >> 2; word_i++)
    {
        ASSERT_EQ(full_new_image[word_i], flash_x86_ptr[word_i]);
    }
    // Recovery region
    alt_u32 capsule_size = get_signed_payload_size(update_capsule);
    alt_u32 start_i = get_recovery_region_offset(SPI_FLASH_BMC) >> 2;
    alt_u32 end_i = (get_recovery_region_offset(SPI_FLASH_BMC) + capsule_size) >> 2;
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

/**
 * @brief This test case corrupts the BMC flash (active and recovery regions) during T0.
 * In the BMC flash, the staging region is initialized to have a good image.
 *
 * In the active region, this test case corrupts a piece of firmware.
 */
TEST_F(T0CorruptionTest, test_corrupt_bmc_active_and_recovery_in_t0_with_good_image_in_staging_case2)
{
    /*
     * Test Preparation
     */
    alt_u32* flash_x86_ptr = SYSTEM_MOCK::get()->get_x86_ptr_to_spi_flash(SPI_FLASH_BMC);

    // Load an update capsule to Staging region
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

    /*
     * Execute the flow.
     */
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(40, pfr_main());

    /*
     * Verify that Nios is currently in T0
     */
    EXPECT_EQ(read_from_mailbox(MB_PLATFORM_STATE), alt_u32(PLATFORM_STATE_T0_BOOT_COMPLETE));
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), alt_u32(0));
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
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_ACTIVE_MAJOR_VER), alt_u32(BMC_ACTIVE_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_ACTIVE_MINOR_VER), alt_u32(BMC_ACTIVE_PFM_MINOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_RECOVERY_MAJOR_VER), alt_u32(BMC_RECOVERY_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_RECOVERY_MINOR_VER), alt_u32(BMC_RECOVERY_PFM_MINOR_VER));

    /*
     * Corrupt BMC flash
     */
    switch_spi_flash(SPI_FLASH_BMC);

    // Corrupt recovery region
    alt_u32* signed_recovery_capsule = get_spi_recovery_region_ptr(SPI_FLASH_BMC);
    signed_recovery_capsule[99] = 0xDEADBEEF;

    // Corrupt active PFM
    alt_u32* spi_ptr = get_spi_flash_ptr_with_offset(testdata_bmc_static_regions_start_addr[0]);
    spi_ptr[0] = 0xDEADBEEF;
    spi_ptr[1] = 0x12345678;
    spi_ptr[2] = 0xDECAF000;

    /*
     * Simulate a BMC reset
     * Make sure Nios recovered the active and recovery region and allowed platform to boot.
     */
    ut_send_bmc_reset_detected_gpi_once_upon_boot_complete();
    bmc_reset_handler();

    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(1));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), alt_u32(LAST_PANIC_BMC_RESET_DETECTED));
    // Three recovery events: Recovery region recovery / Active region recovery
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(2));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(LAST_RECOVERY_BMC_ACTIVE));
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(MAJOR_ERROR_BMC_AUTH_FAILED));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_AUTH_ACTIVE_AND_RECOVERY));

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

    // Both BMC and PCH should be out of reset
    EXPECT_TRUE(check_bit(U_GPO_1_ADDR, GPO_1_RST_PFR_EXTRST_N));
    EXPECT_TRUE(ut_is_pch_out_of_reset());

    // BMC timer should be active
    EXPECT_TRUE(check_bit(WDT_BMC_TIMER_ADDR, U_TIMER_BANK_TIMER_ACTIVE_BIT));
    // PCH timers should already be turned off since this is a BMC only reset
    EXPECT_FALSE(check_bit(WDT_ME_TIMER_ADDR, U_TIMER_BANK_TIMER_ACTIVE_BIT));
    EXPECT_FALSE(check_bit(WDT_ACM_BIOS_TIMER_ADDR, U_TIMER_BANK_TIMER_ACTIVE_BIT));

    /*
     * In BMC flash, the static fw, dynamic fw, PFM and Recovery regions should be recovered.
     */
    // Static firmware regions
    for (alt_u32 region_i = 0; region_i < BMC_NUM_STATIC_REGIONS; region_i++)
    {
        for (alt_u32 word_i = testdata_bmc_static_regions_start_addr[region_i] >> 2;
                word_i < testdata_bmc_static_regions_end_addr[region_i] >> 2; word_i++)
        {
            ASSERT_EQ(full_new_image[word_i], flash_x86_ptr[word_i]);
        }
    }
    // Dynamic firmware regions
    for (alt_u32 region_i = 0; region_i < BMC_NUM_DYNAMIC_REGIONS; region_i++)
    {
        for (alt_u32 word_i = testdata_bmc_dynamic_regions_start_addr[region_i] >> 2;
                word_i < testdata_bmc_dynamic_regions_end_addr[region_i] >> 2; word_i++)
        {
            ASSERT_EQ(full_new_image[word_i], flash_x86_ptr[word_i]);
        }
    }
    // PFM
    alt_u32 bmc_active_pfm_start = get_ufm_pfr_data()->bmc_active_pfm;
    switch_spi_flash(SPI_FLASH_BMC);
    alt_u32 bmc_active_pfm_end = bmc_active_pfm_start + get_signed_payload_size(get_spi_active_pfm_ptr(SPI_FLASH_BMC));
    for (alt_u32 word_i = bmc_active_pfm_start >> 2; word_i < bmc_active_pfm_end >> 2; word_i++)
    {
        ASSERT_EQ(full_new_image[word_i], flash_x86_ptr[word_i]);
    }
    // Recovery region
    alt_u32 capsule_size = get_signed_payload_size(update_capsule);
    alt_u32 start_i = get_recovery_region_offset(SPI_FLASH_BMC) >> 2;
    alt_u32 end_i = (get_recovery_region_offset(SPI_FLASH_BMC) + capsule_size) >> 2;
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
