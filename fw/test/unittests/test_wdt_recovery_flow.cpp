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


class WDTRecoveryFlowTest : public testing::Test
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

        // Load the entire image to flash
        sys->load_to_flash(SPI_FLASH_BMC, FULL_PFR_IMAGE_BMC_FILE, FULL_PFR_IMAGE_BMC_FILE_SIZE);
        sys->load_to_flash(SPI_FLASH_PCH, FULL_PFR_IMAGE_PCH_FILE, FULL_PFR_IMAGE_PCH_FILE_SIZE);

        // Reset Nios firmware
        ut_reset_nios_fw();
    }

    virtual void TearDown() {}
};

TEST_F(WDTRecoveryFlowTest, test_bmc_wdt_timeout_recovery)
{
    // Flow preparation
    ut_prep_nios_gpi_signals();
    while (!check_ready_for_nios_start());

    // BMC timer should be enabled
    EXPECT_TRUE(wdt_enable_status & WDT_ENABLE_BMC_TIMER_MASK);

    /*
     * First timeout
     */
    // Start the timer with 0ms on the countdown, i.e. it's expired already
    start_timer(WDT_BMC_TIMER_ADDR, 0);
    // Run the T0 watchdog timers' handlers
    // Expect it to catch the timeout and perform recovery
    // After getting platform back to T0, this function should be done
    watchdog_routine();

    // Check the platform state and other status
    EXPECT_NE(read_from_mailbox(MB_PLATFORM_STATE), PLATFORM_STATE_T0_BMC_BOOTED);
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(1));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), LAST_PANIC_BMC_WDT_EXPIRED);
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(1));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), LAST_RECOVERY_BMC_LAUNCH_FAIL);

    // Don't expect any error for now
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(0));

    // Recovery level should now be at 2
    EXPECT_EQ(get_fw_recovery_level(SPI_FLASH_BMC), alt_u32(SPI_REGION_PROTECT_MASK_RECOVER_ON_SECOND_RECOVERY));

    /*
     * Second timeout
     */
    // Start the timer with 0ms on the countdown, i.e. it's expired already
    start_timer(WDT_BMC_TIMER_ADDR, 0);
    // Run the T0 watchdog timers' handlers
    watchdog_routine();

    // Check the platform state and other status
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(2));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), LAST_PANIC_BMC_WDT_EXPIRED);
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(2));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), LAST_RECOVERY_BMC_LAUNCH_FAIL);

    // Recovery level should now be at 3
    EXPECT_EQ(get_fw_recovery_level(SPI_FLASH_BMC), alt_u32(SPI_REGION_PROTECT_MASK_RECOVER_ON_THIRD_RECOVERY));

    /*
     * Third timeout
     */
    // Start the timer with 0ms on the countdown, i.e. it's expired already
    start_timer(WDT_BMC_TIMER_ADDR, 0);
    // Run the T0 watchdog timers' handlers
    watchdog_routine();

    // Check the platform state and other status
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(3));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), LAST_PANIC_BMC_WDT_EXPIRED);
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(3));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), LAST_RECOVERY_BMC_LAUNCH_FAIL);

    // Recovery level should now exceed the maximum level (3)
    EXPECT_FALSE(get_fw_recovery_level(SPI_FLASH_BMC) & SPI_REGION_PROTECT_MASK_RECOVER_BITS);

    /*
     * Fourth timeout
     * Nios should just disable the BMC timer and let BMC hang
     */
    // Start the timer with 0ms on the countdown, i.e. it's expired already
    start_timer(WDT_BMC_TIMER_ADDR, 0);
    // Run the T0 watchdog timers' handlers
    watchdog_routine();

    // There should not be any more panic/recovery event
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(3));
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(3));

    // Recovery level should now exceed the maximum level (3)
    EXPECT_FALSE(get_fw_recovery_level(SPI_FLASH_BMC) & SPI_REGION_PROTECT_MASK_RECOVER_BITS);

    // BMC timer should be disabled
    EXPECT_FALSE(wdt_enable_status & WDT_ENABLE_BMC_TIMER_MASK);
}

