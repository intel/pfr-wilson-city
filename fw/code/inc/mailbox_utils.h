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
 * @file mailbox_utils.h
 * @brief Utility functions to communicate with SMBus Host Mailbox.
 */

#ifndef WHITLEY_INC_MAILBOX_UTILS_H_
#define WHITLEY_INC_MAILBOX_UTILS_H_

// Always include pfr_sys.h first
#include "pfr_sys.h"

#include "mailbox_enums.h"
#include "pfm_utils.h"
#include "status_enums.h"
#include "utils.h"

#define U_MAILBOX_AVMM_BRIDGE_ADDR __IO_CALC_ADDRESS_NATIVE_ALT_U32(U_MAILBOX_AVMM_BRIDGE_BASE, 0)

/**
 * @brief This function writes the given value to the mailbox
 *
 * Note that this function shifts the word_offset by 2 bits because addresses are word-aligned.
 *
 * @param word_offset word offset relative to the Host Mailbox base address
 * @param val value to write
 * @return None
 */
static PFR_ALT_INLINE void PFR_ALT_ALWAYS_INLINE write_to_mailbox(MB_REGFILE_OFFSET_ENUM word_offset, alt_u32 val)
{
    IOWR(U_MAILBOX_AVMM_BRIDGE_BASE, word_offset, val);
}

/**
 * @brief This function writes the given value to the mailbox
 *
 * Note that this function shifts the word_offset by 2 bits because addresses are word-aligned.
 *
 * @param word_offset word offset relative to the Host Mailbox base address
 * @param val value to write
 * @return None
 */
static void clear_mailbox_register_bit(MB_REGFILE_OFFSET_ENUM word_offset, alt_u32 bit_offset)
{
    clear_bit(U_MAILBOX_AVMM_BRIDGE_ADDR + word_offset, bit_offset);
}

/**
 * @brief This function reads a word from the mailbox
 *
 * Note that this function shifts the word_offset by 2 bits because addresses are word-aligned.
 *
 * @param word_offset word offset relative to the Host Mailbox base address
 * @return value read from the mailbox
 */
static PFR_ALT_INLINE alt_u32 PFR_ALT_ALWAYS_INLINE read_from_mailbox(MB_REGFILE_OFFSET_ENUM word_offset)
{
    return IORD(U_MAILBOX_AVMM_BRIDGE_BASE, word_offset);
}

/**
 * @brief Write data to the mailbox fifo. Note that only the lowest 8 bits
 * will be written to the fifo. The higher 24 bits are ignored. This restriction
 * is due to the width of the mailbox fifo.
 *
 * @param val data to write to the fifo
 */
static PFR_ALT_INLINE void PFR_ALT_ALWAYS_INLINE write_to_mailbox_fifo(alt_u32 val)
{
    IOWR(U_MAILBOX_AVMM_BRIDGE_BASE, MB_UFM_WRITE_FIFO, val);
}

/**
 * @brief Read data to the mailbox fifo. 
 * The mailbox fifo spits out one byte at a time. This restriction is due to the 
 * width of the mailbox fifo.
 *
 * @return data to read from the fifo
 */
static PFR_ALT_INLINE alt_u32 PFR_ALT_ALWAYS_INLINE read_from_mailbox_fifo()
{
    return IORD(U_MAILBOX_AVMM_BRIDGE_BASE, MB_UFM_READ_FIFO);
}

/**
 * @brief Send command to the Mailbox to flush its fifo. 
 * Write FIFO and Read FIFO are the same FIFO internally in the mailbox. 
 * Hence, Nios only sends a command to flush one of the FIFO here.  
 */ 
static PFR_ALT_INLINE void PFR_ALT_ALWAYS_INLINE flush_mailbox_fifo()
{
    // There's only 1 FIFO internally in the mailbox.
    set_bit(U_MAILBOX_AVMM_BRIDGE_ADDR + MB_UFM_CMD_TRIGGER, MB_UFM_CMD_FLUSH_WR_FIFO_MASK);
}

