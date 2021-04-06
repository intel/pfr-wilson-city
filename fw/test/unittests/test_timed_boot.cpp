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

class PFRTimedBootTest : public testing::Test
{
public:
    virtual void SetUp() {
        SYSTEM_MOCK::get()->reset();
        // Provision UFM data
        SYSTEM_MOCK::get()->provision_ufm_data(UFM_PFR_DATA_EXAMPLE_KEY_FILE);
        // Skip all T-1 operations
        SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::SKIP_TMIN1_OPERATIONS);

        // Reset Nios firmware
        ut_reset_nios_fw();
    }

    virtual void TearDown() {}
};

TEST_F(PFRTimedBootTest, test_simple_timed_boot_flow)
{
    // Timed boot is only enabled in provisioned state.
    EXPECT_TRUE(is_ufm_provisioned());

    ut_prep_nios_gpi_signals();

    // Insert the T0_TIMED_BOOT code block (break out of T0 loop when all timer has stopped)
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_TIMED_BOOT);

    ut_send_block_complete_chkpt_msg();

    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(1, pfr_main());

    // Check observed vs expected global_state
    EXPECT_EQ(ut_get_global_state(), (alt_u32) PLATFORM_STATE_T0_BOOT_COMPLETE);
}

/**
 * @brief This test simulates the multi-level secure boot of ACM and BIOS.
 */
