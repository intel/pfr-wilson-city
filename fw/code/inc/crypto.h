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
 * @file crypto.h
 * @brief Utility functions to communicate with the crypto block.
 */

#ifndef WHITLEY_INC_CRYPTO_H_
#define WHITLEY_INC_CRYPTO_H_

// Always include pfr_sys.h first
#include "pfr_sys.h"

#include "pfr_pointers.h"
#include "utils.h"

// CSR Interface
// Word Address           | Description
//----------------------------------------------------------------------
// 0x01                   | 0: ECDSA + SHA start (WO)
//                        | 1: ECDSA + SHA done (RO, cleared on go)
//                        | 2: ECDSA signature good (RO, cleared on go)
//                        | 3 : Reserved
//                        | 4 : SHA-only start (WO)
//                        | 5 : SHA done (Cleared on start)
//                        | 31:6 : Reserved
//----------------------------------------------------------------------
// 0x02                   | Data (WO)
//----------------------------------------------------------------------
// 0x03                   | Data length in bytes, must be 64 byte aligned (lower 6 bits = 0) (WO)
//----------------------------------------------------------------------
// 0x08-0x0f              | 256 bit data register for SHA result (RO)
//                        | address 0x08 is the lsbs (31:0), address 0x0f is the msbs (255:224)

// Important addresses in crypto block memory space
#define CRYPTO_CSR_ADDR __IO_CALC_ADDRESS_NATIVE_ALT_U32(U_CRYPTO_AVMM_BRIDGE_BASE, 1)
#define CRYPTO_DATA_ADDR __IO_CALC_ADDRESS_NATIVE_ALT_U32(U_CRYPTO_AVMM_BRIDGE_BASE, 2)
#define CRYPTO_DATA_LEN_ADDR __IO_CALC_ADDRESS_NATIVE_ALT_U32(U_CRYPTO_AVMM_BRIDGE_BASE, 3)
#define CRYPTO_DATA_SHA_ADDR(x) __IO_CALC_ADDRESS_NATIVE_ALT_U32(U_CRYPTO_AVMM_BRIDGE_BASE, (8 + x))

// Define bit for crypto block's CSR interface
#define CRYPTO_CSR_EC_SHA_START_MSK (0x01 << 0)
#define CRYPTO_CSR_EC_SHA_DONE_MSK (0x01 << 1)
#define CRYPTO_CSR_EC_SHA_DONE_OFST (1)
#define CRYPTO_CSR_EC_SHA_GOOD_MSK (0x01 << 2)
#define CRYPTO_CSR_EC_SHA_GOOD_OFST (2)
#define CRYPTO_CSR_SHA_START_MSK (0x01 << 4)
#define CRYPTO_CSR_SHA_DONE_MSK (0x01 << 5)
#define CRYPTO_CSR_SHA_DONE_OFST (5)

/**************************************************
 *
 * Constants for NIST P-256 curve
 *
 **************************************************/

// echo "6b17d1f2e12c4247f8bce6e563a440f277037d812deb33a0f4a13945d898c296" | xxd -r -p | xxd -i
static const alt_u8 CRYPTO_EC_AX[PFR_CRYPTO_LENGTH] = {
        0x6b, 0x17, 0xd1, 0xf2, 0xe1, 0x2c, 0x42, 0x47, 0xf8, 0xbc, 0xe6, 0xe5, 0x63, 0xa4, 0x40, 0xf2,
        0x77, 0x03, 0x7d, 0x81, 0x2d, 0xeb, 0x33, 0xa0, 0xf4, 0xa1, 0x39, 0x45, 0xd8, 0x98, 0xc2, 0x96};

// echo "4fe342e2fe1a7f9b8ee7eb4a7c0f9e162bce33576b315ececbb6406837bf51f5" | xxd -r -p | xxd -i
static const alt_u8 CRYPTO_EC_AY[PFR_CRYPTO_LENGTH] = {
        0x4f, 0xe3, 0x42, 0xe2, 0xfe, 0x1a, 0x7f, 0x9b, 0x8e, 0xe7, 0xeb, 0x4a, 0x7c, 0x0f, 0x9e, 0x16,
        0x2b, 0xce, 0x33, 0x57, 0x6b, 0x31, 0x5e, 0xce, 0xcb, 0xb6, 0x40, 0x68, 0x37, 0xbf, 0x51, 0xf5};

