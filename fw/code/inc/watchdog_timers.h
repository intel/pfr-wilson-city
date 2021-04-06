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
 * @file watchdog_timers.h
 * @brief Define the SW watchdog timers that are used to track BMC/PCH firmware boot progress.
 */

#ifndef WHITLEY_INC_WATCHDOG_TIMERS_H
#define WHITLEY_INC_WATCHDOG_TIMERS_H

// Always include pfr_sys.h first
#include "pfr_sys.h"
#include "timer_utils.h"


/*
 * Watchdog Timers
 */
#define WDT_BMC_TIMER_ADDR        U_TIMER_BANK_TIMER1_ADDR
#define WDT_ME_TIMER_ADDR         U_TIMER_BANK_TIMER2_ADDR
#define WDT_ACM_BIOS_TIMER_ADDR   U_TIMER_BANK_TIMER3_ADDR

/*
 * Watchdog timer boot progress tracking
 */
#define WDT_BMC_BOOT_DONE_MASK      0b00001
#define WDT_ME_BOOT_DONE_MASK       0b00010
#define WDT_ACM_BOOT_DONE_MASK      0b00100
#define WDT_IBB_BOOT_DONE_MASK      0b01000
#define WDT_OBB_BOOT_DONE_MASK      0b10000
#define WDT_ACM_BIOS_BOOT_DONE_MASK (WDT_ACM_BOOT_DONE_MASK | WDT_IBB_BOOT_DONE_MASK | WDT_OBB_BOOT_DONE_MASK)
#define WDT_PCH_BOOT_DONE_MASK      (WDT_ME_BOOT_DONE_MASK | WDT_ACM_BOOT_DONE_MASK | WDT_IBB_BOOT_DONE_MASK | WDT_OBB_BOOT_DONE_MASK)
#define WDT_ALL_BOOT_DONE_MASK      (WDT_BMC_BOOT_DONE_MASK | WDT_PCH_BOOT_DONE_MASK)

static alt_u8 wdt_boot_status = 0;

/*
 * Watchdog timer enable/disable
 *
 * In recovery flow, some WDTs may be disabled
 * In update flow, these WDTs can be re-enabled through a successful update.
 */
#define WDT_ENABLE_BMC_TIMER_MASK      0b001
#define WDT_ENABLE_ME_TIMER_MASK       0b010
#define WDT_ENABLE_ACM_BIOS_TIMER_MASK 0b100
#define WDT_ENABLE_PCH_TIMERS_MASK     0b110
#define WDT_ENABLE_ALL_TIMERS_MASK     0b111

static alt_u8 wdt_enable_status = WDT_ENABLE_ALL_TIMERS_MASK;


/**
 * @brief Check if all components (BMC/ME/ACM/BIOS) have completed boot.
 *
 * @return alt_u32 1 if all components have completed boot. 0, otherwise.
 */
static PFR_ALT_INLINE alt_u32 PFR_ALT_ALWAYS_INLINE is_timed_boot_done()
{
    return wdt_boot_status == WDT_ALL_BOOT_DONE_MASK;
}

/**
 * @brief Check if the given timer is expired
 * If the HW timer is active and the count down value reaches 0,
 * then this timer has expired
 *
 * @return alt_u32 1 if the given timer is expired. 0, otherwise.
 */
static alt_u32 is_wd_timer_expired(alt_u32* timer_addr)
{
    return IORD(timer_addr, 0) == U_TIMER_BANK_TIMER_ACTIVE_MASK;
}

#endif /* WHITLEY_INC_WATCHDOG_TIMERS_H */
