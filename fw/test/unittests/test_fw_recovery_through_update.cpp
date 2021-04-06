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


class FWRecoveryThroughUpdateTest : public testing::Test
{
public:
    virtual void SetUp()
    {
        SYSTEM_MOCK* sys = SYSTEM_MOCK::get();
        // Reset system mocks and SPI flash
        sys->reset();
        sys->reset_spi_flash_mock();

        // Perform provisioning
        sys->provision_ufm_data(UFM_PFR_DATA_EXAMPLE_KEY_FILE);

        // Reset Nios firmware
        ut_reset_nios_fw();

        // Load the entire image to flash
        sys->load_to_flash(SPI_FLASH_BMC, FULL_PFR_IMAGE_BMC_FILE, FULL_PFR_IMAGE_BMC_FILE_SIZE);
        sys->load_to_flash(SPI_FLASH_PCH, FULL_PFR_IMAGE_PCH_FILE, FULL_PFR_IMAGE_PCH_FILE_SIZE);
    }

    virtual void TearDown() {}
};

/**
 * @brief When recovery image is corrupted, recovery firmware update with FW capsule different to
 * active firmware should be banned.
 *
 * PCH flash content:
 * - A valid active image
 * - A corrupted recovery image
 * - Empty staging region
 *
 * BMC flash has valid active and recovery image.
 */
TEST_F(FWRecoveryThroughUpdateTest, test_recover_pch_recovery_region_through_ib_fw_update_with_unexpected_capsule)
{
    /*
     * Flow preparation
     */
    ut_prep_nios_gpi_signals();

    // Insert the T0_OPERATIONS code block (break out of T0 loop upon entry)
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS_END_AFTER_50_ITERS);

    // Load the entire image locally for comparison purpose
    alt_u32 *full_image = new alt_u32[FULL_PFR_IMAGE_PCH_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(FULL_PFR_IMAGE_PCH_FILE, full_image);

    // Corrupt recovery image in PCH flash
    switch_spi_flash(SPI_FLASH_PCH);
    alt_u32* signed_recovery_capsule = get_spi_recovery_region_ptr(SPI_FLASH_PCH);
    *signed_recovery_capsule = 0;
    EXPECT_FALSE(is_signature_valid((KCH_SIGNATURE*) signed_recovery_capsule));

    // In T0, send update capsule and recovery firwmare update intent
    SYSTEM_MOCK::get()->register_read_write_callback(
            [](SYSTEM_MOCK::READ_OR_WRITE read_or_write, void* addr, alt_u32 data) {
        if (read_or_write == SYSTEM_MOCK::READ_OR_WRITE::READ)
        {
            alt_u32* update_intent_addr = U_MAILBOX_AVMM_BRIDGE_ADDR + MB_PCH_UPDATE_INTENT;
            if (addr == update_intent_addr)
            {
                if ((read_from_mailbox(MB_PLATFORM_STATE) == PLATFORM_STATE_ENTER_T0) && (read_from_mailbox(MB_PANIC_EVENT_COUNT) == 0))
                {
                    // Load a pch fw update capsule that does not match the active image
                    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_PCH, SIGNED_CAPSULE_PCH_V03P12_FILE,
                            SIGNED_CAPSULE_PCH_V03P12_FILE_SIZE, get_ufm_pfr_data()->pch_staging_region);

                    // Send recovery update capsule
                    SYSTEM_MOCK::get()->set_mem_word(update_intent_addr, MB_UPDATE_INTENT_PCH_RECOVERY_MASK, true);
                }
            }
        }
    });

    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(60, pfr_main());

    /*
     * Check result
     */
    // Expect no recovery
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(0));

    // Expect one panic event for the recovery update
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), alt_u32(LAST_PANIC_PCH_UPDATE_INTENT));
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(1));

    // Expect some indication on the failed recovery update
    EXPECT_EQ(num_failed_update_attempts_from_pch, alt_u32(1));
    // MINOR_ERROR_RECOVERY_FW_UPDATE_AUTH_FAILED error is overwritten by the authentication failure
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(MAJOR_ERROR_PCH_AUTH_FAILED));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_AUTH_RECOVERY));

    // Both BMC and PCH should be out of reset
    EXPECT_TRUE(ut_is_bmc_out_of_reset());
    EXPECT_TRUE(ut_is_pch_out_of_reset());

    // BMC timer should be active
    EXPECT_TRUE(check_bit(WDT_BMC_TIMER_ADDR, U_TIMER_BANK_TIMER_ACTIVE_BIT));
    // PCH timer should be inactive because there's no recovery image to recover to
    EXPECT_FALSE(check_bit(WDT_ACM_BIOS_TIMER_ADDR, U_TIMER_BANK_TIMER_ACTIVE_BIT));

    // Check T-1 entries
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_pch_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_bmc_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(2));

    // Active image should be untouched
    switch_spi_flash(SPI_FLASH_PCH);
    alt_u32 active_pfm_offset = get_ufm_pfr_data()->pch_active_pfm;
    alt_u32* signed_active_pfm = get_spi_flash_ptr_with_offset(active_pfm_offset);
    EXPECT_TRUE(is_active_region_valid(signed_active_pfm));

    // Active PFM must remain the same
    for (alt_u32 word_i = 0; word_i < SIGNATURE_SIZE; word_i++)
    {
        ASSERT_EQ(full_image[word_i + (active_pfm_offset / sizeof(alt_u32))], signed_active_pfm[word_i]);
    }

    // Recovery image should still be invalid (i.e. no update has occurred)
    EXPECT_FALSE(is_signature_valid((KCH_SIGNATURE*) signed_recovery_capsule));

    // clean up
    delete[] full_image;
}

