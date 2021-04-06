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
 * @file initialization.h
 * @brief Functions used to initialize the system.
 */

#ifndef WHITLEY_INC_INITIALIZATION_H
#define WHITLEY_INC_INITIALIZATION_H

// Always include pfr_sys.h first
#include "pfr_sys.h"

#include "crypto.h"
#include "gen_gpi_signals.h"
#include "gen_gpo_controls.h"
#include "mailbox_utils.h"
#include "rfnvram_utils.h"
#include "smbus_relay_utils.h"
#include "spi_ctrl_utils.h"
#include "ufm_rw_utils.h"
#include "ufm_utils.h"
#include "utils.h"
#include "watchdog_timers.h"

/**
 * @brief Check if it's ready for Nios to start.
 * When the common core deasserts reset for both the PCH and BMC,
 * Nios FW is ready to start its execution.
 *
 * @return 1 if ready; 0, otherwise.
 */
static alt_u32 check_ready_for_nios_start()
{
    return check_bit(U_GPI_1_ADDR, GPI_1_cc_RST_RSMRST_PLD_R_N) &&
            check_bit(U_GPI_1_ADDR, GPI_1_cc_RST_SRST_BMC_PLD_R_N);
}

/**
 * @brief Initialize host mailbox register file
 * 
 * CPLD RoT static identifier, version and SVN registers are initialized here.
 * UFM provision/lock status is initialized with the information in ufm_status field
 * of UFM_PFR_DATA in ufm. 
 * CPLD hash is calculated and saved in the mailbox CPLD RoT Hash register as well. 
 * 
 */
static void initialize_mailbox()
{
    // Write the static identifier to mailbox
    write_to_mailbox(MB_CPLD_STATIC_ID, MB_CPLD_STATIC_ID_VALUE);

    // Write CPLD RoT Version
    write_to_mailbox(MB_CPLD_RELEASE_VERSION, CPLD_ROT_RELEASE_VERSION);

    // Write CPLD RoT SVN
    write_to_mailbox(MB_CPLD_SVN, get_ufm_svn(UFM_SVN_POLICY_CPLD));

    // Write UFM provision/lock/PIT status
    if (is_ufm_provisioned())
    {
        mb_set_ufm_provision_status(MB_UFM_PROV_UFM_PROVISIONED_MASK);
    }
    if (is_ufm_locked())
    {
        mb_set_ufm_provision_status(MB_UFM_PROV_UFM_LOCKED_MASK);
    }
    if (check_ufm_status(UFM_STATUS_PIT_L1_ENABLE_BIT_MASK))
    {
        mb_set_ufm_provision_status(MB_UFM_PROV_UFM_PIT_L1_ENABLED_MASK);
    }
    if (check_ufm_status(UFM_STATUS_PIT_L2_PASSED_BIT_MASK))
    {
        mb_set_ufm_provision_status(MB_UFM_PROV_UFM_PIT_L2_PASSED_MASK);
    }

    // Generate hash of CPLD active image
    alt_u32 cpld_hash[PFR_CRYPTO_LENGTH / 4];
    calculate_and_save_sha(cpld_hash, get_ufm_ptr_with_offset(UFM_CPLD_ACTIVE_IMAGE_OFFSET), UFM_CPLD_ACTIVE_IMAGE_LENGTH);

    // Report CPLD hash to mailbox
    alt_u8* cpld_hash_u8_ptr = (alt_u8*) cpld_hash;
    for (alt_u32 byte_i = 0; byte_i < PFR_CRYPTO_LENGTH; byte_i++)
    {
        IOWR(U_MAILBOX_AVMM_BRIDGE_BASE, MB_CPLD_HASH + byte_i, cpld_hash_u8_ptr[byte_i]);
    }
}

/**
 * @brief Initialize the PFR system. 
 *
 * Initialize the PFR system as required. At this point the design has reset,
 * and the nios has started running. 
 * Initialization includes the followings:
 * - Mailbox register file
 * - SMBus relays (disabled by default)
 * - SPI filters (disabled by default)
 * - Configure SPI master and SPI flash devices
 * - Enable write to the UFM
 * 
 * @see initialize_mailbox()
 */
static void initialize_system()
{
    // Initialize mailbox register file
    initialize_mailbox();

    // EXTRST# is the BMC external reset. Initializing it to the inactive state.
    // We use EXTRST# during a BMC-only reset so that we do not have to reset the host.
    set_bit(U_GPO_1_ADDR, GPO_1_RST_PFR_EXTRST_N);

    // Write enable the UFM
    ufm_enable_write();

    // Initialize SMBus relays
    // The SMBus filters are disabled. 
    // All SMBus commands are disabled in the command enable memories.
    init_smbus_relays();

    // Release SPI Flashes from reset
    set_bit(U_GPO_1_ADDR, GPO_1_RST_SPI_PFR_BMC_BOOT_N);
    set_bit(U_GPO_1_ADDR, GPO_1_RST_SPI_PFR_PCH_N);

    // Configure SPI master and SPI flash devices
    configure_spi_master_csr();

    if (!is_ufm_provisioned())
    {
        // Disable SPI filter, when system is unprovisioned.
        // By default, these GPO bits are initialized to 0, which enables the filters.
        set_bit(U_GPO_1_ADDR, GPO_1_BMC_SPI_FILTER_DISABLE);
        set_bit(U_GPO_1_ADDR, GPO_1_PCH_SPI_FILTER_DISABLE);
    }
}

#endif /* WHITLEY_INC_INITIALIZATION_H */
