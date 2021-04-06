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


class UFMProvisioningTest : public testing::Test
{
public:
    virtual void SetUp() {
        SYSTEM_MOCK::get()->reset();
    }

    virtual void TearDown() {}
};

TEST_F(UFMProvisioningTest, test_check_status_after_provisioning)
{
    /*
     * Flow preparation
     */
    ut_prep_nios_gpi_signals();

    // Insert the T0_OPERATIONS code block (break out of T0 loop)
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS);
    // Skip all T-1 operations
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::SKIP_TMIN1_OPERATIONS);

    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(1, pfr_main());

    /*
     * Check the UFM status.
     * Should not be provisioned or locked
     */
    alt_u32 status = read_from_mailbox(MB_PROVISION_STATUS);
    EXPECT_FALSE(MB_UFM_PROV_UFM_PROVISIONED_MASK & status);
    EXPECT_FALSE(MB_UFM_PROV_UFM_LOCKED_MASK & status);

    /*
     * Provision UFM PFR data
     */
    // Perform provisioning
    SYSTEM_MOCK::get()->provision_ufm_data(UFM_PFR_DATA_EXAMPLE_KEY_FILE);

    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(1, pfr_main());

    // Check the UFM status (expect: provisioned but not locked)
    status = read_from_mailbox(MB_PROVISION_STATUS);
    EXPECT_TRUE(MB_UFM_PROV_UFM_PROVISIONED_MASK & status);
    EXPECT_FALSE(MB_UFM_PROV_UFM_LOCKED_MASK & status);

    /*
     * Check the UFM status
     * Should be provisioned, but not locked
     */
    status = read_from_mailbox(MB_PROVISION_STATUS);
    EXPECT_TRUE(MB_UFM_PROV_UFM_PROVISIONED_MASK & status);
    EXPECT_FALSE(MB_UFM_PROV_UFM_LOCKED_MASK & status);
}

/**
 * Test the UFM command: 00h: Erase current (not-locked) provisioning
 */
TEST_F(UFMProvisioningTest, test_erase_ufm_pfr_data)
{
    /*
     * Flow preparation
     */
    ut_prep_nios_gpi_signals();

    // Insert the T0_OPERATIONS code block (break out of T0 loop)
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS);
    // Skip all T-1 operations
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::SKIP_TMIN1_OPERATIONS);

    // Perform provisioning
    SYSTEM_MOCK::get()->provision_ufm_data(UFM_PFR_DATA_EXAMPLE_KEY_FILE);

    /*
     * Send erase provisioning request
     */
    ut_send_in_ufm_command(MB_UFM_PROV_ERASE);

    /*
     * Execute the Nios firmware flow
     */
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(1, pfr_main());

    /*
     * Check results
     */
    // Wait until CPLD finished with the request
    EXPECT_TRUE(ut_check_ufm_prov_status(MB_UFM_PROV_CMD_DONE_MASK));
    // Make sure there's no error before moving on
    EXPECT_FALSE(ut_check_ufm_prov_status(MB_UFM_PROV_CMD_ERROR_MASK));

    // Ensure that all UFM pfr data is erased
    alt_u32* ufm_pfr_data_ptr = (alt_u32*) get_ufm_pfr_data();
    for (int i = 0; i < UFM_PFR_DATA_SIZE / 4; i++)
    {
        EXPECT_EQ(ufm_pfr_data_ptr[i], alt_u32(0xffffffff));
    }
}

