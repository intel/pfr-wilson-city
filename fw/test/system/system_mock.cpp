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

// Standard headers
#include <memory>
#include <string>
#include <unordered_map>
#include <functional>
#include <iostream>
#include <fstream>

#include <boost/stacktrace.hpp>

// Mock headers
#include "alt_types_mock.h"
#include "bsp_mock.h"

// PFR system
#include "pfr_sys.h"

// Code headers

// Test headers
#include "system_mock.h"
#include "memory_mock.h"
#include "unordered_map_memory_mock.h"
#include "array_memory_mock.h"
#include "mailbox_mock.h"
#include "crypto_mock.h"
#include "rfnvram_mock.h"
#include "timer_mock.h"
#include "dual_config_mock.h"

// Static data
SYSTEM_MOCK* SYSTEM_MOCK::s_inst = nullptr;

// Type definitions

// Constructor/Destructor
SYSTEM_MOCK::SYSTEM_MOCK() : m_assert_abort_or_throw(true), m_malloc_rwdata_offset(0)
{
    // Make the mocks
    // Create the Nios RAM
    m_memory_mocks.push_back(
            std::make_unique<UNORDERED_MAP_MEMORY_MOCK<U_NIOS_RAM_BASE, U_NIOS_RAM_SPAN>>());

    // Create the global state register
    m_memory_mocks.push_back(std::make_unique<ARRAY_MEMORY_MOCK<U_GLOBAL_STATE_REG_BASE, U_GLOBAL_STATE_REG_SPAN>>());

    // Create the SMBus Mailbox
    m_memory_mocks.push_back(std::make_unique<MAILBOX_MOCK>());

    // Create the crypto block
    m_memory_mocks.push_back(std::make_unique<CRYPTO_MOCK>());

    // Create the timer
    m_memory_mocks.push_back(std::make_unique<TIMER_MOCK>());

    // Create the RFNVRAM
    m_memory_mocks.push_back(std::make_unique<RFNVRAM_MOCK>());

    // Create the Dual Configuration block
    m_memory_mocks.push_back(std::make_unique<DUAL_CONFIG_MOCK>());

    // Temporary UFM CSR interface
    m_memory_mocks.push_back(std::make_unique<UNORDERED_MAP_MEMORY_MOCK<U_UFM_CSR_BASE, U_UFM_CSR_SPAN>>());
}

SYSTEM_MOCK::~SYSTEM_MOCK() {}

// Static methods
SYSTEM_MOCK* SYSTEM_MOCK::get()
{
    if (s_inst == nullptr)
    {
        s_inst = new SYSTEM_MOCK();
    }
    return s_inst;
}

// Class methods

bool SYSTEM_MOCK::is_addr_in_range(void* addr)
{
    for (auto& mock : m_memory_mocks)
    {
        if (mock->is_addr_in_range(addr))
        {
            return true;
        }
    }

    if (m_nios_gpio_mock_inst->is_addr_in_range(addr)
            || m_spi_control_mock_inst->is_addr_in_range(addr))
    {
        return true;
    }

    return false;
}

void SYSTEM_MOCK::reset_ip_mocks()
{
    // Clear all memory mocks
    for (auto& mock : m_memory_mocks)
    {
        mock->reset();
    }

    // Reset SMBus Relay Mock
    smbus_relay_mock_ptr->reset();

    // Reset NIOS GPIOs
    m_nios_gpio_mock_inst->reset();

    // Reset SPI control block
    m_spi_control_mock_inst->reset();

    // Don't do reset of m_spi_flash_mock_inst because it takes some times.
    // Unittests do this reset as needed in their setup.
}

void SYSTEM_MOCK::reset()
{
    // Reset IP mock modules
    reset_ip_mocks();

    // Set all asserts to call abort();
    set_assert_to_abort();

    // Clear the rw data allocator.
    m_malloc_rwdata_offset = 0;

    // Clear all RW callbacks
    m_read_write_callbacks.clear();

    // Clear all the inserted code blocks
    m_code_blocks_to_be_inserted.clear();

    // Reset internal counters
    m_code_block_counter = 0;
    m_t_minus_1_counter = 0;
    m_t_minus_1_bmc_only_counter = 0;
    m_t_minus_1_pch_only_counter = 0;

    // Reset the UFM & CFM
    m_ufm_mock_inst->reset();
}

