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
 * @file spi_ctrl_utils.h
 * @brief Utility functions to communicate with the SPI control block.
 */

#ifndef WHITLEY_INC_SPI_CTRL_UTILS_H
#define WHITLEY_INC_SPI_CTRL_UTILS_H

// Always include pfr_sys.h first
#include "pfr_sys.h"

#include "gen_gpo_controls.h"
#include "spi_common.h"
#include "spi_rw_utils.h"
#include "timer_utils.h"
#include "ufm_utils.h"
#include "utils.h"


/**
 * @brief Reset a SPI flash through toggling the @p spi_flash_rst_n_gpo GPO control bit.
 * Note that this reset will bring the SPI flash device back to 3-byte addressing mode.
 */
static void reset_spi_flash(alt_u32 spi_flash_rst_n_gpo)
{
    clear_bit(U_GPO_1_ADDR, spi_flash_rst_n_gpo);
    set_bit(U_GPO_1_ADDR, spi_flash_rst_n_gpo);
    sleep_20ms(1);
}

/**
 * @brief Prepare for CPLD SPI master to drive the indicated SPI bus.
 * This function sets the appropriate GPO control bit and sends a command
 * to request flash device to enter 4-byte addressing mode.
 *
 * The assumption here is that the external agent (e.g. BMC/PCH) is in reset.
 *
 * @param spi_flash_type indicates BMC or PCH flash
 */
static void takeover_spi_ctrl(SPI_FLASH_TYPE_ENUM spi_flash_type)
{
    if (spi_flash_type == SPI_FLASH_BMC)
    {
        // BMC SPI Mux. 0-BMC, 1-CPLD
        set_bit(U_GPO_1_ADDR, GPO_1_FM_SPI_PFR_BMC_BT_MASTER_SEL);

        // Reset the BMC SPI flash to power-on state
        reset_spi_flash(GPO_1_RST_SPI_PFR_BMC_BOOT_N);
    }
    else // spi_flash_type == SPI_FLASH_PCH
    {
        // PCH SPI Mux. 0-PCH, 1-CPLD
        set_bit(U_GPO_1_ADDR, GPO_1_FM_SPI_PFR_PCH_MASTER_SEL);

        // Reset the PCH SPI flash to power-on state
        reset_spi_flash(GPO_1_RST_SPI_PFR_PCH_N);
    }

    switch_spi_flash(spi_flash_type);

    // Send the command to put the flash device in 4-byte address mode
    // Write enable command is required prior to sending the enter/exit 4-byte mode commands
    execute_one_byte_spi_cmd(SPI_CMD_WRITE_ENABLE);
    execute_one_byte_spi_cmd(SPI_CMD_ENTER_4B_ADDR_MODE);
}

/**
 * @brief Clear the external SPI Mux to let BMC/PCH drive the SPI bus.
 * For BMC, this function clears FM_SPI_PFR_BMC_BT_MASTER_SEL mux.
 * For PCH, this function clears FM_SPI_PFR_PCH_MASTER_SEL mux.
 *
 * This function should be ran before releasing BMC/PCH from reset.
 *
 * @param spi_flash_type indicates BMC or PCH flash
 */
static void release_spi_ctrl(SPI_FLASH_TYPE_ENUM spi_flash_type)
{
    if (spi_flash_type == SPI_FLASH_BMC)
    {
        // BMC SPI Mux. 0-BMC, 1-CPLD
        clear_bit(U_GPO_1_ADDR, GPO_1_FM_SPI_PFR_BMC_BT_MASTER_SEL);
    }
    else // spi_flash_type == SPI_FLASH_PCH
    {
        // PCH SPI Mux. 0-PCH, 1-CPLD
        clear_bit(U_GPO_1_ADDR, GPO_1_FM_SPI_PFR_PCH_MASTER_SEL);
    }
}

/**
 * @brief Let CPLD SPI master drive the SPI busses on the platform.
 */
static void takeover_spi_ctrls()
{
    takeover_spi_ctrl(SPI_FLASH_BMC);
    takeover_spi_ctrl(SPI_FLASH_PCH);
}

/**
 * @brief configure SPI master IP
 * This function must be called before trying to do any other SPI commands (including memory
 * reads/writes) After calling this, you should be set to do normal reads from the memory mapped
 * windows for each flash device
 */
static void configure_spi_master_csr()
{
#ifdef USE_QUAD_IO
    // Use standard SPI mode (command sent on DQ0 only)
    //   may want to change this to use 4 bits for address/data
    write_to_spi_ctrl_1_csr(SPI_CONTROL_1_CSR_CS_OPERATING_PROTOCOLS_OFST, 0x00022220);
    // Write instruction opcode set to 0x05, write operation opcode set to 0x02 - these may change
    // for different FLASH devices
    write_to_spi_ctrl_1_csr(SPI_CONTROL_1_CSR_CS_WRITE_INSTRUCTION_OFST, 0x00000538);
    // Set dummy cycles to 0 - SOME COMMANDS HAVE DIFFERENT NUMBER OF DUMMY CYCLES
    write_to_spi_ctrl_1_csr(SPI_CONTROL_1_CSR_CS_READ_INSTRUCTION_OFST, 0x00000AEB);
#else
    // Use standard SPI mode (command sent on DQ0 only)
    //   may want to change this to use 4 bits for address/data
    write_to_spi_ctrl_1_csr(SPI_CONTROL_1_CSR_CS_OPERATING_PROTOCOLS_OFST, 0x00000000);
    // Write instruction opcode set to 0x05, write operation opcode set to 0x02 - these may change
    // for different FLASH devices
    write_to_spi_ctrl_1_csr(SPI_CONTROL_1_CSR_CS_WRITE_INSTRUCTION_OFST, 0x00000502);
    // Set dummy cycles to 0 - SOME COMMANDS HAVE DIFFERENT NUMBER OF DUMMY CYCLES
    write_to_spi_ctrl_1_csr(SPI_CONTROL_1_CSR_CS_READ_INSTRUCTION_OFST, 0x00000003);
#endif
}

#endif /* WHITLEY_INC_SPI_CTRL_UTILS_H */
