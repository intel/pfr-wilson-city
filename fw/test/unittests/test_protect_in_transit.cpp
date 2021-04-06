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


class PFRProtectInTransitTest : public testing::Test
{
public:
    virtual void SetUp() {
        SYSTEM_MOCK* sys = SYSTEM_MOCK::get();
        // Reset system mocks and SPI flash
        sys->reset();
        sys->reset_spi_flash_mock();

        // Perform provisioning
        sys->provision_ufm_data(UFM_PFR_DATA_EXAMPLE_KEY_FILE);

        // Load BMC and PCH flash
        sys->load_to_flash(SPI_FLASH_BMC, FULL_PFR_IMAGE_BMC_FILE, FULL_PFR_IMAGE_BMC_FILE_SIZE);
        sys->load_to_flash(SPI_FLASH_PCH, FULL_PFR_IMAGE_PCH_FILE, FULL_PFR_IMAGE_PCH_FILE_SIZE);

        /*
         * Flow preparation
         */
        // Skip flash authentication in T-1 to save time
        sys->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::SKIP_FLASH_AUTHENTICATION);

        // Reset Nios firmware
        ut_reset_nios_fw();

        // Disable WDTs
        ut_disable_watchdog_timers();

        // Allow nios to start
        ut_prep_nios_gpi_signals();
    }

    virtual void TearDown() {}
};

/**
 * Test sending ENABLE_PIT_L1 command without sending PIT ID first
 * Expect to see "Command Error" bit set in the UFM/Provision status register
 */
TEST_F(PFRProtectInTransitTest, test_enable_pit_l1_without_provisioning_pit_id)
{
    /*
     * Flow preparation
     */
    // Insert the T0_OPERATIONS code block (break out of T0 loop)
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS);

    /*
     * Send enable PIT L1 command
     */
    ut_send_in_ufm_command(MB_UFM_PROV_ENABLE_PIT_L1);

    /*
     * Execute the Nios firmware flow
     */
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(1, pfr_main());

    /*
     * Check results
     */
    EXPECT_TRUE(ut_check_ufm_prov_status(MB_UFM_PROV_CMD_DONE_MASK));
    // Expect there to be error with this command
    EXPECT_TRUE(ut_check_ufm_prov_status(MB_UFM_PROV_CMD_ERROR_MASK));
    // PIT L1 should not be enabled
    EXPECT_FALSE(check_ufm_status(UFM_STATUS_PIT_L1_ENABLE_BIT_MASK));
}

/**
 * Test sending ENABLE_PIT_L2 command without enabling PIT L1 first
 * Expect to see "Command Error" bit set in the UFM/Provision status register
 */
TEST_F(PFRProtectInTransitTest, test_enable_pit_l2_without_pit_l1)
{
    /*
     * Flow preparation
     */
    // Insert the T0_OPERATIONS code block (break out of T0 loop)
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS);

    /*
     * Send enable PIT L2 command
     */
    ut_send_in_ufm_command(MB_UFM_PROV_ENABLE_PIT_L2);

    /*
     * Execute the Nios firmware flow
     */
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(1, pfr_main());

    /*
     * Check results
     */
    EXPECT_TRUE(ut_check_ufm_prov_status(MB_UFM_PROV_CMD_DONE_MASK));
    // Expect there to be error with this command
    EXPECT_TRUE(ut_check_ufm_prov_status(MB_UFM_PROV_CMD_ERROR_MASK));
    // PIT L2 should not be enabled
    EXPECT_FALSE(check_ufm_status(UFM_STATUS_PIT_L2_ENABLE_BIT_MASK));
}

