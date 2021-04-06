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
 * @file cpld_reconfig.h
 * @brief Functions that perform CPLD reconfiguration.
 */

#ifndef WHITLEY_INC_CPLD_RECONFIG_H_
#define WHITLEY_INC_CPLD_RECONFIG_H_

// Always include pfr_sys.h first
#include "pfr_sys.h"

#include "gen_gpi_signals.h"
#include "gen_gpo_controls.h"
#include "timer_utils.h"
#include "utils.h"

/**
 * @brief Preparation before reconfiguring CPLD.
 */
static void prep_for_reconfig()
{
    // Clear Deep SX. This will drive Z onto PWRGD_DSW_PWROK_R
    clear_bit(U_GPO_1_ADDR, GPO_1_PWRGD_DSW_PWROK_R);

    // Add some delays between DSW_PWROK and PFR_SLP_SUS
    // The max time for this transition is 100ms. Set a delay of 120ms to be safe.
    // If this delay is not added or is too small, platform would enter
    //   a power loop (constantly shutting down and then power up).
    sleep_20ms(6);
}

/**
 * @brief Perform switching of CPLD image to use either CFM0 or CFM1
 *
 * @param cfm_one_or_zero an enum value indicating which CFM to switch to
 */
static void perform_cfm_switch(CPLD_CFM_TYPE_ENUM cfm_one_or_zero)
{
    // Set ConfigSelect register to either CFM1 or CFM0, bit[1] represent CFM selection, bit[0] commit to register
    IOWR(U_DUAL_CONFIG_BASE, 1, (cfm_one_or_zero << 1) | 1);

    // Check if written new ConfigSelect value being consumed
    poll_on_dual_config_ip_busy_bit();

    // We need to deassert deep sleep before starting reconfiguration
    // We will lose power when all pins to go high-Z otherwise
    prep_for_reconfig();

    // Trigger reconfiguration
    IOWR(U_DUAL_CONFIG_BASE, 0, 0x1);

#ifdef USE_SYSTEM_MOCK
    SYSTEM_MOCK::get()->reset_after_cpld_reconfiguration();

    // User can enable this block to exit the program after a CFM switch
    // On platform, Nios program would have exited here.
    if (SYSTEM_MOCK::get()->should_exec_code_block(
            SYSTEM_MOCK::CODE_BLOCK_TYPES::THROW_AFTER_CFM_SWITCH))
    {
        PFR_ASSERT(0)
    }
#endif
}

#endif /* WHITLEY_INC_CPLD_RECONFIG_H_ */
