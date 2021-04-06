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
#include "smbus_relay_mock.h"

// Code headers
#include "gen_smbus_relay_config.h"

// Static data

// Type definitions

// Constructor/Destructor
SMBUS_RELAY_MOCK::SMBUS_RELAY_MOCK()
{
    m_relay1_cmd_en_mem = new alt_u32[U_RELAY1_AVMM_BRIDGE_SPAN/4];
    m_relay2_cmd_en_mem = new alt_u32[U_RELAY2_AVMM_BRIDGE_SPAN/4];
    m_relay3_cmd_en_mem = new alt_u32[U_RELAY3_AVMM_BRIDGE_SPAN/4];
}

SMBUS_RELAY_MOCK::~SMBUS_RELAY_MOCK()
{
    delete[] m_relay1_cmd_en_mem;
    delete[] m_relay2_cmd_en_mem;
    delete[] m_relay3_cmd_en_mem;
}

// Class methods

void SMBUS_RELAY_MOCK::reset()
{
    for (alt_u32 i = 0; i < U_RELAY1_AVMM_BRIDGE_SPAN/4; i++)
    {
        m_relay1_cmd_en_mem[i] = 0;
    }
    for (alt_u32 i = 0; i < U_RELAY2_AVMM_BRIDGE_SPAN/4; i++)
    {
        m_relay2_cmd_en_mem[i] = 0;
    }
    for (alt_u32 i = 0; i < U_RELAY3_AVMM_BRIDGE_SPAN/4; i++)
    {
        m_relay3_cmd_en_mem[i] = 0;
    }
}

alt_u32* SMBUS_RELAY_MOCK::get_cmd_enable_memory_for_smbus(alt_u32 bus_id)
{
    if (bus_id == 1)
    {
        return m_relay1_cmd_en_mem;
    }
    else if (bus_id == 2)
    {
        return m_relay2_cmd_en_mem;
    }
    // bus_id == 3
    return m_relay3_cmd_en_mem;
};