TEST_F(UFMProvisioningTest, test_root_key_provisioning)
{
    /*
     * Flow preparation
     */
    ut_prep_nios_gpi_signals();

    // Insert the T0_OPERATIONS code block (break out of T0 loop)
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS);

    /*
     * Send key provisioning request
     */
    const alt_u8 prov_key_hash[32] = {
            0xdc, 0xa0, 0xb4, 0xed, 0x14, 0x12, 0xea, 0xe6, 0xf8, 0x5d, 0x02, 0xae,
            0x6e, 0xf3, 0xd3, 0x50, 0xb3, 0xd0, 0xe5, 0xba, 0x6f, 0x20, 0x80, 0x08,
            0x5c, 0xf4, 0xb7, 0xb1, 0xdc, 0xd5, 0xba, 0x17
    };
    for (int i = 0; i < 32; i++)
    {
        IOWR(U_MAILBOX_AVMM_BRIDGE_ADDR, MB_UFM_WRITE_FIFO, prov_key_hash[i]);
    }

    ut_send_in_ufm_command(MB_UFM_PROV_ROOT_KEY);

    /*
     * Execute the Nios firmware flow
     */
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(1, pfr_main());

    /*
     * Check results
     */
    // Wait until CPLD finished with the request
    EXPECT_TRUE(ut_check_ufm_prov_status(MB_UFM_PROV_CMD_DONE_MASK));
    // Make sure there's no error before moving on
    EXPECT_FALSE(ut_check_ufm_prov_status(MB_UFM_PROV_CMD_ERROR_MASK));

    // mb_ufm_provisioning_handler should have flushed out the FIFO already
    // Check if that's the case
    for (int i = 0; i < 32; i++)
    {
        EXPECT_EQ(IORD(U_MAILBOX_AVMM_BRIDGE_ADDR, MB_UFM_READ_FIFO), (alt_u32) 0);
    }

    // Check key hash in the UFM vs expected hash values
    alt_u8* ufm_key_hash = (alt_u8*) get_ufm_pfr_data()->root_key_hash;
    for (int i = 0; i < 32; i++)
    {
        EXPECT_EQ(ufm_key_hash[i], prov_key_hash[i]);
    }
}

TEST_F(UFMProvisioningTest, test_read_provisioned_key)
{
    /*
     * Flow preparation
     */
    ut_prep_nios_gpi_signals();

    // Insert the T0_OPERATIONS code block (break out of T0 loop)
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS);
    // Skip all T-1 operations
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::SKIP_TMIN1_OPERATIONS);

    // Perform provisioning
    SYSTEM_MOCK::get()->provision_ufm_data(UFM_PFR_DATA_EXAMPLE_KEY_FILE);

    // Send read key provisioning request
    ut_send_in_ufm_command(MB_UFM_PROV_RD_ROOT_KEY);

    /*
     * Execute the Nios firmware flow
     */
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(1, pfr_main());

    /*
     * Check results
     */
    // Wait until CPLD finished with the request
    EXPECT_TRUE(ut_check_ufm_prov_status(MB_UFM_PROV_CMD_DONE_MASK));
    // Make sure there's no error before moving on
    EXPECT_FALSE(ut_check_ufm_prov_status(MB_UFM_PROV_CMD_ERROR_MASK));

    // Check key hash in the UFM vs expected hash values
    alt_u8* ufm_key_hash = (alt_u8*) get_ufm_pfr_data()->root_key_hash;
    for (int i = 0; i < 32; i++)
    {
        EXPECT_EQ(ufm_key_hash[i], IORD(U_MAILBOX_AVMM_BRIDGE_ADDR, MB_UFM_READ_FIFO));
    }
}

/**
 * Attempt to lock UFM before it's provisioned. This request should be denied.
 */