/**
 * @brief When recovery image is corrupted, recovery firmware update with FW capsule
 * matching the active firmware is allowed.
 *
 * PCH flash content:
 * - A valid active image
 * - A corrupted recovery image
 * - Empty staging region
 *
 * BMC flash has valid active and recovery image.
 */
TEST_F(FWRecoveryThroughUpdateTest, test_recover_pch_recovery_region_through_ib_fw_update_with_good_capsule)
{
    /*
     * Flow preparation
     */
    ut_prep_nios_gpi_signals();

    // Insert the T0_OPERATIONS code block (break out of T0 loop upon entry)
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS_END_AFTER_50_ITERS);

    // Load the entire image locally for comparison purpose
    alt_u32 *full_image = new alt_u32[FULL_PFR_IMAGE_PCH_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(FULL_PFR_IMAGE_PCH_FILE, full_image);

    // Corrupt recovery image in PCH flash
    switch_spi_flash(SPI_FLASH_PCH);
    alt_u32* signed_recovery_capsule = get_spi_recovery_region_ptr(SPI_FLASH_PCH);
    *signed_recovery_capsule = 0;
    EXPECT_FALSE(is_signature_valid((KCH_SIGNATURE*) signed_recovery_capsule));

    // In T0, send update capsule and recovery firwmare update intent
    SYSTEM_MOCK::get()->register_read_write_callback(
            [](SYSTEM_MOCK::READ_OR_WRITE read_or_write, void* addr, alt_u32 data) {
        if (read_or_write == SYSTEM_MOCK::READ_OR_WRITE::READ)
        {
            alt_u32* update_intent_addr = U_MAILBOX_AVMM_BRIDGE_ADDR + MB_PCH_UPDATE_INTENT;
            if (addr == update_intent_addr)
            {
                if ((read_from_mailbox(MB_PLATFORM_STATE) == PLATFORM_STATE_ENTER_T0) && (read_from_mailbox(MB_PANIC_EVENT_COUNT) == 0))
                {
                    // Load a pch fw update capsule that matches the active image
                    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_PCH, SIGNED_CAPSULE_PCH_FILE,
                            SIGNED_CAPSULE_PCH_FILE_SIZE, get_ufm_pfr_data()->pch_staging_region);

                    // Send recovery update capsule
                    SYSTEM_MOCK::get()->set_mem_word(update_intent_addr, MB_UPDATE_INTENT_PCH_RECOVERY_MASK, true);
                }
            }
        }
    });

    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(60, pfr_main());

    /*
     * Check result
     */
    // Expect no recovery
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(0));

    // Expect one panic event for the recovery update
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), alt_u32(LAST_PANIC_PCH_UPDATE_INTENT));
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(1));

    // Expect no error on the firmware recovery update
    EXPECT_EQ(num_failed_update_attempts_from_pch, alt_u32(0));
    // Authentication failure is likely from the first T-1
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(MAJOR_ERROR_PCH_AUTH_FAILED));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_AUTH_RECOVERY));

    // Both BMC and PCH should be out of reset
    EXPECT_TRUE(ut_is_bmc_out_of_reset());
    EXPECT_TRUE(ut_is_pch_out_of_reset());

    // Both BMC and PCH timers should be active
    EXPECT_TRUE(check_bit(WDT_BMC_TIMER_ADDR, U_TIMER_BANK_TIMER_ACTIVE_BIT));
    EXPECT_TRUE(check_bit(WDT_ACM_BIOS_TIMER_ADDR, U_TIMER_BANK_TIMER_ACTIVE_BIT));

    // Check T-1 entries
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_pch_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_bmc_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(2));

    // Active image should be untouched
    switch_spi_flash(SPI_FLASH_PCH);
    alt_u32 active_pfm_offset = get_ufm_pfr_data()->pch_active_pfm;
    alt_u32* signed_active_pfm = get_spi_flash_ptr_with_offset(active_pfm_offset);
    EXPECT_TRUE(is_active_region_valid(signed_active_pfm));

    // Active PFM must remain the same
    for (alt_u32 word_i = 0; word_i < SIGNATURE_SIZE; word_i++)
    {
        ASSERT_EQ(full_image[word_i + (active_pfm_offset / sizeof(alt_u32))], signed_active_pfm[word_i]);
    }

    // Recovery image should now be valid
    EXPECT_TRUE(is_signature_valid((KCH_SIGNATURE*) signed_recovery_capsule));

    delete[] full_image;
}


/**
 * @brief When PCH recovery image is corrupted, out-of-band PCH recovery firmware update with FW capsule different to
 * active firmware should be banned.
 *
 * PCH flash content:
 * - A valid active image
 * - A corrupted recovery image
 * - Empty staging region
 *
 * BMC flash has valid active and recovery image.
 */
