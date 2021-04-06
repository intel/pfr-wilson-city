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
 * @file ufm_rw_utils.h
 * @brief Responsible for read & write access to the UFM flash.
 */

#ifndef WHITLEY_INC_UFM_RW_UTILS_H_
#define WHITLEY_INC_UFM_RW_UTILS_H_

// Always include pfr_sys.h first
#include "pfr_sys.h"

#include "pfr_pointers.h"


#define U_UFM_CSR_ADDR __IO_CALC_ADDRESS_NATIVE_ALT_U32(U_UFM_CSR_BASE, 0) 

#define UFM_CSR_STATUS_REG U_UFM_CSR_ADDR
#define UFM_CSR_CTRL_REG U_UFM_CSR_ADDR + 1

//default value of the control register when all sectors have write enabled and the sector erase and page erase bits are set to all 1s
#define UFM_CTRL_REG_DEFAULT_VAL 0xf07fffff

//The lower 20 bits hold the page erase address
#define UFM_PAGE_ERASE_MASK 0xfff00000

// bits 22-20 hold the sector erase info
#define UFM_SECTOR_ERASE_MASK 0xff8fffff

// busy bits are bits[1:0]
#define UFM_BUSY_BITS_MASK 0b11

// write success is bit[3]
#define UFM_WRITE_SUCCESS_MASK 0b1000


static PFR_ALT_INLINE void PFR_ALT_ALWAYS_INLINE ufm_enable_write()
{
    IOWR(UFM_CSR_CTRL_REG, 0, UFM_CTRL_REG_DEFAULT_VAL);
}

/**
 * @brief Check if UFM is busy processing erase/write command.
 *
 * @return 1 if busy is busy; 0, otherwise.
 */
static alt_u32 is_ufm_busy()
{
#ifdef USE_SYSTEM_MOCK
    // Assume never busy in system mock
    return 0;
#endif
    return (IORD(UFM_CSR_STATUS_REG, 0) & UFM_BUSY_BITS_MASK) != 0;
}

/**
 * @brief Erase the page where the address is pointing at.
 *
 * @param addr An address within the UFM flash.
 */
static void ufm_erase_page(alt_u32 addr)
{
#ifdef USE_SYSTEM_MOCK
    // Call system mock's UFM page erase function.
    SYSTEM_MOCK::get()->erase_ufm_page(addr);
    return;
#endif

    IOWR(UFM_CSR_CTRL_REG, 0, UFM_CTRL_REG_DEFAULT_VAL & ( (addr) | UFM_PAGE_ERASE_MASK));
    // Poll until erase is finished
    while (is_ufm_busy()) {}
}

/**
 * @brief Perform Sector Erase in UFM for the given sector
 */
static void ufm_erase_sector(alt_u32 sector_id)
{
#ifdef USE_SYSTEM_MOCK
    // Call system mock's UFM sector erase function.
    SYSTEM_MOCK::get()->erase_ufm_sector(sector_id);
    return;
#endif
    IOWR(UFM_CSR_CTRL_REG, 0, (UFM_CTRL_REG_DEFAULT_VAL & UFM_SECTOR_ERASE_MASK) |  (sector_id << 20));
    while (is_ufm_busy()) {}
}

/**
 * @brief Read data from the Mailbox fifo and write to specific location in UFM.
 * There's a statically allocated buffer to store the read data. The largest block
 * of data that Nios is expecting to read from the mailbox FIFO is the root key hash.
 * The size of root key hash is defined by PFR_CRYPTO_LENGTH macro.
 *
 * The mailbox FIFO is flushed after read.
 *
 * @param ufm_addr pointer to the destination address in UFM
 * @param nbytes number of bytes to read/write
 */
static void write_ufm_from_mb_fifo(alt_u32* ufm_addr, alt_u32 nbytes)
{
    // The largest block of data to read from mailbox fifo is PFR_CRYPTO_LENGTH bytes.
    alt_u8 read_data_buffer[PFR_CRYPTO_LENGTH];

    // Read the data a byte by a byte
    for (alt_u32 i = 0; i < nbytes; i++)
    {
        read_data_buffer[i] = read_from_mailbox_fifo();
    }

    // Upon completion, flush the FIFO
    flush_mailbox_fifo();

    // Commit write data to UFM
    alt_u32_memcpy(ufm_addr, (alt_u32*) read_data_buffer, nbytes);
}

/**
 * @brief Read data from the UFM and write to the mailbox fifo
 *
 * @param ufm_addr pointer to the destination address in UFM
 * @param nbytes number of bytes to read/write
 */
static void write_mb_fifo_from_ufm(alt_u8* ufm_addr, alt_u32 nbytes)
{
    // flush the FIFO prior to write for sanity
    flush_mailbox_fifo();

    for (alt_u32 i = 0; i < nbytes; i++)
    {
        write_to_mailbox_fifo(ufm_addr[i]);
    }
}

#endif /* WHITLEY_INC_UFM_RW_UTILS_H_ */
