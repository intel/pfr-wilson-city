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

class PFRAntiRollbackWithKeyIDTest : public testing::Test
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
 * @brief This test performs a key cancellation then attempts to update PCH firmware with the cancelled key.
 */
TEST_F(PFRAntiRollbackWithKeyIDTest, test_pch_active_update_with_cancelled_key)
{
    /*
     * Test Preparation
     */
    // Load the update capsule
    SYSTEM_MOCK::get()->load_to_flash(
            SPI_FLASH_PCH,
            SIGNED_CAPSULE_PCH_WITH_CSK_ID10_FILE,
            SIGNED_CAPSULE_PCH_WITH_CSK_ID10_FILE_SIZE,
            get_ufm_pfr_data()->pch_staging_region);

    /*
     * Flow preparation
     */
    ut_prep_nios_gpi_signals();
    ut_send_block_complete_chkpt_msg();

    // Exit after 50 iterations in the T0 loop
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS_END_AFTER_50_ITERS);

    // Send the active update intent
    ut_send_in_update_intent_once_upon_entry_to_t0(MB_PCH_UPDATE_INTENT, MB_UPDATE_INTENT_PCH_ACTIVE_MASK);

    /*
     * Step 1: Attempt PCH fw update prior to key cancellation
     */
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(60, pfr_main());

    // Nios should transition from T0 to T-1 exactly once for active update flow
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(1));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), LAST_PANIC_PCH_UPDATE_INTENT);

    // Expecting no error
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(0));

    // Have entered T-1 twice
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(1));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_bmc_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_pch_only_counter(), alt_u32(1));

    /*
     * Step 2: Send key cancellation certificate
     */
    // Load the update capsule
    SYSTEM_MOCK::get()->load_to_flash(
            SPI_FLASH_PCH,
            KEY_CAN_CERT_PCH_CAPSULE_KEY10,
            KEY_CAN_CERT_FILE_SIZE,
            get_ufm_pfr_data()->pch_staging_region);

    // Send the update intent
    write_to_mailbox(MB_PCH_UPDATE_INTENT, MB_UPDATE_INTENT_PCH_ACTIVE_MASK);

    // Process update intent in T0
    mb_update_intent_handler();

    // Nios should transition from T0 to T-1 exactly once for key cancellation
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(2));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), LAST_PANIC_PCH_UPDATE_INTENT);

    // Expecting no error
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(0));

    // Expecting key ID 10 to be cancelled
    EXPECT_FALSE(is_csk_key_valid(KCH_PC_PFR_PCH_UPDATE_CAPSULE, 10));

    // Have entered T-1 three times
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(1));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_bmc_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_pch_only_counter(), alt_u32(2));

    /*
     * Step 3: Attempt PCH update (with capsule signed with cancelled CSK key) again.
     * It should be rejected this time.
     */
    // Load the update capsule
    SYSTEM_MOCK::get()->load_to_flash(
            SPI_FLASH_PCH,
            SIGNED_CAPSULE_PCH_WITH_CSK_ID10_FILE,
            SIGNED_CAPSULE_PCH_WITH_CSK_ID10_FILE_SIZE,
            get_ufm_pfr_data()->pch_staging_region);

    // Reset some mailbox registers
    write_to_mailbox(MB_PCH_UPDATE_INTENT, MB_UPDATE_INTENT_PCH_ACTIVE_MASK);

    // Process update intent in T0
    mb_update_intent_handler();

    // Nios should transition from T0 to T-1 exactly once for active update flow
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(3));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), LAST_PANIC_PCH_UPDATE_INTENT);

    // Expecting error in this update
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(MAJOR_ERROR_UPDATE_FROM_PCH_FAILED));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_AUTHENTICATION_FAILED));

    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(0));

    // Have entered T-1 four times
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(1));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_bmc_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_pch_only_counter(), alt_u32(3));
}