TEST_F(FWRecoveryThroughUpdateTest, test_recover_pch_recovery_region_through_oob_fw_update_with_unexpected_capsule)
{
    /*
     * Flow preparation
     */
    ut_prep_nios_gpi_signals();

    // Insert the T0_OPERATIONS code block (break out of T0 loop upon entry)
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS_END_AFTER_50_ITERS);

    // Load the entire image locally for comparison purpose
    alt_u32 *full_image = new alt_u32[FULL_PFR_IMAGE_PCH_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(FULL_PFR_IMAGE_PCH_FILE, full_image);

    // Corrupt recovery image in PCH flash
    switch_spi_flash(SPI_FLASH_PCH);
    alt_u32* signed_recovery_capsule = get_spi_recovery_region_ptr(SPI_FLASH_PCH);
    *signed_recovery_capsule = 0;
    EXPECT_FALSE(is_signature_valid((KCH_SIGNATURE*) signed_recovery_capsule));

    // In T0, send update capsule and recovery firwmare update intent
    SYSTEM_MOCK::get()->register_read_write_callback(
            [](SYSTEM_MOCK::READ_OR_WRITE read_or_write, void* addr, alt_u32 data) {
        if (read_or_write == SYSTEM_MOCK::READ_OR_WRITE::READ)
        {
            alt_u32* update_intent_addr = U_MAILBOX_AVMM_BRIDGE_ADDR + MB_BMC_UPDATE_INTENT;
            if (addr == update_intent_addr)
            {
                if ((read_from_mailbox(MB_PLATFORM_STATE) == PLATFORM_STATE_ENTER_T0) && (read_from_mailbox(MB_PANIC_EVENT_COUNT) == 0))
                {
                    // Load a pch fw update capsule that does not match the active image
                    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, SIGNED_CAPSULE_PCH_V03P12_FILE,
                            SIGNED_CAPSULE_PCH_V03P12_FILE_SIZE, get_ufm_pfr_data()->bmc_staging_region + BMC_STAGING_REGION_PCH_UPDATE_CAPSULE_OFFSET);

                    // Send recovery update capsule
                    SYSTEM_MOCK::get()->set_mem_word(update_intent_addr, MB_UPDATE_INTENT_PCH_RECOVERY_MASK, true);
                }
            }
        }
    });

    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(60, pfr_main());

    /*
     * Check result
     */
    // Expect no recovery
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(0));

    // Expect one panic event for the recovery update
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), alt_u32(LAST_PANIC_BMC_UPDATE_INTENT));
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(1));

    // Expect some indication on the failed recovery update
    EXPECT_EQ(num_failed_update_attempts_from_pch, alt_u32(1));
    EXPECT_EQ(num_failed_update_attempts_from_bmc, alt_u32(0));

    // MINOR_ERROR_RECOVERY_FW_UPDATE_AUTH_FAILED error is overwritten by the authentication failure
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(MAJOR_ERROR_PCH_AUTH_FAILED));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_AUTH_RECOVERY));

    // Both BMC and PCH should be out of reset
    EXPECT_TRUE(ut_is_bmc_out_of_reset());
    EXPECT_TRUE(ut_is_pch_out_of_reset());

    // BMC timer should be active
    EXPECT_TRUE(check_bit(WDT_BMC_TIMER_ADDR, U_TIMER_BANK_TIMER_ACTIVE_BIT));
    // PCH timer should be inactive because there's no recovery image to recover to
    EXPECT_FALSE(check_bit(WDT_ACM_BIOS_TIMER_ADDR, U_TIMER_BANK_TIMER_ACTIVE_BIT));

    // Check T-1 entries
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_pch_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_bmc_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(2));

    // Active image should be untouched
    switch_spi_flash(SPI_FLASH_PCH);
    alt_u32 active_pfm_offset = get_ufm_pfr_data()->pch_active_pfm;
    alt_u32* signed_active_pfm = get_spi_flash_ptr_with_offset(active_pfm_offset);
    EXPECT_TRUE(is_active_region_valid(signed_active_pfm));

    // Active PFM must remain the same
    for (alt_u32 word_i = 0; word_i < SIGNATURE_SIZE; word_i++)
    {
        ASSERT_EQ(full_image[word_i + (active_pfm_offset / sizeof(alt_u32))], signed_active_pfm[word_i]);
    }

    // Recovery image should still be invalid (i.e. no update has occurred)
    EXPECT_FALSE(is_signature_valid((KCH_SIGNATURE*) signed_recovery_capsule));

    // clean up
    delete[] full_image;
}

/**
 * @brief When PCH recovery image is corrupted, out-of-band PCH recovery firmware update with FW capsule
 * matching the active firmware is allowed.
 *
 * PCH flash content:
 * - A valid active image
 * - A corrupted recovery image
 * - Empty staging region
 *
 * BMC flash has valid active and recovery image.
 */
