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
 * @file pfr_pointers.h
 * @brief This header facilitates retrieval of various pointer types in this PFR system.
 */

#ifndef WHITLEY_INC_PFR_POINTERS_H
#define WHITLEY_INC_PFR_POINTERS_H

// Always include pfr_sys.h first
#include "pfr_sys.h"

#include "gen_gpo_controls.h"
#include "gen_smbus_relay_config.h"
#include "keychain.h"
#include "pbc.h"
#include "pfm.h"
#include "ufm.h"
#include "utils.h"


/**
 * @brief This function increments an alt_u32 pointer by a number of bytes.
 * Since alt_u32 pointer is word (4 bytes) aligned, nbytes must be multiple of 4.
 *
 * @param ptr pointer to be incremented
 * @param n_bytes number of bytes to increment
 * @return incremented pointer
 */
static PFR_ALT_INLINE alt_u32* PFR_ALT_ALWAYS_INLINE incr_alt_u32_ptr(alt_u32* ptr, alt_u32 n_bytes)
{
    PFR_ASSERT((n_bytes % 4) == 0);
    return ptr + (n_bytes >> 2);
}

/**
 * @brief Return a pointer to the start of the SPI flash memory space.
 *
 * @return a pointer to a SPI address
 */
static PFR_ALT_INLINE alt_u32* PFR_ALT_ALWAYS_INLINE get_spi_flash_ptr()
{
#ifdef USE_SYSTEM_MOCK
    // Use the pointer to the flash memory mock in unittests
    return SYSTEM_MOCK::get()->get_x86_ptr_to_spi_flash();
#endif
    return __IO_CALC_ADDRESS_NATIVE_ALT_U32(U_SPI_FILTER_AVMM_BRIDGE_BASE, 0);
}

/**
 * @brief Return a pointer to an address in the SPI memory range, given an offset in the range.
 *
 * @param offset an offset within the SPI memory range
 * @return a pointer to a SPI address
 */
static PFR_ALT_INLINE alt_u32* PFR_ALT_ALWAYS_INLINE get_spi_flash_ptr_with_offset(alt_u32 offset)
{
    return incr_alt_u32_ptr(get_spi_flash_ptr(), offset);
}

/**
 * @brief Return a pointer to an address in the UFM memory range.
 *
 * @param offset an offset within the UFM memory range
 * @return a pointer to a UFM address
 */
static alt_u32* get_ufm_ptr_with_offset(alt_u32 offset)
{
#ifdef USE_SYSTEM_MOCK
    // Retrieve the mock ufm data from system_mock in unittest environment
    return incr_alt_u32_ptr(SYSTEM_MOCK::get()->get_ufm_data_ptr(), offset);
#endif
    return incr_alt_u32_ptr(U_UFM_DATA_ADDR, offset);
}

/**
 * @brief Return the pointer to starting address of the UFM PFR data.
 * By default, Nios firmware saves the persistent data in the first page of the UFM,
 * according to the UFM_PFR_DATA structure. This data includes the provisioned data
 * (root key hash, pit password, etc), SVN policy, CSK policy and others.
 *
 * Note that get_ufm_ptr_with_offset() function can be used in this function,
 * but it would cost a few hundred bytes of code more somehow.
 *
 * @return UFM_PFR_DATA* pointer to the UFM PFR data
 */
static PFR_ALT_INLINE UFM_PFR_DATA* PFR_ALT_ALWAYS_INLINE get_ufm_pfr_data()
{
    PFR_ASSERT(UFM_PFR_DATA_OFFSET == 0);
#ifdef USE_SYSTEM_MOCK
    // Retrieve the mock ufm data from system_mock in unittest environment
    return (UFM_PFR_DATA*) incr_alt_u32_ptr(SYSTEM_MOCK::get()->get_ufm_data_ptr(), UFM_PFR_DATA_OFFSET);
#endif
    return (UFM_PFR_DATA*) (U_UFM_DATA_ADDR + UFM_PFR_DATA_OFFSET / 4);
}

static alt_u32 get_recovery_region_offset(SPI_FLASH_TYPE_ENUM spi_flash_type)
{
    if (spi_flash_type == SPI_FLASH_BMC)
    {
        return get_ufm_pfr_data()->bmc_recovery_region;
    }
    // spi_flash_type == SPI_FLASH_PCH
    return get_ufm_pfr_data()->pch_recovery_region;
}

static alt_u32 get_staging_region_offset(SPI_FLASH_TYPE_ENUM spi_flash_type)
{
    if (spi_flash_type == SPI_FLASH_BMC)
    {
        return get_ufm_pfr_data()->bmc_staging_region;
    }
    // spi_flash_type == SPI_FLASH_PCH
    return get_ufm_pfr_data()->pch_staging_region;
}

static alt_u32* get_spi_active_pfm_ptr(SPI_FLASH_TYPE_ENUM spi_flash_type)
{
    if (spi_flash_type == SPI_FLASH_BMC)
    {
        return get_spi_flash_ptr_with_offset(get_ufm_pfr_data()->bmc_active_pfm);
    }
    // spi_flash_type == SPI_FLASH_PCH
    return get_spi_flash_ptr_with_offset(get_ufm_pfr_data()->pch_active_pfm);
}

static alt_u32* get_spi_recovery_region_ptr(SPI_FLASH_TYPE_ENUM spi_flash_type)
{
    return get_spi_flash_ptr_with_offset(get_recovery_region_offset(spi_flash_type));
}

static alt_u32* get_spi_staging_region_ptr(SPI_FLASH_TYPE_ENUM spi_flash_type)
{
    return get_spi_flash_ptr_with_offset(get_staging_region_offset(spi_flash_type));
}

/**
 * @brief Return a pointer to the start address of PBC structure in a signed firmware update capsule.
 */
static PBC_HEADER* get_pbc_ptr_from_signed_capsule(alt_u32* signed_capsule)
{
    // Skip capsule signature
    alt_u32* signed_pfm = incr_alt_u32_ptr(signed_capsule, SIGNATURE_SIZE);
    KCH_BLOCK0* block0 = (KCH_BLOCK0*) signed_pfm;

    // Skip signed pfm (signature + payload/pfm size)
    return (PBC_HEADER*) incr_alt_u32_ptr(signed_pfm, SIGNATURE_SIZE + block0->pc_length);
}

/**
 * @brief Given the bus ID, return the pointer to the SMBus relay AvMM base address in the system.
 *
 * @param bus_id SMBus ID
 *
 * @return the pointer to the SMBus relay AvMM base address
 */
static alt_u32* get_relay_base_ptr(alt_u32 bus_id)
{
    PFR_ASSERT(NUM_RELAYS == 3);

#ifdef USE_SYSTEM_MOCK
    // Use the pointer to the SMBus relay mock in unittests
    return SYSTEM_MOCK::get()->smbus_relay_mock_ptr->get_cmd_enable_memory_for_smbus(bus_id);
#endif

    if (bus_id == 1)
    {
        return __IO_CALC_ADDRESS_NATIVE_ALT_U32(U_RELAY1_AVMM_BRIDGE_BASE, 0);
    }
    else if (bus_id == 2)
    {
        return __IO_CALC_ADDRESS_NATIVE_ALT_U32(U_RELAY2_AVMM_BRIDGE_BASE, 0);
    }
    else if (bus_id == 3)
    {
        return __IO_CALC_ADDRESS_NATIVE_ALT_U32(U_RELAY3_AVMM_BRIDGE_BASE, 0);
    }
    return 0;
}

#endif /* WHITLEY_INC_PFR_POINTERS_H */
