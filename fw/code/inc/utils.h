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
 * @file utils.h
 * @brief General utility functions.
 */

#ifndef WHITLEY_INC_PFR_UTILS_H
#define WHITLEY_INC_PFR_UTILS_H

// Always include pfr_sys.h first
#include "pfr_sys.h"

#include "dual_config_utils.h"

/**
 * A never exiting loop
 */
static void never_exit_loop()
{
    // Put NOPs in the body of the loop to ensure that we can always break into the
    // loop using GDB. If we don't add NOPs the unconditional branch instruction back
    // onto itself will not support breaking into the code without HW breakpoints
    while (1)
    {
        asm("nop");
        asm("nop");
        asm("nop");
        reset_hw_watchdog();

#ifdef USE_SYSTEM_MOCK
        if (SYSTEM_MOCK::get()->should_exec_code_block(
                SYSTEM_MOCK::CODE_BLOCK_TYPES::THROW_FROM_NEVER_EXIT_LOOP))
        {
            PFR_ASSERT(0)
        }
#endif
    }
}

/**
 * @brief This function sets a specific bit of the word at the given address.
 * Note that since this function is always inlined, it's never called.
 *
 * @param addr the address to read from and write to
 * @param shift_bit number of bits to shift to the left
 * @return None
 */
static void set_bit(alt_u32* addr, alt_u32 shift_bit)
{
    // Example of the intended assembly code
    // 1000ac:	00900404 	movi	r2,16400
    // 1000b0:	10c00037 	ldwio	r3,0(r2)
    // 1000b4:	18c00054 	ori	    r3,r3,1
    // 1000b8:	10c00035 	stwio	r3,0(r2)
    alt_u32 val = IORD_32DIRECT(addr, 0);
    val |= (0b1 << shift_bit);
    IOWR_32DIRECT(addr, 0, val);
}

/**
 * @brief This function clears a specific bit of the word at the given address.
 * Note that since this function is always inlined, it's never called.
 *
 * @param addr the address to read from and write to
 * @param shift_bit number of bits to shift to the left
 * @return None
 */
static void clear_bit(alt_u32* addr, alt_u32 shift_bit)
{
    // Example of the intended assembly code
    // 1000ac:	00900404 	movi	r2,16400
    // 1000b0:	11000037 	ldwio	r4,0(r2)
    // 1000b4:	00ffff84 	movi	r3,-2
    // 1000b8:	20c6703a 	and     r3,r4,r3
    // 1000bc:	10c00035 	stwio	r3,0(r2)
    alt_u32 val = IORD_32DIRECT(addr, 0);
    val &= ~(0b1 << shift_bit);
    IOWR_32DIRECT(addr, 0, val);
}

/**
 * @brief This function checks the value of a specific bit of the word at the given address.
 * Note that since this function is always inlined, it's never called.
 *
 * @param addr the address to read from and write to
 * @param shift_bit number of bits to shift to the left
 * @return the value of the bit
 */
static alt_u32 check_bit(alt_u32* addr, alt_u32 shift_bit)
{
    alt_u32 val = IORD_32DIRECT(addr, 0);
    val &= (0b1 << shift_bit);
    val = val >> shift_bit;
    return val;
}

/**
 * @brief This function implements a custom version of memcpy using alt_u32 read/writes.
 *
 * It copies the data word by word, since alt_u32 is the native size, to the same address.
 * Significantly smaller code than memcpy which operates on bytes. Also used for AVMM
 * slaves that do not support byte enable Note that since this function is always inlined, it's
 * never called.
 *
 * @param dest_ptr : pointer to the destination address to write to
 * @param src_ptr : the address to read from
 * @param n_bytes : number of bytes (must be multiples of 4) to write
 * @return None
 */
static void alt_u32_memcpy(alt_u32* dest_ptr, const alt_u32* src_ptr, alt_u32 n_bytes)
{
    PFR_ASSERT((n_bytes % 4) == 0);

    // Copy contents of src[] to dest[] using alt_u32
    // this means 4-bytes at a time
    for (alt_u32 i = 0; i < (n_bytes >> 2); i++)
    {
        dest_ptr[i] = src_ptr[i];
    }
}

/**
 * @brief This function implements a custom version of memcpy which doesn't increment destination
 * address.
 *
 * It copies the data word by word, since alt_u32 is the native size, to the same address.
 * Significantly smaller code than memcpy which operates on bytes. Also used for AVMM slaves that do
 * not support byte enable..
 *
 * @param dest_ptr : pointer to the destination address to write to
 * @param src_ptr : the address to read from
 * @param n_bytes : number of bytes (must be multiples of 4) to write
 * @return None
 */
static void alt_u32_memcpy_non_incr(alt_u32* dest_ptr, const alt_u32* src_ptr, const alt_u32 n_bytes)
{
    PFR_ASSERT((n_bytes % 4) == 0);

    // Copy contents of src[] to dest using alt_u32
    // this means 4-bytes at a time. dest does not increment.
    for (alt_u32 i = 0; i < (n_bytes >> 2); i++)
    {
        // Equivalent code: dest_ptr[0] = src_ptr[i];
        // and this is the smallest code:
        //        1000a0:   18c5883a    add r2,r3,r3
        //        1000a4:   1085883a    add r2,r2,r2
        //        1000a8:   2085883a    add r2,r4,r2
        //        1000ac:   10800037    ldwio   r2,0(r2)
        //        1000b0:   30800035    stwio   r2,0(r6)

        // Equivalent code: dest_ptr[0] = src_ptr[i];
        // Use IOWR so that we can intercept and mock the writes
        IOWR(dest_ptr, 0, src_ptr[i]);
    }
}

#endif /* WHITLEY_INC_PFR_UTILS_H */
