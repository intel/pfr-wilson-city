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

#ifndef SYSTEM_SMBUS_RELAY_MOCK_H
#define SYSTEM_SMBUS_RELAY_MOCK_H

// Standard headers
#include <memory>
#include <string>
#include <unordered_map>

// Mock headers
#include "alt_types_mock.h"
#include "memory_mock.h"
#include "unordered_map_memory_mock.h"

// PFR system
#include "pfr_sys.h"

class SMBUS_RELAY_MOCK
{
public:
    SMBUS_RELAY_MOCK();
    virtual ~SMBUS_RELAY_MOCK();

    void reset();

    alt_u32* get_cmd_enable_memory_for_smbus(alt_u32 bus_id);

private:
    // Memory area for SMBus relays
    alt_u32 *m_relay1_cmd_en_mem = nullptr;
    alt_u32 *m_relay2_cmd_en_mem = nullptr;
    alt_u32 *m_relay3_cmd_en_mem = nullptr;
};

#endif /* SYSTEM_SMBUS_RELAY_MOCK_H_ */
