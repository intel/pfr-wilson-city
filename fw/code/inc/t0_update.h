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
 * @file t0_update.h
 * @brief Monitor the BMC and PCH update intent registers and trigger panic event appropriately.
 */

#ifndef WHITLEY_INC_T0_UPDATE_H_
#define WHITLEY_INC_T0_UPDATE_H_

// Always include pfr_sys.h first
#include "pfr_sys.h"

#include "firmware_update.h"
#include "transition.h"
#include "mailbox_utils.h"
#include "platform_log.h"
#include "spi_flash_state.h"

/**
 * @brief Monitor PCH update intent register and transition to T-1 mode to perform the update if necessary.
 *
 * This function reads the PCH update intent from the mailbox first.
 *
 * If the update intent is not a valid request (i.e. not matching any bit mask), then exit.
 * If the update intent indicates a deferred update, also exit.
 *
 * For the following scenarios, log some error, clear the update intent register and exit.
 * - It's a firmware update and maximum failed update attempts have been reached for that flash device.
 * - It's a CPLD update and maximum failed CPLD update attempts have been reached.
 * - It's a active firmware update and the PCH recovery image is corrupted
 *
 * Once all these checks have passed, Nios firmware proceed to trigger the update.
 * If it's a PCH active firmware update, Nios performs a PCH-only reset. Otherwise, Nios brings down the whole platform.
 * The actual update is done as part of the T-1 operations.
 *
 * @see act_on_update_intent
 * @see perform_tmin1_operations_for_pch
 */
static void mb_update_intent_handler_for_pch()
{
    // Read the update intent
    alt_u32 update_intent = read_from_mailbox(MB_PCH_UPDATE_INTENT);

    // Check if there's any update intent.
    // If the update is not a deferred update, proceed to validate the update intent value.
    if (update_intent &&
            ((update_intent & MB_UPDATE_INTENT_UPDATE_AT_RESET_MASK) != MB_UPDATE_INTENT_UPDATE_AT_RESET_MASK))
    {
        /*
         * Validate the update intent
         */
        if ((update_intent & MB_UPDATE_INTENT_PCH_FW_UPDATE_MASK) == 0)
        {
            // This update intent is not actionable or valid.
            log_update_failure(SPI_FLASH_PCH, MINOR_ERROR_INVALID_UPDATE_INTENT);
            write_to_mailbox(MB_PCH_UPDATE_INTENT, 0);
            return;
        }

        if (num_failed_update_attempts_from_pch >= MAX_FAILED_UPDATE_ATTEMPTS_FROM_PCH)
        {
            // If there are too many failed firmware update attempts, reject this update.
            log_update_failure(SPI_FLASH_PCH, MINOR_ERROR_EXCEEDED_MAX_FAILED_ATTEMPTS);
            write_to_mailbox(MB_PCH_UPDATE_INTENT, 0);
            return;
        }

        if (check_spi_flash_state(SPI_FLASH_PCH, SPI_FLASH_STATE_RECOVERY_FAILED_AUTH_MASK))
        {
            // There's no valid recovery image

            // Block Active update
            if (update_intent & MB_UPDATE_INTENT_PCH_ACTIVE_MASK)
            {
                log_update_failure(SPI_FLASH_PCH, MINOR_ERROR_ACTIVE_FW_UPDATE_NOT_ALLOWED);
                write_to_mailbox(MB_PCH_UPDATE_INTENT, 0);
                return;
            }

            // Allow update to recovery image directly
            if (update_intent & MB_UPDATE_INTENT_PCH_RECOVERY_MASK)
            {
                // In T-1 step, Nios would do the followings when this flag is set:
                // 1. Ensure update capsule matches the active image
                // 2. Promote the update capsule to the recovery region
                set_spi_flash_state(SPI_FLASH_PCH, SPI_FLASH_STATE_HAS_PENDING_RECOVERY_FW_UPDATE_MASK);
                write_to_mailbox(MB_PCH_UPDATE_INTENT, 0);
            }
        }

        /*
         * Proceed to transition to T-1 mode and perform the update
         */
        // Log panic event
        log_panic(LAST_PANIC_PCH_UPDATE_INTENT);

        // Enter T-1 mode to do the update there
        if (update_intent & MB_UPDATE_INTENT_PCH_ACTIVE_MASK)
        {
            // Only bring down PCH if only PCH active firmware is being updated
            perform_pch_only_reset();
        }
        else
        {
            perform_platform_reset();
        }
    }
}

