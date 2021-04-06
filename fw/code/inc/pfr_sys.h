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
 * @file pfr_sys.h
 * @brief Common header for CPLD RoT Nios II code.
 */

#ifndef PFR_SYS_H_
#define PFR_SYS_H_

// Include all BSP headers. No other files should include the BSP headers so that
// we can mock them as required for unit testing

#include "io.h"
#include "system.h"
#include "alt_types.h"

#include "gen_smbus_relay_config.h"

// PFR system assert. Can be compiled out and handled separately for unit testing
#ifdef PFR_ENABLE_ASSERT
#define PFR_ASSERT(x) NOT_YET_IMPLEMENTED
#define PFR_INTERNAL_ERROR(msg) NOT_YET_IMPLEMENTED
#define PFR_INTERNAL_ERROR_VARG(format, ...) NOT_YET_IMPLEMENTED
#else
#ifndef PFR_NO_BSP_DEP
// Define the macro to do nothing
#define PFR_ASSERT(x)
#define PFR_INTERNAL_ERROR(msg)
#define PFR_INTERNAL_ERROR_VARG(format, ...)
#endif
#endif

// Inlining control macros. When in debug mode disable the forced inlining
#define PFR_ALT_DONT_INTLINE __attribute__((noinline))
#ifndef PFR_DEBUG_MODE
#undef PFR_ALT_INLINE
#undef PFR_ALT_ALWAYS_INLINE
#define PFR_ALT_INLINE ALT_INLINE
#define PFR_ALT_ALWAYS_INLINE ALT_ALWAYS_INLINE
#else
#define PFR_ALT_INLINE
#define PFR_ALT_ALWAYS_INLINE
#endif

/*******************************************************************
 * Address Calculation Macros
 *******************************************************************/
#define __IO_CALC_ADDRESS_NATIVE_ALT_U32(BASE, REGNUM) \
    ((alt_u32*) (((alt_u8*) BASE) + ((REGNUM) * (SYSTEM_BUS_WIDTH / 8))))

// To create alt_u32*
#define U_GPO_1_ADDR __IO_CALC_ADDRESS_NATIVE_ALT_U32(U_GPO_1_BASE, 0)
#define U_GPI_1_ADDR __IO_CALC_ADDRESS_NATIVE_ALT_U32(U_GPI_1_BASE, 0)

/*******************************************************************
 * CPLD Design Info
 *******************************************************************/
#define CPLD_ROT_RELEASE_VERSION 1

/*******************************************************************
 * Watchdog Timer Settings
 * Set all timeout values to 25 minutes
 * HW Timer resolution is at 20ms
 *******************************************************************/
// Timeout value for BMC WDT
#define WD_BMC_TIMEOUT_VAL 0x124F8

// Timeout values for ME WDT
#define WD_ME_TIMEOUT_VAL 0x124F8

// Timeout values for ACM WDT
#define WD_ACM_TIMEOUT_VAL 0x124F8

// Timeout values for BIOS WDT
#define WD_IBB_TIMEOUT_VAL 0x124F8
#define WD_OBB_TIMEOUT_VAL 0x124F8

// When there's BTG (ACM) authentication failure, ACM will pass that information to ME firmware.
// After ME performs clean up, it shuts down the system. Then CPLD can perform the WDT recovery.
// This 2s timeout value is the time needs for ACM/ME communication and ME's clean up
#define WD_ACM_AUTH_FAILURE_WAIT_TIME_VAL 40

/*******************************************************************
 * Platform Components
 *******************************************************************/
typedef enum
{
    SPI_FLASH_BMC = 1,
    SPI_FLASH_PCH = 2,
} SPI_FLASH_TYPE_ENUM;

#define BMC_SPI_FLASH_SIZE 0x8000000
#define PCH_SPI_FLASH_SIZE 0x4000000

typedef enum
{
	SPI_FLASH_MICRON = 0x0021ba20,
	SPI_FLASH_MACRONIX = 0x001a20c2,
} SPI_FLASH_ID_ENUM;

typedef enum
{
	// opcode = 0xEB, dummy cycles = 0xA
	SPI_FLASH_QUAD_READ_PROTOCOL_MICRON = 0x00000AEB,
	// opcode = 0xEB, dummy cycles = 0x6
	SPI_FLASH_QUAD_READ_PROTOCOL_MACRONIX = 0x000006EB,
} QUAD_IO_PROTOCOL;

/*******************************************************************
 * BMC flash offsets
 *******************************************************************/
// Offsets for different update capsule
// These offsets are relative to staging region
#define BMC_STAGING_REGION_BMC_UPDATE_CAPSULE_OFFSET 0
#define BMC_STAGING_REGION_PCH_UPDATE_CAPSULE_OFFSET 0x2000000
#define BMC_STAGING_REGION_CPLD_UPDATE_CAPSULE_OFFSET 0x3000000

// Reserved area for CPLD recovery image
// This is a hard-coded value
#define BMC_CPLD_RECOVERY_IMAGE_OFFSET 0x7F00000
#define BMC_CPLD_RECOVERY_LOCATION_IN_WE_MEM (BMC_CPLD_RECOVERY_IMAGE_OFFSET >> 19)

/*******************************************************************
 * CPLD Update Settings
 *******************************************************************/
// External agent that is responsible for sending CPLD update intent
#define CPLD_UPDATE_AGENT SPI_FLASH_BMC

// Max size for a CPLD update capsule
// This size is selected because the offsets of CPLD images in BMC are 1MB apart.
#define MAX_CPLD_UPDATE_CAPSULE_SIZE 0x100000

/*******************************************************************
 * UFM data
 *******************************************************************/
