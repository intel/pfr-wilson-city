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
 * @file mailbox_enums.h
 * @brief Enums that describe offset mapping and value encoding in the mailbox register file.
 */

#ifndef WHITLEY_INC_MAILBOX_ENUMS_H_
#define WHITLEY_INC_MAILBOX_ENUMS_H_

// Always include pfr_sys.h first
#include "pfr_sys.h"


#define MB_CPLD_STATIC_ID_VALUE 0xDE

/**
 * Describe the Host Mailbox Register File accessible over the SMBus Host Mailbox interface.
 */
typedef enum
{
    /* Constant value of DEh */
    MB_CPLD_STATIC_ID = 0x00,
    /* read-only for external agents*/
    MB_CPLD_RELEASE_VERSION = 0x01,
    MB_CPLD_SVN = 0x02,
    MB_PLATFORM_STATE = 0x03,
    MB_RECOVERY_COUNT = 0x04,
    MB_LAST_RECOVERY_REASON = 0x05,
    MB_PANIC_EVENT_COUNT = 0x06,
    MB_LAST_PANIC_REASON = 0x07,
    MB_MAJOR_ERROR_CODE = 0x08,
    MB_MINOR_ERROR_CODE = 0x09,
    /* Status register for UFM provisioning and access commands */
    MB_PROVISION_STATUS = 0x0A,
    /* Command register for UFM provisioning/access commands */
    MB_PROVISION_CMD = 0x0B,
    /* Trigger register for the command set in the previous offset */
    MB_UFM_CMD_TRIGGER = 0x0C,
    /* Ingress byte for Write FIFO */
    MB_UFM_WRITE_FIFO = 0x0D,
    /* Egress byte for Read FIFO */
    MB_UFM_READ_FIFO = 0x0E,
    /* Status of BMC, write from BMC allowed only until BMC signals boot complete, write from BMC
       re-allowed on BMC reset */
    MB_BMC_CHECKPOINT = 0x0F,
    /* Status of ACM, write from CPU allowed only until ACM signals end of execution, write from CPU
       re-allowed on CPU reset */
    MB_ACM_CHECKPOINT = 0x10,
    /* Status of BIOS IBB, write from CPU allowed only until IBB signals end of execution, write
       from CPU re-allowed on CPU reset */
    MB_BIOS_CHECKPOINT = 0x11,
    /* Update intent from the CPU FW, read/write allowed from CPU */
    MB_PCH_UPDATE_INTENT = 0x12,
    /* Update intent from BMC FW, read/write allowed from BMC */
    MB_BMC_UPDATE_INTENT = 0x13,
    /* Info on Active PFM; set by CPLD RoT; read-only for CPU/BMC */
    MB_PCH_PFM_ACTIVE_SVN = 0x14,
    MB_PCH_PFM_ACTIVE_MAJOR_VER = 0x15,
    MB_PCH_PFM_ACTIVE_MINOR_VER = 0x16,
    MB_BMC_PFM_ACTIVE_SVN = 0x17,
    MB_BMC_PFM_ACTIVE_MAJOR_VER = 0x18,
    MB_BMC_PFM_ACTIVE_MINOR_VER = 0x19,
    /* Info on Recovery PFM; set by CPLD RoT; read-only for CPU/BMC */
    MB_PCH_PFM_RECOVERY_SVN = 0x1A,
    MB_PCH_PFM_RECOVERY_MAJOR_VER = 0x1B,
    MB_PCH_PFM_RECOVERY_MINOR_VER = 0x1C,
    MB_BMC_PFM_RECOVERY_SVN = 0x1D,
    MB_BMC_PFM_RECOVERY_MAJOR_VER = 0x1E,
    MB_BMC_PFM_RECOVERY_MINOR_VER = 0x1F,
    /* Hash value of CPLD RoT HW + FW; read-only for CPU/BMC */
    MB_CPLD_HASH = 0x20,
} MB_REGFILE_OFFSET_ENUM;

/**
 * Describe the mailbox checkpoint commands
 */
