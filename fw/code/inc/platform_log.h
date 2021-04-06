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
 * @file platform_log.h
 * @brief Logging functions for various firmware stages/events (e.g. T0, recovery, T-1).
 */

#ifndef WHITLEY_INC_PLATFORM_LOG_H_
#define WHITLEY_INC_PLATFORM_LOG_H_

#include "pfr_sys.h"

#include "global_state.h"
#include "mailbox_utils.h"
#include "watchdog_timers.h"


/**
 * @brief This function logs platform state to mailbox and
 * global state (which drives the 7-seg display content on platform).
 */
static void log_platform_state(const STATUS_PLATFORM_STATE_ENUM state)
{
    write_to_mailbox(MB_PLATFORM_STATE, (alt_u32) state);
    set_global_state(state);
}

/**
 * @brief This function logs major/minor error to mailbox
 */
static void log_errors(STATUS_MAJOR_ERROR_ENUM major_err, alt_u32 minor_err)
{
    write_to_mailbox(MB_MAJOR_ERROR_CODE, major_err);
    write_to_mailbox(MB_MINOR_ERROR_CODE, minor_err);
}

/**
 * @brief This function logs panic reason to mailbox
 */
static void log_panic(STATUS_LAST_PANIC_ENUM panic_reason)
{
    // Log the panic reason
    write_to_mailbox(MB_LAST_PANIC_REASON, (alt_u32) panic_reason);

    // Increment panic count
    write_to_mailbox(MB_PANIC_EVENT_COUNT, 1 + read_from_mailbox(MB_PANIC_EVENT_COUNT));
}

static void log_firmware_update(SPI_FLASH_TYPE_ENUM spi_flash_type)
{
    if (spi_flash_type == SPI_FLASH_PCH)
    {
        log_platform_state(PLATFORM_STATE_PCH_FW_UPDATE);
    }
    else
    {
        log_platform_state(PLATFORM_STATE_BMC_FW_UPDATE);
    }
}

/**
 * @brief This function logs recovery reason to mailbox
 */
static void log_recovery(STATUS_LAST_RECOVERY_ENUM last_recovery_reason)
{
    // Log the recovery reason
    write_to_mailbox(MB_LAST_RECOVERY_REASON, last_recovery_reason);

    // Increment recovery count
    write_to_mailbox(MB_RECOVERY_COUNT, read_from_mailbox(MB_RECOVERY_COUNT) + 1);
}

static void log_tmin1_recovery(STATUS_LAST_RECOVERY_ENUM last_recovery_reason)
{
    log_platform_state(PLATFORM_STATE_TMIN1_FW_RECOVERY);
    log_recovery(last_recovery_reason);
}

static void log_wdt_recovery(STATUS_LAST_RECOVERY_ENUM last_recovery_reason, STATUS_LAST_PANIC_ENUM panic_reason)
{
    log_recovery(last_recovery_reason);
    log_panic(panic_reason);
}

static void log_tmin1_recovery_on_active_image(SPI_FLASH_TYPE_ENUM spi_flash_type)
{
    if (spi_flash_type == SPI_FLASH_BMC)
    {
        log_tmin1_recovery(LAST_RECOVERY_BMC_ACTIVE);
    }
    else // spi_flash_type == SPI_FLASH_PCH
    {
        log_tmin1_recovery(LAST_RECOVERY_PCH_ACTIVE);
    }
}

static void log_tmin1_recovery_on_recovery_image(SPI_FLASH_TYPE_ENUM spi_flash_type)
{
    if (spi_flash_type == SPI_FLASH_BMC)
    {
        log_tmin1_recovery(LAST_RECOVERY_BMC_RECOVERY);
    }
    else // spi_flash_type == SPI_FLASH_PCH
    {
        log_tmin1_recovery(LAST_RECOVERY_PCH_RECOVERY);
    }
}

static void log_update_failure(SPI_FLASH_TYPE_ENUM spi_flash_type, alt_u32 minor_err)
{
    if (spi_flash_type == SPI_FLASH_PCH)
    {
        log_errors(MAJOR_ERROR_UPDATE_FROM_PCH_FAILED, minor_err);
    }
    else // spi_flash_type == SPI_FLASH_BMC
    {
        log_errors(MAJOR_ERROR_UPDATE_FROM_BMC_FAILED, minor_err);
    }
}

static void log_auth_failure(SPI_FLASH_TYPE_ENUM spi_flash_type, alt_u32 minor_err)
{
    if (spi_flash_type == SPI_FLASH_PCH)
    {
        log_errors(MAJOR_ERROR_PCH_AUTH_FAILED, minor_err);
    }
    else // spi_flash_type == SPI_FLASH_BMC
    {
        log_errors(MAJOR_ERROR_BMC_AUTH_FAILED, minor_err);
    }
}

/**
 * @brief Log boot complete status in T0 mode
 *
 * @param current_boot_state the status for the component that has just completed boot
 */
static void log_t0_timed_boot_complete_if_ready(const STATUS_PLATFORM_STATE_ENUM current_boot_state)
{
    if (is_timed_boot_done())
    {
        // If other components have finished booting, log timed boot complete status.
        log_platform_state(PLATFORM_STATE_T0_BOOT_COMPLETE);
    }
    else
    {
        // Otherwise, just log the this boot complete status
        log_platform_state(current_boot_state);
    }
}
#endif /* WHITLEY_INC_PLATFORM_LOG_H_ */