TEST_F(WDTRecoveryFlowTest, test_acm_wdt_timeout_recovery)
{
    // Flow preparation
    ut_prep_nios_gpi_signals();
    while (!check_ready_for_nios_start());

    // BMC timer should be enabled
    EXPECT_TRUE(wdt_enable_status & WDT_ENABLE_ACM_BIOS_TIMER_MASK);

    /*
     * First timeout
     */
    // Start the timer with 0ms on the countdown, i.e. it's expired already
    start_timer(WDT_ACM_BIOS_TIMER_ADDR, 0);
    // Run the T0 watchdog timers' handlers
    // Expect it to catch the timeout and perform recovery
    // After getting platform back to T0, this function should be done
    watchdog_routine();

    // Check the platform state and other status
    EXPECT_EQ(read_from_mailbox(MB_PLATFORM_STATE), PLATFORM_STATE_ENTER_T0);
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(1));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), LAST_PANIC_ACM_BIOS_WDT_EXPIRED);
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(1));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), LAST_RECOVERY_ACM_LAUNCH_FAIL);

    // Don't expect any error for now
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(0));

    // Recovery level should now be at 2
    EXPECT_EQ(get_fw_recovery_level(SPI_FLASH_PCH), alt_u32(SPI_REGION_PROTECT_MASK_RECOVER_ON_SECOND_RECOVERY));

    /*
     * Second timeout
     */
    // Start the timer with 0ms on the countdown, i.e. it's expired already
    start_timer(WDT_ACM_BIOS_TIMER_ADDR, 0);
    // Run the T0 watchdog timers' handlers
    watchdog_routine();

    // Check the platform state and other status
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(2));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), LAST_PANIC_ACM_BIOS_WDT_EXPIRED);
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(2));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), LAST_RECOVERY_ACM_LAUNCH_FAIL);

    // Recovery level should now be at 3
    EXPECT_EQ(get_fw_recovery_level(SPI_FLASH_PCH), alt_u32(SPI_REGION_PROTECT_MASK_RECOVER_ON_THIRD_RECOVERY));

    /*
     * Third timeout
     */
    // Start the timer with 0ms on the countdown, i.e. it's expired already
    start_timer(WDT_ACM_BIOS_TIMER_ADDR, 0);
    // Run the T0 watchdog timers' handlers
    watchdog_routine();

    // Check the platform state and other status
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(3));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), LAST_PANIC_ACM_BIOS_WDT_EXPIRED);
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(3));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), LAST_RECOVERY_ACM_LAUNCH_FAIL);

    // Recovery level should now exceed the maximum level (3)
    EXPECT_FALSE(get_fw_recovery_level(SPI_FLASH_PCH) & SPI_REGION_PROTECT_MASK_RECOVER_BITS);

    /*
     * Fourth timeout
     * Nios should just disable the ACM/BIOS timer and let PCH hang
     */
    // Start the timer with 0ms on the countdown, i.e. it's expired already
    start_timer(WDT_ACM_BIOS_TIMER_ADDR, 0);
    // Run the T0 watchdog timers' handlers
    watchdog_routine();

    // There should not be any more panic/recovery event
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(3));
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(3));

    // Recovery level should now exceed the maximum level (3)
    EXPECT_FALSE(get_fw_recovery_level(SPI_FLASH_PCH) & SPI_REGION_PROTECT_MASK_RECOVER_BITS);

    // BMC timer should be disabled
    EXPECT_FALSE(wdt_enable_status & WDT_ENABLE_ACM_BIOS_TIMER_MASK);
}