TEST_F(UFMProvisioningTest, test_lock_ufm_before_provisioning_all_data)
{
    /*
     * Provision root key hash
     */
    ut_prep_nios_gpi_signals();

    // Insert the T0_OPERATIONS code block (break out of T0 loop)
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS);

    const alt_u8 prov_key_hash[32] = {
            0xdc, 0xa0, 0xb4, 0xed, 0x14, 0x12, 0xea, 0xe6, 0xf8, 0x5d, 0x02, 0xae,
            0x6e, 0xf3, 0xd3, 0x50, 0xb3, 0xd0, 0xe5, 0xba, 0x6f, 0x20, 0x80, 0x08,
            0x5c, 0xf4, 0xb7, 0xb1, 0xdc, 0xd5, 0xba, 0x17
    };
    for (int i = 0; i < 32; i++)
    {
        IOWR(U_MAILBOX_AVMM_BRIDGE_ADDR, MB_UFM_WRITE_FIFO, prov_key_hash[i]);
    }

    ut_send_in_ufm_command(MB_UFM_PROV_ROOT_KEY);

    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(1, pfr_main());

    /*
     * Attempt to lock UFM
     */
    ut_send_in_ufm_command(MB_UFM_PROV_END);

    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(1, pfr_main());

    /*
     * Check Results
     */
    // Nios should have processed this ufm command
    EXPECT_TRUE(ut_check_ufm_prov_status(MB_UFM_PROV_CMD_DONE_MASK));
    // There should be an error for locking UFM prior to completion of provisioning
    EXPECT_TRUE(ut_check_ufm_prov_status(MB_UFM_PROV_CMD_ERROR_MASK));
}

/**
 * Attempt to reconfig CPLD after its UFM has been locked. This request should be denied.
 */
TEST_F(UFMProvisioningTest, test_reconfig_cpld_when_ufm_is_locked)
{
    /*
     * Flow preparation
     */
    ut_prep_nios_gpi_signals();

    // Insert the T0_OPERATIONS code block (break out of T0 loop)
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS);
    // Skip all T-1 operations
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::SKIP_TMIN1_OPERATIONS);

    // Perform provisioning
    SYSTEM_MOCK::get()->provision_ufm_data(UFM_PFR_DATA_EXAMPLE_KEY_FILE);

    /*
     * Send end provisioning (UFM Lock)
     */
    ut_send_in_ufm_command(MB_UFM_PROV_END);

    /*
     * Execute the Nios firmware flow
     */
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(1, pfr_main());

    /*
     * Check results
     */
    // Wait until CPLD finished with the request
    EXPECT_TRUE(ut_check_ufm_prov_status(MB_UFM_PROV_CMD_DONE_MASK));
    // Make sure there's no error before moving on
    EXPECT_FALSE(ut_check_ufm_prov_status(MB_UFM_PROV_CMD_ERROR_MASK));

    // Check the UFM status. It should be locked.
    EXPECT_TRUE(MB_UFM_PROV_UFM_LOCKED_MASK & read_from_mailbox(MB_PROVISION_STATUS));


    /*
     * Now, send in reconfig CPLD command
     */
    ut_send_in_ufm_command(MB_UFM_PROV_RECONFIG_CPLD);

    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(1, pfr_main());

    /*
     * Check results
     */
    // Wait until CPLD finished with the request
    EXPECT_TRUE(ut_check_ufm_prov_status(MB_UFM_PROV_CMD_DONE_MASK));
    // Expect there to be an error
    EXPECT_TRUE(ut_check_ufm_prov_status(MB_UFM_PROV_CMD_ERROR_MASK));
}

/**
 * Test sending request to modify a locked UFM.
 * Expect to see "Command Error" bit set in the UFM/Provision status register
 */
