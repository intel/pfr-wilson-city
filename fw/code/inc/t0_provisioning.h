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
 * @file t0_provisioning.h
 * @brief Responsible for handling UFM provisioning requests.
 */

#ifndef WHITLEY_INC_T0_PROVISIONING_H_
#define WHITLEY_INC_T0_PROVISIONING_H_

// Always include pfr_sys.h first
#include "pfr_sys.h"

#include "cpld_reconfig.h"
#include "mailbox_utils.h"
#include "rfnvram_utils.h"
#include "platform_log.h"
#include "transition.h"
#include "ufm_rw_utils.h"
#include "ufm_utils.h"


/**
 * @brief Clear the Mailbox UFM Command Trigger register.
 */
static PFR_ALT_INLINE void PFR_ALT_ALWAYS_INLINE mb_clear_ufm_cmd_trigger()
{
    write_to_mailbox(MB_UFM_CMD_TRIGGER, 0);
}

/**
 * @brief Check whether bit[0] (execute command) is set in the Mailbox UFM Command Trigger register.
 *
 * @return alt_u32 1 if the bit[0] is set.
 */
static PFR_ALT_INLINE alt_u32 PFR_ALT_ALWAYS_INLINE mb_has_ufm_cmd_trigger()
{
    return read_from_mailbox(MB_UFM_CMD_TRIGGER) & MB_UFM_CMD_EXECUTE_MASK;
}

/**
 * @brief This function processes the Reconfig CPLD UFM command.
 * Nios firmware transitions to T-1 mode first, then perform CFM switch
 * to the current image. Hence, Nios firmware will be re-ran from the beginning.
 *
 * @param ufm_cmd a UFM provisioning command read from the mailbox register.
 */
static PFR_ALT_INLINE void PFR_ALT_ALWAYS_INLINE mb_process_reconfig_cpld_ufm_cmd(alt_u32 ufm_cmd)
{
    if (ufm_cmd == MB_UFM_PROV_RECONFIG_CPLD)
    {
        // Reconfig to the active image (i.e. reboot with the current image)
        perform_entry_to_tmin1();
        perform_cfm_switch(CPLD_CFM1);
    }
}

/**
 * @brief This function processes the PIT L1/L2 enablement UFM command.
 *
 * PIT L1 command can only be enabled if the PIT ID has been provisioned.
 *
 * There are two conditions where ENABLE_PIT_L2 command would be rejected:
 * - PIT L2 has been enabled before. This is an one-time operation. User needs to
 * erase provisioning, before enabling PIT L2 again.
 * - PIT L1 has not been enabled. If PIT L1 is not enabled, attacker can simply
 * boot the platform to disarm the PIT L2 protection.
 *
 * @param ufm_cmd a UFM provisioning command read from the mailbox register.
 */
static void mb_process_enable_pit_ufm_cmd(alt_u32 ufm_cmd)
{
    if (ufm_cmd == MB_UFM_PROV_ENABLE_PIT_L1)
    {
        if (check_ufm_status(UFM_STATUS_PROVISIONED_PIT_ID_BIT_MASK))
        {
            // Save the UFM PIT ID to RFNVRAM
            write_ufm_pit_id_to_rfnvram();

            // Start checking PIT ID in future T-1 mode
            set_ufm_status(UFM_STATUS_PIT_L1_ENABLE_BIT_MASK);

            // Report to UFM that PIT L1 has been enabled
            mb_set_ufm_provision_status(MB_UFM_PROV_UFM_PIT_L1_ENABLED_MASK);
        }
        else
        {
            // Cannot enable PIT L1 protection unless PIT ID is provisioned
            mb_set_ufm_provision_status(MB_UFM_PROV_CMD_ERROR_MASK);
        }
    }
    else if (ufm_cmd == MB_UFM_PROV_ENABLE_PIT_L2)
    {
        if (check_ufm_status(UFM_STATUS_PIT_L2_ENABLE_BIT_MASK) ||
                !check_ufm_status(UFM_STATUS_PIT_L1_ENABLE_BIT_MASK))
        {
            // Once enabled, PIT L2 cannot be enabled again and the associated UFM command is blocked.
            // Cannot enable L2 when PIT L1 is not enabled.
            mb_set_ufm_provision_status(MB_UFM_PROV_CMD_ERROR_MASK);
        }
        else
        {
            set_ufm_status(UFM_STATUS_PIT_L2_ENABLE_BIT_MASK);

            // Update status such that ENABLE_PIT_L2 command appears to be completed successfully
            // This is required in modular configuration, where BIOS can get control back
            //   after issuing the command to all CPLDs.
            mb_set_ufm_provision_status(MB_UFM_PROV_CMD_DONE_MASK);
            mb_clear_ufm_provision_status(MB_UFM_PROV_CMD_BUSY_MASK);

            // Wait 2s before going into T-1
            // In modular configuration, this delay allows all CPLDs to receive the PIT_L2_ENABLE command.
            sleep_20ms(100);

            // Go into T-1 mode and calculate firmware hash
            // Nios will stay in T-1 after calculating firmware hash, to avoid
            //   any modification on flash content by BMC/PCH in T0.
            perform_platform_reset();
        }
    }
}