TEST_F(PFRProtectInTransitTest, test_provision_pit_id_and_enable_pit_l1)
{
    /*
     * Flow preparation
     */
    // Insert the T0_OPERATIONS code block (break out of T0 loop)
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS);

    /*
     * Send PIT ID provisioning request
     */
    const alt_u8 pit_id[8] = {
            0xec, 0xa0, 0xb4, 0xed, 0x14, 0x12, 0xea, 0xe6,
    };
    for (int i = 0; i < 8; i++)
    {
        IOWR(U_MAILBOX_AVMM_BRIDGE_ADDR, MB_UFM_WRITE_FIFO, pit_id[i]);
    }

    ut_send_in_ufm_command(MB_UFM_PROV_PIT_ID);

    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(1, pfr_main());

    /*
     * Check results
     */
    // Wait until CPLD finished with the request
    EXPECT_TRUE(ut_check_ufm_prov_status(MB_UFM_PROV_CMD_DONE_MASK));
    // Make sure there's no error before moving on
    EXPECT_FALSE(ut_check_ufm_prov_status(MB_UFM_PROV_CMD_ERROR_MASK));

    // Check UFM status
    EXPECT_FALSE(check_ufm_status(UFM_STATUS_PIT_L1_ENABLE_BIT_MASK));
    EXPECT_FALSE(check_ufm_status(UFM_STATUS_PIT_L2_ENABLE_BIT_MASK));

    /*
     * Send enable PIT L1 request
     */
    // Send in enable PIT L1 UFM command
    ut_send_in_ufm_command(MB_UFM_PROV_ENABLE_PIT_L1);

    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(1, pfr_main());

    /*
     * Check results
     */
    // Wait until CPLD finished with the request
    EXPECT_TRUE(ut_check_ufm_prov_status(MB_UFM_PROV_CMD_DONE_MASK));
    // Make sure there's no error before moving on
    EXPECT_FALSE(ut_check_ufm_prov_status(MB_UFM_PROV_CMD_ERROR_MASK));
    // UFM status register should say that PIT L1 is enabled
    EXPECT_TRUE(ut_check_ufm_prov_status(MB_UFM_PROV_UFM_PIT_L1_ENABLED_MASK));

    // Check UFM status
    EXPECT_TRUE(check_ufm_status(UFM_STATUS_PIT_L1_ENABLE_BIT_MASK));
    EXPECT_FALSE(check_ufm_status(UFM_STATUS_PIT_L2_ENABLE_BIT_MASK));

    /*
     * Simulate power cycle of the platform.
     * There's no change to the RFNVRAM. Platform should be transitioned to T0 mode.
     */
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(1, pfr_main());

    // Check UFM status
    EXPECT_TRUE(check_ufm_status(UFM_STATUS_PIT_L1_ENABLE_BIT_MASK));
    EXPECT_FALSE(check_ufm_status(UFM_STATUS_PIT_L2_ENABLE_BIT_MASK));

    // Nios should be in T0 mode
    EXPECT_EQ(ut_get_global_state(), (alt_u32) PLATFORM_STATE_ENTER_T0);
}

TEST_F(PFRProtectInTransitTest, test_pit_l1_lockdown)
{
    /*
     * Flow preparation
     */
    // Insert the T0_OPERATIONS code block (break out of T0 loop)
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS);

    /*
     * Send PIT ID provisioning request
     */
    const alt_u8 pit_id[8] = {
            0xec, 0xa0, 0xb4, 0xed, 0x14, 0x12, 0xea, 0xe6,
    };
    for (int i = 0; i < 8; i++)
    {
        IOWR(U_MAILBOX_AVMM_BRIDGE_ADDR, MB_UFM_WRITE_FIFO, pit_id[i]);
    }

    ut_send_in_ufm_command(MB_UFM_PROV_PIT_ID);

    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(1, pfr_main());

    /*
     * Check results
     */
    // Wait until CPLD finished with the request
    EXPECT_TRUE(ut_check_ufm_prov_status(MB_UFM_PROV_CMD_DONE_MASK));
    // Make sure there's no error before moving on
    EXPECT_FALSE(ut_check_ufm_prov_status(MB_UFM_PROV_CMD_ERROR_MASK));

    // Check UFM status
    EXPECT_FALSE(check_ufm_status(UFM_STATUS_PIT_L1_ENABLE_BIT_MASK));
    EXPECT_FALSE(check_ufm_status(UFM_STATUS_PIT_L2_ENABLE_BIT_MASK));

    /*
     * Send enable PIT L1 request
     */
    // Send in enable PIT L1 UFM command
    ut_send_in_ufm_command(MB_UFM_PROV_ENABLE_PIT_L1);

    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(1, pfr_main());

    /*
     * Simulate power cycle of the platform.
     * RFNVRAM ID has been cleared. Platform should be in lockdown state.
     */
    // Set asserts to throw as opposed to abort
    SYSTEM_MOCK::get()->set_assert_to_throw();
    // Expect that Nios firmware will stuck in the never_exit_loop.
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::THROW_FROM_NEVER_EXIT_LOOP);

    // Clear the PIT ID in RFNVRAM
    IOWR_32DIRECT(U_RFNVRAM_SMBUS_MASTER_ADDR, 0, 0x2DC);
    IOWR_32DIRECT(U_RFNVRAM_SMBUS_MASTER_ADDR, 0, RFNVRAM_PIT_ID_MSB_OFFSET);
    IOWR_32DIRECT(U_RFNVRAM_SMBUS_MASTER_ADDR, 0, RFNVRAM_PIT_ID_LSB_OFFSET);
    for (alt_u32 byte_i = 0; byte_i < RFNVRAM_PIT_ID_LENGTH - 1; byte_i++)
    {
        IOWR_32DIRECT(U_RFNVRAM_SMBUS_MASTER_ADDR, 0, 0);
    }
    IOWR_32DIRECT(U_RFNVRAM_SMBUS_MASTER_ADDR, 0, RFNVRAM_IDLE_MASK | 0);

    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(1, EXPECT_ANY_THROW({ pfr_main(); }));

    // Check UFM status
    EXPECT_TRUE(check_ufm_status(UFM_STATUS_PIT_L1_ENABLE_BIT_MASK));
    EXPECT_FALSE(check_ufm_status(UFM_STATUS_PIT_L2_ENABLE_BIT_MASK));

    // Nios should be in T0 mode
    EXPECT_EQ(ut_get_global_state(), (alt_u32) PLATFORM_STATE_PIT_L1_LOCKDOWN);
}

