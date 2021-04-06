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

/**
 * @file spi_rw_utils.h
 * @brief Perform read/write/erase to the SPI flashes through SPI control block.
 */

#ifndef WHITLEY_INC_SPI_RW_UTILS_H
#define WHITLEY_INC_SPI_RW_UTILS_H

// Always include pfr_sys.h first
#include "pfr_sys.h"

#include "keychain_utils.h"
#include "pfr_pointers.h"
#include "spi_common.h"
#include "utils.h"

#define SPI_FLASH_PAGE_SIZE_OF_4KB 0x1000
#define SPI_FLASH_PAGE_SIZE_OF_64KB 0x10000

static PFR_ALT_INLINE void PFR_ALT_ALWAYS_INLINE write_to_spi_ctrl_1_csr(
        SPI_CONTROL_1_CSR_OFFSET_ENUM offset, alt_u32 data)
{
    // The CSR offset here is safe to cast to alt_u32
    // There is only a handful of these offsets and they are 1 apart.
    IOWR(SPI_CONTROL_1_CSR_BASE_ADDR, (alt_u32) offset, data);
}

static PFR_ALT_INLINE alt_u32 PFR_ALT_ALWAYS_INLINE read_from_spi_ctrl_1_csr(
        SPI_CONTROL_1_CSR_OFFSET_ENUM offset)
{
    // The CSR offset here is safe to cast to alt_u32
    // There is only a handful of these offsets and they are 1 apart.
    return IORD(SPI_CONTROL_1_CSR_BASE_ADDR, (alt_u32) offset);
}

/**
 * @brief Notify the SPI master to start sending the command.
 */
static PFR_ALT_INLINE void PFR_ALT_ALWAYS_INLINE trigger_spi_send_cmd()
{
    write_to_spi_ctrl_1_csr(SPI_CONTROL_1_CSR_CS_FLASH_COMMAND_CONTROL_OFST, 0x0001);
}

/**
 *  @brief Read the status register from the flash device
 */
static alt_u32 read_spi_chip_id()
{
    write_to_spi_ctrl_1_csr(SPI_CONTROL_1_CSR_CS_OPERATING_PROTOCOLS_OFST, 0x22220);
    write_to_spi_ctrl_1_csr(SPI_CONTROL_1_CSR_CS_FLASH_COMMAND_SETTING_OFST, 0x3800 | SPI_CMD_READ_ID);
    trigger_spi_send_cmd();
    return read_from_spi_ctrl_1_csr(SPI_CONTROL_1_CSR_CS_FLASH_COMMAND_READ_DATA0_OFST);
}

/**
 * @brief Allow CPLD to switch between the BMC flash and the PCH flash
 * Set GPO_1_SPI_MASTER_BMC_PCHN to 1 to have CPLD master talk to the BMC flash
 * Set to 0 to have CPLD master talk to the PCH Flash
 *
 * @param spi_flash_type indicates BMC or PCH flash
 */
static void switch_spi_flash(
        SPI_FLASH_TYPE_ENUM spi_flash_type)
{
    if (spi_flash_type == SPI_FLASH_BMC)
    {
        set_bit(U_GPO_1_ADDR, GPO_1_SPI_MASTER_BMC_PCHN);
    }
    else // spi_flash_type == SPI_FLASH_PCH
    {
        clear_bit(U_GPO_1_ADDR, GPO_1_SPI_MASTER_BMC_PCHN);
    }
#ifdef USE_QUAD_IO
    if (spi_flash_type == SPI_FLASH_BMC/*read_spi_chip_id() == SPI_FLASH_MICRON*/)
    {
        write_to_spi_ctrl_1_csr(SPI_CONTROL_1_CSR_CS_READ_INSTRUCTION_OFST, SPI_FLASH_QUAD_READ_PROTOCOL_MICRON);
    }
    else /*if (read_spi_chip_id() == SPI_FLASH_MACRONIX)*/
    {
        write_to_spi_ctrl_1_csr(SPI_CONTROL_1_CSR_CS_READ_INSTRUCTION_OFST, SPI_FLASH_QUAD_READ_PROTOCOL_MACRONIX);
    }
#endif
}

/**
 * @brief Send a simple one-byte command to the SPI master
 */
static void write_one_byte_spi_cmd(SPI_COMMAND_ENUM spi_cmd)
{
    // Settings: 'data type' to 1, 'number of data bytes' to 0,
    //   'number of address bytes' to 0, 'number of dummy cycles' to 0
    alt_u32 cmd_word = 0x00000800 | ((alt_u32) spi_cmd & 0x0000ffff);
    write_to_spi_ctrl_1_csr(SPI_CONTROL_1_CSR_CS_FLASH_COMMAND_SETTING_OFST, cmd_word);
}

