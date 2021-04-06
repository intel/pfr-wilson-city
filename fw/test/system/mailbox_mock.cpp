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

// Test headers
#include "bsp_mock.h"
#include "mailbox_mock.h"

// Code headers
#include "mailbox_utils.h"

// Static data

// Type definitions

// Constructor/Destructor
MAILBOX_MOCK::MAILBOX_MOCK() {}
MAILBOX_MOCK::~MAILBOX_MOCK() {}

// Class methods

void MAILBOX_MOCK::reset()
{
    this->flush_fifo();
    m_mailbox_reg_file.reset();
}

bool MAILBOX_MOCK::is_addr_in_range(void* addr)
{
    return m_mailbox_reg_file.is_addr_in_range(addr);
}

alt_u32 MAILBOX_MOCK::get_mem_word(void* addr)
{
    // Dispatch to the appropriate handler based on the address
    if ((std::uintptr_t) addr == (U_MAILBOX_AVMM_BRIDGE_BASE + (MB_UFM_WRITE_FIFO << 2)))
    {
        if (!m_fifo.empty())
            return m_fifo.back();
        return 0;
    }
    else if ((std::uintptr_t) addr == (U_MAILBOX_AVMM_BRIDGE_BASE + (MB_UFM_READ_FIFO << 2)))
    {
        if (!m_fifo.empty())
        {
            alt_u32 pop_val = m_fifo.front();
            m_fifo.pop();
            return pop_val;
        }
        return 0;
    }
    else
    {
        return m_mailbox_reg_file.get_mem_word(addr);
    }
    return 0;
}

void MAILBOX_MOCK::set_mem_word(void* addr, alt_u32 data)
{
    if ((std::uintptr_t) addr == (U_MAILBOX_AVMM_BRIDGE_BASE + (MB_UFM_WRITE_FIFO << 2)))
    {
        // By design, the fifo has significantly more space than is needed and will never be full
        m_fifo.push((alt_u8) data);
    }
    else if ((std::uintptr_t) addr == (U_MAILBOX_AVMM_BRIDGE_BASE + (MB_UFM_READ_FIFO << 2)))
    {
        // Throw away writes to the read fifo address
        // Do nothing
    }
    else
    {
        // If bits[2:1] of the command trigger are set, flush the fifo
        if ((std::uintptr_t) addr == (U_MAILBOX_AVMM_BRIDGE_BASE + (MB_UFM_CMD_TRIGGER << 2)) &&
            (data & 0x6))
        {
            this->flush_fifo();
        }
        m_mailbox_reg_file.set_mem_word(addr, data);
    }
}

void MAILBOX_MOCK::flush_fifo()
{
    while (!m_fifo.empty())
        m_fifo.pop();
}
