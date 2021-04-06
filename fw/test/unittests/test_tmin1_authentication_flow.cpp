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
// Unit test for the PFR system flows

// Include the GTest headers
#include "gtest_headers.h"

// Include the SYSTEM MOCK and PFR headers
#include "ut_nios_wrapper.h"
#include "testdata_info.h"

class Tmin1AuthenticationFlowTest : public testing::Test
{
public:
    virtual void SetUp() {
        SYSTEM_MOCK* sys = SYSTEM_MOCK::get();
        // Reset system mocks and SPI flash
        sys->reset();
        sys->reset_spi_flash_mock();

        // Perform provisioning
        SYSTEM_MOCK::get()->provision_ufm_data(UFM_PFR_DATA_EXAMPLE_KEY_FILE);

        // Prepare the flashes
        SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_PCH, FULL_PFR_IMAGE_PCH_FILE, FULL_PFR_IMAGE_PCH_FILE_SIZE);
        SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, FULL_PFR_IMAGE_BMC_FILE, FULL_PFR_IMAGE_BMC_FILE_SIZE);

        // Reset Nios firmware
        ut_reset_nios_fw();
    }

    virtual void TearDown() {}
};

TEST_F(Tmin1AuthenticationFlowTest, test_corrupted_bmc_and_pch_active_firmware)
{
    // Timed boot is only enabled in provisioned state.
    EXPECT_TRUE(is_ufm_provisioned());

    /*
     * Flow preparation
     */
    ut_prep_nios_gpi_signals();
    ut_send_block_complete_chkpt_msg();

    // Corrupt PCH firmware
    switch_spi_flash(SPI_FLASH_PCH);
    alt_u32* pch_flash_ptr = get_spi_flash_ptr();
    erase_spi_region(testdata_pch_static_regions_start_addr[0], testdata_pch_static_regions_end_addr[0] - testdata_pch_static_regions_start_addr[0]);
    // Corrupt BMC firmware
    switch_spi_flash(SPI_FLASH_BMC);
    alt_u32* bmc_flash_ptr = get_spi_flash_ptr();
    erase_spi_region(testdata_bmc_static_regions_start_addr[1], testdata_bmc_static_regions_end_addr[1] - testdata_bmc_static_regions_start_addr[1]);

    // Insert the T0_TIMED_BOOT code block (break out of T0 loop when all timer has stopped)
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_TIMED_BOOT);

    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(120, pfr_main());

    /*
     * Verify recovered data
     */
    alt_u32 *full_image = new alt_u32[FULL_PFR_IMAGE_PCH_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(FULL_PFR_IMAGE_PCH_FILE, full_image);

    for (alt_u32 region_i = 0; region_i < PCH_NUM_STATIC_REGIONS; region_i++)
    {
        for (alt_u32 word_i = testdata_pch_static_regions_start_addr[region_i] >> 2;
                word_i < testdata_pch_static_regions_end_addr[region_i] >> 2; word_i++)
        {
            ASSERT_EQ(full_image[word_i], pch_flash_ptr[word_i]);
        }
    }
    delete[] full_image;

    full_image = new alt_u32[FULL_PFR_IMAGE_BMC_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(FULL_PFR_IMAGE_BMC_FILE, full_image);

    for (alt_u32 region_i = 0; region_i < BMC_NUM_STATIC_REGIONS; region_i++)
    {
        for (alt_u32 word_i = testdata_bmc_static_regions_start_addr[region_i] >> 2;
                word_i < testdata_bmc_static_regions_end_addr[region_i] >> 2; word_i++)
        {
            ASSERT_EQ(full_image[word_i], bmc_flash_ptr[word_i]);
        }
    }
    delete[] full_image;

    // Check observed vs expected global_state
    EXPECT_EQ(ut_get_global_state(), (alt_u32) PLATFORM_STATE_T0_BOOT_COMPLETE);

    // Expect no panic events
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(0));
    // Expect two recovery events: one for PCH active and one for BMC active
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(2));
    alt_u32 last_recovery_reason_matches_expectation = read_from_mailbox(MB_LAST_RECOVERY_REASON) == LAST_RECOVERY_PCH_ACTIVE;
    last_recovery_reason_matches_expectation |= read_from_mailbox(MB_LAST_RECOVERY_REASON) == LAST_RECOVERY_BMC_ACTIVE;
    EXPECT_TRUE(last_recovery_reason_matches_expectation);

    // Expect to see active firmware authentication failure minor error
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), MINOR_ERROR_AUTH_ACTIVE);
    // Expect to see authentication failure major error
    alt_u32 major_err_matches_expectation = read_from_mailbox(MB_MAJOR_ERROR_CODE) == MAJOR_ERROR_BMC_AUTH_FAILED;
    major_err_matches_expectation |= read_from_mailbox(MB_MAJOR_ERROR_CODE) == MAJOR_ERROR_PCH_AUTH_FAILED;
    EXPECT_TRUE(major_err_matches_expectation);
}

