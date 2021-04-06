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
 * @file pbc.h
 * @brief Define PFR compression structure format.
 */

#ifndef WHITLEY_INC_PBC_H_
#define WHITLEY_INC_PBC_H_

#include "pfr_sys.h"

// These values are specified in the HAS
#define PBC_EXPECTED_TAG 0x5F504243
#define PBC_EXPECTED_VERSION 0x2
#define PBC_EXPECTED_PAGE_SIZE 0x1000
#define PBC_EXPECTED_PATTERN_SIZE 0x0001
#define PBC_EXPECTED_PATTERN 0xFF

/**
 * Page Block Compression header structure
 *
 * The entire compression structure includes a header and two bit maps.
 * The compressed payload immediately follows the compression structure.
 */
typedef struct
{
    alt_u32 tag;
    alt_u32 version;
    alt_u32 page_size;
    alt_u32 pattern_size;
    alt_u32 pattern;
    alt_u32 bitmap_nbit;
    alt_u32 payload_len;
    alt_u32 _reserved[25];
} PBC_HEADER;

#endif /* WHITLEY_INC_PBC_H_ */
