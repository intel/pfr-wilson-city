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
#include "nios_gpio_mock.h"

// Static data
NIOS_GPIO_MOCK* NIOS_GPIO_MOCK::s_inst = nullptr;

// Return the singleton instance of Nios GPIO mock
NIOS_GPIO_MOCK* NIOS_GPIO_MOCK::get()
{
    if (s_inst == nullptr)
    {
        s_inst = new NIOS_GPIO_MOCK();
    }
    return s_inst;
}

// Constructor/Destructor
NIOS_GPIO_MOCK::NIOS_GPIO_MOCK() {}
NIOS_GPIO_MOCK::~NIOS_GPIO_MOCK() {}

// Class methods
void NIOS_GPIO_MOCK::reset()
{
    m_nios_gpo_1.reset();
    m_nios_gpi_1.reset();
}

bool NIOS_GPIO_MOCK::is_addr_in_range(void* addr)
{
    return m_nios_gpo_1.is_addr_in_range(addr) || m_nios_gpi_1.is_addr_in_range(addr);
}

alt_u32 NIOS_GPIO_MOCK::get_mem_word(void* addr)
{
    if (m_nios_gpo_1.is_addr_in_range(addr))
    {
        return m_nios_gpo_1.get_mem_word(addr);
    }
    else if (m_nios_gpi_1.is_addr_in_range(addr))
    {
        return m_nios_gpi_1.get_mem_word(addr);
    }
    return 0;
}

void NIOS_GPIO_MOCK::set_mem_word(void* addr, alt_u32 data)
{
    if (m_nios_gpo_1.is_addr_in_range(addr))
    {
        m_nios_gpo_1.set_mem_word(addr, data);
    }
    else if (m_nios_gpi_1.is_addr_in_range(addr))
    {
        m_nios_gpi_1.set_mem_word(addr, data);
    }
}

alt_u32 NIOS_GPIO_MOCK::check_bit(void* addr, alt_u32 shift_bit)
{
    alt_u32 gpio_word = 0;
    if (m_nios_gpo_1.is_addr_in_range(addr))
    {
        gpio_word = m_nios_gpo_1.get_mem_word(addr);
    }
    else if (m_nios_gpi_1.is_addr_in_range(addr))
    {
        gpio_word = m_nios_gpi_1.get_mem_word(addr);
    }

    gpio_word &= (0b1 << shift_bit);
    return (gpio_word >> shift_bit);
}