TEST_F(Tmin1AuthenticationFlowTest, test_corrupted_bmc_recovery)
{
    // Timed boot is only enabled in provisioned state.
    EXPECT_TRUE(is_ufm_provisioned());

    /*
     * Flow preparation
     */
    ut_prep_nios_gpi_signals();
    ut_send_block_complete_chkpt_msg();

    // Load the fw update capsule to staging region in flash
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, SIGNED_CAPSULE_BMC_FILE,
            SIGNED_CAPSULE_BMC_FILE_SIZE, get_ufm_pfr_data()->bmc_staging_region);

    // Corrupt BMC recovery firmware
    switch_spi_flash(SPI_FLASH_BMC);
    alt_u32* bmc_flash_ptr = get_spi_flash_ptr();
    alt_u32 bmc_recovery_start_addr = get_ufm_pfr_data()->bmc_recovery_region;
    erase_spi_region(bmc_recovery_start_addr, SIGNED_CAPSULE_BMC_FILE_SIZE);

    // Insert the T0_TIMED_BOOT code block (break out of T0 loop when all timer has stopped)
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_TIMED_BOOT);

    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(120, pfr_main());

    /*
     * Verify recovered data
     */

    alt_u32* full_image = new alt_u32[FULL_PFR_IMAGE_BMC_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(FULL_PFR_IMAGE_BMC_FILE, full_image);

    for (alt_u32 word_i = bmc_recovery_start_addr >> 2;
            word_i < ((bmc_recovery_start_addr + SIGNED_CAPSULE_BMC_FILE_SIZE) >> 2); word_i++)
    {
        ASSERT_EQ(full_image[word_i], bmc_flash_ptr[word_i]);
    }

    delete[] full_image;

    // Check observed vs expected global_state
    EXPECT_EQ(ut_get_global_state(), (alt_u32) PLATFORM_STATE_T0_BOOT_COMPLETE);

    // Expect no panic event
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(0));

    // Expect a recovery event
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(LAST_RECOVERY_BMC_RECOVERY));
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(1));

    // Check major/minor error
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), MINOR_ERROR_AUTH_RECOVERY);
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), MAJOR_ERROR_BMC_AUTH_FAILED);
}

TEST_F(Tmin1AuthenticationFlowTest, test_corrupted_pch_recovery)
{
    // Timed boot is only enabled in provisioned state.
    EXPECT_TRUE(is_ufm_provisioned());

    /*
     * Flow preparation
     */
    ut_prep_nios_gpi_signals();
    ut_send_block_complete_chkpt_msg();

    // Load the fw update capsule to staging region in flash
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_PCH, SIGNED_CAPSULE_PCH_FILE,
            SIGNED_CAPSULE_PCH_FILE_SIZE, get_ufm_pfr_data()->pch_staging_region);

    // Corrupt PCH recovery firmware
    switch_spi_flash(SPI_FLASH_PCH);
    alt_u32* pch_flash_ptr = get_spi_flash_ptr();
    alt_u32 pch_recovery_start_addr = get_ufm_pfr_data()->pch_recovery_region;
    erase_spi_region(pch_recovery_start_addr, SIGNED_CAPSULE_PCH_FILE_SIZE);

    // Insert the T0_TIMED_BOOT code block (break out of T0 loop when all timer has stopped)
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_TIMED_BOOT);

    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(120, pfr_main());

    /*
     * Verify recovered data
     */
    alt_u32* full_image = new alt_u32[FULL_PFR_IMAGE_PCH_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(FULL_PFR_IMAGE_PCH_FILE, full_image);

    for (alt_u32 word_i = pch_recovery_start_addr >> 2;
            word_i < ((pch_recovery_start_addr + SIGNED_CAPSULE_PCH_FILE_SIZE) >> 2); word_i++)
    {
        ASSERT_EQ(full_image[word_i], pch_flash_ptr[word_i]);
    }

    delete[] full_image;

    // Check observed vs expected global_state
    EXPECT_EQ(ut_get_global_state(), (alt_u32) PLATFORM_STATE_T0_BOOT_COMPLETE);

    // Expect no panic event
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(0));

    // Expect a recovery event
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(LAST_RECOVERY_PCH_RECOVERY));
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(1));

    // Check major/minor error
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), MINOR_ERROR_AUTH_RECOVERY);
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), MAJOR_ERROR_PCH_AUTH_FAILED);
}

TEST_F(Tmin1AuthenticationFlowTest, test_completely_corrupted_bmc_and_pch_flashes)
{
    // Timed boot is only enabled in provisioned state.
    EXPECT_TRUE(is_ufm_provisioned());

    /*
     * Flow preparation
     */
    ut_prep_nios_gpi_signals();
    ut_send_block_complete_chkpt_msg();

    // Clear the bmc and pch flash (one scenario of completely corrupted flash)
    SYSTEM_MOCK::get()->reset_spi_flash_mock();

    // Insert the T0_TIMED_BOOT code block (break out of T0 loop when all timer has stopped)
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_TIMED_BOOT);

    // Set asserts to throw as opposed to abort
    SYSTEM_MOCK::get()->set_assert_to_throw();
    // Expect that Nios firmware will stuck in the never_exit_loop.
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::THROW_FROM_NEVER_EXIT_LOOP);

    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(120, EXPECT_ANY_THROW({ pfr_main(); }));

    // Check observed vs expected global_state
    EXPECT_EQ(ut_get_global_state(), (alt_u32) PLATFORM_STATE_AUTHENTICATION_FAILED_LOCKDOWN);

    // Expect no panic event
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(0));

    // Expect a recovery event
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(0));

    // Check major/minor error
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), MINOR_ERROR_AUTH_ALL_REGIONS);
    alt_u32 major_err_matches_expectation = read_from_mailbox(MB_MAJOR_ERROR_CODE) == MAJOR_ERROR_BMC_AUTH_FAILED;
    major_err_matches_expectation |= read_from_mailbox(MB_MAJOR_ERROR_CODE) == MAJOR_ERROR_PCH_AUTH_FAILED;
    EXPECT_TRUE(major_err_matches_expectation);
}
