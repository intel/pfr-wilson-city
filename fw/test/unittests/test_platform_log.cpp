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

class PlatformLogTest : public testing::Test
{
public:
    virtual void SetUp()
    {
        SYSTEM_MOCK::get()->reset();
        ut_reset_nios_fw();
    }

    virtual void TearDown() {}
};

TEST_F(PlatformLogTest, test_log_wdt_recovery)
{
    log_wdt_recovery(LAST_RECOVERY_ACM_LAUNCH_FAIL, LAST_PANIC_ACM_BIOS_WDT_EXPIRED);
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(1));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(LAST_RECOVERY_ACM_LAUNCH_FAIL));
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(1));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), alt_u32(LAST_PANIC_ACM_BIOS_WDT_EXPIRED));

    log_wdt_recovery(LAST_RECOVERY_ME_LAUNCH_FAIL, LAST_PANIC_ME_WDT_EXPIRED);
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(2));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(LAST_RECOVERY_ME_LAUNCH_FAIL));
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(2));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), alt_u32(LAST_PANIC_ME_WDT_EXPIRED));
}
