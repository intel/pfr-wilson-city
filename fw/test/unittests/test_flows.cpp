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

class PFRFlowTest : public testing::Test
{
public:
    virtual void SetUp() {
        SYSTEM_MOCK::get()->reset();
        ut_disable_watchdog_timers();
    }

    virtual void TearDown() {}
};

TEST_F(PFRFlowTest, test_happy_path_unprovisioned)
{
    ut_prep_nios_gpi_signals();

    // Insert the T0_OPERATIONS code block (break out of T0 loop)
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS);

    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(1, pfr_main());

    // Check observed vs expected global_state
    EXPECT_EQ(ut_get_global_state(), (alt_u32) PLATFORM_STATE_ENTER_T0);
}

TEST_F(PFRFlowTest, test_happy_path_provisioned)
{
    // Provision UFM data
    SYSTEM_MOCK::get()->provision_ufm_data(UFM_PFR_DATA_EXAMPLE_KEY_FILE);

    ut_prep_nios_gpi_signals();

    // Insert the T0_OPERATIONS code block (break out of T0 loop)
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS);
    // Skip all T-1 operations
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::SKIP_TMIN1_OPERATIONS);

    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(1, pfr_main());

    // Check observed vs expected global_state
    EXPECT_EQ(ut_get_global_state(), (alt_u32) PLATFORM_STATE_ENTER_T0);
}
