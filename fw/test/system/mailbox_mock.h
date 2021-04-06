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

#ifndef SYSTEM_MAILBOX_MOCK_H
#define SYSTEM_MAILBOX_MOCK_H

// Standard headers
#include <memory>
#include <string>
#include <unordered_map>
#include <queue>

// Mock headers
#include "alt_types_mock.h"
#include "memory_mock.h"
#include "unordered_map_memory_mock.h"

// PFR system
#include "pfr_sys.h"

class MAILBOX_MOCK : public MEMORY_MOCK_IF
{
public:
    MAILBOX_MOCK();
    virtual ~MAILBOX_MOCK();

    void reset() override;

    alt_u32 get_mem_word(void* addr) override;
    void set_mem_word(void* addr, alt_u32 data) override;

    bool is_addr_in_range(void* addr) override;

private:
    // Memory area for SMBus Mailbox Register File
    UNORDERED_MAP_MEMORY_MOCK<U_MAILBOX_AVMM_BRIDGE_BASE, U_MAILBOX_AVMM_BRIDGE_SPAN>
        m_mailbox_reg_file;

    std::queue<alt_u8> m_fifo;
    void flush_fifo();
};

#endif /* SYSTEM_MAILBOX_MOCK_H_ */
