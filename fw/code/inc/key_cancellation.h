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
 * @file key_cancellation.h
 * @brief Responsible for key cancellation flow.
 */

#ifndef WHITLEY_INC_KEY_CANCELLATION_H_
#define WHITLEY_INC_KEY_CANCELLATION_H_

// Always include pfr_sys.h first
#include "pfr_sys.h"

#include "keychain_utils.h"
#include "pfr_pointers.h"
#include "ufm_utils.h"
#include "utils.h"


/**
 * @brief Given a protected content type and Key ID, cancel that ID in the corresponding
 * key cancellation policy in UFM. 
 * 
 * Nios first finds the correspoinding bit in the appropriate key cancellation policy 
 * in UFM. The key id is cancelled by setting that bit to 0. The first bit corresponds with
 * key ID 0. 
 * For example, if the PC type is CPLD and key ID is 20, Nios finds the key cancellation
 * policy for CPLD and then set the 21st bit to 0. 
 * 
 * Prior to calling this function, Nios should have authenticated the corresponding key 
 * cancellation certificate. It's assumed that the input key ID is a valid ID (i.e. 
 * within 0 - 127).
 * 
 * @param pc_type protected content type
 * @param key_id CSK key ID
 */
static void cancel_key(alt_u32 pc_type, alt_u32 key_id)
{
    // Key ID must be within 0-127 (checked in authentication)
    PFR_ASSERT(key_id <= KCH_MAX_KEY_ID)

    // Get the pointer to key cancellation policy for the given payload type.
    alt_u32* key_cancel_ptr = get_ufm_key_cancel_policy_ptr(pc_type);

    // Move pointer to the word that this key ID is mapped to. (e.g. key ID 5 is in the first word)
    key_cancel_ptr += key_id / 32;
    // Count the bit from the left. (e.g. 1111_1011_1111_... means key ID 5 is cancelled)
    alt_u32 bit_offset = 31 - (key_id % 32);

    // Cancel the key ID
    *key_cancel_ptr &= ~(0b1 << bit_offset);
}

/**
 * @brief Return 1 if @p key_id is a valid ID and the given @p key_id is cancelled in the 
 * key cancellation policy for @p pc_type.
 *
 * @param pc_type protected content type
 * @param key_id The ID of this key
 * @return 1 if signing with this CSK key is valid; 0, otherwise.
 */
static alt_u32 is_csk_key_valid(alt_u32 pc_type, alt_u32 key_id)
{
    if (key_id > KCH_MAX_KEY_ID)
    {
        // This is an invalid key
        return 0;
    }

    // Get the pointer to key cancellation policy for the given payload type.
    alt_u32* key_cancel_ptr = get_ufm_key_cancel_policy_ptr(pc_type);

    // Move pointer to the word that this key ID is mapped to. (e.g. key ID 5 is in the first word)
    key_cancel_ptr += key_id / 32;
    // Count the bit from the left. (e.g. 1111_1011_1111_... means key ID 5 is cancelled)
    alt_u32 bit_offset = 31 - (key_id % 32);

    // Return 0 if this key is cancelled
    return *key_cancel_ptr & (0b1 << bit_offset);
}

#endif /* WHITLEY_INC_KEY_CANCELLATION_H_ */