/**
 * @brief Monitor BMC update intent register and transition to T-1 mode to perform the update if necessary.
 *
 * This function reads the BMC update intent from the mailbox first.
 *
 * If the update intent is not a valid request (i.e. not matching any bit mask), then exit.
 * If the update intent indicates a deferred update, also exit.
 *
 * For the following scenarios, log some error, clear the update intent register and exit.
 * - It's a firmware update and maximum failed update attempts have been reached for that flash device.
 * - It's a CPLD update and maximum failed CPLD update attempts have been reached.
 * - It's a BMC active firmware update and the BMC recovery image is corrupted
 *
 * Once all these checks have passed, Nios firmware proceed to trigger the update.
 * If it's a BMC active firmware update, Nios performs a BMC-only reset. Otherwise, Nios brings down the whole platform.
 * The actual update is done as part of the T-1 operations.
 *
 * @see act_on_update_intent
 * @see perform_tmin1_operations_for_bmc
 */
static void mb_update_intent_handler_for_bmc()
{
    // Read the update intent
    alt_u32 update_intent = read_from_mailbox(MB_BMC_UPDATE_INTENT);

    // Check if there's any update intent.
    // If the update is not a deferred update, proceed to validate the update intent value.
    if (update_intent &&
            ((update_intent & MB_UPDATE_INTENT_UPDATE_AT_RESET_MASK) != MB_UPDATE_INTENT_UPDATE_AT_RESET_MASK))
    {
        /*
         * Validate the update intent
         */
        if ((update_intent & MB_UPDATE_INTENT_FW_OR_CPLD_UPDATE_MASK) == 0)
        {
            // This update intent is not actionable or valid.
            log_update_failure(SPI_FLASH_BMC, MINOR_ERROR_INVALID_UPDATE_INTENT);
            write_to_mailbox(MB_BMC_UPDATE_INTENT, 0);
            return;
        }

        if (num_failed_update_attempts_from_bmc >= MAX_FAILED_UPDATE_ATTEMPTS_FROM_BMC)
        {
            // If there are too many failed firmware/CPLD update attempts, reject this update.
            log_update_failure(SPI_FLASH_BMC, MINOR_ERROR_EXCEEDED_MAX_FAILED_ATTEMPTS);
            write_to_mailbox(MB_BMC_UPDATE_INTENT, 0);
            return;
        }

        if (check_spi_flash_state(SPI_FLASH_BMC, SPI_FLASH_STATE_RECOVERY_FAILED_AUTH_MASK))
        {
            // There's no valid recovery image

            // Block Active update
            if (update_intent & MB_UPDATE_INTENT_BMC_ACTIVE_MASK)
            {
                log_update_failure(SPI_FLASH_BMC, MINOR_ERROR_ACTIVE_FW_UPDATE_NOT_ALLOWED);
                write_to_mailbox(MB_BMC_UPDATE_INTENT, 0);
                return;
            }

            // Allow update to recovery image directly
            if (update_intent & MB_UPDATE_INTENT_BMC_RECOVERY_MASK)
            {
                // In T-1 step, Nios would do the followings when this flag is set:
                // 1. Ensure update capsule matches the active image
                // 2. Promote the update capsule to the recovery region
                set_spi_flash_state(SPI_FLASH_BMC, SPI_FLASH_STATE_HAS_PENDING_RECOVERY_FW_UPDATE_MASK);
                write_to_mailbox(MB_BMC_UPDATE_INTENT, 0);
            }
        }

        if (check_spi_flash_state(SPI_FLASH_PCH, SPI_FLASH_STATE_RECOVERY_FAILED_AUTH_MASK))
        {
            // Block PCH active update when there's no valid recovery image in PCH flash
            if (update_intent & MB_UPDATE_INTENT_PCH_ACTIVE_MASK)
            {
                log_update_failure(SPI_FLASH_BMC, MINOR_ERROR_ACTIVE_FW_UPDATE_NOT_ALLOWED);
                write_to_mailbox(MB_BMC_UPDATE_INTENT, 0);
                return;
            }
        }

        /*
         * Proceed to transition to T-1 mode and perform the update
         */
        // Log panic event
        log_panic(LAST_PANIC_BMC_UPDATE_INTENT);

        // Enter T-1 mode to do the update there
        if (update_intent & MB_UPDATE_INTENT_BMC_REQUIRE_PLATFORM_RESET_MASK)
        {
            perform_platform_reset();
        }
        else
        {
            // Only bring down BMC if only BMC active firmware is being updated
            perform_bmc_only_reset();
        }
    }
}

