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

// Test headers
#include "bsp_mock.h"
#include "dual_config_mock.h"

// Constructor/Destructor
DUAL_CONFIG_MOCK::DUAL_CONFIG_MOCK() {}
DUAL_CONFIG_MOCK::~DUAL_CONFIG_MOCK() {}

// Class methods

void DUAL_CONFIG_MOCK::reset()
{
	m_dual_config.reset();
    // Set power up state in msm
    IOWR(U_DUAL_CONFIG_BASE, RECONFIG_STATE_REG_OFFSET, RECONFIG_REASON_POWER_UP_OR_SWITCH_TO_CFM0 << RECONFIG_STATE_BIT_OFFSET);
}

bool DUAL_CONFIG_MOCK::is_addr_in_range(void* addr)
{
    return m_dual_config.is_addr_in_range(addr);
}

alt_u32 DUAL_CONFIG_MOCK::get_mem_word(void* addr)
{
    if (m_dual_config.is_addr_in_range(addr))
    {
        return m_dual_config.get_mem_word(addr);
    }
    return 0;
}

void DUAL_CONFIG_MOCK::set_mem_word(void* addr, alt_u32 data)
{
    if (m_dual_config.is_addr_in_range(addr))
    {
    	m_dual_config.set_mem_word(addr, data);

        std::uintptr_t addr_int = reinterpret_cast<std::uintptr_t>(addr);
        alt_u32 dual_config_offset = (addr_int - U_DUAL_CONFIG_BASE) >> 2;

        if (dual_config_offset == 0)
        {
            // Bit 0 - trigger reconfiguration.
            // Bit 1 - reset the watchdog timer

            if (data == 0x1)
            {
                // Check ConfigSelect register
                if (m_dual_config.get_mem_word(((alt_u32*) addr) + 1) & (CPLD_CFM1 << 1))
                {
                    // To switch to CFM1
                    // Clear reconfig reason in master state machine (because it's not read by CFM1)
                    m_dual_config.set_mem_word(((alt_u32*) addr) + 4, 0);
                }
                else
                {
                    // To switch to CFM0
                    // Save this reconfig reason in master state machine
                    m_dual_config.set_mem_word(((alt_u32*) addr) + 4, RECONFIG_REASON_POWER_UP_OR_SWITCH_TO_CFM0 << RECONFIG_STATE_BIT_OFFSET);
                }

                // Do nothing more. SYSTEM MOCK can't actually perform the switch between pfr_main and recovery_main
            }
        }
        else if (dual_config_offset == 1)
        {
            // Bit 0 - trigger config_sel_overwrite to the input register
            // Bit 1 - writes config_sel to the input register. Set 0 or 1 to load from
            //   configuration image 0 or 1 respectively.

        }
        else if (dual_config_offset == 2)
        {
            // Bit 0 - trigger read operation from the user watchdog.
            // Bit 1 - trigger read operation from the previous state application 1 register.
            // Bit 2 - trigger read operation from the previous state application 2 register.
            // Bit 3 - trigger read operation from the input register.
        }
        else if (dual_config_offset == 3)
        {
            // Bit 0 - IP busy signal
        }
        else if (dual_config_offset == 4)
        {
            // Bit 11:0 - user watchdog value.(13)
            // Bit 12 - current state of the user watchdog.
            // Bit 16:13 - msm_cs value of the current state.
        }
    }
}