/**
 *  @brief Read the status register from the flash device
 */
static alt_u32 read_spi_status_register()
{
#ifdef USE_QUAD_IO
    write_to_spi_ctrl_1_csr(SPI_CONTROL_1_CSR_CS_OPERATING_PROTOCOLS_OFST, 0x22220);
#else
    write_to_spi_ctrl_1_csr(SPI_CONTROL_1_CSR_CS_OPERATING_PROTOCOLS_OFST, 0x00000);
#endif
    write_to_spi_ctrl_1_csr(SPI_CONTROL_1_CSR_CS_FLASH_COMMAND_SETTING_OFST, 0x1800 | SPI_CMD_READ_STATUS_REG);
    trigger_spi_send_cmd();
    return read_from_spi_ctrl_1_csr(SPI_CONTROL_1_CSR_CS_FLASH_COMMAND_READ_DATA0_OFST);
}

/**
 * @brief Poll the status register and return when Write in Progress (WIP) bit becomes 1.
 */
static void poll_status_reg_done()
{
    while (read_spi_status_register() & SPI_STATUS_WIP_BIT_MASK)
    {
        reset_hw_watchdog();
    }
}

/**
 * @brief Execute a SPI command (through SPI master) and wait for it to complete.
 */
static void execute_spi_cmd()
{
    trigger_spi_send_cmd();
    poll_status_reg_done();
}

/**
 * @brief Execute a simple one-byte command (through SPI master) and wait for it to complete.
 */
static void execute_one_byte_spi_cmd(SPI_COMMAND_ENUM spi_cmd)
{
    write_one_byte_spi_cmd(spi_cmd);
    execute_spi_cmd();
}

/**
 * @brief Erase either a 4kB or 64kB sector of the flash.
 *
 * After this erase, you can write to addresses in this sector through the memory mapped interface.
 * Note that sector address is a FLASH address (starts at 0) not the AVMM address
 * (which starts at some base offset address).
 *
 * @param addr_in_flash an address in the target SPI flash
 * @param erase_cmd a SPI command for 4kB or 64kB erase. Nios expects one of SPI_CMD_4KB_SECTOR_ERASE | SPI_CMD_32KB_SECTOR_ERASE | SPI_CMD_64KB_SECTOR_ERASE.
 */
static void execute_spi_erase_cmd(alt_u32 addr_in_flash, SPI_COMMAND_ENUM erase_cmd)
{
    execute_one_byte_spi_cmd(SPI_CMD_WRITE_ENABLE);
    // Zero dummy cycles, zero data bytes, 4 address bytes, erase command
    write_to_spi_ctrl_1_csr(SPI_CONTROL_1_CSR_CS_FLASH_COMMAND_SETTING_OFST, 0x0400 | erase_cmd);
    // Set the FLASH Address Register with the address of the sector to erase
    write_to_spi_ctrl_1_csr(SPI_CONTROL_1_CSR_CS_FLASH_COMMAND_ADDRESS_OFST, addr_in_flash);
    // Execute the command and wait for it to finish
    execute_spi_cmd();
}

/**
 * @brief Erase the defined region on SPI flash device.
 * This function assumes that Nios has set the right muxes to talk to the device.
 * Also, the SPI region start and end addresses are expected to be 4KB aligned.
 *
 * @param region_start_addr Start address of the target SPI region
 * @param nbytes size (in byte) of the target SPI region
 */
static void erase_spi_region(alt_u32 region_start_addr, alt_u32 nbytes)
{
    alt_u32 spi_addr = region_start_addr;
    alt_u32 region_end_addr = region_start_addr + nbytes;
    while (spi_addr < region_end_addr)
    {
        if (((region_end_addr - spi_addr) >= SPI_FLASH_PAGE_SIZE_OF_64KB) && ((spi_addr & 0xFFFF) == 0))
        {
            // The target SPI Region is at least 64KB in size and aligns to 0x10000 boundary.
            // Send 64KB erase command
            execute_spi_erase_cmd(spi_addr, SPI_CMD_64KB_SECTOR_ERASE);
            spi_addr += SPI_FLASH_PAGE_SIZE_OF_64KB;
        }
        else
        {
            // Otherwise, send 4KB erase command
            execute_spi_erase_cmd(spi_addr, SPI_CMD_4KB_SECTOR_ERASE);
            spi_addr += SPI_FLASH_PAGE_SIZE_OF_4KB;
        }
    }
}

/**
 * @brief Use custom memcpy to copy the signed payload to a given destination address.
 *
 * @param spi_dest_addr pointer to destination address in the SPI address range
 * @param signed_payload pointer to the start address of the signed payload
 */
