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
 * @file ufm_utils.h
 * @brief Utility functions for accessing/updating UFM.
 */

#ifndef WHITLEY_INC_UFM_UTILS_H_
#define WHITLEY_INC_UFM_UTILS_H_

// Always include pfr_sys.h first
#include "pfr_sys.h"

#include "pfm.h"
#include "pfr_pointers.h"
#include "spi_common.h"
#include "ufm.h"


static alt_u32 set_ufm_status(alt_u32 ufm_status_bit_mask)
{
    return get_ufm_pfr_data()->ufm_status &= ~ufm_status_bit_mask;
}

static alt_u32 check_ufm_status(alt_u32 ufm_status_bit_mask)
{
    return (((~get_ufm_pfr_data()->ufm_status) & ufm_status_bit_mask) == ufm_status_bit_mask);
}

/**
 * @brief Return whether the UFM has been provisioned with root key hash and flash offsets.
 *
 * @return 1 if UFM is provisioned; 0, otherwise.
 */
static PFR_ALT_INLINE alt_u32 PFR_ALT_ALWAYS_INLINE is_ufm_provisioned()
{
    return check_ufm_status(UFM_STATUS_PROVISIONED_BIT_MASK);
}

/**
 * @brief Return whether the provisioned data in UFM has been locked.
 *
 * @return 1 if UFM is locked; 0, otherwise.
 */
static PFR_ALT_INLINE alt_u32 PFR_ALT_ALWAYS_INLINE is_ufm_locked()
{
    return check_ufm_status(UFM_STATUS_LOCK_BIT_MASK);
}

/**
 * @brief Return a pointer to the appropriate CSK cancellation policy, given the protected content type.
 */
static alt_u32* get_ufm_key_cancel_policy_ptr(alt_u32 pc_type)
{
    if (pc_type == KCH_PC_PFR_CPLD_UPDATE_CAPSULE)
    {
        return get_ufm_pfr_data()->csk_cancel_cpld_update_cap;
    }
    else if (pc_type == KCH_PC_PFR_PCH_PFM)
    {
        return get_ufm_pfr_data()->csk_cancel_pch_pfm;
    }
    else if (pc_type == KCH_PC_PFR_PCH_UPDATE_CAPSULE)
    {
        return get_ufm_pfr_data()->csk_cancel_pch_update_cap;
    }
    else if (pc_type == KCH_PC_PFR_BMC_PFM)
    {
        return get_ufm_pfr_data()->csk_cancel_bmc_pfm;
    }
    // pc_type == KCH_PC_PFR_BMC_UPDATE_CAPSULE
    return get_ufm_pfr_data()->csk_cancel_bmc_update_cap;
}

/**
 * @brief Return a pointer to the appropriate SVN policy, given the policy type.
 */
static alt_u32* get_ufm_svn_policy_ptr(UFM_SVN_POLICY_TYPE_ENUM svn_policy_type)
{
    if (svn_policy_type == UFM_SVN_POLICY_CPLD)
    {
        return get_ufm_pfr_data()->svn_policy_cpld;
    }
    else if (svn_policy_type == UFM_SVN_POLICY_PCH)
    {
        return get_ufm_pfr_data()->svn_policy_pch;
    }
    // svn_policy_type == UFM_SVN_POLICY_BMC
    return get_ufm_pfr_data()->svn_policy_bmc;
}

/**
 * @brief Translates the SVN in UFM from a bit field to a number
 *
 * The intention of this is to work with write_ufm_svn hide the internal implementation of the svn
 * and to handle all the bit manipulations
 *
 * @return An integer representing the SVN
 */
static alt_u32 get_ufm_svn(UFM_SVN_POLICY_TYPE_ENUM svn_policy_type)
{
    alt_u32* svn_policy = get_ufm_svn_policy_ptr(svn_policy_type);
    for (alt_u32 i = 0; i < 64; i++)
    {
        if ((svn_policy[(i / 32)] & (1 << (i % 32))) != 0)
        {
            return i;
        }
    }
    // a svn of all 0s is defined as 64
    return 64;
}

/**
 * @brief Check if the given SVN is acceptable.
 */
static alt_u32 is_svn_valid(UFM_SVN_POLICY_TYPE_ENUM svn_policy_type, alt_u8 svn)
{
    return (svn >= get_ufm_svn(svn_policy_type)) && (svn <= UFM_MAX_SVN);
}

/**
 * @brief Writes the SVN into UFM with the correct formatting
 *
 * The intention of this is to work with get_ufm_svn hide the internal implementation of the svn
 * and to handle all the bit manipulations
 *
 * This function will only overwrite the SVN if the input is greater or equal to the current SVN in UFM
 *
 * @param svn_val A integer from 0-64 representing the SVN. This will be translated to a bit field and written to UFM
 * @return none
 */
static void write_ufm_svn(alt_u32 svn_val, UFM_SVN_POLICY_TYPE_ENUM svn_policy_type)
{
    alt_u32 ufm_svn_val = get_ufm_svn(svn_policy_type);
    alt_u32* svn_policy_ptr = get_ufm_svn_policy_ptr(svn_policy_type);

    if (svn_val > ufm_svn_val)
    {
        alt_u32 new_svn_policy = ~((1 << (svn_val % 32)) - 1);
        if (svn_val < 32)
        {
            svn_policy_ptr[0] = new_svn_policy;
        }
        else
        {
            svn_policy_ptr[0] = 0;
            if (svn_val < 64)
            {
                svn_policy_ptr[1] = new_svn_policy;
            }
            else if (svn_val == 64)
            {
                svn_policy_ptr[1] = 0;
            }
        }
    }
}

/**
 * @brief Set a flag in UFM that indicates a CPLD recovery update is in progress.
 *
 * Nios writes 0x0 to CPLD update status. It means that a CPLD recovery update is in progress and
 * that after a successful boot, Nios needs to back up the staging capsule into the CPLD recovery region.
 *
 * This update status needs to be in UFM because everything else will be lost after a CPLD update.
 */
static void set_cpld_rc_update_in_progress_ufm_flag()
{
    alt_u32* cpld_update_status = get_ufm_ptr_with_offset(UFM_CPLD_UPDATE_STATUS_OFFSET);
    *cpld_update_status = 0;
}

/**
 * @brief Check if there's a CPLD recovery update in progress.
 *
 * If CPLD update status is set to 0, it means that a CPLD recovery update is in progress and that after
 * a successful boot, Nios needs to back up the staging capsule into the CPLD recovery region.
 *
 * @return alt_u32 1 if there's an CPLD recovery update in progress; 0, otherwise
 */
static alt_u32 is_cpld_rc_update_in_progress()
{
    alt_u32* cpld_update_status = get_ufm_ptr_with_offset(UFM_CPLD_UPDATE_STATUS_OFFSET);
    return *cpld_update_status == 0;
}

#endif /* WHITLEY_INC_UFM_UTILS_H_ */
