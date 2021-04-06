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

#ifndef SYSTEM_SPI_CONTROL_MOCK_H
#define SYSTEM_SPI_CONTROL_MOCK_H

// Standard headers
#include <memory>
#include <string>
#include <unordered_map>
#include <queue>

// Mock headers
#include "alt_types_mock.h"
#include "memory_mock.h"
#include "unordered_map_memory_mock.h"
#include "spi_flash_mock.h"
#include "nios_gpio_mock.h"

// PFR system
#include "pfr_sys.h"
#include "spi_common.h"

class SPI_CONTROL_MOCK : public MEMORY_MOCK_IF
{
public:
    static SPI_CONTROL_MOCK* get();

    void reset() override;

    alt_u32 get_mem_word(void* addr) override;
    void set_mem_word(void* addr, alt_u32 data) override;

    bool is_addr_in_range(void* addr) override;

    alt_u32 get_4kb_erase_count() {return m_4kb_erase_counter;}
    alt_u32 get_64kb_erase_count() {return m_64kb_erase_counter;}

private:
    // Singleton inst
    static SPI_CONTROL_MOCK* s_inst;

    SPI_CONTROL_MOCK();
    ~SPI_CONTROL_MOCK();

    // Memory area for SPI write enable memory
    UNORDERED_MAP_MEMORY_MOCK<U_SPI_FILTER_BMC_WE_AVMM_BRIDGE_BASE, U_SPI_FILTER_BMC_WE_AVMM_BRIDGE_SPAN> m_bmc_we_mem;
    UNORDERED_MAP_MEMORY_MOCK<U_SPI_FILTER_PCH_WE_AVMM_BRIDGE_BASE, U_SPI_FILTER_PCH_WE_AVMM_BRIDGE_SPAN> m_pch_we_mem;

    // Memory area for SPI CSR interface
    UNORDERED_MAP_MEMORY_MOCK<U_SPI_FILTER_CSR_AVMM_BRIDGE_0_BASE, U_SPI_FILTER_CSR_AVMM_BRIDGE_0_SPAN> m_spi_master_csr;

    // Counters
    alt_u32 m_4kb_erase_counter;
    alt_u32 m_64kb_erase_counter;

    // Instance of the SPI flash mock
    SPI_FLASH_MOCK* m_spi_flash_mock_inst = SPI_FLASH_MOCK::get();
};

#endif /* SYSTEM_SPI_CONTROL_MOCK_H */
