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
 * @file timer_utils.h
 * @brief Utility functions for using system timer bank.
 */

#ifndef WHITLEY_INC_TIMER_UTILS_H_
#define WHITLEY_INC_TIMER_UTILS_H_

// Always include pfr_sys.h first
#include "pfr_sys.h"

#include "utils.h"

/*
 * Timer Bank
 */
// A bank of timers, for which the resolution is set to 20ms.
// Each word represents an independent timer.
// The count down value is bits 19:0. The start/stop bit is bit 28.
// There are 3 timers currently available on the timer bank
#define U_TIMER_BANK_TIMER1_ADDR  __IO_CALC_ADDRESS_NATIVE_ALT_U32(U_TIMER_BANK_AVMM_BRIDGE_BASE, 0)
#define U_TIMER_BANK_TIMER2_ADDR  __IO_CALC_ADDRESS_NATIVE_ALT_U32(U_TIMER_BANK_AVMM_BRIDGE_BASE, 1)
#define U_TIMER_BANK_TIMER3_ADDR  __IO_CALC_ADDRESS_NATIVE_ALT_U32(U_TIMER_BANK_AVMM_BRIDGE_BASE, 2)

#define U_TIMER_BANK_TIMER_VALUE_MASK    0x000FFFFF
#define U_TIMER_BANK_TIMER_ACTIVE_MASK   0x10000000
#define U_TIMER_BANK_TIMER_ACTIVE_BIT    28


/**
 * @brief Start a count down of @p time_unit * 20 milliseconds with the timer at @p timer_base_addr in 
 * timer bank. 
 * 
 * HW timer has resolution of 20ms. 
 * 
 * @param timer_base_addr pointer to a timer register in the timer bank
 * @param time_unit count down value (1 unit represents 20 milliseconds)
 */
static void start_timer(alt_u32* timer_base_addr, alt_u32 time_unit)
{
    IOWR(timer_base_addr, 0, (time_unit & U_TIMER_BANK_TIMER_VALUE_MASK) | U_TIMER_BANK_TIMER_ACTIVE_MASK);
}

/**
 * @brief Check if the given timer has expired.
 *
 * @param timer_base_addr pointer to a timer register in the timer bank
 * @return alt_u32 1 if the given timer has expired; 0, otherwise.
 */
static alt_u32 is_timer_expired(alt_u32* timer_base_addr)
{
    return (IORD(timer_base_addr, 0) & U_TIMER_BANK_TIMER_VALUE_MASK) == 0;
}

/**
 * @brief Pause the Nios processor and sleep for @p time_unit * 20 milliseconds.
 * This function uses the third timer for the countdown. Note that, this timer
 * is used as T0 watchdog timer. This function restores the original values before exiting.
 *
 * Due to the hardware implementation, the first unit of time is not guaranteed to be 20ms. The
 * actual count down can be between 0 to 20ms. To guarantee a minimum of @p time_unit * 20 ms
 * countdown, Nios adds 1 to @p time_unit when starting the hardware timer.
 *
 * Note that the CPLD HW timer is not pet in this function. If the count down value is too large
 * (e.g. more than 2.5 mins), CPLD may reconfig into ROM image and perform recovery on Active image.
 *
 * @param time_unit count down value (1 unit represents 20 milliseconds)
 */
static void sleep_20ms(alt_u32 time_unit)
{
    alt_u32 timer_value_before = IORD(U_TIMER_BANK_TIMER3_ADDR, 0);
    start_timer(U_TIMER_BANK_TIMER3_ADDR, time_unit + 1);
    while (!is_timer_expired(U_TIMER_BANK_TIMER3_ADDR)) {}
    IOWR(U_TIMER_BANK_TIMER3_ADDR, 0, timer_value_before);
}

/**
 * @brief Pause the given timer
 * @param timer_base_addr pointer to a timer register in the timer bank
 */
static PFR_ALT_INLINE void PFR_ALT_ALWAYS_INLINE pause_timer(alt_u32* timer_base_addr)
{
    clear_bit(timer_base_addr, U_TIMER_BANK_TIMER_ACTIVE_BIT);
}

/**
 * @brief Resume the given timer
 * @param timer_base_addr pointer to a timer register in the timer bank
 */
static PFR_ALT_INLINE void PFR_ALT_ALWAYS_INLINE resume_timer(alt_u32* timer_base_addr)
{
    set_bit(timer_base_addr, U_TIMER_BANK_TIMER_ACTIVE_BIT);
}

#endif /* WHITLEY_INC_TIMER_UTILS_H_ */
