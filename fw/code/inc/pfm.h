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
 * @file pfm.h
 * @brief Define PFM data structures, such as PFM SPI region definition and SMBus rule definition.
 */

#ifndef WHITLEY_INC_PFM_H_
#define WHITLEY_INC_PFM_H_

#include "pfr_sys.h"

#define SMBUS_RULE_DEF_TYPE 2
#define SMBUS_RULE_DEF_SIZE 40
// Command whitelist contains 256 bits. 256 / 8 = 32 bytes.
#define SMBUS_NUM_BYTE_IN_WHITELIST 32

#define SPI_REGION_DEF_TYPE 1
#define SPI_REGION_DEF_MIN_SIZE 16
#define SPI_REGION_PROTECT_MASK_READ_ALLOWED 0b1
#define SPI_REGION_PROTECT_MASK_WRITE_ALLOWED 0b10
#define SPI_REGION_PROTECT_MASK_RECOVER_ON_FIRST_RECOVERY 0b100
#define SPI_REGION_PROTECT_MASK_RECOVER_ON_SECOND_RECOVERY 0b1000
#define SPI_REGION_PROTECT_MASK_RECOVER_ON_THIRD_RECOVERY 0b10000
#define SPI_REGION_PROTECT_MASK_RECOVER_BITS 0b11100

#define PFM_MAGIC 0x02b3ce1d
#define PFM_HASH_ALGO_SHA256_MASK 0b1
#define PFM_HASH_ALGO_SHA384_MASK 0b10
#define PFM_PADDING 0xFF
#define PFM_HEADER_SIZE 0x20

/*!
 * Platform firmware manifest SMBus rule definition
 */
typedef struct
{
    alt_u8 def_type;
    // Cannot use type alt_u32 for this reserved region since it crosses the word boundary.
    alt_u8 _reserved;
    alt_u8 _reserved2;
    alt_u8 _reserved3;
    alt_u8 _reserved4;
    alt_u8 bus_id;
    alt_u8 rule_id;
    alt_u8 device_addr;
    alt_u8 cmd_whitelist[SMBUS_NUM_BYTE_IN_WHITELIST];
} PFM_SMBUS_RULE_DEF;

/*!
 * Platform firmware manifest SPI region definition
 */
typedef struct
{
    alt_u8 def_type;
    alt_u8 protection_mask;
    alt_u16 hash_algorithm;
    alt_u32 _reserved;
    alt_u32 start_addr;
    alt_u32 end_addr;
    alt_u32 region_hash[PFR_CRYPTO_LENGTH / 4];
} PFM_SPI_REGION_DEF;

/*!
 * Platform firmware manifest data structure (header)
 * A complete PFM data can contain many SPI region and SMBus rule definitions.
 */
typedef struct
{
    alt_u32 tag;
    alt_u8 svn;
    alt_u8 bkc_version;
    alt_u8 major_rev;
    alt_u8 minor_rev;
    alt_u32 _reserved;
    alt_u32 oem_specific_data[4];
    alt_u32 length;
    alt_u32 pfm_body[];
} PFM;

#endif /* WHITLEY_INC_PFM_H_ */