typedef enum
{
    /*
     * Execution Checkpoints
     */
    /* Started execution block */
    MB_CHKPT_START = 0x01,
    /* Next execution block authentication pass */
    MB_CHKPT_AUTH_PASS = 0x02,
    /* Next execution block authentication fail */
    MB_CHKPT_AUTH_FAIL = 0x03,
    /* Exiting Platform manufacturer authority */
    MB_CHKPT_EXIT = 0x04,
    /* Starting external execution block */
    MB_CHKPT_START_EXTERNAL = 0x05,
    /* Returned from external execution block */
    MB_CHKPT_RETURN = 0x06,
    /* Pausing execution block */
    MB_CHKPT_PAUSE = 0x07,
    /* Resumed execution block */
    MB_CHKPT_RESUME = 0x08,
    /* Completing execution block */
    MB_CHKPT_COMPLETE = 0x09,
    /* Entered management mode */
    MB_CHKPT_ENTER_MGMT = 0x0A,
    /* Leaving management mode */
    MB_CHKPT_EXIT_MGMT = 0x0B,

    /*
     * Miscellaneous Execution Checkpoints
     */
    /* Host: Ready To Boot OS */
    MB_CHKPT_READY_TO_BOOT_OS = 0x80,
    /* Host: Exit Boot Services */
    MB_CHKPT_EXIT_BOOT_SERVICE = 0x81,
    /* Reset Host */
    MB_CHKPT_RESET_HOST = 0x82,
    /* Reset ME */
    MB_CHKPT_RESET_ME = 0x83,
    /* Reset BMC */
    MB_CHKPT_RESET_BMC = 0x84,
} MB_CHKPT_CMD_ENUM;


/**
 * Update intent encoding
 */
typedef enum
{
    MB_UPDATE_INTENT_PCH_ACTIVE_MASK       = 0b1,
    MB_UPDATE_INTENT_PCH_RECOVERY_MASK     = 0b10,
    MB_UPDATE_INTENT_CPLD_ACTIVE_MASK      = 0b100,
    MB_UPDATE_INTENT_BMC_ACTIVE_MASK       = 0b1000,
    MB_UPDATE_INTENT_BMC_RECOVERY_MASK     = 0b10000,
    MB_UPDATE_INTENT_CPLD_RECOVERY_MASK    = 0b100000,
    MB_UPDATE_INTENT_UPDATE_DYNAMIC_MASK   = 0b1000000,
    MB_UPDATE_INTENT_UPDATE_AT_RESET_MASK  = 0b10000000,

    // Some combinations of the above
    MB_UPDATE_INTENT_CPLD_MASK                        = MB_UPDATE_INTENT_CPLD_RECOVERY_MASK | MB_UPDATE_INTENT_CPLD_ACTIVE_MASK,
    MB_UPDATE_INTENT_PCH_FW_UPDATE_MASK               = MB_UPDATE_INTENT_PCH_ACTIVE_MASK | MB_UPDATE_INTENT_PCH_RECOVERY_MASK,
    MB_UPDATE_INTENT_BMC_FW_UPDATE_MASK               = MB_UPDATE_INTENT_BMC_ACTIVE_MASK | MB_UPDATE_INTENT_BMC_RECOVERY_MASK,
    MB_UPDATE_INTENT_FW_ACTIVE_UPDATE_MASK            = MB_UPDATE_INTENT_PCH_ACTIVE_MASK | MB_UPDATE_INTENT_BMC_ACTIVE_MASK,
    MB_UPDATE_INTENT_FW_RECOVERY_UPDATE_MASK          = MB_UPDATE_INTENT_PCH_RECOVERY_MASK | MB_UPDATE_INTENT_BMC_RECOVERY_MASK,
    MB_UPDATE_INTENT_FW_UPDATE_MASK                   = MB_UPDATE_INTENT_PCH_FW_UPDATE_MASK | MB_UPDATE_INTENT_BMC_FW_UPDATE_MASK,
    MB_UPDATE_INTENT_FW_OR_CPLD_UPDATE_MASK           = MB_UPDATE_INTENT_FW_UPDATE_MASK | MB_UPDATE_INTENT_CPLD_MASK,
    MB_UPDATE_INTENT_VALID_FOR_BMC_ONLY_TMIN1_MASK    = MB_UPDATE_INTENT_BMC_ACTIVE_MASK | MB_UPDATE_INTENT_UPDATE_DYNAMIC_MASK,
    MB_UPDATE_INTENT_BMC_REQUIRE_PLATFORM_RESET_MASK  = MB_UPDATE_INTENT_BMC_RECOVERY_MASK | MB_UPDATE_INTENT_CPLD_MASK | MB_UPDATE_INTENT_PCH_FW_UPDATE_MASK,
} MB_UPDATE_INTENT_MASK_ENUM;

