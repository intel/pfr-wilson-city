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
#include "spi_control_mock.h"

// Code headers
#include "spi_common.h"

// Static data
SPI_CONTROL_MOCK* SPI_CONTROL_MOCK::s_inst = nullptr;

// Type definitions

#define SPI_CONTROL_1_CSR_CS_FLASH_COMMAND_SETTING_ADDR \
    __IO_CALC_ADDRESS_NATIVE_ALT_U32(U_SPI_FILTER_CSR_AVMM_BRIDGE_0_BASE, SPI_CONTROL_1_CSR_CS_FLASH_COMMAND_SETTING_OFST)
#define SPI_CONTROL_1_CSR_CS_FLASH_COMMAND_ADDRESS_ADDR \
    __IO_CALC_ADDRESS_NATIVE_ALT_U32(U_SPI_FILTER_CSR_AVMM_BRIDGE_0_BASE, SPI_CONTROL_1_CSR_CS_FLASH_COMMAND_ADDRESS_OFST)

// Return the singleton instance of spi flash mock
SPI_CONTROL_MOCK* SPI_CONTROL_MOCK::get()
{
    if (s_inst == nullptr)
    {
        s_inst = new SPI_CONTROL_MOCK();
    }
    return s_inst;
}

// Constructor/Destructor
SPI_CONTROL_MOCK::SPI_CONTROL_MOCK()
{
    m_4kb_erase_counter = 0;
    m_64kb_erase_counter = 0;
}

SPI_CONTROL_MOCK::~SPI_CONTROL_MOCK() {}

// Class methods

void SPI_CONTROL_MOCK::reset()
{
    m_bmc_we_mem.reset();
    m_pch_we_mem.reset();
    m_spi_master_csr.reset();

    m_4kb_erase_counter = 0;
    m_64kb_erase_counter = 0;
}

bool SPI_CONTROL_MOCK::is_addr_in_range(void* addr)
{
    return m_bmc_we_mem.is_addr_in_range(addr) || m_pch_we_mem.is_addr_in_range(addr) || m_spi_master_csr.is_addr_in_range(addr);
}

alt_u32 SPI_CONTROL_MOCK::get_mem_word(void* addr)
{
    if (m_bmc_we_mem.is_addr_in_range(addr))
    {
        return m_bmc_we_mem.get_mem_word(addr);
    }
    else if (m_pch_we_mem.is_addr_in_range(addr))
    {
        return m_pch_we_mem.get_mem_word(addr);
    }
    return m_spi_master_csr.get_mem_word(addr);
}

void SPI_CONTROL_MOCK::set_mem_word(void* addr, alt_u32 data)
{
    if (m_bmc_we_mem.is_addr_in_range(addr))
    {
        m_bmc_we_mem.set_mem_word(addr, data);
    }
    else if (m_pch_we_mem.is_addr_in_range(addr))
    {
        m_pch_we_mem.set_mem_word(addr, data);
    }
    // In CSR memory range
    m_spi_master_csr.set_mem_word(addr, data);
    std::uintptr_t addr_int = reinterpret_cast<std::uintptr_t>(addr);
    alt_u32 csr_offset = (addr_int - U_SPI_FILTER_CSR_AVMM_BRIDGE_0_BASE) >> 2;

    if (csr_offset == SPI_CONTROL_1_CSR_CS_FLASH_COMMAND_CONTROL_OFST)
    {
        // This triggers the SPI master IP to send command to the SPI device
        if (data == 0x1)
        {
            // Collect the command and address info
            alt_u32 spi_command = m_spi_master_csr.get_mem_word(SPI_CONTROL_1_CSR_CS_FLASH_COMMAND_SETTING_ADDR) & 0xFF;
            alt_u32 spi_addr = m_spi_master_csr.get_mem_word(SPI_CONTROL_1_CSR_CS_FLASH_COMMAND_ADDRESS_ADDR);
            alt_u32* spi_ptr = m_spi_flash_mock_inst->get_spi_flash_ptr();

            // Go through list of supported command
            if (spi_command == SPI_CMD_4KB_SECTOR_ERASE)
            {
                alt_u32* spi_region_start_addr = spi_ptr + (spi_addr >> 2);
                alt_u32* spi_region_end_addr = spi_region_start_addr + (0x1000 >> 2);
                std::fill(spi_region_start_addr, spi_region_end_addr, 0xFFFFFFFF);

                m_4kb_erase_counter++;
            }
            else if (spi_command == SPI_CMD_64KB_SECTOR_ERASE)
            {
                alt_u32* spi_region_start_addr = spi_ptr + (spi_addr >> 2);
                alt_u32* spi_region_end_addr = spi_region_start_addr + (0x10000 >> 2);
                std::fill(spi_region_start_addr, spi_region_end_addr, 0xFFFFFFFF);

                m_64kb_erase_counter++;
            }
        }
    }
}