TEST_F(PFRProtectInTransitTest, test_pit_l2_happy_path)
{
    /*
     * Flow preparation
     */
    // Set asserts to throw as opposed to abort
    SYSTEM_MOCK::get()->set_assert_to_throw();
    // Expect that Nios firmware will stuck in the never_exit_loop.
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::THROW_FROM_NEVER_EXIT_LOOP);
    // Insert the T0_OPERATIONS code block (break out of T0 loop)
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS);

    /*
     * Step 1.
     * Provision PIT ID and enable PIT L1.
     * This is the prerequisite of enabling PIT L2
     */
    const alt_u8 pit_id[8] = {
            0xec, 0xa0, 0xb4, 0xed, 0x14, 0x12, 0xea, 0xe6,
    };
    for (int i = 0; i < 8; i++)
    {
        IOWR(U_MAILBOX_AVMM_BRIDGE_ADDR, MB_UFM_WRITE_FIFO, pit_id[i]);
    }

    ut_send_in_ufm_command(MB_UFM_PROV_PIT_ID);
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(1, pfr_main());

    ut_send_in_ufm_command(MB_UFM_PROV_ENABLE_PIT_L1);
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(1, pfr_main());

    // Make sure there's no error before moving on
    EXPECT_FALSE(ut_check_ufm_prov_status(MB_UFM_PROV_CMD_ERROR_MASK));

    // Check UFM status
    EXPECT_TRUE(check_ufm_status(UFM_STATUS_PIT_L1_ENABLE_BIT_MASK));
    EXPECT_FALSE(check_ufm_status(UFM_STATUS_PIT_L2_ENABLE_BIT_MASK));

    /*
     * Step 2.
     * Eanble PIT L2
     */
    // Send in enable PIT l2 UFM command
    ut_send_in_ufm_command(MB_UFM_PROV_ENABLE_PIT_L2);

    // Run PFR Main. Always run with the timeout
    // Expecting a throw from the never_exit_loop
    ASSERT_DURATION_LE(200, EXPECT_ANY_THROW({ pfr_main(); }));

    /*
     * Check results
     * Now the CPLD has hold the platform in T-1 mode after calculating the FW hash.
     */
    // Make sure there's no error before moving on
    EXPECT_FALSE(ut_check_ufm_prov_status(MB_UFM_PROV_CMD_ERROR_MASK));

    // Check UFM status
    EXPECT_TRUE(check_ufm_status(UFM_STATUS_PIT_L1_ENABLE_BIT_MASK));
    EXPECT_TRUE(check_ufm_status(UFM_STATUS_PIT_L2_ENABLE_BIT_MASK));
    EXPECT_TRUE(check_ufm_status(UFM_STATUS_PIT_HASH_STORED_BIT_MASK));

    alt_u8* ufm_pit_pch_fw_hash = (alt_u8*) get_ufm_pfr_data()->pit_pch_fw_hash;
    alt_u8* ufm_pit_bmc_fw_hash = (alt_u8*) get_ufm_pfr_data()->pit_bmc_fw_hash;
    for (alt_u32 byte_i = 0; byte_i < PFR_CRYPTO_LENGTH; byte_i++)
    {
        EXPECT_EQ(ufm_pit_pch_fw_hash[byte_i], full_pfr_image_pch_file_sha256sum[byte_i]);
        EXPECT_EQ(ufm_pit_bmc_fw_hash[byte_i], full_pfr_image_bmc_file_sha256sum[byte_i]);
    }

    /*
     * Step 3.
     * Reboot the platform
     * CPLD should re-calculate the hashes. If they match the saved hashes, CPLD transitions platform to T0.
     */
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(200, pfr_main());

    // Check UFM status
    EXPECT_TRUE(check_ufm_status(UFM_STATUS_PIT_L1_ENABLE_BIT_MASK));
    EXPECT_TRUE(check_ufm_status(UFM_STATUS_PIT_L2_ENABLE_BIT_MASK));
    EXPECT_TRUE(check_ufm_status(UFM_STATUS_PIT_HASH_STORED_BIT_MASK));
    EXPECT_TRUE(check_ufm_status(UFM_STATUS_PIT_L2_PASSED_BIT_MASK));

    // Nios should be in T0 mode
    EXPECT_EQ(ut_get_global_state(), (alt_u32) PLATFORM_STATE_ENTER_T0);

    // Check UFM status register for PIT L2 result
    EXPECT_TRUE(read_from_mailbox(MB_PROVISION_STATUS) & MB_UFM_PROV_UFM_PIT_L2_PASSED_MASK);

    // Clear the UFM status. It should be set properly in the next power up.
    write_to_mailbox(MB_PROVISION_STATUS, 0);

    /*
     * Step 4.
     * Reboot the platform again
     * CPLD should boot the platform quickly since PIT L2 is completed.
     */
    // Next power-cycle should be really quick.
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(1, pfr_main());

    // Nios should be in T0 mode
    EXPECT_EQ(ut_get_global_state(), (alt_u32) PLATFORM_STATE_ENTER_T0);

    // Check UFM status register for PIT L2 result
    EXPECT_TRUE(read_from_mailbox(MB_PROVISION_STATUS) & MB_UFM_PROV_UFM_PIT_L2_PASSED_MASK);
}

