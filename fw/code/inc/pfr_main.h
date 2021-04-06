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
 * @file pfr_main.h
 * @brief Mainline function
 */

// Includes

// Always include pfr_sys first
#include "pfr_sys.h"

#include "initialization.h"
#include "t0_routines.h"


/**
 * @brief Mainline of PFR system. Called from main()
 *
 * Before running the firmware flow, Nios firmware should wait for the Common Core signals.
 * When the Common Core attempts to release BMC and PCH from resets, Nios firmware intercepts those 
 * signals and starts running the firmware flows.
 *
 * First, Nios firmware initializes the PFR system. For example, it writes the CPLD RoT Static Identifier 
 * to the mailbox register. After that, Nios firmware runs the T-1 operations, including PIT checks and 
 * authentication of SPI flashes. If the PFR system is unprovisioned, this step will be skipped. 
 * 
 * Upon completing the T0 operations, Nios firmware transitions the platform to T0 mode and runs the never
 * exit T0 loop. In the T0 loop, Nios firmware monitors for UFM provisioning commands. If the PFR system is 
 * provisioned, Nios firmware also monitors the boot progress, update intent registers, etc. 
 * 
 * @return None
 */
static PFR_ALT_INLINE void PFR_ALT_ALWAYS_INLINE pfr_main()
{
    // Wait for ready from common core
    log_platform_state(PLATFORM_STATE_CPLD_NIOS_WAITING_TO_START);
    while (!check_ready_for_nios_start())
    {
        reset_hw_watchdog();
    }

    // Nios starts now
    log_platform_state(PLATFORM_STATE_CPLD_NIOS_STARTED);

    // Initialize the system
    initialize_system();

    // Go into T-1 mode and do T-1 operations (e.g. authentication). Then transition back to T0.
    perform_platform_reset();

    // Perform T0 operations. This will never exit.
    perform_t0_operations();
}
