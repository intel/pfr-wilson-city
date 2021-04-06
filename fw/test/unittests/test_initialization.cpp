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

class NiosFWInitializationTest : public testing::Test
{
public:
    virtual void SetUp()
    {
        SYSTEM_MOCK::get()->reset();
    }

    virtual void TearDown() {}
};

TEST_F(NiosFWInitializationTest, test_cpld_hash)
{
    /*
     * Run Nios FW to T-1 in unprovisioned state.
     */
    ut_prep_nios_gpi_signals();

    // Insert the T0_OPERATIONS code block (break out of T0 loop)
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS);

    // Load active CPLD image to CFM1
    SYSTEM_MOCK::get()->load_active_image_to_cfm1();

    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(1, pfr_main());

    /*
     * Check if CPLD hash is reported correctly to the mailbox.
     */
    alt_u8 expected_cpld_hash[PFR_CRYPTO_LENGTH] = {
        0xdf, 0x14, 0x77, 0x7c, 0x1f, 0xf4, 0xb9, 0x88, 0xb5, 0xeb, 0xb2, 0xc4,
        0x79, 0xf2, 0xde, 0x10, 0x22, 0xd2, 0xf1, 0xcf, 0x5b, 0xa8, 0x6a, 0x15,
        0xce, 0xd1, 0x48, 0x29, 0x04, 0xad, 0x22, 0x28
    };

    for (alt_u32 byte_i = 0; byte_i < PFR_CRYPTO_LENGTH; byte_i++)
    {
        EXPECT_EQ(IORD(U_MAILBOX_AVMM_BRIDGE_BASE, MB_CPLD_HASH + byte_i), expected_cpld_hash[byte_i]);
    }
}