/**
 * @brief This function processes Lock UFM command.
 *
 * Beside locking the UFM, this function may also initial the SVN policy
 * for PCH and BMC firmware. The SVN values come from the PCH and BMC firmware
 * on the platform.
 *
 * @param ufm_cmd a UFM provisioning command read from the mailbox register.
 */
static void mb_process_lock_ufm_cmd(alt_u32 ufm_cmd)
{
    if (ufm_cmd == MB_UFM_PROV_END)
    {
        if (!is_ufm_provisioned())
        {
            // Cannot lock UFM unless root key hash and offsets are provisioned
            mb_set_ufm_provision_status(MB_UFM_PROV_CMD_ERROR_MASK);
        }
        else
        {
            set_ufm_status(UFM_STATUS_LOCK_BIT_MASK);
            // Update mailbox UFM provisioning status
            mb_set_ufm_provision_status(MB_UFM_PROV_UFM_LOCKED_MASK);

            // Update the SVN policy based on the PCH/BMC images currently on platforms
            // The default images put on by the manufacturer may already have an SVN > 0.
            // Assume that an AC cycle has been done after the provisioning step.
            // The mailbox registers required only have correct values after successful T-1 authentication.
            alt_u32 pch_svn = read_from_mailbox(MB_PCH_PFM_RECOVERY_SVN);
            if (pch_svn <= UFM_MAX_SVN)
            {
                write_ufm_svn(pch_svn, UFM_SVN_POLICY_PCH);
            }
            alt_u32 bmc_svn = read_from_mailbox(MB_BMC_PFM_RECOVERY_SVN);
            if (bmc_svn <= UFM_MAX_SVN)
            {
                write_ufm_svn(bmc_svn, UFM_SVN_POLICY_BMC);
            }
        }
    }
}

/**
 * @brief This function processes the UFM command provided to the dedicated mailbox register.
 *
 * If the provisioning data has been provisioned, log an error and exit.
 * For example, once PIT ID has been provisioned, it cannot be provisioned
 * again unless an Erase provisioning command has been issued.
 *
 * When UFM is locked, UFM commands other than read provisioned data commands are rejected. Nios
 * would log an error.
 *
 * All provisioned data is stored in UFM according to the offset in UFM_PFR_DATA.
 *
 * @see mb_process_reconfig_cpld_ufm_cmd
 * @see mb_process_enable_pit_ufm_cmd
 * @see mb_process_lock_ufm_cmd
 */
