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
 * @file rfnvram_utils.h
 * @brief Utility functions to communicate with RFNVRAM IP.
 */

#ifndef WHITLEY_INC_RFNVRAM_UTILS_H_
#define WHITLEY_INC_RFNVRAM_UTILS_H_

// Always include pfr_sys.h first
#include "pfr_sys.h"

#include "pfr_pointers.h"
#include "rfnvram.h"
#include "timer_utils.h"


static void read_from_rfnvram(alt_u8* read_data, alt_u32 offset, alt_u32 nbytes)
{
    // Start the sequence (with write)
    // Lowest bit: High -> Read ; Low -> Write
    IOWR_32DIRECT(U_RFNVRAM_SMBUS_MASTER_ADDR, 0, 0x2DC);

    // Write most significant byte
    IOWR_32DIRECT(U_RFNVRAM_SMBUS_MASTER_ADDR, 0, ((offset & 0xFF00) >> 8));
    // Write least significant byte
    IOWR_32DIRECT(U_RFNVRAM_SMBUS_MASTER_ADDR, 0, (offset & 0xFF));

    // Start to read
    IOWR_32DIRECT(U_RFNVRAM_SMBUS_MASTER_ADDR, 0, 0x2DD);

    alt_u32 write_word = 0x00;
    for (alt_u32 i = 0; i < nbytes; i++)
    {
        // Read another word
        if (i == nbytes - 1)
        {
            // Also put RFNVRAM to idle at the last read
            write_word |= RFNVRAM_IDLE_MASK;
        }
        IOWR_32DIRECT(U_RFNVRAM_SMBUS_MASTER_ADDR, 0, write_word);

        // Poll to get data
        while (IORD_32DIRECT(RFNVRAM_RX_FIFO_BYTES_LEFT, 0) == 0) {}

        // Read and save the word
        read_data[i] = IORD_32DIRECT(RFNVRAM_RX_FIFO, 0);
    }
}

/**
 * @brief This function sets the DCI_RF_EN bit in the RF access control register in the RFNVRAM.
 *
 * Nios can read 1 byte starting from any byte address, but write must align on word boundaries (write 2 bytes/4 bytes).
 * Before setting the DCI_RF_EN bit, Nios also reads in the existing register value first and modifies based on that value.
 */
static void enable_rf_access()
{
    // Read in the existing value from the RF Access Control register
    alt_u8 rf_access_control[RFNVRAM_RF_ACCESS_CONTROL_LENGTH] = {};
    read_from_rfnvram(rf_access_control, RFNVRAM_RF_ACCESS_CONTROL_OFFSET, RFNVRAM_RF_ACCESS_CONTROL_LENGTH);

    // Modify and write back after setting the DCI_RF_EN bit.
    // Lowest bit: High -> Read ; Low -> Write
    IOWR_32DIRECT(U_RFNVRAM_SMBUS_MASTER_ADDR, 0, 0x2DC);
    // Write most significant byte
    IOWR_32DIRECT(U_RFNVRAM_SMBUS_MASTER_ADDR, 0, RFNVRAM_RF_ACCESS_CONTROL_MSB_OFFSET);
    // Write least significant byte
    IOWR_32DIRECT(U_RFNVRAM_SMBUS_MASTER_ADDR, 0, RFNVRAM_RF_ACCESS_CONTROL_LSB_OFFSET);

    IOWR_32DIRECT(U_RFNVRAM_SMBUS_MASTER_ADDR, 0, rf_access_control[0]);
    IOWR_32DIRECT(U_RFNVRAM_SMBUS_MASTER_ADDR, 0, (rf_access_control[1] | RFNVRAM_IDLE_MASK | RFNVRAM_RF_ACCESS_CONTROL_DCI_RF_EN_MASK));
}

/**
 * @brief This function writes the PIT ID at a pre-defined address in the RFNVRAM.
 * RFNVRAM expects 4 data byte at a time. Nios has to write the 8-byte PIT ID in two chunk.
 */
static void write_ufm_pit_id_to_rfnvram()
{
    // Get the PIT ID from UFM and write it to RFNVRAM
    alt_u8* ufm_pit_id = (alt_u8*) get_ufm_pfr_data()->pit_id;

    // Write 4 data byte at a time

    // Start to write the first half of PIT ID
    // Lowest bit: High -> Read ; Low -> Write
    IOWR_32DIRECT(U_RFNVRAM_SMBUS_MASTER_ADDR, 0, 0x2DC);

    // Write most significant byte
    IOWR_32DIRECT(U_RFNVRAM_SMBUS_MASTER_ADDR, 0, RFNVRAM_PIT_ID_MSB_OFFSET);
    // Write least significant byte
    IOWR_32DIRECT(U_RFNVRAM_SMBUS_MASTER_ADDR, 0, RFNVRAM_PIT_ID_LSB_OFFSET);

    // Write 4 data byte
    // (Using for-loop here costs more space)
    IOWR_32DIRECT(U_RFNVRAM_SMBUS_MASTER_ADDR, 0, ufm_pit_id[0]);
    IOWR_32DIRECT(U_RFNVRAM_SMBUS_MASTER_ADDR, 0, ufm_pit_id[1]);
    IOWR_32DIRECT(U_RFNVRAM_SMBUS_MASTER_ADDR, 0, ufm_pit_id[2]);
    // Write the last byte and put RFNVRAM to idle
    IOWR_32DIRECT(U_RFNVRAM_SMBUS_MASTER_ADDR, 0, RFNVRAM_IDLE_MASK | ufm_pit_id[3]);

    // In RFNVRAM, write operation can take 4.7-5ms. During this time,
    // it will ignore any command. There's also no busy bit to pull in the IP.
    // Sleep for 20ms to be safe.
    sleep_20ms(1);

    // Now, RFNVRAM should be ready to accept the next write
    // Start to write the second half of PIT ID
    // Lowest bit: High -> Read ; Low -> Write
    IOWR_32DIRECT(U_RFNVRAM_SMBUS_MASTER_ADDR, 0, 0x2DC);

    // Write most significant byte
    IOWR_32DIRECT(U_RFNVRAM_SMBUS_MASTER_ADDR, 0, RFNVRAM_PIT_ID_MSB_OFFSET);
    // Write least significant byte
    IOWR_32DIRECT(U_RFNVRAM_SMBUS_MASTER_ADDR, 0, RFNVRAM_PIT_ID_LSB_OFFSET + 4);

    // Write 4 data byte
    // (Using for-loop here costs more space)
    IOWR_32DIRECT(U_RFNVRAM_SMBUS_MASTER_ADDR, 0, ufm_pit_id[4]);
    IOWR_32DIRECT(U_RFNVRAM_SMBUS_MASTER_ADDR, 0, ufm_pit_id[5]);
    IOWR_32DIRECT(U_RFNVRAM_SMBUS_MASTER_ADDR, 0, ufm_pit_id[6]);
    // Write the last byte and put RFNVRAM to idle
    IOWR_32DIRECT(U_RFNVRAM_SMBUS_MASTER_ADDR, 0, RFNVRAM_IDLE_MASK | ufm_pit_id[RFNVRAM_PIT_ID_LENGTH - 1]);

    // In RFNVRAM, write operation can take 4.7-5ms. During this time,
    // it will ignore any command. There's also no busy bit to pull in the IP.
    // Sleep for 20ms to be safe.
    sleep_20ms(1);

    // Enable RF access
    enable_rf_access();
}

#endif /* WHITLEY_INC_RFNVRAM_UTILS_H_ */
