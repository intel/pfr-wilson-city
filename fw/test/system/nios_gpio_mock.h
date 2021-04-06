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

#ifndef SYSTEM_NIOS_GPIO_MOCK_H
#define SYSTEM_NIOS_GPIO_MOCK_H

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

#include "gen_gpi_signals.h"
#include "gen_gpo_controls.h"

class NIOS_GPIO_MOCK : public MEMORY_MOCK_IF
{
public:
    static NIOS_GPIO_MOCK* get();

    void reset() override;

    alt_u32 get_mem_word(void* addr) override;
    void set_mem_word(void* addr, alt_u32 data) override;

    bool is_addr_in_range(void* addr) override;

    // Re-implement Nios firmware's check_bit for use in system mock environnment
    alt_u32 check_bit(void* addr, alt_u32 shift_bit);

private:
    // Singleton inst
    static NIOS_GPIO_MOCK* s_inst;

    NIOS_GPIO_MOCK();
    ~NIOS_GPIO_MOCK();

    // Memory area for GPIOs
    UNORDERED_MAP_MEMORY_MOCK<U_GPO_1_BASE, U_GPO_1_SPAN> m_nios_gpo_1;
    UNORDERED_MAP_MEMORY_MOCK<U_GPI_1_BASE, U_GPI_1_SPAN> m_nios_gpi_1;
};

#endif /* SYSTEM_NIOS_GPIO_MOCK_H */