TEST_F(FWRecoveryThroughUpdateTest, test_recover_pch_recovery_region_through_oob_fw_update_with_good_capsule)
{
    /*
     * Flow preparation
     */
    ut_prep_nios_gpi_signals();

    // Insert the T0_OPERATIONS code block (break out of T0 loop upon entry)
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS_END_AFTER_50_ITERS);

    // Load the entire image locally for comparison purpose
    alt_u32 *full_image = new alt_u32[FULL_PFR_IMAGE_PCH_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(FULL_PFR_IMAGE_PCH_FILE, full_image);

    // Corrupt recovery image in PCH flash
    switch_spi_flash(SPI_FLASH_PCH);
    alt_u32* signed_recovery_capsule = get_spi_recovery_region_ptr(SPI_FLASH_PCH);
    *signed_recovery_capsule = 0;
    EXPECT_FALSE(is_signature_valid((KCH_SIGNATURE*) signed_recovery_capsule));

    // In T0, send update capsule and recovery firwmare update intent
    SYSTEM_MOCK::get()->register_read_write_callback(
            [](SYSTEM_MOCK::READ_OR_WRITE read_or_write, void* addr, alt_u32 data) {
        if (read_or_write == SYSTEM_MOCK::READ_OR_WRITE::READ)
        {
            alt_u32* update_intent_addr = U_MAILBOX_AVMM_BRIDGE_ADDR + MB_BMC_UPDATE_INTENT;
            if (addr == update_intent_addr)
            {
                if ((read_from_mailbox(MB_PLATFORM_STATE) == PLATFORM_STATE_ENTER_T0) && (read_from_mailbox(MB_PANIC_EVENT_COUNT) == 0))
                {
                    // Load a pch fw update capsule that matches the active image
                    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, SIGNED_CAPSULE_PCH_FILE,
                            SIGNED_CAPSULE_PCH_FILE_SIZE, get_ufm_pfr_data()->bmc_staging_region + BMC_STAGING_REGION_PCH_UPDATE_CAPSULE_OFFSET);

                    // Send recovery update capsule
                    SYSTEM_MOCK::get()->set_mem_word(update_intent_addr, MB_UPDATE_INTENT_PCH_RECOVERY_MASK, true);
                }
            }
        }
    });

    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(60, pfr_main());

    /*
     * Check result
     */
    // Expect no recovery
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(0));

    // Expect one panic event for the recovery update
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), alt_u32(LAST_PANIC_BMC_UPDATE_INTENT));
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(1));

    // Expect no error on the firmware recovery update
    EXPECT_EQ(num_failed_update_attempts_from_pch, alt_u32(0));
    EXPECT_EQ(num_failed_update_attempts_from_bmc, alt_u32(0));

    // Authentication failure is likely from the first T-1
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(MAJOR_ERROR_PCH_AUTH_FAILED));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_AUTH_RECOVERY));

    // Both BMC and PCH should be out of reset
    EXPECT_TRUE(ut_is_bmc_out_of_reset());
    EXPECT_TRUE(ut_is_pch_out_of_reset());

    // Both BMC and PCH timers should be active
    EXPECT_TRUE(check_bit(WDT_BMC_TIMER_ADDR, U_TIMER_BANK_TIMER_ACTIVE_BIT));
    EXPECT_TRUE(check_bit(WDT_ACM_BIOS_TIMER_ADDR, U_TIMER_BANK_TIMER_ACTIVE_BIT));

    // Check T-1 entries
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_pch_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_bmc_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(2));

    // Active image should be untouched
    switch_spi_flash(SPI_FLASH_PCH);
    alt_u32 active_pfm_offset = get_ufm_pfr_data()->pch_active_pfm;
    alt_u32* signed_active_pfm = get_spi_flash_ptr_with_offset(active_pfm_offset);
    EXPECT_TRUE(is_active_region_valid(signed_active_pfm));

    // Active PFM must remain the same
    for (alt_u32 word_i = 0; word_i < SIGNATURE_SIZE; word_i++)
    {
        ASSERT_EQ(full_image[word_i + (active_pfm_offset / sizeof(alt_u32))], signed_active_pfm[word_i]);
    }

    // Recovery image should now be valid
    EXPECT_TRUE(is_signature_valid((KCH_SIGNATURE*) signed_recovery_capsule));

    delete[] full_image;
}

/**
 * @brief When recovery image is corrupted, recovery firmware update with FW capsule different to
 * active firmware should be banned.
 *
 * BMC flash content:
 * - A valid active image
 * - A corrupted recovery image
 * - Empty staging region
 *
 * PCH flash has valid active and recovery image.
 */