TEST_F(PFRProtectInTransitTest, test_pit_l2_lockdown_with_tampered_pch_flash)
{
    /*
     * Flow preparation
     */
    // Set asserts to throw as opposed to abort
    SYSTEM_MOCK::get()->set_assert_to_throw();
    // Expect that Nios firmware will stuck in the never_exit_loop.
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::THROW_FROM_NEVER_EXIT_LOOP);
    // Insert the T0_OPERATIONS code block (break out of T0 loop)
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS);

    /*
     * Step 1.
     * Provision PIT ID and enable PIT L1.
     * This is the prerequisite of enabling PIT L2
     */
    const alt_u8 pit_id[8] = {
            0xec, 0xa0, 0xb4, 0xed, 0x14, 0x12, 0xea, 0xe6,
    };
    for (int i = 0; i < 8; i++)
    {
        IOWR(U_MAILBOX_AVMM_BRIDGE_ADDR, MB_UFM_WRITE_FIFO, pit_id[i]);
    }

    ut_send_in_ufm_command(MB_UFM_PROV_PIT_ID);
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(1, pfr_main());

    ut_send_in_ufm_command(MB_UFM_PROV_ENABLE_PIT_L1);
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(1, pfr_main());

    // Make sure there's no error before moving on
    EXPECT_FALSE(ut_check_ufm_prov_status(MB_UFM_PROV_CMD_ERROR_MASK));

    // Check UFM status
    EXPECT_TRUE(check_ufm_status(UFM_STATUS_PIT_L1_ENABLE_BIT_MASK));
    EXPECT_FALSE(check_ufm_status(UFM_STATUS_PIT_L2_ENABLE_BIT_MASK));

    /*
     * Step 2.
     * Eanble PIT L2
     */
    // Send in enable PIT l2 UFM command
    ut_send_in_ufm_command(MB_UFM_PROV_ENABLE_PIT_L2);

    // Run PFR Main. Always run with the timeout
    // Expecting a throw from the never_exit_loop
    ASSERT_DURATION_LE(200, EXPECT_ANY_THROW({ pfr_main(); }));

    /*
     * Step 3.
     * Simulate power cycle of the platform.
     * PCH SPI flash has been tampered, platform should remained in lockdown.
     */
    // Exit in T0 at the end of first iteration in the T0 loop
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS);

    switch_spi_flash(SPI_FLASH_PCH);
    alt_u32* signed_active_pfm = get_spi_flash_ptr_with_offset(get_ufm_pfr_data()->pch_active_pfm);
    *signed_active_pfm = 0xffffffff;

    // Run PFR Main. Always run with the timeout
    // Expecting a throw from the never_exit_loop
    ASSERT_DURATION_LE(200, EXPECT_ANY_THROW({ pfr_main(); }));

    // Check UFM status
    EXPECT_TRUE(check_ufm_status(UFM_STATUS_PIT_L1_ENABLE_BIT_MASK));
    EXPECT_TRUE(check_ufm_status(UFM_STATUS_PIT_L2_ENABLE_BIT_MASK));
    EXPECT_TRUE(check_ufm_status(UFM_STATUS_PIT_HASH_STORED_BIT_MASK));
    EXPECT_FALSE(check_ufm_status(UFM_STATUS_PIT_L2_PASSED_BIT_MASK));

    // Nios should be in T-1 mode
    EXPECT_EQ(ut_get_global_state(), (alt_u32) PLATFORM_STATE_PIT_L2_PCH_HASH_MISMATCH_LOCKDOWN);
}

