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
 * @file rfnvram.h
 * @brief Define macros and structs that eases RFNVRAM IP access.
 */

#ifndef WHITLEY_INC_RFNVRAM_H_
#define WHITLEY_INC_RFNVRAM_H_

// Always include pfr_sys.h first
#include "pfr_sys.h"


#define U_RFNVRAM_SMBUS_MASTER_ADDR __IO_CALC_ADDRESS_NATIVE_ALT_U32(U_RFNVRAM_SMBUS_MASTER_BASE, 0)
#define RFNVRAM_RX_FIFO U_RFNVRAM_SMBUS_MASTER_ADDR + 1
#define RFNVRAM_TX_FIFO_BYTES_LEFT U_RFNVRAM_SMBUS_MASTER_ADDR + 6
#define RFNVRAM_RX_FIFO_BYTES_LEFT U_RFNVRAM_SMBUS_MASTER_ADDR + 7
#define RFNVRAM_SMBUS_ADDR 0xDC
#define RFNVRAM_INTERNAL_SIZE 1088
#define RFNVRAM_IDLE_MASK 0x100

// The RF ACCESS CONTROL register stored in offset 0x014:0x015 inside the RFID component
#define RFNVRAM_RF_ACCESS_CONTROL_OFFSET 0x0014
#define RFNVRAM_RF_ACCESS_CONTROL_MSB_OFFSET ((RFNVRAM_RF_ACCESS_CONTROL_OFFSET & 0xff00) >> 8)
#define RFNVRAM_RF_ACCESS_CONTROL_LSB_OFFSET (RFNVRAM_RF_ACCESS_CONTROL_OFFSET & 0xff)
#define RFNVRAM_RF_ACCESS_CONTROL_LENGTH 2
#define RFNVRAM_RF_ACCESS_CONTROL_DCI_RF_EN_MASK 0x4

// The PIT ID stored in the external RFNVRAM is written at offset 0x0160:0x0167 inside the RFID component.
#define RFNVRAM_PIT_ID_OFFSET 0x0160
#define RFNVRAM_PIT_ID_MSB_OFFSET ((RFNVRAM_PIT_ID_OFFSET & 0xff00) >> 8)
#define RFNVRAM_PIT_ID_LSB_OFFSET (RFNVRAM_PIT_ID_OFFSET & 0xff)
#define RFNVRAM_PIT_ID_LENGTH 8

#endif /* WHITLEY_INC_RFNVRAM_H_ */