/**
 * @brief Monitor the PCH and BMC update intent fields in the mailbox register file.
 *
 * @see mb_update_intent_handler_for(SPI_FLASH_TYPE_ENUM)
 */
static void mb_update_intent_handler()
{
    mb_update_intent_handler_for_bmc();
    mb_update_intent_handler_for_pch();
}

/**
 * @brief Trigger platform reset to perform (FW/CPLD) recovery update, after the respective active update
 * has completed and platform has booted.
 *
 * Firmware recovery update involves two parts:
 *   1. Update active firmware first and make sure the new firmware can boot
 *   2. Update the recovery firmware
 *
 * This function triggers a platform reset after #1 is done. In T-1, Nios performs #2 when the
 * FW_RECOVERY_UPDATE_IN_PROGRESS flag is set.
 * This function should only be called when timed boot has completed.
 *
 * For CPLD recovery update, there's a CPLD update status word in UFM, which records the original
 * CPLD recovery update requests before the reconfigurations and CPLD active image update.
 *
 * @see process_updates_in_tmin1
 */
static void post_update_routine()
{
    if (is_timed_boot_done())
    {
        STATUS_LAST_PANIC_ENUM panic_reason = LAST_PANIC_DEFAULT;

        // Check if Nios is in the middle of Firmware Recovery update.
        if (check_spi_flash_state(SPI_FLASH_BMC, SPI_FLASH_STATE_HAS_PENDING_RECOVERY_FW_UPDATE_MASK))
        {
            panic_reason = LAST_PANIC_BMC_UPDATE_INTENT;
        }
        else if (check_spi_flash_state(SPI_FLASH_PCH, SPI_FLASH_STATE_HAS_PENDING_RECOVERY_FW_UPDATE_MASK))
        {
            panic_reason = LAST_PANIC_PCH_UPDATE_INTENT;
        }

        // Check if Nios is in the middle of CPLD Recovery update.
        if (is_cpld_rc_update_in_progress())
        {
            // CFM1 has successfully booted. Nios can proceed to promote staged CPLD update capsule to CPLD recovery region
            set_spi_flash_state(SPI_FLASH_BMC, SPI_FLASH_STATE_READY_FOR_CPLD_RECOVERY_UPDATE_MASK);
            // Leave panic_reason as BMC update intent because currently only the BMC can trigger a CPLD update
            panic_reason = LAST_PANIC_BMC_UPDATE_INTENT;
        }

        // Nios has set the proper SPI state for both BMC and PCH at this point.
        // If there are two firmware recovery updates, they will be performed in the same T-1 cycle.
        if (panic_reason)
        {
            // Firmware/CPLD Recovery update will be performed in T-1
            log_panic(panic_reason);
            perform_platform_reset();
        }
    }
}

#endif /* WHITLEY_INC_T0_UPDATE_H_ */
