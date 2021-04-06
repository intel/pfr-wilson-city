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
// Include the GTest headers
#include "gtest_headers.h"

#include <math.h>
#include <chrono>
#include <thread>
#include <stdlib.h>

// Include the SYSTEM MOCK and PFR headers
#include "ut_nios_wrapper.h"

class PFRTimerUtilsTest : public testing::Test
{
public:
    virtual void SetUp() { SYSTEM_MOCK::get()->reset(); }

    virtual void TearDown() {}
};

TEST_F(PFRTimerUtilsTest, test_sanity)
{
    EXPECT_EQ(U_TIMER_BANK_TIMER_ACTIVE_MASK, 1 << U_TIMER_BANK_TIMER_ACTIVE_BIT);
}

TEST_F(PFRTimerUtilsTest, test_check_timer_expired)
{
    // Countdown from 20ms
    start_timer(U_TIMER_BANK_TIMER1_ADDR, 1);
    EXPECT_FALSE(is_timer_expired(U_TIMER_BANK_TIMER1_ADDR));

    // Sleep for 500ms. Timer should be expired.
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    EXPECT_TRUE(is_timer_expired(U_TIMER_BANK_TIMER1_ADDR));
}

TEST_F(PFRTimerUtilsTest, test_pause_timer)
{
    // Countdown from 40ms
    start_timer(U_TIMER_BANK_TIMER2_ADDR, 2);
    pause_timer(U_TIMER_BANK_TIMER2_ADDR);

    // Sleep for 100 ms.
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    // Since we paused the timer, it should not have expired
    EXPECT_FALSE(is_timer_expired(U_TIMER_BANK_TIMER2_ADDR));

    resume_timer(U_TIMER_BANK_TIMER2_ADDR);

    // Sleep for 500 ms.
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    // Since we resumed the timer, it should be expired by now
    EXPECT_TRUE(is_timer_expired(U_TIMER_BANK_TIMER2_ADDR));
}

TEST_F(PFRTimerUtilsTest, DISABLED_test_sleep_20ms)
{
    auto clock_start = std::chrono::steady_clock::now();

    // Sleep for 20ms.
    sleep_20ms(1);

    auto clock_end = std::chrono::steady_clock::now();
    auto duration_us = std::chrono::duration_cast<std::chrono::microseconds>(clock_end - clock_start);

    // Expect the delay is within 1 second (giving some tolerance)
    EXPECT_LE((alt_u32) duration_us.count(), alt_u32(1000000));
}

TEST_F(PFRTimerUtilsTest, test_restart_timer)
{
    start_timer(U_TIMER_BANK_TIMER3_ADDR, 1);

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    EXPECT_TRUE(is_timer_expired(U_TIMER_BANK_TIMER3_ADDR));

    // Restart the timer
    start_timer(U_TIMER_BANK_TIMER3_ADDR, 100);
    // Timer should be active
    EXPECT_FALSE(is_timer_expired(U_TIMER_BANK_TIMER3_ADDR));
    EXPECT_GT(IORD(U_TIMER_BANK_TIMER3_ADDR, 0) & U_TIMER_BANK_TIMER_VALUE_MASK, (alt_u32) 0);
    EXPECT_TRUE(IORD(U_TIMER_BANK_TIMER3_ADDR, 0) & U_TIMER_BANK_TIMER_ACTIVE_MASK);
    EXPECT_TRUE(check_bit(U_TIMER_BANK_TIMER3_ADDR, U_TIMER_BANK_TIMER_ACTIVE_BIT));
}