/**
 * @brief Read the active and recovery PFM information from PCH flash and write the 
 * corresponding mailbox registers. 
 */
static void mb_write_pch_pfm_info()
{
    // PFM of the active region
    PFM* pfm = get_active_pfm(SPI_FLASH_PCH);
    write_to_mailbox(MB_PCH_PFM_ACTIVE_SVN, pfm->svn);
    write_to_mailbox(MB_PCH_PFM_ACTIVE_MAJOR_VER, pfm->major_rev);
    write_to_mailbox(MB_PCH_PFM_ACTIVE_MINOR_VER, pfm->minor_rev);

    // PFM in the recovery region capsule
    alt_u32* recovery_region_ptr = get_spi_recovery_region_ptr(SPI_FLASH_PCH);
    pfm = get_capsule_pfm(recovery_region_ptr);
    write_to_mailbox(MB_PCH_PFM_RECOVERY_SVN, pfm->svn);
    write_to_mailbox(MB_PCH_PFM_RECOVERY_MAJOR_VER, pfm->major_rev);
    write_to_mailbox(MB_PCH_PFM_RECOVERY_MINOR_VER, pfm->minor_rev);
}

/**
 * @brief Read the active and recovery PFM information from BMC flash and write the 
 * corresponding mailbox registers. 
 */
static void mb_write_bmc_pfm_info()
{
    // PFM of the active region
    PFM* pfm = get_active_pfm(SPI_FLASH_BMC);
    write_to_mailbox(MB_BMC_PFM_ACTIVE_SVN, pfm->svn);
    write_to_mailbox(MB_BMC_PFM_ACTIVE_MAJOR_VER, pfm->major_rev);
    write_to_mailbox(MB_BMC_PFM_ACTIVE_MINOR_VER, pfm->minor_rev);

    // PFM in the recovery region capsule
    alt_u32* recovery_region_ptr = get_spi_recovery_region_ptr(SPI_FLASH_BMC);
    pfm = get_capsule_pfm(recovery_region_ptr);
    write_to_mailbox(MB_BMC_PFM_RECOVERY_SVN, pfm->svn);
    write_to_mailbox(MB_BMC_PFM_RECOVERY_MAJOR_VER, pfm->major_rev);
    write_to_mailbox(MB_BMC_PFM_RECOVERY_MINOR_VER, pfm->minor_rev);
}

/**
 * @brief Report PFM information read from the @p spi_flash_type flash. 
 * 
 * @see mb_write_pch_pfm_info
 * @see mb_write_bmc_pfm_info
 */ 
static void mb_write_pfm_info(SPI_FLASH_TYPE_ENUM spi_flash_type)
{
    if (spi_flash_type == SPI_FLASH_PCH)
    {
        mb_write_pch_pfm_info();
    }
    else // spi_flash_type == SPI_FLASH_BMC
    {
        mb_write_bmc_pfm_info();
    }
}

/**
 * @brief Set the UFM provisioning status register of mailbox, with @p status_mask.
 * 
 * @param status_mask The UFM provisioning status mask, defined in MB_UFM_PROV_STATUS_MASK_ENUM enums
 */ 
static void mb_set_ufm_provision_status(MB_UFM_PROV_STATUS_MASK_ENUM status_mask)
{
    alt_u32 status = read_from_mailbox(MB_PROVISION_STATUS);
    status |= status_mask;
    write_to_mailbox(MB_PROVISION_STATUS, status);
}

/**
 * @brief Clear @p status_mask bits in UFM provisioning status register of mailbox.
 * 
 * @param status_mask The UFM provisioning status mask, defined in MB_UFM_PROV_STATUS_MASK_ENUM enums
 */ 
static void mb_clear_ufm_provision_status(MB_UFM_PROV_STATUS_MASK_ENUM status_mask)
{
    alt_u32 status = read_from_mailbox(MB_PROVISION_STATUS);
    status &= ~status_mask;
    write_to_mailbox(MB_PROVISION_STATUS, status);
}

#endif /* WHITLEY_INC_MAILBOX_UTILS_H_ */
