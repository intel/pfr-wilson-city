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
#include "spi_flash_mock.h"

// Code headers

// Static data
SPI_FLASH_MOCK* SPI_FLASH_MOCK::s_inst = nullptr;

// Type definitions

// Return the singleton instance of spi flash mock
SPI_FLASH_MOCK* SPI_FLASH_MOCK::get()
{
    if (s_inst == nullptr)
    {
        s_inst = new SPI_FLASH_MOCK();
    }
    return s_inst;
}

// Constructor/Destructor
SPI_FLASH_MOCK::SPI_FLASH_MOCK()
{
    // Both PCH and BMC are initialized to be 0x08000000 Bytes in size
    m_pch_flash_mem = new alt_u32[U_SPI_FILTER_AVMM_BRIDGE_SPAN/4];
    m_bmc_flash_mem = new alt_u32[U_SPI_FILTER_AVMM_BRIDGE_SPAN/4];
}

SPI_FLASH_MOCK::~SPI_FLASH_MOCK()
{
    delete[] m_pch_flash_mem;
    delete[] m_bmc_flash_mem;
}

// Class methods

void SPI_FLASH_MOCK::reset()
{
    // SPI flash contains all FFs when empty
    for (int i = 0; i < U_SPI_FILTER_AVMM_BRIDGE_SPAN / 4; i++)
    {
        m_pch_flash_mem[i] = 0xffffffff;
        m_bmc_flash_mem[i] = 0xffffffff;
    }
}

void SPI_FLASH_MOCK::reset(SPI_FLASH_TYPE_ENUM spi_flash_type)
{
    // Select one of the SPI flashes to clear
    alt_u32* flash_mem_ptr =  m_pch_flash_mem;
    if (spi_flash_type == SPI_FLASH_BMC)
    {
        flash_mem_ptr =  m_bmc_flash_mem;
    }

    // SPI flash contains all FFs when empty
    for (int i = 0; i < U_SPI_FILTER_AVMM_BRIDGE_SPAN / 4; i++)
    {
        flash_mem_ptr[i] = 0xffffffff;
    }
}

void SPI_FLASH_MOCK::load(SPI_FLASH_TYPE_ENUM spi_flash_type, const std::string& file_path,
        int file_size, int load_offset=0)
{
    PFR_ASSERT(load_offset % 4 == 0);

    // Use one of the SPI flashes
    alt_u32* flash_mem_ptr =  m_pch_flash_mem;
    if (spi_flash_type == SPI_FLASH_BMC)
    {
        flash_mem_ptr =  m_bmc_flash_mem;
    }

    // Skip memory to get to the load offset
    flash_mem_ptr +=  load_offset >> 2;

    // Starting at load_offset, load the binary to the flash memory mock.
    std::ifstream bin_file;
    bin_file.open(file_path, std::ios::binary | std::ios::in);

    if (!bin_file.is_open())
    {
        PFR_INTERNAL_ERROR_VARG("Unable to open %s for read. ", file_path.c_str());
    }

    bin_file.read((char *) flash_mem_ptr, file_size);
    bin_file.close();
}

