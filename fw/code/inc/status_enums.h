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
 * @file status_enums.h
 * @brief Define encoding for various mailbox registers, such as platform state, last recovery reason and last panic reason.
 */

#ifndef WHITLEY_INC_STATUS_ENUMS_H_
#define WHITLEY_INC_STATUS_ENUMS_H_

/**
 * Define platform state
 */
typedef enum
{
    // CPLD Nios firmware T0 flow
    PLATFORM_STATE_CPLD_NIOS_WAITING_TO_START        = 0x01,
    PLATFORM_STATE_CPLD_NIOS_STARTED                 = 0x02,
    PLATFORM_STATE_ENTER_TMIN1                       = 0x03,
    PLATFORM_STATE_TMIN1_RESERVED1                   = 0x04,
    PLATFORM_STATE_TMIN1_RESERVED2                   = 0x05,
    PLATFORM_STATE_BMC_FLASH_AUTHENTICATION          = 0x06,
    PLATFORM_STATE_PCH_FLASH_AUTHENTICATION          = 0x07,
    PLATFORM_STATE_AUTHENTICATION_FAILED_LOCKDOWN    = 0x08,
    PLATFORM_STATE_ENTER_T0                          = 0x09,
    // Timed Boot Progress
    PLATFORM_STATE_T0_BMC_BOOTED                     = 0x0A,
    PLATFORM_STATE_T0_ME_BOOTED                      = 0x0B,
    PLATFORM_STATE_T0_ACM_BOOTED                     = 0x0C,
    PLATFORM_STATE_T0_BIOS_BOOTED                    = 0x0D,
    PLATFORM_STATE_T0_BOOT_COMPLETE                  = 0x0E,
    // Update event
    PLATFORM_STATE_PCH_FW_UPDATE                     = 0x10,
    PLATFORM_STATE_BMC_FW_UPDATE                     = 0x11,
    PLATFORM_STATE_CPLD_UPDATE                       = 0x12,
    PLATFORM_STATE_CPLD_UPDATE_IN_RECOVERY_MODE      = 0x13,
    // Recovery
    PLATFORM_STATE_TMIN1_FW_RECOVERY                 = 0x40,
    PLATFORM_STATE_TMIN1_FORCED_ACTIVE_FW_RECOVERY   = 0x41,
    PLATFORM_STATE_WDT_TIMEOUT_RECOVERY              = 0x42,
    PLATFORM_STATE_CPLD_RECOVERY_IN_RECOVERY_MODE    = 0x43,
    // PIT
    PLATFORM_STATE_PIT_L1_LOCKDOWN                   = 0x44,
    PLATFORM_STATE_PIT_L2_FW_SEALED                  = 0x45,
    PLATFORM_STATE_PIT_L2_PCH_HASH_MISMATCH_LOCKDOWN = 0x46,
    PLATFORM_STATE_PIT_L2_BMC_HASH_MISMATCH_LOCKDOWN = 0x47,
} STATUS_PLATFORM_STATE_ENUM;

/**
 * Define the value indicating last firmware recovery reason
 */
typedef enum
{
    LAST_RECOVERY_PCH_ACTIVE                = 0x1,
    LAST_RECOVERY_PCH_RECOVERY              = 0x2,
    LAST_RECOVERY_ME_LAUNCH_FAIL            = 0x3,
    LAST_RECOVERY_ACM_LAUNCH_FAIL           = 0x4,
    LAST_RECOVERY_IBB_LAUNCH_FAIL           = 0x5,
    LAST_RECOVERY_OBB_LAUNCH_FAIL           = 0x6,
    LAST_RECOVERY_BMC_ACTIVE                = 0x7,
    LAST_RECOVERY_BMC_RECOVERY              = 0x8,
    LAST_RECOVERY_BMC_LAUNCH_FAIL           = 0x9,
    LAST_RECOVERY_FORCED_ACTIVE_FW_RECOVERY = 0xA,
} STATUS_LAST_RECOVERY_ENUM;

/**
 * Define the value indicating last Panic reason
 */
typedef enum
{
    LAST_PANIC_DEFAULT                    = 0x00,
    LAST_PANIC_PCH_UPDATE_INTENT          = 0x01,
    LAST_PANIC_BMC_UPDATE_INTENT          = 0x02,
    LAST_PANIC_BMC_RESET_DETECTED         = 0x03,
    LAST_PANIC_BMC_WDT_EXPIRED            = 0x04,
    LAST_PANIC_ME_WDT_EXPIRED             = 0x05,
    LAST_PANIC_ACM_BIOS_WDT_EXPIRED       = 0x06,
    LAST_PANIC_RESERVED_1                 = 0x07,
    LAST_PANIC_RESERVED_2                 = 0x08,
    LAST_PANIC_ACM_BIOS_AUTH_FAILED       = 0x09,
} STATUS_LAST_PANIC_ENUM;

/**
 * Define the value indicating major error code observed on the system
 */
typedef enum
{
    MAJOR_ERROR_BMC_AUTH_FAILED        = 0x01,
    MAJOR_ERROR_PCH_AUTH_FAILED        = 0x02,
    MAJOR_ERROR_UPDATE_FROM_PCH_FAILED = 0x03,
    MAJOR_ERROR_UPDATE_FROM_BMC_FAILED = 0x04,
} STATUS_MAJOR_ERROR_ENUM;

/**
 * Define the value indicating minor error code observed on the system.
 * This set of minor code is associated with authentication failure.
 * Hence, this is paired with the MAJOR_ERROR_BMC_AUTH_FAILED and MAJOR_ERROR_PCH_AUTH_FAILED.
 */
typedef enum
{
    MINOR_ERROR_AUTH_ACTIVE                 = 0x01,
    MINOR_ERROR_AUTH_RECOVERY               = 0x02,
    MINOR_ERROR_AUTH_ACTIVE_AND_RECOVERY    = 0x03,
    MINOR_ERROR_AUTH_ALL_REGIONS            = 0x04,
} STATUS_MINOR_ERROR_AUTH_ENUM;

/**
 * Define the value indicating minor error code observed on the system.
 * This set of minor code is associated with firmware update failure.
 * Hence, this is paired with the MAJOR_ERROR_UPDATE_FROM_PCH_FAILED and MAJOR_ERROR_UPDATE_FROM_BMC_FAILED
 */
typedef enum
{
    MINOR_ERROR_INVALID_UPDATE_INTENT                    = 0x01,
    MINOR_ERROR_INVALID_SVN                              = 0x02,
    MINOR_ERROR_AUTHENTICATION_FAILED                    = 0x03,
    MINOR_ERROR_EXCEEDED_MAX_FAILED_ATTEMPTS             = 0x04,
    MINOR_ERROR_ACTIVE_FW_UPDATE_NOT_ALLOWED             = 0x05,
    MINOR_ERROR_RECOVERY_FW_UPDATE_AUTH_FAILED           = 0x06,
} STATUS_MINOR_ERROR_FW_CPLD_UPDATE_ENUM;

#endif /* WHITLEY_INC_STATUS_ENUMS_H_ */
