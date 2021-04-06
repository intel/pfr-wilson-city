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

class KeyCancellationUtilityTest : public testing::Test
{
public:
    virtual void SetUp()
    {
        SYSTEM_MOCK::get()->reset();
        // Perform provisioning
        SYSTEM_MOCK::get()->provision_ufm_data(UFM_PFR_DATA_EXAMPLE_KEY_FILE);
    }

    virtual void TearDown() {}
};

TEST_F(KeyCancellationUtilityTest, test_cancel_key)
{
    UFM_PFR_DATA* ufm = get_ufm_pfr_data();

    cancel_key(KCH_PC_PFR_CPLD_UPDATE_CAPSULE, 3);
    cancel_key(KCH_PC_PFR_CPLD_UPDATE_CAPSULE, 0);
    EXPECT_EQ(ufm->csk_cancel_cpld_update_cap[0], alt_u32(0b01101111111111111111111111111111));

    cancel_key(KCH_PC_PFR_PCH_PFM, 127);
    cancel_key(KCH_PC_PFR_PCH_PFM, 120);
    cancel_key(KCH_PC_PFR_PCH_PFM, 118);
    EXPECT_EQ(ufm->csk_cancel_pch_pfm[3], alt_u32(0b11111111111111111111110101111110));

    cancel_key(KCH_PC_PFR_PCH_UPDATE_CAPSULE, 8);
    EXPECT_EQ(ufm->csk_cancel_pch_update_cap[0], alt_u32(0b11111111011111111111111111111111));

    cancel_key(KCH_PC_PFR_BMC_PFM, 32);
    cancel_key(KCH_PC_PFR_BMC_PFM, 33);
    EXPECT_EQ(ufm->csk_cancel_bmc_pfm[1], alt_u32(0b00111111111111111111111111111111));

    cancel_key(KCH_PC_PFR_BMC_UPDATE_CAPSULE, 66);
    EXPECT_EQ(ufm->csk_cancel_bmc_update_cap[2], alt_u32(0b11011111111111111111111111111111));
}

TEST_F(KeyCancellationUtilityTest, test_is_key_cancelled)
{
    cancel_key(KCH_PC_PFR_CPLD_UPDATE_CAPSULE, 3);
    cancel_key(KCH_PC_PFR_CPLD_UPDATE_CAPSULE, 0);

    cancel_key(KCH_PC_PFR_PCH_PFM, 127);
    cancel_key(KCH_PC_PFR_PCH_PFM, 120);
    cancel_key(KCH_PC_PFR_PCH_PFM, 118);

    cancel_key(KCH_PC_PFR_PCH_UPDATE_CAPSULE, 8);

    cancel_key(KCH_PC_PFR_BMC_PFM, 32);
    cancel_key(KCH_PC_PFR_BMC_PFM, 64);

    cancel_key(KCH_PC_PFR_BMC_UPDATE_CAPSULE, 66);
    cancel_key(KCH_PC_PFR_BMC_UPDATE_CAPSULE, 0);

    EXPECT_FALSE(is_csk_key_valid(KCH_PC_PFR_CPLD_UPDATE_CAPSULE, 3));
    EXPECT_FALSE(is_csk_key_valid(KCH_PC_PFR_CPLD_UPDATE_CAPSULE, 0));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_CPLD_UPDATE_CAPSULE, 1));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_CPLD_UPDATE_CAPSULE, 2));

    EXPECT_FALSE(is_csk_key_valid(KCH_PC_PFR_PCH_PFM, 127));
    EXPECT_FALSE(is_csk_key_valid(KCH_PC_PFR_PCH_PFM, 118));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_PCH_PFM, 126));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_PCH_PFM, 2));

    EXPECT_FALSE(is_csk_key_valid(KCH_PC_PFR_PCH_UPDATE_CAPSULE, 8));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_PCH_UPDATE_CAPSULE, 3));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_PCH_UPDATE_CAPSULE, 0));

    EXPECT_FALSE(is_csk_key_valid(KCH_PC_PFR_BMC_PFM, 32));
    EXPECT_FALSE(is_csk_key_valid(KCH_PC_PFR_BMC_PFM, 64));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_BMC_PFM, 33));

    EXPECT_FALSE(is_csk_key_valid(KCH_PC_PFR_BMC_UPDATE_CAPSULE, 66));
    EXPECT_FALSE(is_csk_key_valid(KCH_PC_PFR_BMC_UPDATE_CAPSULE, 0));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_BMC_UPDATE_CAPSULE, 1));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_BMC_UPDATE_CAPSULE, 2));
}

TEST_F(KeyCancellationUtilityTest, test_is_key_cancelled_with_example_ufm)
{
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_CPLD_UPDATE_CAPSULE, 3));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_CPLD_UPDATE_CAPSULE, 0));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_CPLD_UPDATE_CAPSULE, 1));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_CPLD_UPDATE_CAPSULE, 2));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_CPLD_UPDATE_CAPSULE, 87));

    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_PCH_PFM, 127));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_PCH_PFM, 118));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_PCH_PFM, 126));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_PCH_PFM, 2));

    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_PCH_UPDATE_CAPSULE, 8));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_PCH_UPDATE_CAPSULE, 3));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_PCH_UPDATE_CAPSULE, 0));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_PCH_UPDATE_CAPSULE, 127));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_PCH_UPDATE_CAPSULE, 99));

    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_BMC_PFM, 32));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_BMC_PFM, 64));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_BMC_PFM, 33));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_BMC_PFM, 0));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_BMC_PFM, 127));

    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_BMC_UPDATE_CAPSULE, 66));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_BMC_UPDATE_CAPSULE, 0));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_BMC_UPDATE_CAPSULE, 1));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_BMC_UPDATE_CAPSULE, 2));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_BMC_UPDATE_CAPSULE, 100));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_BMC_UPDATE_CAPSULE, 127));
}