TEST_F(FWRecoveryThroughUpdateTest, test_recover_bmc_recovery_region_through_fw_update_with_unexpected_capsule)
{
    /*
     * Flow preparation
     */
    ut_prep_nios_gpi_signals();

    // Insert the T0_OPERATIONS code block (break out of T0 loop upon entry)
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS_END_AFTER_50_ITERS);

    // Load the entire image locally for comparison purpose
    alt_u32 *full_image = new alt_u32[FULL_PFR_IMAGE_BMC_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(FULL_PFR_IMAGE_BMC_FILE, full_image);

    // Corrupt recovery image in BMC flash
    switch_spi_flash(SPI_FLASH_BMC);
    alt_u32* signed_recovery_capsule = get_spi_recovery_region_ptr(SPI_FLASH_BMC);
    *signed_recovery_capsule = 0;
    EXPECT_FALSE(is_signature_valid((KCH_SIGNATURE*) signed_recovery_capsule));

    // In T0, send update capsule and recovery firwmare update intent
    SYSTEM_MOCK::get()->register_read_write_callback(
            [](SYSTEM_MOCK::READ_OR_WRITE read_or_write, void* addr, alt_u32 data) {
        if (read_or_write == SYSTEM_MOCK::READ_OR_WRITE::READ)
        {
            alt_u32* update_intent_addr = U_MAILBOX_AVMM_BRIDGE_ADDR + MB_BMC_UPDATE_INTENT;
            if (addr == update_intent_addr)
            {
                if ((read_from_mailbox(MB_PLATFORM_STATE) == PLATFORM_STATE_ENTER_T0) && (read_from_mailbox(MB_PANIC_EVENT_COUNT) == 0))
                {
                    // Load a bmc fw update capsule that does not match the active image
                    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, SIGNED_CAPSULE_BMC_V14_FILE,
                            SIGNED_CAPSULE_BMC_V14_FILE_SIZE, get_ufm_pfr_data()->bmc_staging_region);

                    // Send recovery update capsule
                    SYSTEM_MOCK::get()->set_mem_word(update_intent_addr, MB_UPDATE_INTENT_BMC_RECOVERY_MASK, true);
                }
            }
        }
    });

    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(60, pfr_main());

    /*
     * Check result
     */
    // Expect no recovery
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(0));

    // Expect one panic event for the recovery update
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), alt_u32(LAST_PANIC_BMC_UPDATE_INTENT));
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(1));

    // Expect some indication on the failed recovery update
    EXPECT_EQ(num_failed_update_attempts_from_bmc, alt_u32(1));
    // MINOR_ERROR_RECOVERY_FW_UPDATE_AUTH_FAILED error is overwritten by the authentication failure
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(MAJOR_ERROR_BMC_AUTH_FAILED));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_AUTH_RECOVERY));

    // Both BMC and PCH should be out of reset
    EXPECT_TRUE(ut_is_bmc_out_of_reset());
    EXPECT_TRUE(ut_is_pch_out_of_reset());

    // PCH timer should be active
    EXPECT_TRUE(check_bit(WDT_ACM_BIOS_TIMER_ADDR, U_TIMER_BANK_TIMER_ACTIVE_BIT));
    // BMC timer should be inactive because there's no recovery image to recover to
    EXPECT_FALSE(check_bit(WDT_BMC_TIMER_ADDR, U_TIMER_BANK_TIMER_ACTIVE_BIT));

    // Check T-1 entries
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_pch_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_bmc_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(2));

    // Active image should be untouched
    switch_spi_flash(SPI_FLASH_BMC);
    alt_u32 active_pfm_offset = get_ufm_pfr_data()->bmc_active_pfm;
    alt_u32* signed_active_pfm = get_spi_flash_ptr_with_offset(active_pfm_offset);
    EXPECT_TRUE(is_active_region_valid(signed_active_pfm));

    // Active PFM must remain the same
    for (alt_u32 word_i = 0; word_i < SIGNATURE_SIZE; word_i++)
    {
        ASSERT_EQ(full_image[word_i + (active_pfm_offset / sizeof(alt_u32))], signed_active_pfm[word_i]);
    }

    // Recovery image should still be invalid (i.e. no update has occurred)
    EXPECT_FALSE(is_signature_valid((KCH_SIGNATURE*) signed_recovery_capsule));

    // clean up
    delete[] full_image;
}

/**
 * @brief When recovery image is corrupted, recovery firmware update with FW capsule
 * matching the active firmware is allowed.
 *
 * BMC flash content:
 * - A valid active image
 * - A corrupted recovery image
 * - Empty staging region
 *
 * PCH flash has valid active and recovery image.
 */
