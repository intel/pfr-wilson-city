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
 * @file global_state.h
 * @brief Global state machine and state for the PFR Nios System.
 */

#ifndef WHITLEY_INC_PFR_GLOBAL_STATE_H
#define WHITLEY_INC_PFR_GLOBAL_STATE_H

// Always include pfr_sys.h first
#include "pfr_sys.h"

#include "status_enums.h"
#include "utils.h"

#define U_GLOBAL_STATE_REG_ADDR __IO_CALC_ADDRESS_NATIVE_ALT_U32(U_GLOBAL_STATE_REG_BASE, 0)

/**
 * @brief Set the global state to an enum defined state.
 * This state is shown on the 7-seg display on the platform, if there's one. 
 * The same state encoding is used for the platform state register in the mailbox. 
 */
static void set_global_state(const STATUS_PLATFORM_STATE_ENUM state)
{
    // Write the bottom 16 bits of the global state register using the state
    alt_u32 val = IORD_32DIRECT(U_GLOBAL_STATE_REG_ADDR, 0);
    val &= (0xFFFF0000);
    val |= (0x0000FFFF & (alt_u32) state);
    IOWR_32DIRECT(U_GLOBAL_STATE_REG_ADDR, 0, val);
}

#endif /* WHITLEY_INC_PFR_GLOBAL_STATE_H */
