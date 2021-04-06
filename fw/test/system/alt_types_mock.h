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

#ifndef INC_ALT_TYPES_MOCK_H
#define INC_ALT_TYPES_MOCK_H

#include <iostream>

// Include alt_types so it doesn't get reincluded. Define types for running on X86 below
#ifndef ALT_ASM_SRC
#define ALT_ASM_SRC
#endif
#include "alt_types.h"

// Equivalent of alt_types for use in testing

// Mock types. These match x86 types
typedef unsigned char  alt_u8;
typedef unsigned short alt_u16;
typedef unsigned int alt_u32;
typedef unsigned long long alt_u64;

// Mock the PFR_ASSERT to throw an exception. This can then be caught with EXPECT_ANY_THROW
#ifdef NO_SYSTEM_MOCK
#define PFR_ASSERT(condition) if (!condition) {throw -1;}
#define PFR_INTERNAL_ERROR(msg) std::cerr << "Internal Error: " << msg << " File: " << __FILE__ << "\tLine: " << __LINE__ << std::endl; throw -1
#endif


#endif /* ALT_TYPES_MOCK_H_ */
