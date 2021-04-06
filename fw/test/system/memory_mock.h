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

#ifndef INC_SYSTEM_MEMORY_MOCK_H
#define INC_SYSTEM_MEMORY_MOCK_H

// Standard headers
#include <memory>

// Mock headers
#include "alt_types_mock.h"

// All system mocks should implement this interface
class MEMORY_MOCK_IF
{
public:
    MEMORY_MOCK_IF() {}
    virtual ~MEMORY_MOCK_IF() {}

    virtual alt_u32 get_mem_word(void* addr) = 0;
    virtual void set_mem_word(void* addr, alt_u32 data) = 0;
    virtual void reset() = 0;

    bool is_addr_in_range(std::uintptr_t addr, std::uintptr_t start_addr, alt_u32 span)
    {
        return (addr >= (start_addr)) && (addr < (start_addr + span));
    }
    bool is_addr_in_range(void* addr, void* start_addr, alt_u32 span)
    {
        std::uintptr_t addr_int = reinterpret_cast<std::uintptr_t>(addr);
        std::uintptr_t start_addr_int = reinterpret_cast<std::uintptr_t>(start_addr);
        return is_addr_in_range(addr_int, start_addr_int, span);
    }
    virtual bool is_addr_in_range(void* addr) = 0;
};

#endif /* INC_SYSTEM_MEMORY_MOCK_H */