TEST_F(UFMProvisioningTest, test_edit_locked_ufm)
{
    /*
     * Flow preparation
     */
    ut_prep_nios_gpi_signals();

    // Insert the T0_OPERATIONS code block (break out of T0 loop)
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS);
    // Skip all T-1 operations
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::SKIP_TMIN1_OPERATIONS);

    // Perform provisioning
    SYSTEM_MOCK::get()->provision_ufm_data(UFM_PFR_DATA_EXAMPLE_KEY_FILE);

    /*
     * Send end provisioning (UFM Lock)
     */
    ut_send_in_ufm_command(MB_UFM_PROV_END);

    /*
     * Execute the Nios firmware flow
     */
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(1, pfr_main());

    /*
     * Check results
     */
    // Wait until CPLD finished with the request
    EXPECT_TRUE(ut_check_ufm_prov_status(MB_UFM_PROV_CMD_DONE_MASK));
    // Make sure there's no error before moving on
    EXPECT_FALSE(ut_check_ufm_prov_status(MB_UFM_PROV_CMD_ERROR_MASK));

    // Check the UFM status. It should be locked.
    EXPECT_TRUE(MB_UFM_PROV_UFM_LOCKED_MASK & read_from_mailbox(MB_PROVISION_STATUS));

    /*
     * Now, send in commands that attempts to modify UFM.
     */
    ut_send_in_ufm_command(MB_UFM_PROV_ERASE);
    ASSERT_DURATION_LE(1, pfr_main());
    EXPECT_TRUE(ut_check_ufm_prov_status(MB_UFM_PROV_CMD_ERROR_MASK));

    ut_send_in_ufm_command(MB_UFM_PROV_ROOT_KEY);
    ASSERT_DURATION_LE(1, pfr_main());
    EXPECT_TRUE(ut_check_ufm_prov_status(MB_UFM_PROV_CMD_ERROR_MASK));

    ut_send_in_ufm_command(MB_UFM_PROV_PCH_OFFSETS);
    ASSERT_DURATION_LE(1, pfr_main());
    EXPECT_TRUE(ut_check_ufm_prov_status(MB_UFM_PROV_CMD_ERROR_MASK));

    ut_send_in_ufm_command(MB_UFM_PROV_BMC_OFFSETS);
    ASSERT_DURATION_LE(1, pfr_main());
    EXPECT_TRUE(ut_check_ufm_prov_status(MB_UFM_PROV_CMD_ERROR_MASK));

    ut_send_in_ufm_command(MB_UFM_PROV_END);
    ASSERT_DURATION_LE(1, pfr_main());
    EXPECT_TRUE(ut_check_ufm_prov_status(MB_UFM_PROV_CMD_ERROR_MASK));
}

/**
 * Provision all the required data to get UFM into provisioned (but not locked) state.
 */