#define MB_UPDATE_INTENT_BMC_ACTIVE_BIT_POS 3
#define MB_UPDATE_INTENT_UPDATE_AT_RESET_BIT_POS 7

/**
 * UFM provisioning status
 */
typedef enum
{
    MB_UFM_PROV_CMD_BUSY_MASK            = 0b1,
    MB_UFM_PROV_CMD_DONE_MASK            = 0b10,
    MB_UFM_PROV_CMD_ERROR_MASK           = 0b100,
    MB_UFM_PROV_RESERVED_MASK            = 0b1000,
    MB_UFM_PROV_UFM_LOCKED_MASK          = 0b10000,
    MB_UFM_PROV_UFM_PROVISIONED_MASK     = 0b100000,
    MB_UFM_PROV_UFM_PIT_L1_ENABLED_MASK  = 0b1000000,
    MB_UFM_PROV_UFM_PIT_L2_PASSED_MASK   = 0b10000000,
    // Combination of the above enums
    // Bits to clear when Nios receives a new UFM command
    MB_UFM_PROV_CLEAR_ON_NEW_CMD_MASK    = MB_UFM_PROV_CMD_BUSY_MASK | MB_UFM_PROV_CMD_DONE_MASK | MB_UFM_PROV_CMD_ERROR_MASK,
    MB_UFM_PROV_CLEAR_ON_ERASE_CMD_MASK  = MB_UFM_PROV_UFM_PROVISIONED_MASK | MB_UFM_PROV_UFM_PIT_L1_ENABLED_MASK | MB_UFM_PROV_UFM_PIT_L2_PASSED_MASK,
} MB_UFM_PROV_STATUS_MASK_ENUM;

/**
 * UFM provisioning commands
 */
typedef enum
{
    MB_UFM_PROV_ERASE          = 0x00,
    MB_UFM_PROV_ROOT_KEY       = 0x01,
    MB_UFM_PROV_PIT_ID         = 0x02,
    MB_UFM_PROV_PCH_OFFSETS    = 0x05,
    MB_UFM_PROV_BMC_OFFSETS    = 0x06,
    MB_UFM_PROV_END            = 0x07,
    MB_UFM_PROV_RD_ROOT_KEY    = 0x08,
    MB_UFM_PROV_RD_PCH_OFFSETS = 0x0C,
    MB_UFM_PROV_RD_BMC_OFFSETS = 0x0D,
    MB_UFM_PROV_RECONFIG_CPLD  = 0x0E,
    MB_UFM_PROV_ENABLE_PIT_L1  = 0x10,
    MB_UFM_PROV_ENABLE_PIT_L2  = 0x11,
} MB_UFM_PROV_CMD_ENUM;

/**
 * UFM command trigger
 */
typedef enum
{
    MB_UFM_CMD_EXECUTE_MASK       = 0b1,
    MB_UFM_CMD_FLUSH_WR_FIFO_MASK = 0b10,
    MB_UFM_CMD_FLUSH_RD_FIFO_MASK = 0b100,
} MB_UFM_CMD_TRIGGER_MASK_ENUM;

#endif /* WHITLEY_INC_MAILBOX_ENUMS_H_ */