void SYSTEM_MOCK::throw_internal_error(const std::string& msg, int line, const std::string& file)
{
    if (m_assert_abort_or_throw)
    {
        //        std::cerr << "Internal Error: " << msg << ", File: " << file << ", Line: " << line
        //        << std::endl; std::cerr << boost::stacktrace::stacktrace();
        std::cout << "Internal Error: " << msg << ", File: " << file << ", Line: " << line
                << std::endl;
        std::cout << boost::stacktrace::stacktrace();
        std::abort();
    }
    else
    {
        throw - 1;
    }
}

alt_u32* SYSTEM_MOCK::malloc_rwdata(alt_u32 num_bytes)
{
    alt_u32* start_addr;

    // Mock rodata in the ram space
    start_addr = __IO_CALC_ADDRESS_NATIVE_ALT_U32(U_NIOS_RAM_BASE, 0) +
            m_malloc_rwdata_offset / (sizeof(alt_u32));

    m_malloc_rwdata_offset += num_bytes;

    return start_addr;
}

alt_u32 SYSTEM_MOCK::get_mem_word(void* addr, bool nocallbacks)
{
    alt_u32 ret = 0;
    bool found_mock = false;

    if (!nocallbacks)
    {
        for (auto fn : m_read_write_callbacks)
        {
            fn(READ_OR_WRITE::READ, addr, ret);
        }
    }

    // Dispatch to the appropriate handler based on the address
    for (auto& mock : m_memory_mocks)
    {
        if (mock->is_addr_in_range(addr))
        {
            ret = mock->get_mem_word(addr);
            found_mock = true;
            break;
        }
    }

    // Handle Nios GPIO memory range
    if (m_nios_gpio_mock_inst->is_addr_in_range(addr))
    {
        ret = m_nios_gpio_mock_inst->get_mem_word(addr);
        found_mock = true;
    }

    // Handle SPI Control block range
    if (m_spi_control_mock_inst->is_addr_in_range(addr))
    {
        ret = m_spi_control_mock_inst->get_mem_word(addr);
        found_mock = true;
    }

    if (!found_mock)
    {
        PFR_INTERNAL_ERROR_VARG("Undefined handler for address %p", addr);
    }

    return ret;
}

void SYSTEM_MOCK::set_mem_word(void* addr, alt_u32 data, bool nocallbacks)
{
    bool found_mock = false;

    if (!nocallbacks)
    {
        for (auto fn : m_read_write_callbacks)
        {
            fn(READ_OR_WRITE::WRITE, addr, data);
        }
    }

    for (auto& mock : m_memory_mocks)
    {
        if (mock->is_addr_in_range(addr))
        {
            mock->set_mem_word(addr, data);
            found_mock = true;
            break;
        }
    }

    // Handle Nios GPIO memory range
    if (m_nios_gpio_mock_inst->is_addr_in_range(addr))
    {
        m_nios_gpio_mock_inst->set_mem_word(addr, data);
        found_mock = true;
    }

    // Handle SPI Control block range
    if (m_spi_control_mock_inst->is_addr_in_range(addr))
    {
        m_spi_control_mock_inst->set_mem_word(addr, data);
        found_mock = true;
    }

    if (!found_mock)
    {
        PFR_INTERNAL_ERROR_VARG("Undefined handler for address %p", addr);
    }
}

alt_u32 SYSTEM_MOCK::get_spi_cmd_count(SPI_COMMAND_ENUM spi_cmd)
{
    if (spi_cmd == SPI_CMD_4KB_SECTOR_ERASE)
    {
        return m_spi_control_mock_inst->get_4kb_erase_count();
    }
    else if (spi_cmd == SPI_CMD_64KB_SECTOR_ERASE)
    {
        return m_spi_control_mock_inst->get_64kb_erase_count();
    }

    return 0;
}

/**
 * @brief Reset some portion of System Mock to simulate what would happen
 * after a CPLD reconfiguration. Mailbox registers, for example, are being
 * reset to 0s in this function.
 */
