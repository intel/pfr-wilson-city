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

// Standard headers

// Test headers
#include "bsp_mock.h"
#include "timer_mock.h"

// Code headers

// Static data

// Type definitions

TIMER_MOCK::TIMER_MOCK() :
    m_timer_bank_timer1(0),
    m_timer_bank_timer2(0),
    m_timer_bank_timer3(0)
{}

TIMER_MOCK::~TIMER_MOCK() {}



void TIMER_MOCK::reset()
{
    m_timer_bank_timer1 = 0;
    m_timer_bank_timer2 = 0;
    m_timer_bank_timer3 = 0;
}


alt_u32 TIMER_MOCK::get_20ms_passed(std::chrono::time_point<std::chrono::steady_clock> clk)
{
    auto clock_now = std::chrono::steady_clock::now();
    auto duration_us = std::chrono::duration_cast<std::chrono::microseconds>(clock_now - clk);
    return (alt_u32) (duration_us.count() / 1000 / 20);
}

void TIMER_MOCK::update_timers()
{
    if ((m_timer_bank_timer1 & U_TIMER_BANK_TIMER_ACTIVE_MASK) &&
            ((m_timer_bank_timer1 & U_TIMER_BANK_TIMER_VALUE_MASK) > 0))
    {
        // If the timer is active and not expired, update it
        alt_u32 time_passed = get_20ms_passed(m_clock_timer1);
        if (time_passed)
        {
            // If it has been at least 1 second passed, update the timer value
            if (time_passed >= (m_timer_bank_timer1 & U_TIMER_BANK_TIMER_VALUE_MASK))
            {
                m_timer_bank_timer1 = 0 | U_TIMER_BANK_TIMER_ACTIVE_MASK;
            }
            else
            {
                m_timer_bank_timer1 -= time_passed;
            }
            m_clock_timer1 = std::chrono::steady_clock::now();
        }
    }

    if ((m_timer_bank_timer2 & U_TIMER_BANK_TIMER_ACTIVE_MASK) &&
                ((m_timer_bank_timer2 & U_TIMER_BANK_TIMER_VALUE_MASK) > 0))
    {
        // If the timer is active and not expired, update it
        alt_u32 time_passed = get_20ms_passed(m_clock_timer2);
        if (time_passed)
        {
            // If it has been at least 1 second passed, update the timer value
            if (time_passed >= (m_timer_bank_timer2 & U_TIMER_BANK_TIMER_VALUE_MASK))
            {
                m_timer_bank_timer2 = 0 | U_TIMER_BANK_TIMER_ACTIVE_MASK;
            }
            else
            {
                m_timer_bank_timer2 -= time_passed;
            }
            m_clock_timer2 = std::chrono::steady_clock::now();
        }
    }

    if ((m_timer_bank_timer3 & U_TIMER_BANK_TIMER_ACTIVE_MASK) &&
                ((m_timer_bank_timer3 & U_TIMER_BANK_TIMER_VALUE_MASK) > 0))
    {
            // If the timer is active and not expired, update it
        alt_u32 time_passed = get_20ms_passed(m_clock_timer3);
        if (time_passed)
        {
            // If it has been at least 1 second passed, update the timer value
            if (time_passed >= (m_timer_bank_timer3 & U_TIMER_BANK_TIMER_VALUE_MASK))
            {
                m_timer_bank_timer3 = 0 | U_TIMER_BANK_TIMER_ACTIVE_MASK;
            }
            else
            {
                m_timer_bank_timer3 -= time_passed;
            }
            m_clock_timer3 = std::chrono::steady_clock::now();
        }
    }
}

bool TIMER_MOCK::is_addr_in_range(void* addr)
{
    return MEMORY_MOCK_IF::is_addr_in_range(
        addr, __IO_CALC_ADDRESS_NATIVE_ALT_U32(U_TIMER_BANK_AVMM_BRIDGE_BASE, 0), U_TIMER_BANK_AVMM_BRIDGE_SPAN);
}

alt_u32 TIMER_MOCK::get_mem_word(void* addr)
{
    // Update the timers
    update_timers();

    // Read from timers
    if ((std::uintptr_t) addr == U_TIMER_BANK_AVMM_BRIDGE_BASE)
    {
        return m_timer_bank_timer1;
    }
    if ((std::uintptr_t) addr == (U_TIMER_BANK_AVMM_BRIDGE_BASE + (1 << 2)))
    {
        return m_timer_bank_timer2;
    }
    if ((std::uintptr_t) addr == (U_TIMER_BANK_AVMM_BRIDGE_BASE + (2 << 2)))
    {
        return m_timer_bank_timer3;
    }
    else
    {
        PFR_INTERNAL_ERROR("Undefined handler for address");
    }
    return 0;
}

void TIMER_MOCK::set_mem_word(void* addr, alt_u32 data)
{
    if ((std::uintptr_t) addr == U_TIMER_BANK_AVMM_BRIDGE_BASE)
    {
        m_timer_bank_timer1 = data;
        if ((data & U_TIMER_BANK_TIMER_ACTIVE_MASK) &&
                ((data & U_TIMER_BANK_TIMER_VALUE_MASK) > 0))
        {
            // start the internal timer
            m_clock_timer1 = std::chrono::steady_clock::now();
        }
    }
    if ((std::uintptr_t) addr == (U_TIMER_BANK_AVMM_BRIDGE_BASE + (1 << 2)))
    {
        m_timer_bank_timer2 = data;
        if ((data & U_TIMER_BANK_TIMER_ACTIVE_MASK) &&
                ((data & U_TIMER_BANK_TIMER_VALUE_MASK) > 0))
        {
            // start the internal timer
            m_clock_timer2 = std::chrono::steady_clock::now();
        }
    }
    if ((std::uintptr_t) addr == (U_TIMER_BANK_AVMM_BRIDGE_BASE + (2 << 2)))
    {
        m_timer_bank_timer3 = data;
        if ((data & U_TIMER_BANK_TIMER_ACTIVE_MASK) &&
                ((data & U_TIMER_BANK_TIMER_VALUE_MASK) > 0))
        {
            // start the internal timer
            m_clock_timer3 = std::chrono::steady_clock::now();
        }
    }
}