TEST_F(FWRecoveryThroughUpdateTest, test_recover_bmc_recovery_region_through_fw_update_with_good_capsule)
{
    /*
     * Flow preparation
     */
    ut_prep_nios_gpi_signals();

    // Insert the T0_OPERATIONS code block (break out of T0 loop upon entry)
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS_END_AFTER_50_ITERS);

    // Load the entire image locally for comparison purpose
    alt_u32 *full_image = new alt_u32[FULL_PFR_IMAGE_BMC_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(FULL_PFR_IMAGE_BMC_FILE, full_image);

    // Corrupt recovery image in BMC flash
    switch_spi_flash(SPI_FLASH_BMC);
    alt_u32* signed_recovery_capsule = get_spi_recovery_region_ptr(SPI_FLASH_BMC);
    *signed_recovery_capsule = 0;
    EXPECT_FALSE(is_signature_valid((KCH_SIGNATURE*) signed_recovery_capsule));

    // In T0, send update capsule and recovery firwmare update intent
    SYSTEM_MOCK::get()->register_read_write_callback(
            [](SYSTEM_MOCK::READ_OR_WRITE read_or_write, void* addr, alt_u32 data) {
        if (read_or_write == SYSTEM_MOCK::READ_OR_WRITE::READ)
        {
            alt_u32* update_intent_addr = U_MAILBOX_AVMM_BRIDGE_ADDR + MB_BMC_UPDATE_INTENT;
            if (addr == update_intent_addr)
            {
                if ((read_from_mailbox(MB_PLATFORM_STATE) == PLATFORM_STATE_ENTER_T0) && (read_from_mailbox(MB_PANIC_EVENT_COUNT) == 0))
                {
                    // Load a pch fw update capsule that matches the active image
                    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, SIGNED_CAPSULE_BMC_FILE,
                            SIGNED_CAPSULE_BMC_FILE_SIZE, get_ufm_pfr_data()->bmc_staging_region);

                    // Send recovery update capsule
                    SYSTEM_MOCK::get()->set_mem_word(update_intent_addr, MB_UPDATE_INTENT_BMC_RECOVERY_MASK, true);
                }
            }
        }
    });

    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(60, pfr_main());

    /*
     * Check result
     */
    // Expect no recovery
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(0));

    // Expect one panic event for the recovery update
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), alt_u32(LAST_PANIC_BMC_UPDATE_INTENT));
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(1));

    // Expect no error on the firmware recovery update
    EXPECT_EQ(num_failed_update_attempts_from_bmc, alt_u32(0));
    // Authentication failure is likely from the first T-1
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(MAJOR_ERROR_BMC_AUTH_FAILED));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_AUTH_RECOVERY));

    // Both BMC and PCH should be out of reset
    EXPECT_TRUE(ut_is_bmc_out_of_reset());
    EXPECT_TRUE(ut_is_pch_out_of_reset());

    // Both BMC and PCH timers should be active
    EXPECT_TRUE(check_bit(WDT_BMC_TIMER_ADDR, U_TIMER_BANK_TIMER_ACTIVE_BIT));
    EXPECT_TRUE(check_bit(WDT_ACM_BIOS_TIMER_ADDR, U_TIMER_BANK_TIMER_ACTIVE_BIT));

    // Check T-1 entries
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_pch_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_bmc_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(2));

    // Active image should be untouched
    switch_spi_flash(SPI_FLASH_BMC);
    alt_u32 active_pfm_offset = get_ufm_pfr_data()->bmc_active_pfm;
    alt_u32* signed_active_pfm = get_spi_flash_ptr_with_offset(active_pfm_offset);
    EXPECT_TRUE(is_active_region_valid(signed_active_pfm));

    // Active PFM must remain the same
    for (alt_u32 word_i = 0; word_i < SIGNATURE_SIZE; word_i++)
    {
        ASSERT_EQ(full_image[word_i + (active_pfm_offset / sizeof(alt_u32))], signed_active_pfm[word_i]);
    }

    // Recovery image should now be valid
    EXPECT_TRUE(is_signature_valid((KCH_SIGNATURE*) signed_recovery_capsule));

    delete[] full_image;
}

/**
 * @brief When PCH image is corrupted, issues an out-of-band PCH active firmware update.
 * Active firmware update is expected to be blocked in current implementation. The current
 * limitation requires oob recovery update to recover a fully corrupted PCH flash.
 *
 * PCH flash content:
 * - A corrupted active image
 * - A corrupted recovery image
 * - Empty staging region
 *
 * BMC flash has valid active and recovery image.
 */
