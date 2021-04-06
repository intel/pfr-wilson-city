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

#ifndef INC_BSP_MOCK_H
#define INC_BSP_MOCK_H

#include "alt_types_mock.h"

#ifndef NO_SYSTEM_MOCK
// Define the system mock object
#include "system_mock.h"
#ifndef USE_SYSTEM_MOCK
#define USE_SYSTEM_MOCK
#endif
static alt_u32 __builtin_ldwio(void* src)
{
    return SYSTEM_MOCK::get()->get_mem_word(src);
}
static void __builtin_stwio(void* dst, alt_u32 data)
{
    SYSTEM_MOCK::get()->set_mem_word(dst, data);
}

// Define the asserts to use the system mock. This allows runtime control of abort() vs. throw
#define PFR_ASSERT(condition)                                          \
    if (!(condition))                                                  \
    {                                                                  \
        SYSTEM_MOCK::get()->throw_internal_error(                      \
            std::string("Assert: ") + #condition, __LINE__, __FILE__); \
    }
#define PFR_INTERNAL_ERROR(msg)                                            \
    {                                                                      \
        SYSTEM_MOCK::get()->throw_internal_error(msg, __LINE__, __FILE__); \
    }

#define PFR_INTERNAL_ERROR_VARG(format, ...) \
{ \
    char buffer ## __LINE__ [4096]; \
    ::snprintf(buffer ## __LINE__, sizeof(buffer ## __LINE__), format, __VA_ARGS__); \
    PFR_INTERNAL_ERROR(buffer ## __LINE__); \
}

#else
// No system mock. Just directly dereference the pointers to access host memory
static alt_u32 __builtin_ldwio(void* src)
{
    return *((alt_u32*)src);
}
static void __builtin_stwio(void* dst, alt_u32 data)
{
    alt_u32* dst_alt_u32 = (alt_u32*) dst;
    *dst_alt_u32 = data;
}

#endif  // NO_SYSTEM_MOCK

#endif  // INC_BSP_MOCK_H
