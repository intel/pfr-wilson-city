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
 * @file keychain.h
 * @brief Define the key chaining structures, such as block 0 and block 1.
 */

#ifndef WHITLEY_INC_KEYCHAIN_H_
#define WHITLEY_INC_KEYCHAIN_H_

// Always include pfr_sys.h first
#include "pfr_sys.h"

// Magic Numbers
#define BLOCK0_MAGIC 0xB6EAFD19
#define BLOCK1_MAGIC 0xF27F28D7
#define BLOCK1_ROOT_ENTRY_MAGIC 0xA757A046
#define BLOCK1_CSK_ENTRY_MAGIC 0x14711C2F
#define BLOCK1_B0_ENTRY_MAGIC 0x15364367

#define CURVE_SECP256R1_MAGIC 0xC7B88C74
#define CURVE_SECP384R1_MAGIC 0x08F07B47
#define SIG_SECP256R1_MAGIC 0xDE64437D
#define SIG_SECP384R1_MAGIC 0xEA2A50E9

#define KCH_EXPECTED_CURVE_MAGIC CURVE_SECP256R1_MAGIC
#define KCH_EXPECTED_SIG_MAGIC SIG_SECP256R1_MAGIC

#define KCH_PC_TYPE_MASK 0x000000FF
// Bit[8] = 1 means this is a key cancellation certificate
#define KCH_PC_TYPE_KEY_CAN_CERT_MASK 0x00000100
// Bit[9] = 1 means this is a decommission capsule
#define KCH_PC_TYPE_DECOMM_CAP_MASK 0x00000200

/*!
 * Protected Content Type
 */
typedef enum
{
    KCH_PC_PFR_CPLD_UPDATE_CAPSULE = 0,
    KCH_PC_PFR_PCH_PFM             = 1,
    KCH_PC_PFR_PCH_UPDATE_CAPSULE  = 2,
    KCH_PC_PFR_BMC_PFM             = 3,
    KCH_PC_PFR_BMC_UPDATE_CAPSULE  = 4,
} KCH_PC_TYPE_ENUM;

/*!
 * Key Permissions
 */
#define SIGN_PCH_PFM_MASK 0b1
#define SIGN_PCH_UPDATE_CAPSULE_MASK 0b10
#define SIGN_BMC_PFM_MASK 0b100
#define SIGN_BMC_UPDATE_CAPSULE_MASK 0b1000
#define SIGN_CPLD_UPDATE_CAPSULE_MASK 0b10000
// 0xFFFFFFFF (-1) means that a key has all permissions
#define SIGN_ALL 0xFFFFFFFF

// Key cancellation
#define KCH_KEY_NON_CANCELLABLE 0xFFFFFFFF
#define KCH_MAX_KEY_ID 127

// Sizes (bytes) of these structures
#define BLOCK0_SIZE 128
#define BLOCK1_SIZE 896
#define SIGNATURE_SIZE 1024

#define BLOCK0_FRIST_RESERVED_SIZE 4
#define BLOCK0_SECOND_RESERVED_SIZE 32

// Number of bytes before the signature chain
#define BLOCK1_HEADER_SIZE 16

#define BLOCK1_ROOT_ENTRY_SIZE 132
#define BLOCK1_ROOT_ENTRY_RESERVED_SIZE 20

#define BLOCK1_CSK_ENTRY_SIZE 232
#define BLOCK1_CSK_ENTRY_HASH_REGION_SIZE 128
#define BLOCK1_CSK_ENTRY_RESERVED_SIZE 20

#define BLOCK1_B0_ENTRY_SIZE 104
#define BLOCK1_RESERVED_SIZE                                                             \
    (BLOCK1_SIZE - BLOCK1_HEADER_SIZE - BLOCK1_ROOT_ENTRY_SIZE - BLOCK1_CSK_ENTRY_SIZE - \
     BLOCK1_B0_ENTRY_SIZE)