static void memcpy_signed_payload(alt_u32 spi_dest_addr, alt_u32* signed_payload)
{
    alt_u32 nbytes = get_signed_payload_size(signed_payload);

    // Erase destination area first
    erase_spi_region(spi_dest_addr, nbytes);

    // Copy entire signed payload to destination location
    // Largest signed payload is BMC firmware capsule (max 32 MB).
    // Always copy signed payload in four pieces and pet the HW timer in between.
    // Then CPLD HW timer most likely won't timeout in this function, based on platform testing results.
    alt_u32 quarter_nbytes = nbytes >> 2;
    for (alt_u32 i = 0; i < 4; i++)
    {
        alt_u32_memcpy(get_spi_flash_ptr_with_offset(spi_dest_addr),
                       incr_alt_u32_ptr(signed_payload, i * quarter_nbytes),
                       quarter_nbytes);
        spi_dest_addr += quarter_nbytes;

        // Pet CPLD HW timer
        reset_hw_watchdog();
    }
}

/**
 * @brief Copy a binary blob from one SPI flash to the other.
 *
 * It's assumed that size has been authenticated, as part of the update capsule
 * authentication, before it's used. Then, it's guaranteed that (dest_spi_addr + size)
 * and (src_spi_addr + size) will not overflow the designated region on the SPI flash.
 *
 * @param dest_spi_addr an address in the destination SPI flash
 * @param src_spi_addr an address in the destination SPI flash
 * @param dest_spi_type destination SPI flash: BMC or PCH SPI flash
 * @param src_spi_type source SPI flash: BMC or PCH SPI flash
 * @param size number of bytes to copy
 */
static void copy_between_flashes(alt_u32 dest_spi_addr, alt_u32 src_spi_addr,
        SPI_FLASH_TYPE_ENUM dest_spi_type, SPI_FLASH_TYPE_ENUM src_spi_type, alt_u32 size)
{
    switch_spi_flash(dest_spi_type);
    // Get a pointer to the destination SPI flash at offset dest_spi_addr
    alt_u32* dest_flash_ptr = get_spi_flash_ptr_with_offset(dest_spi_addr);

    // Erase destination SPI region
    erase_spi_region(dest_spi_addr, size);

    // Get a pointer to the source SPI flash at offset src_spi_addr
    switch_spi_flash(src_spi_type);
    alt_u32* src_flash_ptr = get_spi_flash_ptr_with_offset(src_spi_addr);

    // Copy the binary like this: BMC -> CPLD, CPLD -> PCH
    for (alt_u32 word_i = 0; word_i < (size / 4); word_i++)
    {
        // Read a word from source SPI flash
        alt_u32 tmp = src_flash_ptr[word_i];
        switch_spi_flash(dest_spi_type);

        // Write the word to destination SPI flash
        dest_flash_ptr[word_i] = tmp;
        poll_status_reg_done();
        switch_spi_flash(src_spi_type);
    }

    // Pet the HW watchdog
    reset_hw_watchdog();
}

/**
 * @brief Set the CPLD recovery region to read only.
 *
 * Nios should not rely on BMC PFM to mark this SPI region with correct permission.
 */
static void write_protect_cpld_recovery_region()
{
    alt_u32* we_mem_ptr = __IO_CALC_ADDRESS_NATIVE_ALT_U32(U_SPI_FILTER_BMC_WE_AVMM_BRIDGE_BASE, 0);
    IOWR(we_mem_ptr, BMC_CPLD_RECOVERY_LOCATION_IN_WE_MEM, 0x0);
    IOWR(we_mem_ptr, BMC_CPLD_RECOVERY_LOCATION_IN_WE_MEM + 1, 0x0);
}

/**
 * @brief Set the CPLD staging region to read only.
 */
static void write_protect_cpld_staging_region()
{
    alt_u32* we_mem_ptr = __IO_CALC_ADDRESS_NATIVE_ALT_U32(U_SPI_FILTER_BMC_WE_AVMM_BRIDGE_BASE, 0);
    alt_u32 bmc_cpld_staging_capsule_location_in_we_mem = (get_ufm_pfr_data()->bmc_staging_region + BMC_STAGING_REGION_CPLD_UPDATE_CAPSULE_OFFSET) >> 19;
    IOWR(we_mem_ptr, bmc_cpld_staging_capsule_location_in_we_mem, 0x0);
    IOWR(we_mem_ptr, bmc_cpld_staging_capsule_location_in_we_mem + 1, 0x0);
}

#endif /* WHITLEY_INC_SPI_RW_UTILS_H */
