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
 * @file spi_flash_state.h
 * @brief Track the state of SPI flash devices.
 */

#ifndef WHITLEY_INC_SPI_FLASH_STATE_H_
#define WHITLEY_INC_SPI_FLASH_STATE_H_

// Always include pfr_sys.h first
#include "pfr_sys.h"

#include "spi_common.h"

/**
 * Define the state that BMC/PCH flash may be in
 */
typedef enum
{
    SPI_FLASH_STATE_RECOVERY_FAILED_AUTH_MASK            = 0b1,
    SPI_FLASH_STATE_ALL_REGIONS_FAILED_AUTH_MASK         = 0b10,
    SPI_FLASH_STATE_HAS_PENDING_RECOVERY_FW_UPDATE_MASK  = 0b100,
    SPI_FLASH_STATE_REQUIRE_WDT_RECOVERY_MASK            = 0b1000,
    SPI_FLASH_STATE_READY_FOR_CPLD_RECOVERY_UPDATE_MASK  = 0b100000,

    // Some combinations of the above
    SPI_FLASH_STATE_CLEAR_AUTH_RESULT                    = SPI_FLASH_STATE_RECOVERY_FAILED_AUTH_MASK | SPI_FLASH_STATE_ALL_REGIONS_FAILED_AUTH_MASK,
} SPI_FLASH_STATE_MASK_ENUM;

// Static variables to track the state of flash devices
static alt_u8 bmc_flash_state = 0;
static alt_u8 pch_flash_state = 0;

/******************************************************
 *
 * Helper functions to work with the static variables
 *
 ******************************************************/

/**************************
 * State settings
 **************************/
static void clear_spi_flash_state(SPI_FLASH_TYPE_ENUM spi_flash_type, SPI_FLASH_STATE_MASK_ENUM state)
{
    if (spi_flash_type == SPI_FLASH_BMC)
    {
        bmc_flash_state &= ~((alt_u32) state);
    }
    else // spi_flash_type == SPI_FLASH_PCH
    {
        pch_flash_state &= ~((alt_u32) state);
    }
}

static alt_u32 check_spi_flash_state(SPI_FLASH_TYPE_ENUM spi_flash_type, SPI_FLASH_STATE_MASK_ENUM state)
{
    if (spi_flash_type == SPI_FLASH_BMC)
    {
        return (bmc_flash_state & state) == state;
    }
    // spi_flash_type == SPI_FLASH_PCH
    return (pch_flash_state & state) == state;
}

static void set_spi_flash_state(SPI_FLASH_TYPE_ENUM spi_flash_type, SPI_FLASH_STATE_MASK_ENUM state)
{
    if (spi_flash_type == SPI_FLASH_BMC)
    {
        bmc_flash_state |= state;
    }
    else // spi_flash_type == SPI_FLASH_PCH
    {
        pch_flash_state |= state;
    }
}

#endif /* WHITLEY_INC_SPI_FLASH_STATE_H_ */