static void mb_process_ufm_cmd()
{
    alt_u32 ufm_cmd = read_from_mailbox(MB_PROVISION_CMD);
    UFM_PFR_DATA* ufm_data = get_ufm_pfr_data();

    /*
     * Process Read commands first.
     *
     * This includes commands to read Root Key hash, PCH offsets and BMC offsets.
     */
    if (ufm_cmd == MB_UFM_PROV_RD_ROOT_KEY)
    {
        write_mb_fifo_from_ufm((alt_u8*) ufm_data->root_key_hash, PFR_CRYPTO_LENGTH);
    }
    else if (ufm_cmd == MB_UFM_PROV_RD_PCH_OFFSETS)
    {
        // Read provisioned PCH offsets (Active/PFM, Recovery, Staging)
        // Write the offsets in the above order/ Those offsets are stored in the same order in UFM
        // Each offset is 4-byte long. Read/write 12 bytes in total
        write_mb_fifo_from_ufm((alt_u8*) &ufm_data->pch_active_pfm, 12);
    }
    else if (ufm_cmd == MB_UFM_PROV_RD_BMC_OFFSETS)
    {
        // Read provisioned BMC offsets (Active/PFM, Recovery, Staging)
        // Write the offsets in the above order/ Those offsets are stored in the same order in UFM
        // Each offset is 4-byte long. Read/write 12 bytes in total
        write_mb_fifo_from_ufm((alt_u8*) &ufm_data->bmc_active_pfm, 12);
    }
    else if (!is_ufm_locked())
    {
        /*
         * Process commands that can modify UFM content, only when UFM is not locked.
         */
        if (ufm_cmd == MB_UFM_PROV_ERASE)
        {
            // Perform page erase on the first page (which contains the UFM PFR data)
            ufm_erase_page(UFM_PFR_DATA_OFFSET);

            // Clear UFM provisioning status in the mailbox
            mb_clear_ufm_provision_status(MB_UFM_PROV_CLEAR_ON_ERASE_CMD_MASK);
        }
        else if (ufm_cmd == MB_UFM_PROV_ROOT_KEY)
        {
            if (check_ufm_status(UFM_STATUS_PROVISIONED_ROOT_KEY_HASH_BIT_MASK))
            {
                // Root key hash has been provisioned. Must erase before provisioning again.
                mb_set_ufm_provision_status(MB_UFM_PROV_CMD_ERROR_MASK);
            }
            else
            {
                // Update internal provisioning status
                set_ufm_status(UFM_STATUS_PROVISIONED_ROOT_KEY_HASH_BIT_MASK);

                write_ufm_from_mb_fifo(ufm_data->root_key_hash, PFR_CRYPTO_LENGTH);
            }
        }
        else if (ufm_cmd == MB_UFM_PROV_PIT_ID)
        {
            if (check_ufm_status(UFM_STATUS_PROVISIONED_PIT_ID_BIT_MASK))
            {
                // PIT ID has been provisioned. Must erase before provisioning again.
                mb_set_ufm_provision_status(MB_UFM_PROV_CMD_ERROR_MASK);
            }
            else
            {
                // Update internal provisioning status
                set_ufm_status(UFM_STATUS_PROVISIONED_PIT_ID_BIT_MASK);

                // PIT ID Provisioning
                write_ufm_from_mb_fifo(ufm_data->pit_id, RFNVRAM_PIT_ID_LENGTH);
            }
        }
        else if (ufm_cmd == MB_UFM_PROV_PCH_OFFSETS)
        {
            if (check_ufm_status(UFM_STATUS_PROVISIONED_PCH_OFFSETS_BIT_MASK))
            {
                // PCH offsets have been provisioned. Must erase before provisioning again.
                mb_set_ufm_provision_status(MB_UFM_PROV_CMD_ERROR_MASK);
            }
            else
            {
                set_ufm_status(UFM_STATUS_PROVISIONED_PCH_OFFSETS_BIT_MASK);

                // Read provisioned PCH offsets (Active/PFM, Recovery, Staging)
                // Write the offsets in the above order/ Those offsets are stored in the same order in UFM
                // Each offset is 4-byte long. Read/write 12 bytes in total
                write_ufm_from_mb_fifo(&ufm_data->pch_active_pfm, 12);
            }
        }
        else if (ufm_cmd == MB_UFM_PROV_BMC_OFFSETS)
        {
            if (check_ufm_status(UFM_STATUS_PROVISIONED_BMC_OFFSETS_BIT_MASK))
            {
                // BMC offsets have been provisioned. Must erase before provisioning again.
                mb_set_ufm_provision_status(MB_UFM_PROV_CMD_ERROR_MASK);
            }
            else
            {
                set_ufm_status(UFM_STATUS_PROVISIONED_BMC_OFFSETS_BIT_MASK);

                // Read provisioned BMC offsets (Active/PFM, Recovery, Staging)
                // Write the offsets in the above order/ Those offsets are stored in the same order in UFM
                // Each offset is 4-byte long. Read/write 12 bytes in total
                write_ufm_from_mb_fifo(&ufm_data->bmc_active_pfm, 12);
            }
        }

        // Update mailbox UFM provisioning status,
        // Set provisioned bit to 1, when root key hash and the BMC/PCH offsets are provisioned
        if (is_ufm_provisioned())
        {
            mb_set_ufm_provision_status(MB_UFM_PROV_UFM_PROVISIONED_MASK);
        }

        mb_process_reconfig_cpld_ufm_cmd(ufm_cmd);
        mb_process_enable_pit_ufm_cmd(ufm_cmd);
        mb_process_lock_ufm_cmd(ufm_cmd);
    }
    else
    {
        /*
         * Otherwise, this command is not allowed. Log an error.
         */
        mb_set_ufm_provision_status(MB_UFM_PROV_CMD_ERROR_MASK);
    }
}

/**
 * @brief This function responses to UFM command and sets appropriate provisioning status afterwards.
 *
 * If the bit[0] (execute command) is set, then proceed to respond to the UFM provisioning command.
 * First, Nios firmware clears the command trigger register in Mailbox and validate the UFM command.
 * Once validation passed, the UFM command is processed appropriately.
 *
 * UFM status register is updated during the provisioning flow as well. For example, if the UFM command
 * failed any validation check, the command error bit will be set. UFM command busy bit is set before
 * Nios firmware begins to process the UFM command. The busy bit is cleared once the UFM command has been
 * processed.
 *
 * @see mb_process_ufm_cmd
 */
static void mb_ufm_provisioning_handler()
{
    // Check command trigger
    if (mb_has_ufm_cmd_trigger())
    {
        // Clear done and error status and then the command trigger register
        mb_clear_ufm_provision_status(MB_UFM_PROV_CLEAR_ON_NEW_CMD_MASK);
        mb_clear_ufm_cmd_trigger();

        // Ready to execute the command
        mb_set_ufm_provision_status(MB_UFM_PROV_CMD_BUSY_MASK);
        mb_process_ufm_cmd();
        mb_clear_ufm_provision_status(MB_UFM_PROV_CMD_BUSY_MASK);

        // Done with this UFM command
        mb_set_ufm_provision_status(MB_UFM_PROV_CMD_DONE_MASK);
    }
}

#endif /* WHITLEY_INC_T0_PROVISIONING_H_ */