TEST_F(WDTRecoveryFlowTest, test_me_wdt_timeout_recovery)
{
    // Flow preparation
    ut_prep_nios_gpi_signals();
    while (!check_ready_for_nios_start());

    // BMC timer should be enabled
    EXPECT_TRUE(wdt_enable_status & WDT_ENABLE_ME_TIMER_MASK);

    /*
     * First timeout
     */
    // Start the timer with 0ms on the countdown, i.e. it's expired already
    start_timer(WDT_ME_TIMER_ADDR, 0);
    // Run the T0 watchdog timers' handlers
    // Expect it to catch the timeout and perform recovery
    // After getting platform back to T0, this function should be done
    watchdog_routine();

    // Check the platform state and other status
    EXPECT_EQ(read_from_mailbox(MB_PLATFORM_STATE), PLATFORM_STATE_ENTER_T0);
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(1));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), LAST_PANIC_ME_WDT_EXPIRED);
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(1));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), LAST_RECOVERY_ME_LAUNCH_FAIL);

    // Don't expect any error for now
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(0));

    // Recovery level should now be at 2
    EXPECT_EQ(get_fw_recovery_level(SPI_FLASH_PCH), alt_u32(SPI_REGION_PROTECT_MASK_RECOVER_ON_SECOND_RECOVERY));

    /*
     * Second timeout
     */
    // Start the timer with 0ms on the countdown, i.e. it's expired already
    start_timer(WDT_ME_TIMER_ADDR, 0);
    // Run the T0 watchdog timers' handlers
    watchdog_routine();

    // Check the platform state and other status
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(2));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), LAST_PANIC_ME_WDT_EXPIRED);
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(2));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), LAST_RECOVERY_ME_LAUNCH_FAIL);

    // Recovery level should now be at 3
    EXPECT_EQ(get_fw_recovery_level(SPI_FLASH_PCH), alt_u32(SPI_REGION_PROTECT_MASK_RECOVER_ON_THIRD_RECOVERY));

    /*
     * Third timeout
     */
    // Start the timer with 0ms on the countdown, i.e. it's expired already
    start_timer(WDT_ME_TIMER_ADDR, 0);
    // Run the T0 watchdog timers' handlers
    watchdog_routine();

    // Check the platform state and other status
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(3));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), LAST_PANIC_ME_WDT_EXPIRED);
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(3));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), LAST_RECOVERY_ME_LAUNCH_FAIL);

    // Recovery level should now exceed the maximum level (3)
    EXPECT_FALSE(get_fw_recovery_level(SPI_FLASH_PCH) & SPI_REGION_PROTECT_MASK_RECOVER_BITS);

    /*
     * Fourth timeout
     * Nios should just disable the ACM/BIOS timer and let PCH hang
     */
    // Start the timer with 0ms on the countdown, i.e. it's expired already
    start_timer(WDT_ME_TIMER_ADDR, 0);
    // Run the T0 watchdog timers' handlers
    watchdog_routine();

    // There should not be any more panic/recovery event
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(3));
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(3));

    // Recovery level should now exceed the maximum level (3)
    EXPECT_FALSE(get_fw_recovery_level(SPI_FLASH_PCH) & SPI_REGION_PROTECT_MASK_RECOVER_BITS);

    // BMC timer should be disabled
    EXPECT_FALSE(wdt_enable_status & WDT_ENABLE_ME_TIMER_MASK);
}


/**
 * @brief Test how CPLD reacts to BIOS hang.
 *
 * The PCH image used in this test case was taken from HSD case 1507855229. The BIOS
 * regions (0x37F0000 - 0x3800000; 0x3850000 - 0x3880000; 0x3880000 - 0x3900000) are
 * configured to only be recovered on second recovery. There are checks to make sure this is what happened.
 *
 * In this test case, we let BIOS watchdog timeout four times. On the fourth timeout, CPLD should just let BIOS hang.
 */