// echo "ffffffff00000001000000000000000000000000ffffffffffffffffffffffff" | xxd -r -p | xxd -i
static const alt_u8 CRYPTO_EC_P[PFR_CRYPTO_LENGTH] = {
        0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

// echo "ffffffff00000001000000000000000000000000fffffffffffffffffffffffc" | xxd -r -p | xxd -i
static const alt_u8 CRYPTO_EC_A[PFR_CRYPTO_LENGTH] = {
        0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfc};

// echo "ffffffff00000000ffffffffffffffffbce6faada7179e84f3b9cac2fc632551" | xxd -r -p | xxd -i
static const alt_u8 CRYPTO_EC_N[PFR_CRYPTO_LENGTH] = {
        0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xbc, 0xe6, 0xfa, 0xad, 0xa7, 0x17, 0x9e, 0x84, 0xf3, 0xb9, 0xca, 0xc2, 0xfc, 0x63, 0x25, 0x51};

/**
 * @brief This function calculates the SHA hash of the given data.
 * It sends all the inputs to the crypto block and the sha result can be retrieved through CSR interface.
 */
static void calculate_sha(const alt_u32* data, alt_u32 data_size)
{
    // Step 1: Write data size
    IOWR_32DIRECT(CRYPTO_DATA_LEN_ADDR, 0, data_size);

    // Step 2: Set SHA-only start
    IOWR_32DIRECT(CRYPTO_CSR_ADDR, 0, CRYPTO_CSR_SHA_START_MSK);

    // Step 3: Copy payload from SPI flash to CSR
    // Step 4: Wait for SHA_DONE (SHA only)
    alt_u32* data_local_ptr = (alt_u32*) data;
    while (!( (data_size == 0) && check_bit(CRYPTO_CSR_ADDR, CRYPTO_CSR_SHA_DONE_OFST) ))
    {
        // Stay in this loop until payload copying is done and SHA computation is done.

        // Perform step 3 in PFR_CRYPTO_SAFE_COPY_DATA_SIZE chunk
        if (data_size > PFR_CRYPTO_SAFE_COPY_DATA_SIZE)
        {
            alt_u32_memcpy_non_incr(CRYPTO_DATA_ADDR, data_local_ptr, PFR_CRYPTO_SAFE_COPY_DATA_SIZE);
            data_local_ptr = incr_alt_u32_ptr(data_local_ptr, PFR_CRYPTO_SAFE_COPY_DATA_SIZE);
            data_size -= PFR_CRYPTO_SAFE_COPY_DATA_SIZE;
        }
        else if (data_size > 0)
        {
            alt_u32_memcpy_non_incr(CRYPTO_DATA_ADDR, data_local_ptr, data_size);
            data_size = 0;
        }

        // Pet HW timer
        reset_hw_watchdog();
    }
}

/**
 * @brief This function calculates the SHA hash of the given data and
 * saves it at the destination address.
 */
static void calculate_and_save_sha(alt_u32* dest_addr, const alt_u32* data, alt_u32 data_size)
{
    calculate_sha(data, data_size);

    for (alt_u32 word_i = 0; word_i < (PFR_CRYPTO_LENGTH / 4); word_i++)
    {
        dest_addr[word_i] = IORD(CRYPTO_DATA_SHA_ADDR((PFR_CRYPTO_LENGTH / 4) - 1 - word_i), 0);
    }
}

/**
 * @brief This function verifies the expected SHA256 hash for a given data.
 * It sends all the inputs to the crypto block.
 * Once the crypto block is done, compare the expected hash against the calculated hash.
 *
 * @return 1 if expected hash matches the calculated hash; 0, otherwise.
 */
static alt_u32 verify_sha(const alt_u32* expected_hash, const alt_u32* data, alt_u32 data_size)
{
    calculate_sha(data, data_size);

    // Go through CSR word offset 0x08-0x0f to check the expected hash against the calculated hash.
    for (alt_u32 word_i = 0; word_i < (PFR_CRYPTO_LENGTH / 4); word_i++)
    {
        if (IORD(CRYPTO_DATA_SHA_ADDR((PFR_CRYPTO_LENGTH / 4) - 1 - word_i), 0) != expected_hash[word_i])
        {
            return 0;
        }
    }
    return 1;
}

/**
 * @brief This function verifies the expected SHA256 hash and EC signature for a given data.
 * It sends all the inputs and NIST P-256 curve constants to the crypto block for calculation.
 *
 * @return 1 if EC and SHA are good; 0, otherwise.
 */
static alt_u32 verify_ecdsa_and_sha(const alt_u32* cx, const alt_u32* cy,
        const alt_u32* sig_r, const alt_u32* sig_s, const alt_u32* data, alt_u32 data_size)
{
    // Step 1: Write data size
    IOWR_32DIRECT(CRYPTO_DATA_LEN_ADDR, 0, data_size);

    // Step 2: Set SHA and EC start
    IOWR_32DIRECT(CRYPTO_CSR_ADDR, 0, CRYPTO_CSR_EC_SHA_START_MSK);

    // Step 3: Copy payload from flash to CSR
    // Nios currently only checks signature for Block 1 Root Entry and Block 0.
    // Therefore, data_size is expected to be small here and no need to worry about HW timeout.
    alt_u32_memcpy_non_incr(CRYPTO_DATA_ADDR, data, data_size);

    // Step 4: Write AX from flash to CSR
    alt_u32_memcpy_non_incr(CRYPTO_DATA_ADDR, (const alt_u32*) CRYPTO_EC_AX, PFR_CRYPTO_LENGTH);

    // Step 5: Write AY from flash to CSR
    alt_u32_memcpy_non_incr(CRYPTO_DATA_ADDR, (const alt_u32*) CRYPTO_EC_AY, PFR_CRYPTO_LENGTH);

    // Step 6: Write BX (AKA CX) from flash to CSR
    alt_u32_memcpy_non_incr(CRYPTO_DATA_ADDR, cx, PFR_CRYPTO_LENGTH);

    // Step 7: Write BY (AKA CY) from flash to CSR
    alt_u32_memcpy_non_incr(CRYPTO_DATA_ADDR, cy, PFR_CRYPTO_LENGTH);

    // Step 8: Write P from flash to CSR
    alt_u32_memcpy_non_incr(CRYPTO_DATA_ADDR, (const alt_u32*) CRYPTO_EC_P, PFR_CRYPTO_LENGTH);

    // Step 9: Write A from flash to CSR
    alt_u32_memcpy_non_incr(CRYPTO_DATA_ADDR, (const alt_u32*) CRYPTO_EC_A, PFR_CRYPTO_LENGTH);

    // Step 10: Write N from flash to CSR
    alt_u32_memcpy_non_incr(CRYPTO_DATA_ADDR, (const alt_u32*) CRYPTO_EC_N, PFR_CRYPTO_LENGTH);

    // Step 11: Write R from flash to CSR
    alt_u32_memcpy_non_incr(CRYPTO_DATA_ADDR, sig_r, PFR_CRYPTO_LENGTH);

    // Step 12: Write S from flash to CSR
    alt_u32_memcpy_non_incr(CRYPTO_DATA_ADDR, sig_s, PFR_CRYPTO_LENGTH);

    // Wait for done signals (EC and SHA)
    while (!check_bit(CRYPTO_CSR_ADDR, CRYPTO_CSR_EC_SHA_DONE_OFST))
    {
        reset_hw_watchdog();
    }

    // Return match result (EC and SHA)
    return check_bit(CRYPTO_CSR_ADDR, CRYPTO_CSR_EC_SHA_GOOD_OFST);
}

#endif /* WHITLEY_INC_CRYPTO_H_ */