// Key cancellation certificate and decommission capsule shares the same size
#define KCH_CAN_CERT_OR_DECOMM_CAP_PC_SIZE 128
#define KCH_CAN_CERT_RESERVED_SIZE 124
#define DECOMM_CAP_RESERVED_SIZE 128

/*!
 * Block 0 describes the content which is protected by the authentication format
 */
typedef struct
{
    alt_u32 magic;
    alt_u32 pc_length;
    alt_u32 pc_type;
    alt_u32 reserved;
    alt_u32 pc_hash256[SHA256_LENGTH / 4];
    alt_u32 pc_hash384[SHA384_LENGTH / 4];
    alt_u32 reserved2[BLOCK0_SECOND_RESERVED_SIZE / 4];
} KCH_BLOCK0;

/*!
 * The Root Entry in Block 1
 * Root entry contains the public component of the root authentication key
 */
typedef struct
{
    alt_u32 magic;
    /* Hashed region for validation: Start */
    alt_u32 curve_magic;
    alt_u32 permissions;
    alt_u32 key_id;
    alt_u32 pubkey_x[SHA384_LENGTH / 4];
    alt_u32 pubkey_y[SHA384_LENGTH / 4];
    alt_u32 _reserved[BLOCK1_ROOT_ENTRY_RESERVED_SIZE / 4];
    /* Hashed region for validation: End */
} KCH_BLOCK1_ROOT_ENTRY;

/*!
 * Code Signing Key entry in Block 1.
 * CSK is used to sign the Block 0 describing the Protected Content payload.
 */
typedef struct
{
    alt_u32 magic;
    /* Hashed region for signing: Start */
    alt_u32 curve_magic;
    alt_u32 permissions;
    alt_u32 key_id;
    alt_u32 pubkey_x[SHA384_LENGTH / 4];
    alt_u32 pubkey_y[SHA384_LENGTH / 4];
    alt_u32 reserved[BLOCK1_CSK_ENTRY_RESERVED_SIZE / 4];
    /* Hashed region for signing: End */
    /* Signature over the hashed region using the Root Key in the previous entry*/
    alt_u32 sig_magic;
    alt_u32 sig_r[SHA384_LENGTH / 4];
    alt_u32 sig_s[SHA384_LENGTH / 4];
} KCH_BLOCK1_CSK_ENTRY;

/*!
 * Block 0 entry in Block 1 contains a signature over the hash of Block 0.
 * This is the final entry in Block 1.
 */
typedef struct
{
    alt_u32 magic;
    alt_u32 sig_magic;
    alt_u32 sig_r[SHA384_LENGTH / 4];
    alt_u32 sig_s[SHA384_LENGTH / 4];
} KCH_BLOCK1_B0_ENTRY;

/*!
 * Block 1 format in normal flow (PFM/Update capsule PC types)
 * Block 1 contains a signature chain which is used to sign Block 0.
 */
typedef struct
{
    alt_u32 magic;
    alt_u32 _reserved[3];
    /* Signature Chain (Root entry, CSK entry and Block0 entry) */
    KCH_BLOCK1_ROOT_ENTRY root_entry;
    KCH_BLOCK1_CSK_ENTRY csk_entry;
    KCH_BLOCK1_B0_ENTRY b0_entry;
    alt_u32 _reserved2[BLOCK1_RESERVED_SIZE / 4];
} KCH_BLOCK1;

/*!
 * Key chain contains block 0 and block 1.
 */
typedef struct
{
    KCH_BLOCK0 b0;
    KCH_BLOCK1 b1;
} KCH_SIGNATURE;

/*!
 * Key cancellation certificate.
 */
typedef struct
{
    alt_u32 csk_id;
    alt_u32 reserved[KCH_CAN_CERT_RESERVED_SIZE / 4];
} KCH_CAN_CERT;

#endif /* WHITLEY_INC_KEYCHAIN_H_ */
