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
 * @file dual_config_utils.h
 * @brief Utility functions to communicate with the dual config IP.
 */

#ifndef WHITLEY_INC_DUAL_CONFIG_UTILS_H_
#define WHITLEY_INC_DUAL_CONFIG_UTILS_H_

// Always include pfr_sys.h first
#include "pfr_sys.h"

#define RECONFIG_REASON_POWER_UP_OR_SWITCH_TO_CFM0 0b010

#define RECONFIG_STATE_REG_OFFSET 4
#define RECONFIG_STATE_BIT_OFFSET 13

/**
 * @brief Polling on the busy bit of the Dual Config IP.
 * Exit the function when the busy bit becomes 0.
 */
static void poll_on_dual_config_ip_busy_bit()
{
    while (IORD(U_DUAL_CONFIG_BASE, 3) & 0x1) {}
}

/**
 * @brief Reset the hardware watchdog timer.
 * This must be done every 1 second or the system will reboot into recovery.
 */
static void reset_hw_watchdog()
{
    poll_on_dual_config_ip_busy_bit();
    // Reset the watchdog timer
    // The reset bit is bit[1] of the dual config IP
    IOWR(U_DUAL_CONFIG_BASE, 0, (1 << 1));
}

/**
 * @brief Reads the master state machine value of the dual config IP to determine the
 * reason for the last reconfiguration.
 */
static alt_u32 read_reconfig_reason()
{
    // request all available information on the current and previous configuration state
    IOWR(U_DUAL_CONFIG_BASE, 2, 0xf);

    // Wait until the read data has been fetched
    poll_on_dual_config_ip_busy_bit();

    // The currents master state machine is at bits 16:13
    return (IORD(U_DUAL_CONFIG_BASE, RECONFIG_STATE_REG_OFFSET) & 0x1E000 ) >> RECONFIG_STATE_BIT_OFFSET;
}

#endif /* WHITLEY_INC_DUAL_CONFIG_UTILS_H_ */
