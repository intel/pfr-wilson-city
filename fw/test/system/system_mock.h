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

#ifndef INC_SYSTEM_MOCK_H
#define INC_SYSTEM_MOCK_H

// Standard headers
#include <memory>
#include <string>
#include <vector>
#include <functional>
#include <algorithm>
#include <string.h>

// Mock headers
#include "alt_types_mock.h"
#include "memory_mock.h"
#include "smbus_relay_mock.h"
#include "spi_control_mock.h"
#include "spi_flash_mock.h"
#include "ufm_mock.h"
#include "nios_gpio_mock.h"
#include "testdata_files.h"

// BSP headers
#include "pfr_sys.h"

// Code headers
#include "ufm.h"
#include "spi_common.h"

// Macros
// Use Nios RAM as a scratchpad for simple NIOS RW testing
#define NIOS_SCRATCHPAD_ADDR __IO_CALC_ADDRESS_NATIVE_ALT_U32(U_NIOS_RAM_BASE, 0)

// Forward class definitions

// Class Definitions
class SYSTEM_MOCK
{
public:
    enum class READ_OR_WRITE
    {
        READ,
        WRITE
    };

    // Names for code blocks that are inserted at compile time
    enum class CODE_BLOCK_TYPES
    {
        T0_OPERATIONS,
        T0_TIMED_BOOT,
        T0_OPERATIONS_END_AFTER_50_ITERS,
        SKIP_TMIN1_OPERATIONS,
        SKIP_FLASH_AUTHENTICATION,
        THROW_FROM_NEVER_EXIT_LOOP,
        THROW_FROM_ROM_IMAGE_TERMINAL_STATE,
        THROW_AFTER_CFM_SWITCH,
    };

    static SYSTEM_MOCK* get();

    void reset();
    void reset_ip_mocks();

    alt_u32 get_mem_word(void* addr, bool nocallbacks = false);
    void set_mem_word(void* addr, alt_u32 data, bool nocallbacks = false);

    void write_words_to_nios_mem(alt_u32* nios_dest_mem_ptr,
            const alt_u32* x86_src_mem_ptr,
            const alt_u32 num_bytes);
    void write_words_to_nios_mem(alt_u32* nios_dest_mem_ptr,
            const alt_u8* x86_src_mem_ptr,
            const alt_u32 num_bytes);
    void write_words_to_x86_mem(alt_u32* x86_dest_mem_ptr,
            const alt_u32* nios_src_mem_ptr,
            const alt_u32 num_bytes);

    // Functions that work with .dat test data file
    void init_nios_mem_from_file(const std::string& file_path, alt_u32* dest_addr);
    void init_x86_mem_from_file(const std::string& file_path, alt_u32* dest_addr);
    void write_x86_mem_to_file(const std::string& file_path, alt_u8* src_addr, alt_u32 nbytes);

    void throw_internal_error(const std::string& msg, int line, const std::string& file);
    void set_assert_to_throw() { m_assert_abort_or_throw = false; }
    void set_assert_to_abort() { m_assert_abort_or_throw = true; }

    bool is_addr_in_range(void* addr);

    alt_u32* malloc_rwdata(alt_u32 num_bytes);

    // These two functions allow us to insert any generic block of code
    // at the pre-defined location upon request in unittests.
    bool should_exec_code_block(CODE_BLOCK_TYPES code_blk_type)
    {
        return (std::find(m_code_blocks_to_be_inserted.begin(), m_code_blocks_to_be_inserted.end(), code_blk_type) !=
                m_code_blocks_to_be_inserted.end());
    }
    void insert_code_block(CODE_BLOCK_TYPES code_blk_type)
    {
        m_code_blocks_to_be_inserted.push_back(code_blk_type);
    }

    alt_u32 get_code_block_counter() {return m_code_block_counter;}
    void incr_code_block_counter() {m_code_block_counter++;}
    void reset_code_block_counter() {m_code_block_counter = 0;}

    /*
     * Track T0/T-1 transitions
     */
    alt_u32 get_t_minus_1_counter() {return m_t_minus_1_counter;}
    void incr_t_minus_1_counter() {m_t_minus_1_counter++;}
    void reset_t_minus_1_counter() {m_t_minus_1_counter = 0;}

    alt_u32 get_t_minus_1_bmc_only_counter() {return m_t_minus_1_bmc_only_counter;}
    void incr_t_minus_1_bmc_only_counter() {m_t_minus_1_bmc_only_counter++;}

    alt_u32 get_t_minus_1_pch_only_counter() {return m_t_minus_1_pch_only_counter;}
    void incr_t_minus_1_pch_only_counter() {m_t_minus_1_pch_only_counter++;}

