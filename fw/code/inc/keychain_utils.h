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
 * @file keychain_utils.h
 * @brief Utility functions for easier handling of key chaining structures.
 */

#ifndef WHITLEY_INC_KEYCHAIN_UTILS_H_
#define WHITLEY_INC_KEYCHAIN_UTILS_H_

// Always include pfr_sys.h first
#include "pfr_sys.h"

#include "keychain.h"
#include "pfr_pointers.h"
#include "utils.h"

/**
 * @brief Extract the lower 8 bits of the protected content type from Block 0. 
 */ 
static PFR_ALT_INLINE alt_u32 PFR_ALT_ALWAYS_INLINE get_kch_pc_type(KCH_BLOCK0* b0)
{
    return b0->pc_type & KCH_PC_TYPE_MASK;
}

/**
 * @brief Return the key permissions mask that is associated with a protected content type.
 *
 * @param pc_type protected content type
 * @return key permissions mask
 */
static alt_u32 get_required_perm(alt_u32 pc_type)
{
    if (pc_type == KCH_PC_PFR_CPLD_UPDATE_CAPSULE)
    {
        return SIGN_CPLD_UPDATE_CAPSULE_MASK;
    }
    if (pc_type == KCH_PC_PFR_PCH_PFM)
    {
        return SIGN_PCH_PFM_MASK;
    }
    if (pc_type == KCH_PC_PFR_PCH_UPDATE_CAPSULE)
    {
        return SIGN_PCH_UPDATE_CAPSULE_MASK;
    }
    if (pc_type == KCH_PC_PFR_BMC_PFM)
    {
        return SIGN_BMC_PFM_MASK;
    }
    if (pc_type == KCH_PC_PFR_BMC_UPDATE_CAPSULE)
    {
        return SIGN_BMC_UPDATE_CAPSULE_MASK;
    }
    return 0;
}

/**
 * @brief Get the size (in bytes) of the signed payload (signature + payload).
 *
 * @param signed_payload_addr the start address of the signed payload
 * @return the size of the signed payload
 */
static PFR_ALT_INLINE alt_u32 PFR_ALT_ALWAYS_INLINE get_signed_payload_size(alt_u32* signed_payload_addr)
{
    return SIGNATURE_SIZE + ((KCH_BLOCK0*) signed_payload_addr)->pc_length;
}

#endif /* WHITLEY_INC_KEYCHAIN_UTILS_H_ */
