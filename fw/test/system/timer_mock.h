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

#ifndef INC_SYSTEM_TIMER_MOCK_H
#define INC_SYSTEM_TIMER_MOCK_H

// Standard headers
#include <memory>
#include <vector>
#include <array>
#include <chrono>
#include <thread>

// Mock headers
#include "alt_types_mock.h"
#include "memory_mock.h"

// BSP headers
#include "pfr_sys.h"
#include "timer_utils.h"

class TIMER_MOCK : public MEMORY_MOCK_IF
{
public:
    TIMER_MOCK();
    virtual ~TIMER_MOCK();

    alt_u32 get_mem_word(void* addr) override;
    void set_mem_word(void* addr, alt_u32 data) override;

    void reset() override;

    bool is_addr_in_range(void* addr) override;

private:
    alt_u32 m_timer_bank_timer1;
    alt_u32 m_timer_bank_timer2;
    alt_u32 m_timer_bank_timer3;

    std::chrono::time_point<std::chrono::steady_clock> m_clock_timer1;
    std::chrono::time_point<std::chrono::steady_clock> m_clock_timer2;
    std::chrono::time_point<std::chrono::steady_clock> m_clock_timer3;

    alt_u32 get_20ms_passed(std::chrono::time_point<std::chrono::steady_clock> clk);
    void update_timers();
};

#endif /* INC_SYSTEM_TIMER_MOCK_H */
