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
#include "ufm_mock.h"

// Code headers

// Static data
UFM_MOCK* UFM_MOCK::s_inst = nullptr;

// Type definitions

// Return the singleton instance of spi flash mock
UFM_MOCK* UFM_MOCK::get()
{
    if (s_inst == nullptr)
    {
        s_inst = new UFM_MOCK();
    }
    return s_inst;
}

// Constructor/Destructor
UFM_MOCK::UFM_MOCK()
{
    m_flash_mem = new alt_u32[U_UFM_DATA_SPAN/4];
}

UFM_MOCK::~UFM_MOCK()
{
    delete[] m_flash_mem;
}

// Class methods

void UFM_MOCK::reset()
{
    // SPI flash contains all FFs when empty
    alt_u32* flash_mem_ptr =  m_flash_mem;
    for (int i = 0; i < U_UFM_DATA_SPAN / 4; i++)
    {
        flash_mem_ptr[i] = 0xffffffff;
    }
}