    void register_read_write_callback(
            const std::function<void(READ_OR_WRITE read_or_write, void* addr, alt_u32 data)>& fn)
    {
        m_read_write_callbacks.push_back(fn);
    }

    /*
     * CPLD reconfig support
     */
    void reset_after_cpld_reconfiguration();

    /*
     * UFM mock utility
     */
    alt_u32* get_ufm_data_ptr() { return m_ufm_mock_inst->get_flash_ptr(); }
    void provision_ufm_data(const std::string& file) { init_x86_mem_from_file(file, m_ufm_mock_inst->get_flash_ptr()); }
    void load_active_image_to_cfm1()
    {
        init_x86_mem_from_file(CFM1_ACTIVE_IMAGE_FILE,
                m_ufm_mock_inst->get_flash_ptr() + UFM_CPLD_ACTIVE_IMAGE_OFFSET/4);
    }
    void erase_ufm_page(alt_u32 addr) { m_ufm_mock_inst->erase_page(addr); }
    void erase_ufm_sector(alt_u32 sector_id) { m_ufm_mock_inst->erase_sector(sector_id); }
    void write_ufm_data(alt_u32 addr, alt_u32* data, alt_u32 nbytes) { m_ufm_mock_inst->write_data(addr, data, nbytes); }

    /*
     * SPI flash mock utility
     */
    // Performing reset on SPI flash takes some time due to the large size.
    // Hence, this reset is not part of the system_mock reset.
    void reset_spi_flash_mock() { m_spi_flash_mock_inst->reset(); }
    void reset_spi_flash(SPI_FLASH_TYPE_ENUM spi_flash_type) { m_spi_flash_mock_inst->reset(spi_flash_type); }

    // Load a binary to the SPI flash.
    void load_to_flash(SPI_FLASH_TYPE_ENUM spi_flash_type, const std::string& file_path,
            int file_size, int load_offset=0)
    {
        m_spi_flash_mock_inst->load(spi_flash_type, file_path, file_size, load_offset);
    }

    // Allow unittests to use x86 address of SPI flash memory.
    // This is required to support pointer dereferencing in the Nios code.
    alt_u32* get_x86_ptr_to_spi_flash() {
        return m_spi_flash_mock_inst->get_spi_flash_ptr();
    }
    alt_u32* get_x86_ptr_to_spi_flash(SPI_FLASH_TYPE_ENUM spi_flash_type) {
        return m_spi_flash_mock_inst->get_spi_flash_ptr(spi_flash_type);
    }

    /*
     * SPI Control block mock utility
     */
    alt_u32 get_spi_cmd_count(SPI_COMMAND_ENUM spi_cmd);

    // Mock SMBus relays
    std::unique_ptr<SMBUS_RELAY_MOCK> smbus_relay_mock_ptr = std::make_unique<SMBUS_RELAY_MOCK>();

private:
    // Singleton inst
    static SYSTEM_MOCK* s_inst;

    // Private constructor/destructor
    SYSTEM_MOCK();
    ~SYSTEM_MOCK();

    // Control the behaviour of PFR_ASSERT
    bool m_assert_abort_or_throw;

    // Internal counters
    // A counter that can be used to track loop iterations in code block
    alt_u32 m_code_block_counter = 0;
    alt_u32 m_t_minus_1_counter = 0;
    alt_u32 m_t_minus_1_bmc_only_counter = 0;
    alt_u32 m_t_minus_1_pch_only_counter = 0;

    // Vector of memory mocks
    std::vector<std::unique_ptr<MEMORY_MOCK_IF>> m_memory_mocks;

    alt_u32 m_malloc_rwdata_offset;

    std::vector<CODE_BLOCK_TYPES> m_code_blocks_to_be_inserted;

    std::vector<std::function<void(READ_OR_WRITE read_or_write, void* addr, alt_u32 data)>>
            m_read_write_callbacks;

    // Mock instances
    // Mock SPI flashes
    SPI_FLASH_MOCK* m_spi_flash_mock_inst = SPI_FLASH_MOCK::get();
    // Mock SPI Control block
    SPI_CONTROL_MOCK* m_spi_control_mock_inst = SPI_CONTROL_MOCK::get();

    // Mock UFM
    UFM_MOCK* m_ufm_mock_inst = UFM_MOCK::get();

    // Mock NIOS GPIO
    NIOS_GPIO_MOCK* m_nios_gpio_mock_inst = NIOS_GPIO_MOCK::get();
};

#endif /* INC_SYSTEM_MOCK_H_ */