TEST_F(PFRTimedBootTest, test_pch_boot_step_by_step)
{
    // Setup
    ut_prep_nios_gpi_signals();
    // Ends T0 loop after 1 iteration
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS);

    // Skip BMC timer in this test
    wdt_boot_status |= WDT_BMC_BOOT_DONE_MASK;

    // Boot PCH
    tmin1_boot_pch();

    // Loop 1
    perform_t0_operations();

    // ME timer should be completed
    EXPECT_TRUE(wdt_boot_status & WDT_ME_BOOT_DONE_MASK);
    EXPECT_TRUE((read_from_mailbox(MB_PLATFORM_STATE) == PLATFORM_STATE_T0_ME_BOOTED) ||
            (read_from_mailbox(MB_PLATFORM_STATE) == PLATFORM_STATE_T0_BMC_BOOTED));

    // Indicate entry to T0 (PLTRST should be toggled by this)
    // Note that this status usually comes before any boot done status.
    // Inserting here for testing purpose only
    write_to_mailbox(MB_PLATFORM_STATE, PLATFORM_STATE_ENTER_T0);

    // Loop 2
    perform_t0_operations();

    // ACM timer should be started
    EXPECT_TRUE(IORD(WDT_ACM_BIOS_TIMER_ADDR, 0) & U_TIMER_BANK_TIMER_ACTIVE_MASK);
    EXPECT_TRUE(IORD(WDT_ACM_BIOS_TIMER_ADDR, 0) & U_TIMER_BANK_TIMER_VALUE_MASK);

    // Send ACM BOOT_START
    write_to_mailbox(MB_PLATFORM_STATE, PLATFORM_STATE_T0_ME_BOOTED);
    write_to_mailbox(MB_ACM_CHECKPOINT, MB_CHKPT_START);

    // Loop 3
    perform_t0_operations();

    // ACM timer should be restarted
    EXPECT_TRUE(IORD(WDT_ACM_BIOS_TIMER_ADDR, 0) & U_TIMER_BANK_TIMER_ACTIVE_MASK);
    EXPECT_TRUE(IORD(WDT_ACM_BIOS_TIMER_ADDR, 0) & U_TIMER_BANK_TIMER_VALUE_MASK);

    // Send ACM BOOT_DONE
    // Nios used to process BOOT_DONE from ACM. This checkpoint message is now being ignored.
    write_to_mailbox(MB_ACM_CHECKPOINT, MB_CHKPT_COMPLETE);

    // Loop 4
    perform_t0_operations();

    // ACM timer shouldn't be turned off yet
    EXPECT_TRUE(IORD(WDT_ACM_BIOS_TIMER_ADDR, 0) & U_TIMER_BANK_TIMER_ACTIVE_MASK);
    EXPECT_TRUE(IORD(WDT_ACM_BIOS_TIMER_ADDR, 0) & U_TIMER_BANK_TIMER_VALUE_MASK);
    EXPECT_FALSE(wdt_boot_status & WDT_ACM_BOOT_DONE_MASK);
    EXPECT_NE(read_from_mailbox(MB_PLATFORM_STATE), PLATFORM_STATE_T0_ACM_BOOTED);

    // Send IBB BOOT_START
    write_to_mailbox(MB_BIOS_CHECKPOINT, MB_CHKPT_START);

    // Loop 5
    perform_t0_operations();

    // ACM timer is done; BIOS IBB should start now
    EXPECT_TRUE(IORD(WDT_ACM_BIOS_TIMER_ADDR, 0) & U_TIMER_BANK_TIMER_ACTIVE_MASK);
    EXPECT_TRUE(IORD(WDT_ACM_BIOS_TIMER_ADDR, 0) & U_TIMER_BANK_TIMER_VALUE_MASK);
    EXPECT_TRUE(wdt_boot_status & WDT_ACM_BOOT_DONE_MASK);
    EXPECT_EQ(read_from_mailbox(MB_PLATFORM_STATE), PLATFORM_STATE_T0_ACM_BOOTED);

    // Send IBB PAUSE_BLOCK
    write_to_mailbox(MB_BIOS_CHECKPOINT, MB_CHKPT_PAUSE);

    // Loop 6
    perform_t0_operations();

    // BIOS IBB timer should be paused
    EXPECT_FALSE(IORD(WDT_ACM_BIOS_TIMER_ADDR, 0) & U_TIMER_BANK_TIMER_ACTIVE_MASK);
    alt_u32 bios_timer_value = IORD(WDT_ACM_BIOS_TIMER_ADDR, 0);
    EXPECT_TRUE(bios_timer_value);
    EXPECT_EQ(read_from_mailbox(MB_PLATFORM_STATE), PLATFORM_STATE_T0_ACM_BOOTED);

    // Making sure the timer is actually paused
    sleep(1);
    EXPECT_EQ(bios_timer_value, IORD(WDT_ACM_BIOS_TIMER_ADDR, 0));

    // Send IBB RESUME_BLOCK
    write_to_mailbox(MB_BIOS_CHECKPOINT, MB_CHKPT_RESUME);

    // Loop 7
    perform_t0_operations();

    // BIOS IBB timer should be resumed
    EXPECT_TRUE(IORD(WDT_ACM_BIOS_TIMER_ADDR, 0) & U_TIMER_BANK_TIMER_ACTIVE_MASK);
    bios_timer_value = IORD(WDT_ACM_BIOS_TIMER_ADDR, 0) & U_TIMER_BANK_TIMER_VALUE_MASK;
    EXPECT_TRUE(bios_timer_value);
    EXPECT_EQ(read_from_mailbox(MB_PLATFORM_STATE), PLATFORM_STATE_T0_ACM_BOOTED);

    // Making sure the timer is counting down
    sleep(1);
    EXPECT_GE(bios_timer_value, IORD(WDT_ACM_BIOS_TIMER_ADDR, 0) & U_TIMER_BANK_TIMER_VALUE_MASK);

    // Send IBB BOOT_DONE
    write_to_mailbox(MB_BIOS_CHECKPOINT, MB_CHKPT_COMPLETE);

    // Loop 8
    perform_t0_operations();

    // BIOS IBB timer is done; BIOS OBB should start now
    EXPECT_TRUE(IORD(WDT_ACM_BIOS_TIMER_ADDR, 0) & U_TIMER_BANK_TIMER_ACTIVE_MASK);
    EXPECT_TRUE(IORD(WDT_ACM_BIOS_TIMER_ADDR, 0) & U_TIMER_BANK_TIMER_VALUE_MASK);
    EXPECT_TRUE(wdt_boot_status & WDT_IBB_BOOT_DONE_MASK);
    EXPECT_EQ(read_from_mailbox(MB_PLATFORM_STATE), PLATFORM_STATE_T0_ACM_BOOTED);

    // Send OBB BOOT_START
    write_to_mailbox(MB_BIOS_CHECKPOINT, MB_CHKPT_START);

    // Loop 9
    perform_t0_operations();

    // BIOS OBB timer should be restarted
    EXPECT_TRUE(IORD(WDT_ACM_BIOS_TIMER_ADDR, 0) & U_TIMER_BANK_TIMER_ACTIVE_MASK);
    EXPECT_TRUE(IORD(WDT_ACM_BIOS_TIMER_ADDR, 0) & U_TIMER_BANK_TIMER_VALUE_MASK);
    EXPECT_EQ(read_from_mailbox(MB_PLATFORM_STATE), PLATFORM_STATE_T0_ACM_BOOTED);

    // Send OBB PAUSE_BLOCK
    write_to_mailbox(MB_BIOS_CHECKPOINT, MB_CHKPT_PAUSE);

    // Loop 10
    perform_t0_operations();

    // BIOS OBB timer should be paused
    EXPECT_FALSE(IORD(WDT_ACM_BIOS_TIMER_ADDR, 0) & U_TIMER_BANK_TIMER_ACTIVE_MASK);
    bios_timer_value = IORD(WDT_ACM_BIOS_TIMER_ADDR, 0);
    EXPECT_TRUE(bios_timer_value);
    EXPECT_EQ(read_from_mailbox(MB_PLATFORM_STATE), PLATFORM_STATE_T0_ACM_BOOTED);

    // Making sure the timer is actually paused
    sleep(1);
    EXPECT_EQ(bios_timer_value, IORD(WDT_ACM_BIOS_TIMER_ADDR, 0));

    // Send OBB RESUME_BLOCK
    write_to_mailbox(MB_BIOS_CHECKPOINT, MB_CHKPT_RESUME);

    // Loop 11
    perform_t0_operations();

    // BIOS OBB timer should be resumed
    EXPECT_TRUE(IORD(WDT_ACM_BIOS_TIMER_ADDR, 0) & U_TIMER_BANK_TIMER_ACTIVE_MASK);
    bios_timer_value = IORD(WDT_ACM_BIOS_TIMER_ADDR, 0) & U_TIMER_BANK_TIMER_VALUE_MASK;
    EXPECT_TRUE(bios_timer_value);
    EXPECT_TRUE((read_from_mailbox(MB_PLATFORM_STATE) == PLATFORM_STATE_T0_ACM_BOOTED));

    // Making sure the timer is counting down
    sleep(1);
    EXPECT_GE(bios_timer_value, IORD(WDT_ACM_BIOS_TIMER_ADDR, 0) & U_TIMER_BANK_TIMER_VALUE_MASK);

    // Send OBB BOOT_DONE
    write_to_mailbox(MB_BIOS_CHECKPOINT, MB_CHKPT_COMPLETE);

    // Loop 12
    perform_t0_operations();

    // Time boot is completed
    EXPECT_EQ(IORD(WDT_ACM_BIOS_TIMER_ADDR, 0), alt_u32(0));
    EXPECT_TRUE(wdt_boot_status & WDT_OBB_BOOT_DONE_MASK);
    EXPECT_EQ(read_from_mailbox(MB_PLATFORM_STATE), PLATFORM_STATE_T0_BOOT_COMPLETE);
}