#define U_UFM_DATA_ADDR __IO_CALC_ADDRESS_NATIVE_ALT_U32(U_UFM_DATA_BASE, 0)

// UFM PFR data (e.g. the provisioned data, svn and csk policies)
#define UFM_PFR_DATA_OFFSET U_UFM_DATA_SECTOR1_START_ADDR
#define UFM_CPLD_UPDATE_STATUS_OFFSET U_UFM_DATA_SECTOR2_START_ADDR

// UFM page erase is not currently working but sector erase is,
// so we are using a whole sector to store the CPLD update status word
#define UFM_CPLD_UPDATE_STATUS_SECTOR_ID 0b010

// CFM
typedef enum
{
    CPLD_CFM0 = 0,
    CPLD_CFM1 = 1,
} CPLD_CFM_TYPE_ENUM;

// The Recovery Manager CPLD image is stored in CFM0
// CFM0 locates in Sector 5 of the UFM
#define CMF0_SECTOR_ID 0b101
#define UFM_CPLD_ROM_IMAGE_OFFSET U_UFM_DATA_SECTOR5_START_ADDR
// CPLD image is the entire size of CFM0
#define UFM_CPLD_ROM_IMAGE_LENGTH (U_UFM_DATA_SECTOR5_END_ADDR - U_UFM_DATA_SECTOR5_START_ADDR + 1)

// The active CPLD image is stored in CFM1
// CFM1 locates in Sector 3 and 4 of the UFM
#define CMF1_TOP_HALF_SECTOR_ID 0b011
#define CMF1_BOTTOM_HALF_SECTOR_ID 0b100
#define UFM_CPLD_ACTIVE_IMAGE_OFFSET U_UFM_DATA_SECTOR3_START_ADDR
// CPLD image is the entire size of CFM1
#define UFM_CPLD_ACTIVE_IMAGE_LENGTH (U_UFM_DATA_SECTOR4_END_ADDR - U_UFM_DATA_SECTOR3_START_ADDR + 1)

// CFM1 breadcrumb
// Nios sets it to 0 if Nios is in the recovery due to a CPLD update, 0xFFFFFFFF otherwise
#define CFM1_BREAD_CRUMB (U_UFM_DATA_SECTOR5_START_ADDR - 4)

/*******************************************************************
 * CPLD Update Setting
 *******************************************************************/
// Image 1 & Image 2 has the same pre-defined size for all the Max10 devices.
// For example, on 10M16 device, both Image 1 & 2 are 264 KB (66 pages * 32 Kb/page)
// RPD file size is also the same as Image 1/2 size.
// Protected content of CPLD update capsule contains:
//    SVN (4 bytes) + RPD (with correct bit/byte ordering) + Paddings (124 bytes)
#define EXPECTED_CPLD_UPDATE_CAPSULE_PC_LENGTH (128 + UFM_CPLD_ACTIVE_IMAGE_LENGTH)

/*******************************************************************
 * Firmware Update Setting
 *******************************************************************/

// Max size for a BMC FW update capsule
// In BMC flash, there're 32MB allocated for BMC update capsule
#define MAX_BMC_FW_UPDATE_CAPSULE_SIZE 0x2000000
// Max size for a PCH FW update capsule
// In BMC flash, there're 16MB allocated for PCH update capsule
#define MAX_PCH_FW_UPDATE_CAPSULE_SIZE 0x1000000

// PFM/Recovery/Staging region size
// Maximum size for signed PFM is 64KB.
// In SPI maps, 64KB is reserved for signed PFM on PCH side and 128KB is reserved on BMC side.
#define SIGNED_PFM_MAX_SIZE 0x10000

#define SPI_FLASH_BMC_RECOVERY_SIZE 0x2000000
#define SPI_FLASH_PCH_RECOVERY_SIZE 0x1000000

#define SPI_FLASH_BMC_STAGING_SIZE  0x3600000
#define SPI_FLASH_PCH_STAGING_SIZE  0x1000000

/*******************************************************************
 * Firmware/CPLD Update Setting
 *******************************************************************/
// Max number of failed attempts for firmware/CPLD update
#define MAX_FAILED_UPDATE_ATTEMPTS_FROM_BMC 3
#define MAX_FAILED_UPDATE_ATTEMPTS_FROM_PCH 3

/*******************************************************************
 * Crypto
 *******************************************************************/
#define SHA256_LENGTH 32
#define SHA384_LENGTH 48

// Note that this design currently only supports one Hash algorithm
#define PFR_CRYPTO_LENGTH SHA256_LENGTH

// PFR Root Key Hash
// Nios computes the root key hash with:
//   Root public X (little-endian byte order) + Root Public key Y (little-endian byte order)
#define PFR_ROOT_KEY_HASH_DATA_SIZE 64

// Nios can copy 4MB data from SPI flash to crypto block without petting the HW timer.
#define PFR_CRYPTO_SAFE_COPY_DATA_SIZE 0x400000

/*******************************************************************
 * SMBus Relay Settings
 *******************************************************************/
#define PFR_SYS_NUM_SMBUS_RELAYS 3

// Assume the command enable memory has the same size for all three relays
// This is done to save some code space.
#define SMBUS_RELAY_CMD_EN_MEM_NUM_WORDS (U_RELAY1_AVMM_BRIDGE_SPAN / 4)

// SMBus relays mapping for device address/bus ID/rule ID
static const alt_u8 smbus_device_addr[NUM_RELAYS][MAX_I2C_ADDRESSES_PER_RELAY] = RELAYS_I2C_ADDRESSES;

#endif /* PFR_SYS_H_ */