TEST_F(FWRecoveryThroughUpdateTest, test_recover_pch_flash_through_oob_active_fw_update)
{
    // Make the PCH flash empty with no firmware
    SYSTEM_MOCK::get()->reset_spi_flash_mock();
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, FULL_PFR_IMAGE_BMC_FILE, FULL_PFR_IMAGE_BMC_FILE_SIZE);
    alt_u32* flash_x86_ptr = get_spi_flash_ptr();

    // Load the entire image locally for comparison purpose
    alt_u32 *full_new_image = new alt_u32[FULL_PFR_IMAGE_PCH_V03P12_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(FULL_PFR_IMAGE_PCH_V03P12_FILE, full_new_image);

    /*
     * Flow preparation
     */
    ut_prep_nios_gpi_signals();
    ut_send_block_complete_chkpt_msg();

    // Insert the T0_OPERATIONS code block (break out of T0 loop upon entry)
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS_END_AFTER_50_ITERS);

    // In T0, send update capsule and recovery firwmare update intent
    SYSTEM_MOCK::get()->register_read_write_callback(
            [](SYSTEM_MOCK::READ_OR_WRITE read_or_write, void* addr, alt_u32 data) {
        if (read_or_write == SYSTEM_MOCK::READ_OR_WRITE::READ)
        {
            alt_u32* update_intent_addr = U_MAILBOX_AVMM_BRIDGE_ADDR + MB_BMC_UPDATE_INTENT;
            if (addr == update_intent_addr)
            {
                if ((read_from_mailbox(MB_PLATFORM_STATE) == PLATFORM_STATE_ENTER_T0) && (read_from_mailbox(MB_PANIC_EVENT_COUNT) == 0))
                {
                    // Expecting PCH flash failed authentication
                    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(MAJOR_ERROR_PCH_AUTH_FAILED));
                    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_AUTH_ALL_REGIONS));

                    // BMC should be out of reset, while PCH should be in reset
                    EXPECT_TRUE(ut_is_bmc_out_of_reset());
                    EXPECT_FALSE(ut_is_pch_out_of_reset());

                    // Load a pch fw update capsule that does not match the active image
                    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, SIGNED_CAPSULE_PCH_V03P12_FILE,
                            SIGNED_CAPSULE_PCH_V03P12_FILE_SIZE, get_ufm_pfr_data()->bmc_staging_region + BMC_STAGING_REGION_PCH_UPDATE_CAPSULE_OFFSET);

                    // Send recovery update capsule
                    SYSTEM_MOCK::get()->set_mem_word(update_intent_addr, MB_UPDATE_INTENT_PCH_ACTIVE_MASK, true);
                }
            }
        }
    });

    // Both active & recovery images are corrupted in PCH flash
    switch_spi_flash(SPI_FLASH_PCH);
    EXPECT_FALSE(is_active_region_valid(get_spi_active_pfm_ptr(SPI_FLASH_PCH)));
    EXPECT_FALSE(is_signature_valid((KCH_SIGNATURE*) get_spi_recovery_region_ptr(SPI_FLASH_PCH)));

    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(120, pfr_main());

    /*
     * Check result
     */
    // No recovery was performed
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(0));

    // Expect one panic event for the recovery update
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), alt_u32(LAST_PANIC_BMC_UPDATE_INTENT));
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(1));

    // Expect an error on the update
    EXPECT_EQ(num_failed_update_attempts_from_pch, alt_u32(0));
    EXPECT_EQ(num_failed_update_attempts_from_bmc, alt_u32(1));

    // No update/recovery is done; PCH flash still fails authentication
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(MAJOR_ERROR_PCH_AUTH_FAILED));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_AUTH_ALL_REGIONS));

    // BMC should be out of reset while PCH remains in reset.
    EXPECT_TRUE(ut_is_bmc_out_of_reset());
    EXPECT_FALSE(ut_is_pch_out_of_reset());

    // Check T-1 entries
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_pch_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_bmc_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(2));

    // Check PCH FW version
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_ACTIVE_MAJOR_VER), alt_u32(0xFF));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_ACTIVE_MINOR_VER), alt_u32(0xFF));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_RECOVERY_MAJOR_VER), alt_u32(0xFF));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_RECOVERY_MINOR_VER), alt_u32(0xFF));

    // Check BMC FW version
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_ACTIVE_MAJOR_VER), alt_u32(BMC_ACTIVE_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_ACTIVE_MINOR_VER), alt_u32(BMC_ACTIVE_PFM_MINOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_RECOVERY_MAJOR_VER), alt_u32(BMC_RECOVERY_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_RECOVERY_MINOR_VER), alt_u32(BMC_RECOVERY_PFM_MINOR_VER));

    // No update/recovery occurred on static regions
    for (alt_u32 region_i = 0; region_i < PCH_NUM_STATIC_REGIONS; region_i++)
    {
        for (alt_u32 word_i = testdata_pch_static_regions_start_addr[region_i] >> 2;
                word_i < testdata_pch_static_regions_end_addr[region_i] >> 2; word_i++)
        {
            ASSERT_EQ(flash_x86_ptr[word_i], alt_u32(0xFFFFFFFF));
        }
    }

    // No update/recovery occurred on dynamic regions
    for (alt_u32 region_i = 0; region_i < PCH_NUM_DYNAMIC_REGIONS; region_i++)
    {
        for (alt_u32 word_i = testdata_pch_dynamic_regions_start_addr[region_i] >> 2;
                word_i < testdata_pch_dynamic_regions_end_addr[region_i] >> 2; word_i++)
        {
            ASSERT_EQ(flash_x86_ptr[word_i], alt_u32(0xFFFFFFFF));
        }
    }

    // No update/recovery occurred on PFM
    alt_u32 pch_active_pfm_start = get_ufm_pfr_data()->pch_active_pfm;
    switch_spi_flash(SPI_FLASH_PCH);
    alt_u32 pch_active_pfm_end = pch_active_pfm_start + get_signed_payload_size(get_spi_active_pfm_ptr(SPI_FLASH_PCH));
    for (alt_u32 word_i = pch_active_pfm_start >> 2; word_i < pch_active_pfm_end >> 2; word_i++)
    {
        ASSERT_EQ(flash_x86_ptr[word_i], alt_u32(0xFFFFFFFF));
    }

    // No update/recovery occurred on recovery image
    alt_u32 start_i = get_recovery_region_offset(SPI_FLASH_PCH) >> 2;
    alt_u32 end_i = (get_recovery_region_offset(SPI_FLASH_PCH) + SPI_FLASH_BMC_RECOVERY_SIZE) >> 2;
    for (alt_u32 word_i = 0; word_i < (end_i - start_i); word_i++)
    {
        ASSERT_EQ(flash_x86_ptr[start_i + word_i], alt_u32(0xFFFFFFFF));
    }

    delete[] full_new_image;
}

/**
 * @brief When PCH image is corrupted, issues an out-of-band PCH recovery firmware update.
 *
 * PCH flash content:
 * - A corrupted active image
 * - A corrupted recovery image
 * - Empty staging region
 *
 * BMC flash has valid active and recovery image.
 */