TEST_F(PFRProtectInTransitTest, test_pit_l2_lockdown_with_tampered_bmc_flash)
{
    /*
     * Flow preparation
     */
    // Set asserts to throw as opposed to abort
    SYSTEM_MOCK::get()->set_assert_to_throw();
    // Expect that Nios firmware will stuck in the never_exit_loop.
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::THROW_FROM_NEVER_EXIT_LOOP);
    // Insert the T0_OPERATIONS code block (break out of T0 loop)
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS);

    /*
     * Step 1.
     * Provision PIT ID and enable PIT L1.
     * This is the prerequisite of enabling PIT L2
     */
    const alt_u8 pit_id[8] = {
            0xec, 0xa0, 0xb4, 0xed, 0x14, 0x12, 0xea, 0xe6,
    };
    for (int i = 0; i < 8; i++)
    {
        IOWR(U_MAILBOX_AVMM_BRIDGE_ADDR, MB_UFM_WRITE_FIFO, pit_id[i]);
    }

    ut_send_in_ufm_command(MB_UFM_PROV_PIT_ID);
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(1, pfr_main());

    ut_send_in_ufm_command(MB_UFM_PROV_ENABLE_PIT_L1);
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(1, pfr_main());

    // Make sure there's no error before moving on
    EXPECT_FALSE(ut_check_ufm_prov_status(MB_UFM_PROV_CMD_ERROR_MASK));

    // Check UFM status
    EXPECT_TRUE(check_ufm_status(UFM_STATUS_PIT_L1_ENABLE_BIT_MASK));
    EXPECT_FALSE(check_ufm_status(UFM_STATUS_PIT_L2_ENABLE_BIT_MASK));

    /*
     * Step 2.
     * Eanble PIT L2
     */
    // Send in enable PIT l2 UFM command
    ut_send_in_ufm_command(MB_UFM_PROV_ENABLE_PIT_L2);

    // Run PFR Main. Always run with the timeout
    // Expecting a throw from the never_exit_loop
    ASSERT_DURATION_LE(200, EXPECT_ANY_THROW({ pfr_main(); }));

    /*
     * Step 3.
     * Simulate power cycle of the platform.
     * BMC SPI flash has been tampered, platform should remained in lockdown.
     */
    // Exit in T0 at the end of first iteration in the T0 loop
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS);

    switch_spi_flash(SPI_FLASH_BMC);
    alt_u32* signed_active_pfm = get_spi_flash_ptr_with_offset(get_ufm_pfr_data()->bmc_active_pfm);
    *signed_active_pfm = 0xffffffff;

    // Run PFR Main. Always run with the timeout
    // Expecting a throw from the never_exit_loop
    ASSERT_DURATION_LE(200, EXPECT_ANY_THROW({ pfr_main(); }));

    // Check UFM status
    EXPECT_TRUE(check_ufm_status(UFM_STATUS_PIT_L1_ENABLE_BIT_MASK));
    EXPECT_TRUE(check_ufm_status(UFM_STATUS_PIT_L2_ENABLE_BIT_MASK));
    EXPECT_TRUE(check_ufm_status(UFM_STATUS_PIT_HASH_STORED_BIT_MASK));
    EXPECT_FALSE(check_ufm_status(UFM_STATUS_PIT_L2_PASSED_BIT_MASK));

    // Nios should be in T-1 mode
    EXPECT_EQ(ut_get_global_state(), (alt_u32) PLATFORM_STATE_PIT_L2_BMC_HASH_MISMATCH_LOCKDOWN);

    // Check UFM status register for PIT L2 result
    EXPECT_FALSE(read_from_mailbox(MB_PROVISION_STATUS) & MB_UFM_PROV_UFM_PIT_L2_PASSED_MASK);
}