TEST_F(UFMProvisioningTest, test_provisioning_all_data)
{
    /************
     * Root Key Hash Provisioning Request
     ************/
    const alt_u8 w_test_key_hash[32] = {
            0xdc, 0xa0, 0xb4, 0xed, 0x14, 0x12, 0xea, 0xe6, 0xf8, 0x5d, 0x02, 0xae,
            0x6e, 0xf3, 0xd3, 0x50, 0xb3, 0xd0, 0xe5, 0xba, 0x6f, 0x20, 0x80, 0x08,
            0x5c, 0xf4, 0xb7, 0xb1, 0xdc, 0xd5, 0xba, 0x17
    };
    for (int i = 0; i < 32; i++) { IOWR(U_MAILBOX_AVMM_BRIDGE_ADDR, MB_UFM_WRITE_FIFO, w_test_key_hash[i]); }

    alt_u32* mb_ufm_prov_cmd_addr = U_MAILBOX_AVMM_BRIDGE_ADDR + MB_PROVISION_CMD;
    alt_u32* mb_ufm_cmd_trigger_addr = U_MAILBOX_AVMM_BRIDGE_ADDR + MB_UFM_CMD_TRIGGER;

    // Execute the root key provisioning command
    SYSTEM_MOCK::get()->set_mem_word(mb_ufm_prov_cmd_addr, MB_UFM_PROV_ROOT_KEY, true);
    SYSTEM_MOCK::get()->set_mem_word(mb_ufm_cmd_trigger_addr, MB_UFM_CMD_EXECUTE_MASK, true);

    mb_ufm_provisioning_handler();

    // Wait until CPLD finished with the request
    ut_wait_for_ufm_prov_cmd_done();
    // Make sure there's no error before moving on
    EXPECT_EQ(IORD(U_MAILBOX_AVMM_BRIDGE_ADDR, MB_PROVISION_STATUS) & MB_UFM_PROV_CMD_ERROR_MASK, (alt_u32) 0);

    mb_ufm_provisioning_handler();

    /************
     * PCH Offsets Provisioning Request
     ************/
    const alt_u8 w_pch_offsets[12] = {
            // start address of PCH SPI Active Region PFM
            0x00, 0x80, 0xFD, 0x03,
            // start address of PCH SPI Recovery Region
            0x00, 0x80, 0xFD, 0x02,
            // start address of PCH SPI Staging Region
            0x00, 0x80, 0xFD, 0x01,
    };
    for (int i = 0; i < 12; i++)
    {
        IOWR(U_MAILBOX_AVMM_BRIDGE_ADDR, MB_UFM_WRITE_FIFO, w_pch_offsets[i]);
    }

    // In T0, mb_ufm_provisioning_handler is running in a while loop
    mb_ufm_provisioning_handler();

    // Execute the root key provisioning command
    SYSTEM_MOCK::get()->set_mem_word(mb_ufm_prov_cmd_addr, MB_UFM_PROV_PCH_OFFSETS, true);
    SYSTEM_MOCK::get()->set_mem_word(mb_ufm_cmd_trigger_addr, MB_UFM_CMD_EXECUTE_MASK, true);

    // In T0, mb_ufm_provisioning_handler is running in a while loop
    mb_ufm_provisioning_handler();

    // Wait until CPLD finished with the request
    ut_wait_for_ufm_prov_cmd_done();
    // Make sure there's no error before moving on
    EXPECT_EQ(IORD(U_MAILBOX_AVMM_BRIDGE_ADDR, MB_PROVISION_STATUS) & MB_UFM_PROV_CMD_ERROR_MASK, (alt_u32) 0);

    // In T0, mb_ufm_provisioning_handler is running in a while loop
    mb_ufm_provisioning_handler();

    /*
     * BMC Offsets Provisioning Request
     */
    const alt_u8 w_bmc_offsets[12] = {
            // start address of BMC SPI Active Region PFM
            0x00, 0x00, 0xbe, 0x10,
            // start address of BMC SPI Recovery Region
            0x00, 0x00, 0x40, 0x02,
            // start address of BMC SPI Staging Region
            0x00, 0x00, 0x00, 0x04,
    };
    for (int i = 0; i < 12; i++)
    {
        IOWR(U_MAILBOX_AVMM_BRIDGE_ADDR, MB_UFM_WRITE_FIFO, w_bmc_offsets[i]);
    }

    // In T0, mb_ufm_provisioning_handler is running in a while loop
    mb_ufm_provisioning_handler();

    // Execute the root key provisioning command
    SYSTEM_MOCK::get()->set_mem_word(mb_ufm_prov_cmd_addr, MB_UFM_PROV_BMC_OFFSETS, true);
    SYSTEM_MOCK::get()->set_mem_word(mb_ufm_cmd_trigger_addr, MB_UFM_CMD_EXECUTE_MASK, true);

    // In T0, mb_ufm_provisioning_handler is running in a while loop
    mb_ufm_provisioning_handler();

    // Wait until CPLD finished with the request
    ut_wait_for_ufm_prov_cmd_done();
    // Make sure there's no error before moving on
    EXPECT_EQ(IORD(U_MAILBOX_AVMM_BRIDGE_ADDR, MB_PROVISION_STATUS) & MB_UFM_PROV_CMD_ERROR_MASK, (alt_u32) 0);

    // In T0, mb_ufm_provisioning_handler is running in a while loop
    mb_ufm_provisioning_handler();

    /************
     * Result checking
     ************/

    // Check that UFM is currently provisioned but not locked
    EXPECT_TRUE(is_ufm_provisioned());
    EXPECT_FALSE(is_ufm_locked());
    EXPECT_TRUE(IORD(U_MAILBOX_AVMM_BRIDGE_ADDR, MB_PROVISION_STATUS) & MB_UFM_PROV_UFM_PROVISIONED_MASK);
    EXPECT_FALSE(IORD(U_MAILBOX_AVMM_BRIDGE_ADDR, MB_PROVISION_STATUS) & MB_UFM_PROV_UFM_LOCKED_MASK);

    // In T0, mb_ufm_provisioning_handler is running in a while loop
    mb_ufm_provisioning_handler();

    // Check key hash in the UFM vs expected hash values
    UFM_PFR_DATA* system_ufm_data = (UFM_PFR_DATA*) SYSTEM_MOCK::get()->get_ufm_data_ptr();

    alt_u8* ufm_key_hash = (alt_u8*) system_ufm_data->root_key_hash;
    alt_u8* expected_key_hash = (alt_u8*) w_test_key_hash;
    for (int i = 0; i < 32; i++)
    {
        EXPECT_EQ(ufm_key_hash[i], expected_key_hash[i]);
    }

    // Check PCH offsets
    alt_u8* offset = (alt_u8*) &system_ufm_data->pch_active_pfm;
    alt_u8* expected_offset = (alt_u8*) w_pch_offsets;
    for (int i = 0; i < 4; i++)
    {
        EXPECT_EQ(offset[i], *expected_offset);
        expected_offset++;
    }

    offset = (alt_u8*) &system_ufm_data->pch_recovery_region;
    for (int i = 0; i < 4; i++)
    {
        EXPECT_EQ(offset[i], *expected_offset);
        expected_offset++;
    }

    offset = (alt_u8*) &system_ufm_data->pch_staging_region;
    for (int i = 0; i < 4; i++)
    {
        EXPECT_EQ(offset[i], *expected_offset);
        expected_offset++;
    }

    // Check BMC offsets
    offset = (alt_u8*) &system_ufm_data->bmc_active_pfm;
    expected_offset = (alt_u8*) w_bmc_offsets;
    for (int i = 0; i < 4; i++)
    {
        EXPECT_EQ(offset[i], *expected_offset);
        expected_offset++;
    }

    offset = (alt_u8*) &system_ufm_data->bmc_recovery_region;
    for (int i = 0; i < 4; i++)
    {
        EXPECT_EQ(offset[i], *expected_offset);
        expected_offset++;
    }

    offset = (alt_u8*) &system_ufm_data->bmc_staging_region;
    for (int i = 0; i < 4; i++)
    {
        EXPECT_EQ(offset[i], *expected_offset);
        expected_offset++;
    }

    /************
     * Finally, test erase command and verify that UFM is no longer provisioned
     ************/
    // Execute the erase provisioning command
    SYSTEM_MOCK::get()->set_mem_word(mb_ufm_prov_cmd_addr, MB_UFM_PROV_ERASE, true);
    SYSTEM_MOCK::get()->set_mem_word(mb_ufm_cmd_trigger_addr, MB_UFM_CMD_EXECUTE_MASK, true);

    mb_ufm_provisioning_handler();

    // Wait until CPLD finished with the request
    ut_wait_for_ufm_prov_cmd_done();
    // Make sure there's no error before moving on
    EXPECT_FALSE(IORD(U_MAILBOX_AVMM_BRIDGE_ADDR, MB_PROVISION_STATUS) & MB_UFM_PROV_CMD_ERROR_MASK);

    // Check that UFM is currently not provisioned and not locked
    EXPECT_FALSE(is_ufm_provisioned());
    EXPECT_FALSE(is_ufm_locked());
    EXPECT_FALSE(IORD(U_MAILBOX_AVMM_BRIDGE_ADDR, MB_PROVISION_STATUS) & MB_UFM_PROV_UFM_PROVISIONED_MASK);
    EXPECT_FALSE(IORD(U_MAILBOX_AVMM_BRIDGE_ADDR, MB_PROVISION_STATUS) & MB_UFM_PROV_UFM_LOCKED_MASK);
}