void SYSTEM_MOCK::reset_after_cpld_reconfiguration()
{
    // Reset code block counter
    reset_code_block_counter();

    // Reset counters for T-1 entries
    reset_t_minus_1_counter();
    m_t_minus_1_bmc_only_counter = 0;
    m_t_minus_1_pch_only_counter = 0;

    // Clear all RW callbacks
    m_read_write_callbacks.clear();

    // Reset the mailbox
    alt_u32* mb_ptr = (alt_u32*) __IO_CALC_ADDRESS_NATIVE(U_MAILBOX_AVMM_BRIDGE_BASE, 0);
    for (alt_u32 offset = 0; offset < U_MAILBOX_AVMM_BRIDGE_SPAN /4 ; offset++)
    {
        set_mem_word(mb_ptr + offset, 0, false);
    }
}

void SYSTEM_MOCK::init_nios_mem_from_file(const std::string& file_path, alt_u32* dest_addr)
{
    std::ifstream bin_file;
    bin_file.open(file_path, std::ios::binary | std::ios::in);

    if (!bin_file.is_open())
    {
        PFR_INTERNAL_ERROR_VARG("Unable to open %s for read. ", file_path.c_str());
    }

    char buffer[4];
    while (bin_file.read(buffer, 4))
    {
        write_words_to_nios_mem(dest_addr, (const alt_u8*) buffer, 4);
        dest_addr++;
    }

    bin_file.close();
}

void SYSTEM_MOCK::init_x86_mem_from_file(const std::string& file_path, alt_u32* dest_addr)
{
    std::ifstream bin_file;
    bin_file.open(file_path, std::ios::binary | std::ios::in);

    if (!bin_file.is_open())
    {
        PFR_INTERNAL_ERROR_VARG("Unable to open %s for read. ", file_path.c_str());
    }

    char buffer[4];
    while (bin_file.read(buffer, 4))
    {
        *dest_addr = *((alt_u32*) buffer);
        dest_addr++;
    }

    bin_file.close();
}

void SYSTEM_MOCK::write_x86_mem_to_file(const std::string& file_path,
        alt_u8* src_addr,
        alt_u32 nbytes)
{
    std::ofstream bin_file;
    bin_file.open(file_path, std::ios::binary | std::ios::out);

    if (!bin_file.is_open())
    {
        PFR_INTERNAL_ERROR_VARG("Unable to open %s for write. ", file_path.c_str());
    }

    bin_file.write((char*) src_addr, nbytes);
    bin_file.close();
}

/**
 * This function allows user to pass data from host memory to BMC flash.
 */
void SYSTEM_MOCK::write_words_to_nios_mem(alt_u32* nios_dest_mem_ptr,
        const alt_u32* x86_src_mem_ptr,
        const alt_u32 num_bytes)
{
    alt_u32 word_sz = num_bytes / sizeof(alt_u32);

    for (alt_u32 i = 0; i < word_sz; i++)
    {
        set_mem_word(nios_dest_mem_ptr + i, x86_src_mem_ptr[i]);
    }
}

void SYSTEM_MOCK::write_words_to_nios_mem(alt_u32* nios_dest_mem_ptr,
        const alt_u8* x86_src_mem_ptr,
        const alt_u32 num_bytes)
{
    for (alt_u32 i = 0; i < num_bytes; i += 4)
    {
        // Reorder the bytes to create the same byte stream
        alt_u32 data = (x86_src_mem_ptr[i] << 24) | (x86_src_mem_ptr[i + 1] << 16) |
                (x86_src_mem_ptr[i + 2] << 8) | (x86_src_mem_ptr[i + 3] << 0);

        set_mem_word(nios_dest_mem_ptr + (i / 4), data);
    }
}

void SYSTEM_MOCK::write_words_to_x86_mem(alt_u32* x86_dest_mem_ptr,
        const alt_u32* nios_src_mem_ptr,
        const alt_u32 num_bytes)
{
    alt_u32 word_sz = num_bytes / sizeof(alt_u32);

    for (alt_u32 i = 0; i < word_sz; i++)
    {
        x86_dest_mem_ptr[i] = get_mem_word((void*) (nios_src_mem_ptr + i), true);
    }
}