TEST_F(FWRecoveryThroughUpdateTest, test_recover_pch_flash_through_oob_recovery_fw_update)
{
    // Make the PCH flash empty with no firmware
    SYSTEM_MOCK::get()->reset_spi_flash_mock();
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, FULL_PFR_IMAGE_BMC_FILE, FULL_PFR_IMAGE_BMC_FILE_SIZE);
    alt_u32* flash_x86_ptr = get_spi_flash_ptr();

    // Load the entire image locally for comparison purpose
    alt_u32 *full_new_image = new alt_u32[FULL_PFR_IMAGE_PCH_V03P12_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(FULL_PFR_IMAGE_PCH_V03P12_FILE, full_new_image);

    alt_u32 *update_capsule = new alt_u32[SIGNED_CAPSULE_PCH_V03P12_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(SIGNED_CAPSULE_PCH_V03P12_FILE, update_capsule);

    /*
     * Flow preparation
     */
    ut_prep_nios_gpi_signals();
    ut_send_block_complete_chkpt_msg();

    // Insert the T0_OPERATIONS code block (break out of T0 loop upon entry)
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS_END_AFTER_50_ITERS);

    // In T0, send update capsule and recovery firwmare update intent
    SYSTEM_MOCK::get()->register_read_write_callback(
            [](SYSTEM_MOCK::READ_OR_WRITE read_or_write, void* addr, alt_u32 data) {
        if (read_or_write == SYSTEM_MOCK::READ_OR_WRITE::READ)
        {
            alt_u32* update_intent_addr = U_MAILBOX_AVMM_BRIDGE_ADDR + MB_BMC_UPDATE_INTENT;
            if (addr == update_intent_addr)
            {
                if ((read_from_mailbox(MB_PLATFORM_STATE) == PLATFORM_STATE_ENTER_T0) && (read_from_mailbox(MB_PANIC_EVENT_COUNT) == 0))
                {
                    // Expecting PCH flash failed authentication
                    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(MAJOR_ERROR_PCH_AUTH_FAILED));
                    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_AUTH_ALL_REGIONS));

                    // BMC should be out of reset, while PCH should be in reset
                    EXPECT_TRUE(ut_is_bmc_out_of_reset());
                    EXPECT_FALSE(ut_is_pch_out_of_reset());

                    // Load a pch fw update capsule that does not match the active image
                    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, SIGNED_CAPSULE_PCH_V03P12_FILE,
                            SIGNED_CAPSULE_PCH_V03P12_FILE_SIZE, get_ufm_pfr_data()->bmc_staging_region + BMC_STAGING_REGION_PCH_UPDATE_CAPSULE_OFFSET);

                    // Send recovery update capsule
                    SYSTEM_MOCK::get()->set_mem_word(update_intent_addr, MB_UPDATE_INTENT_PCH_RECOVERY_MASK, true);
                }
            }
        }
    });

    // Both active & recovery images are corrupted in PCH flash
    switch_spi_flash(SPI_FLASH_PCH);
    EXPECT_FALSE(is_active_region_valid(get_spi_active_pfm_ptr(SPI_FLASH_PCH)));
    EXPECT_FALSE(is_signature_valid((KCH_SIGNATURE*) get_spi_recovery_region_ptr(SPI_FLASH_PCH)));

    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(120, pfr_main());

    /*
     * Check result
     */
    // After active firmware update, the staged capsule would match active image.
    // Then in the subsequent authentication step, recovery image is recovered by copying staged capsule to recovery region.
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(LAST_RECOVERY_PCH_RECOVERY));
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(1));

    // Expect 2 panic events for the recovery update
    // 1st: oob recovery update (LAST_PANIC_BMC_UPDATE_INTENT)
    // 2nd: completing the oob recovery update (LAST_PANIC_PCH_UPDATE_INTENT)
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), alt_u32(LAST_PANIC_PCH_UPDATE_INTENT));
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(2));

    // Expect no error on the firmware recovery update
    EXPECT_EQ(num_failed_update_attempts_from_pch, alt_u32(0));
    EXPECT_EQ(num_failed_update_attempts_from_bmc, alt_u32(0));

    // Only recovery region failed authentication in the T-1 cycle after active img has been updated
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(MAJOR_ERROR_PCH_AUTH_FAILED));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_AUTH_RECOVERY));

    // Both BMC and PCH should be out of reset
    EXPECT_TRUE(ut_is_bmc_out_of_reset());
    EXPECT_TRUE(ut_is_pch_out_of_reset());

    // Check T-1 entries
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_pch_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_bmc_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(3));

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

    // Verify updated recovery capsule
    alt_u32 capsule_size = get_signed_payload_size(update_capsule);
    alt_u32 start_i = get_recovery_region_offset(SPI_FLASH_PCH) >> 2;
    alt_u32 end_i = (get_recovery_region_offset(SPI_FLASH_PCH) + capsule_size) >> 2;
    for (alt_u32 word_i = 0; word_i < (end_i - start_i); word_i++)
    {
        ASSERT_EQ(update_capsule[word_i], flash_x86_ptr[start_i + word_i]);
    }

    delete[] full_new_image;
    delete[] update_capsule;
}