/**
 * @brief Quickly scan through various UFM commands and check if
 * they are rejected as expected under different scenarios.
 */
TEST_F(UFMProvisioningTest, test_acceptance_of_various_ufm_cmds)
{
    /*
     * Lock ufm without provisioning data first. This should be rejected
     */
    IOWR(U_MAILBOX_AVMM_BRIDGE_ADDR, MB_PROVISION_CMD, MB_UFM_PROV_END);
    IOWR(U_MAILBOX_AVMM_BRIDGE_ADDR, MB_UFM_CMD_TRIGGER, MB_UFM_CMD_EXECUTE_MASK);
    mb_ufm_provisioning_handler();
    EXPECT_TRUE(IORD(U_MAILBOX_AVMM_BRIDGE_ADDR, MB_PROVISION_STATUS) & MB_UFM_PROV_CMD_ERROR_MASK);

    EXPECT_FALSE(is_ufm_provisioned());
    EXPECT_FALSE(is_ufm_locked());
    EXPECT_FALSE(IORD(U_MAILBOX_AVMM_BRIDGE_ADDR, MB_PROVISION_STATUS) & MB_UFM_PROV_UFM_PROVISIONED_MASK);
    EXPECT_FALSE(IORD(U_MAILBOX_AVMM_BRIDGE_ADDR, MB_PROVISION_STATUS) & MB_UFM_PROV_UFM_LOCKED_MASK);

    /*
     * Test various commands after the UFM is provisioned
     * Unmapped value (e.g. 0xFF) is not tested here. In provisioned state,
     * Nios currently ignores them and doesn't issue error for them.
     */
    alt_u32 num_test_cmds_1 = 12;
    alt_u32 ufm_cmd_list_1[12] = {
            MB_UFM_PROV_ERASE,
            MB_UFM_PROV_ROOT_KEY,
            MB_UFM_PROV_PIT_ID,
            MB_UFM_PROV_PCH_OFFSETS,
            MB_UFM_PROV_BMC_OFFSETS,
            MB_UFM_PROV_END,
            MB_UFM_PROV_RD_ROOT_KEY,
            MB_UFM_PROV_RD_PCH_OFFSETS,
            MB_UFM_PROV_RD_BMC_OFFSETS,
            MB_UFM_PROV_RECONFIG_CPLD,
            MB_UFM_PROV_ENABLE_PIT_L1,
            MB_UFM_PROV_ENABLE_PIT_L2,
    };

    // Check whether the command would be rejected (1 means rejected by Nios code)
    //   Provisioning of Root Key/PCH offsets/BMC offsets is rejected because they have been provisioned
    //   Enable PIT L1 is rejected because PIT ID has not been provisioned
    //   Enable PIT L2 is rejected because PIT L1 has not been enabled
    alt_u32 result_list_1[12] = {0, 1, 0, 1, 1, 0, 0, 0, 0, 0, 1, 1};

    for (alt_u32 cmd_i = 0; cmd_i < num_test_cmds_1; cmd_i++)
    {
        // Re-provision data
        SYSTEM_MOCK::get()->provision_ufm_data(UFM_PFR_DATA_EXAMPLE_KEY_FILE);

        // Send in the UFM command
        IOWR(U_MAILBOX_AVMM_BRIDGE_ADDR, MB_PROVISION_CMD, ufm_cmd_list_1[cmd_i]);
        IOWR(U_MAILBOX_AVMM_BRIDGE_ADDR, MB_UFM_CMD_TRIGGER, MB_UFM_CMD_EXECUTE_MASK);

        // Run Nios code
        mb_ufm_provisioning_handler();

        // Check UFM status
        EXPECT_EQ(IORD(U_MAILBOX_AVMM_BRIDGE_ADDR, MB_PROVISION_STATUS) & MB_UFM_PROV_CMD_ERROR_MASK,
                (result_list_1[cmd_i] << 2));
        EXPECT_FALSE(IORD(U_MAILBOX_AVMM_BRIDGE_ADDR, MB_PROVISION_STATUS) & MB_UFM_PROV_CMD_BUSY_MASK);
        EXPECT_TRUE(IORD(U_MAILBOX_AVMM_BRIDGE_ADDR, MB_PROVISION_STATUS) & MB_UFM_PROV_CMD_DONE_MASK);
    }

    /*
     * Lock UFM
     */
    SYSTEM_MOCK::get()->provision_ufm_data(UFM_PFR_DATA_EXAMPLE_KEY_FILE);
    IOWR(U_MAILBOX_AVMM_BRIDGE_ADDR, MB_PROVISION_CMD, MB_UFM_PROV_END);
    IOWR(U_MAILBOX_AVMM_BRIDGE_ADDR, MB_UFM_CMD_TRIGGER, MB_UFM_CMD_EXECUTE_MASK);
    mb_ufm_provisioning_handler();
    EXPECT_FALSE(IORD(U_MAILBOX_AVMM_BRIDGE_ADDR, MB_PROVISION_STATUS) & MB_UFM_PROV_CMD_ERROR_MASK);

    EXPECT_TRUE(is_ufm_provisioned());
    EXPECT_TRUE(is_ufm_locked());
    EXPECT_TRUE(IORD(U_MAILBOX_AVMM_BRIDGE_ADDR, MB_PROVISION_STATUS) & MB_UFM_PROV_UFM_PROVISIONED_MASK);
    EXPECT_TRUE(IORD(U_MAILBOX_AVMM_BRIDGE_ADDR, MB_PROVISION_STATUS) & MB_UFM_PROV_UFM_LOCKED_MASK);

    /*
     * Test various commands after the UFM is locked
     */
    alt_u32 num_test_cmds_2 = 15;
    alt_u32 ufm_cmd_list_2[15] = {
            MB_UFM_PROV_ERASE,
            MB_UFM_PROV_ROOT_KEY,
            MB_UFM_PROV_PIT_ID,
            MB_UFM_PROV_PCH_OFFSETS,
            MB_UFM_PROV_BMC_OFFSETS,
            MB_UFM_PROV_END,
            MB_UFM_PROV_RD_ROOT_KEY,
            MB_UFM_PROV_RD_PCH_OFFSETS,
            MB_UFM_PROV_RD_BMC_OFFSETS,
            MB_UFM_PROV_RECONFIG_CPLD,
            MB_UFM_PROV_ENABLE_PIT_L1,
            MB_UFM_PROV_ENABLE_PIT_L2,
            // Unmapped value
            0x9,
            0xF,
            0xFF,
    };
    // Check whether the command would be rejected (1 means rejected by Nios code)
    alt_u32 result_list_2[15] = {1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1};

    for (alt_u32 cmd_i = 0; cmd_i < num_test_cmds_2; cmd_i++)
    {
        // Send in the UFM command
        IOWR(U_MAILBOX_AVMM_BRIDGE_ADDR, MB_PROVISION_CMD, ufm_cmd_list_2[cmd_i]);
        IOWR(U_MAILBOX_AVMM_BRIDGE_ADDR, MB_UFM_CMD_TRIGGER, MB_UFM_CMD_EXECUTE_MASK);

        // Run Nios code
        mb_ufm_provisioning_handler();

        // Check UFM status
        EXPECT_EQ(IORD(U_MAILBOX_AVMM_BRIDGE_ADDR, MB_PROVISION_STATUS) & MB_UFM_PROV_CMD_ERROR_MASK,
                (result_list_2[cmd_i] << 2));
        EXPECT_FALSE(IORD(U_MAILBOX_AVMM_BRIDGE_ADDR, MB_PROVISION_STATUS) & MB_UFM_PROV_CMD_BUSY_MASK);
        EXPECT_TRUE(IORD(U_MAILBOX_AVMM_BRIDGE_ADDR, MB_PROVISION_STATUS) & MB_UFM_PROV_CMD_DONE_MASK);

        // Check provisioned/locked status
        EXPECT_TRUE(is_ufm_provisioned());
        EXPECT_TRUE(is_ufm_locked());
        EXPECT_TRUE(IORD(U_MAILBOX_AVMM_BRIDGE_ADDR, MB_PROVISION_STATUS) & MB_UFM_PROV_UFM_PROVISIONED_MASK);
        EXPECT_TRUE(IORD(U_MAILBOX_AVMM_BRIDGE_ADDR, MB_PROVISION_STATUS) & MB_UFM_PROV_UFM_LOCKED_MASK);
    }
}

