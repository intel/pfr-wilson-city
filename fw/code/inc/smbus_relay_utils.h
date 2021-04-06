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
 * @file smbus_relay_utils.h
 * @brief Responsible for configuring SMBus relays.
 */

#ifndef WHITLEY_INC_SMBUS_UTILS_H_
#define WHITLEY_INC_SMBUS_UTILS_H_

// Always include pfr_sys.h first
#include "pfr_sys.h"

#include "gen_gpo_controls.h"
#include "pfm.h"
#include "pfr_pointers.h"


/**
 * @brief Set/Clear FILTER_DISABLE bit for all relays
 *
 * @param bit_value clear the bit if bit_value == 0; otherwise, set the bit
 */
static void set_filter_disable_all(alt_u32 bit_value)
{
    PFR_ASSERT(NUM_RELAYS == 3);

    if (bit_value == 0)
    {
        clear_bit(U_GPO_1_ADDR, GPO_1_RELAY1_FILTER_DISABLE);
        clear_bit(U_GPO_1_ADDR, GPO_1_RELAY2_FILTER_DISABLE);
        clear_bit(U_GPO_1_ADDR, GPO_1_RELAY3_FILTER_DISABLE);
    }
    else
    {
        set_bit(U_GPO_1_ADDR, GPO_1_RELAY1_FILTER_DISABLE);
        set_bit(U_GPO_1_ADDR, GPO_1_RELAY2_FILTER_DISABLE);
        set_bit(U_GPO_1_ADDR, GPO_1_RELAY3_FILTER_DISABLE);
    }
}

/**
 * @brief Initialize the SMBus relays' command enable memory to all 0s.
 * All 0s means that all commands are disabled.
 */
static void init_smbus_relays()
{
    alt_u32* relay1_mem = get_relay_base_ptr(1);
    alt_u32* relay2_mem = get_relay_base_ptr(2);
    alt_u32* relay3_mem = get_relay_base_ptr(3);

    for (alt_u32 i = 0; i < SMBUS_RELAY_CMD_EN_MEM_NUM_WORDS; i++)
    {
        relay1_mem[i] = 0;
        relay2_mem[i] = 0;
        relay3_mem[i] = 0;
    }

    // Disable SMBus filtering in system initialization
    set_filter_disable_all(1);
}

/**
 * @brief Extract command whitelist from the PFM SMBus rule definition and copy it to
 * the relay's command enable memory.
 *
 * There's a dedicated command enable memory for each relay. The command enable memory
 * is organized as a series of command whitelists for each SMBus slave device. The order
 * of these whitelists is based on the rule ID. The whitelist of rule ID 1 is located at
 * lowest address of the command enable memory. The whiltelist of rule ID 2 is located
 * immediately after that and so on.
 *
 * @param rule_def SMBus rule definition
 */
static void apply_smbus_rule(PFM_SMBUS_RULE_DEF* rule_def)
{
    alt_u32 rule_offset = (rule_def->rule_id - 1) * SMBUS_NUM_BYTE_IN_WHITELIST;
    alt_u32* base_addr = get_relay_base_ptr(rule_def->bus_id);
    base_addr = incr_alt_u32_ptr(base_addr, rule_offset);

    // Memcpy the entire command whitelist to the specific location in the command enable memory.
    alt_u32_memcpy(base_addr, (alt_u32*) rule_def->cmd_whitelist, SMBUS_NUM_BYTE_IN_WHITELIST);
}

#endif /* WHITLEY_INC_SMBUS_UTILS_H_ */