TEST_F(WDTRecoveryFlowTest, test_bios_timeout_recovery_from_case_1507855229)
{
    /*
     * Prepare PCH flash
     */
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_PCH,
            FULL_PFR_IMAGE_PCH_ONLY_RECOVER_BIOS_REGIONS_ON_2ND_LEVEL_FILE,
            FULL_PFR_IMAGE_PCH_ONLY_RECOVER_BIOS_REGIONS_ON_2ND_LEVEL_FILE_SIZE);

    // Corrupt the target BIOS regions to see the wdt recovery
    // These regions are writable too, so authentication won't file.
    switch_spi_flash(SPI_FLASH_PCH);
    alt_u32* spi_ptr;

    spi_ptr = get_spi_flash_ptr_with_offset(0x37F0000);
    alt_u32 word_at_offset_37F00000 = *spi_ptr;
    EXPECT_NE(word_at_offset_37F00000, 0xDEADBEEF);
    *spi_ptr = 0xDEADBEEF;

    spi_ptr = get_spi_flash_ptr_with_offset(0x3850000);
    alt_u32 word_at_offset_38500000 = *spi_ptr;
    EXPECT_NE(word_at_offset_38500000, 0xDEADBEEF);
    *spi_ptr = 0xDEADBEEF;

    spi_ptr = get_spi_flash_ptr_with_offset(0x3880000);
    alt_u32 word_at_offset_38800000 = *spi_ptr;
    EXPECT_NE(word_at_offset_38800000, 0xDEADBEEF);
    *spi_ptr = 0xDEADBEEF;

    /*
     * Flow preparation
     */
    ut_prep_nios_gpi_signals();

    // Add some code blocks to test this flow path
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS);

    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(120, pfr_main());

    // Let other components complete boot
    write_to_mailbox(MB_BMC_CHECKPOINT, MB_CHKPT_COMPLETE);
    write_to_mailbox(MB_ACM_CHECKPOINT, MB_CHKPT_COMPLETE);
    write_to_mailbox(MB_BIOS_CHECKPOINT, MB_CHKPT_START);
    perform_t0_operations();

    /*
     * Check the flow setup
     */
    // There shouldn't be any error/recovery/panic event yet
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(0));

    // All but IBB/OBB should have completed boot.
    EXPECT_TRUE(wdt_boot_status & WDT_ACM_BOOT_DONE_MASK);
    EXPECT_TRUE(wdt_boot_status & WDT_ME_BOOT_DONE_MASK);
    EXPECT_TRUE(wdt_boot_status & WDT_BMC_BOOT_DONE_MASK);
    EXPECT_FALSE(wdt_boot_status & WDT_IBB_BOOT_DONE_MASK);
    EXPECT_FALSE(wdt_boot_status & WDT_OBB_BOOT_DONE_MASK);

    // There shouldn't be any entry to T-1 yet.
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(1));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_bmc_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_pch_only_counter(), alt_u32(0));

    /*
     * First timeout
     */
    // Simulate a BIOS hang by forcing the BIOS watchdog timer to timeout.
    IOWR(WDT_ACM_BIOS_TIMER_ADDR, 0, U_TIMER_BANK_TIMER_ACTIVE_MASK);

    // Run a T0 loop
    perform_t0_operations();

    // The target regions shouldn't be recovered yet
    switch_spi_flash(SPI_FLASH_PCH);

    spi_ptr = get_spi_flash_ptr_with_offset(0x37F0000);
    EXPECT_EQ(*spi_ptr, 0xDEADBEEF);

    spi_ptr = get_spi_flash_ptr_with_offset(0x3850000);
    EXPECT_EQ(*spi_ptr, 0xDEADBEEF);

    spi_ptr = get_spi_flash_ptr_with_offset(0x3880000);
    EXPECT_EQ(*spi_ptr, 0xDEADBEEF);

    /*
     * Second timeout
     */
    // Simulate a BIOS hang by forcing the BIOS watchdog timer to timeout.
    IOWR(WDT_ACM_BIOS_TIMER_ADDR, 0, U_TIMER_BANK_TIMER_ACTIVE_MASK);

    // Run a T0 loop
    perform_t0_operations();

    // The target regions be recovered now
    switch_spi_flash(SPI_FLASH_PCH);

    spi_ptr = get_spi_flash_ptr_with_offset(0x37F0000);
    EXPECT_EQ(*spi_ptr, word_at_offset_37F00000);

    spi_ptr = get_spi_flash_ptr_with_offset(0x3850000);
    EXPECT_EQ(*spi_ptr, word_at_offset_38500000);

    spi_ptr = get_spi_flash_ptr_with_offset(0x3880000);
    EXPECT_EQ(*spi_ptr, word_at_offset_38800000);

    /*
     * Third timeout
     * Normally, BIOS should boot up fine now after the recovery
     * For test coverage sake, we are going to corrupt the BIOS regions again.
     * Those regions should not be recovered on this third recovery, since the corresponding PFM bit setting is not set.
     */
    // Corrupt the target regions again
    switch_spi_flash(SPI_FLASH_PCH);

    spi_ptr = get_spi_flash_ptr_with_offset(0x37F0000);
    *spi_ptr = 0xDEADBEEF;

    spi_ptr = get_spi_flash_ptr_with_offset(0x3850000);
    *spi_ptr = 0xDEADBEEF;

    spi_ptr = get_spi_flash_ptr_with_offset(0x3880000);
    *spi_ptr = 0xDEADBEEF;

    // Simulate a BIOS hang by forcing the BIOS watchdog timer to timeout.
    IOWR(WDT_ACM_BIOS_TIMER_ADDR, 0, U_TIMER_BANK_TIMER_ACTIVE_MASK);

    // Run a T0 loop
    perform_t0_operations();

    // The target regions be not be recovered on the third recovery. 
    switch_spi_flash(SPI_FLASH_PCH);

    spi_ptr = get_spi_flash_ptr_with_offset(0x37F0000);
    EXPECT_EQ(*spi_ptr, 0xDEADBEEF);

    spi_ptr = get_spi_flash_ptr_with_offset(0x3850000);
    EXPECT_EQ(*spi_ptr, 0xDEADBEEF);

    spi_ptr = get_spi_flash_ptr_with_offset(0x3880000);
    EXPECT_EQ(*spi_ptr, 0xDEADBEEF);

    /*
     * Fourth timeout.
     * CPLD should let BIOS hang.
     */
    // Simulate a BIOS hang by forcing the BIOS watchdog timer to timeout.
    IOWR(WDT_ACM_BIOS_TIMER_ADDR, 0, U_TIMER_BANK_TIMER_ACTIVE_MASK);

    // Run a T0 loop
    perform_t0_operations();

    /*
     * More checks
     */
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(3));
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(3));
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(0));

    // We should transition from T0 to T-1 3 times in total.
    // Also, there's 1 entry to T-1 from the initial pfr_main() call.
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(4));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_bmc_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_pch_only_counter(), alt_u32(0));
}

