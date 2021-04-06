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
 * @file pbc_utils.h
 * @brief This header file contains functions to work with PBC structure.
 */

#ifndef WHITLEY_INC_PBC_UTILS_H_
#define WHITLEY_INC_PBC_UTILS_H_

// Always include pfr_sys.h first
#include "pfr_sys.h"

#include "pbc.h"
#include "pfr_pointers.h"

/**
 * @brief Return the size of the bitmap in bytes. 
 * 
 * @param pbc pointer to the start of a PBC_HEADER structure
 * @return alt_u32 the size of the bitmap in bytes
 */
static PFR_ALT_INLINE alt_u32 PFR_ALT_ALWAYS_INLINE get_bitmap_size(PBC_HEADER* pbc)
{
    return pbc->bitmap_nbit / 8;
}

/**
 * @brief Return the pointer to the active bitmap in PBC. 
 * 
 * @param pbc pointer to the start of a PBC_HEADER structure
 * @return alt_u32* the pointer to the active bitmap in PBC
 */
static PFR_ALT_INLINE alt_u32* PFR_ALT_ALWAYS_INLINE get_active_bitmap(PBC_HEADER* pbc)
{
    return (alt_u32*) (pbc + 1);
}

/**
 * @brief Return the pointer to the compression bitmap in PBC. 
 * 
 * @param pbc pointer to the start of a PBC_HEADER structure
 * @return alt_u32* the pointer to the compression bitmap in PBC
 */
static alt_u32* get_compression_bitmap(PBC_HEADER* pbc)
{
    alt_u32* active_bitmap_addr = get_active_bitmap(pbc);
    return incr_alt_u32_ptr(active_bitmap_addr, get_bitmap_size(pbc));
}

/**
 * @brief Return the pointer to the compressed payload in PBC. 
 * 
 * @param pbc pointer to the start of a PBC_HEADER structure
 * @return alt_u32* the pointer to the compressed payload in PBC
 */
static alt_u32* get_compressed_payload(PBC_HEADER* pbc)
{
    alt_u32* compression_bitmap_addr = get_compression_bitmap(pbc);
    return incr_alt_u32_ptr(compression_bitmap_addr, get_bitmap_size(pbc));
}

#endif /* WHITLEY_INC_PBC_UTILS_H_ */
