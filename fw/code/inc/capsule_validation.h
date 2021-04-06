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
 * @file capsule_validation.h
 * @brief Perform validation on a firmware update capsule.
 */

#ifndef WHITLEY_INC_CAPSULE_VALIDATION_H
#define WHITLEY_INC_CAPSULE_VALIDATION_H

// Always include pfr_sys.h first
#include "pfr_sys.h"

#include "authentication.h"
#include "cpld_update.h"
#include "pbc.h"
#include "pfr_pointers.h"


// Tracking number of failed update attempts from BMC/PCH
static alt_u32 num_failed_update_attempts_from_pch = 0;
static alt_u32 num_failed_update_attempts_from_bmc = 0;

/**
 * @brief Increment the global variable that tracks number of failed FW/CPLD update
 * attempts from the @p spi_flash_type flash.
 *
 * @param spi_flash_type indicate BMC or PCH SPI flash device
 */
static void incr_failed_update_attempts(SPI_FLASH_TYPE_ENUM spi_flash_type)
{
    if (spi_flash_type == SPI_FLASH_PCH)
    {
        num_failed_update_attempts_from_pch++;
    }
    else
    {
        // spi_flash_type == SPI_FLASH_BMC
        num_failed_update_attempts_from_bmc++;
    }
}

/**
 * @brief Reset the global variable that tracks number of failed FW/CPLD update
 * attempts from the @p spi_flash_type flash.
 *
 * @param spi_flash_type indicate BMC or PCH SPI flash device
 */
static void reset_failed_update_attempts(SPI_FLASH_TYPE_ENUM spi_flash_type)
{
    if (spi_flash_type == SPI_FLASH_PCH)
    {
        num_failed_update_attempts_from_pch = 0;
    }
    else
    {
        // spi_flash_type == SPI_FLASH_BMC
        num_failed_update_attempts_from_bmc = 0;
    }
}


/**
 * @brief This function validates a PBC structure.
 * For example, this function checks the tag, version and page size.
 *
 * @param pbc pointer to a PBC structure.
 *
 * @return alt_u32 1 if all checks passed; 0, otherwise.
 */
static alt_u32 is_pbc_valid(PBC_HEADER* pbc)
{
    // Check Magic number
    if (pbc->tag != PBC_EXPECTED_TAG)
    {
        return 0;
    }

    // Check version
    if (pbc->version != PBC_EXPECTED_VERSION)
    {
        return 0;
    }

    // only 4kB pages are valid for SPI NOR flash used in PFR systems.
    if (pbc->page_size != PBC_EXPECTED_PAGE_SIZE)
    {
        return 0;
    }

    // Number of bits in bitmap must be multiple of 8
    // This is because Nios processes bitmap byte by byte.
    if (pbc->bitmap_nbit % 8)
    {
        return 0;
    }

    return 1;
}

/**
 * @brief Perform authentication on a signed capsule.
 * This includes verifying signature of the capsule and
 * signature of the signed PFM inside the capsule.
 *
 * @param signed_capsule start address of a signed capsule
 * @return alt_u32 1 if the signed capsule is authentic; 0, otherwise.
 */
static alt_u32 are_signatures_in_capsule_valid(alt_u32* signed_capsule)
{
    // Verify the signature of the capsule
    if (is_signature_valid((KCH_SIGNATURE*) signed_capsule))
    {
        // Verify the signature of the PFM inside the capsule
        KCH_SIGNATURE *signed_pfm_addr = (KCH_SIGNATURE*) incr_alt_u32_ptr(signed_capsule, SIGNATURE_SIZE);
        return is_signature_valid(signed_pfm_addr);
    }
    return 0;
}

/**
 * @brief Perform validation on a signed capsule.
 * This includes validating expected data in the PBC structure and
 * verifying signatures in the capsule and PFM.
 *
 * @param signed_capsule start address of a signed capsule
 * @return alt_u32 1 if the signed capsule is valid; 0, otherwise.
 */
static alt_u32 is_capsule_valid(alt_u32* signed_capsule)
{
    // Check the two signatures in a firmware update capsule
    if (are_signatures_in_capsule_valid(signed_capsule))
    {
        // Check the compression structure definition
        return is_pbc_valid(get_pbc_ptr_from_signed_capsule(signed_capsule));
    }
    return 0;
}

/**
 * @brief Compare staged firmware against active firmware.
 * Nios concludes that active firwmare and staged firmware are identical, if
 * the hashes of their PFM match exactly.
 *
 * @param spi_flash_type indicate BMC or PCH SPI flash device
 * @return alt_u32 1 if active firmware and staged firmware match exactly; 0, otherwise
 */
static alt_u32 does_staged_fw_image_match_active_fw_image(SPI_FLASH_TYPE_ENUM spi_flash_type)
{
    KCH_BLOCK0* active_pfm_sig_b0 = (KCH_BLOCK0*) get_spi_active_pfm_ptr(spi_flash_type);
    KCH_BLOCK0* capsule_pfm_sig_b0 = (KCH_BLOCK0*) get_spi_flash_ptr_with_offset(get_staging_region_offset(spi_flash_type) + SIGNATURE_SIZE);

    // If the hashes of PFM match, the active image and staging image must be the same firmware.
    alt_u32* active_pfm_hash = active_pfm_sig_b0->pc_hash256;
    alt_u32* capsule_pfm_hash = capsule_pfm_sig_b0->pc_hash256;
    for (alt_u32 word_i = 0; word_i < PFR_CRYPTO_LENGTH / 4; word_i++)
    {
        if (active_pfm_hash[word_i] != capsule_pfm_hash[word_i])
        {
            // There's a hash mismatch. Image in capsule is different from active image.
            return 0;
        }
    }

    return 1;
}

/**
 * @brief Return non-zero value if the pc type in the given signature matches the update intent.
 *
 * If this capsule is a signed key cancellation certificate, then no need to check update intent. User
 * can use any update intent to send in key cancellation certificate.
 *
 * Otherwise, if the update intent is BMC active/recovery firwmare update, then the PC type must be BMC
 * update capsule.
 * If the update intent is PCH active/recovery firwmare update, then the PC type must be PCH
 * update capsule.
 * If the update intent is CPLD active/recovery firwmare update, then the PC type must be CPLD
 * update capsule.
 *
 * @return alt_u32 1 if pc type matches the update intent; 0, otherwise.
 */
static alt_u32 does_pc_type_match_update_intent(KCH_BLOCK0* sig_b0, alt_u32 update_intent)
{
    // Allow user to issue key cancellation with any update intent
    if ((sig_b0->pc_type & KCH_PC_TYPE_KEY_CAN_CERT_MASK))
    {
        return 1;
    }

    if (update_intent & MB_UPDATE_INTENT_BMC_FW_UPDATE_MASK)
    {
        // If the update intent is BMC firmware update, the capsule must be for BMC firmware
        return sig_b0->pc_type == KCH_PC_PFR_BMC_UPDATE_CAPSULE;
    }
    else if (update_intent & MB_UPDATE_INTENT_PCH_FW_UPDATE_MASK)
    {
        // If the update intent is PCH firmware update, the capsule must be for PCH firmware
        return sig_b0->pc_type == KCH_PC_PFR_PCH_UPDATE_CAPSULE;
    }
    else if (update_intent & MB_UPDATE_INTENT_CPLD_MASK)
    {
        // If the update intent is CPLD update, this must be a CPLD capsule or decommission capsule
        return (sig_b0->pc_type == KCH_PC_PFR_CPLD_UPDATE_CAPSULE) ||
                (sig_b0->pc_type & KCH_PC_TYPE_DECOMM_CAP_MASK);
    }
    return 0;
}

/**
 * @brief Pre-process the update capsule prior to performing the FW or CPLD update.
 *
 * Nios ensures the PC type of the capsule matches the update intent first, before authentication.
 * Attacker may put in valid capsule at the unexpected and cause Nios problem doing authentication. For
 * example, a BMC capsule signature may be put in at CPLD staging area in BMC SPI flash. CPLD staging
 * area is near the end of BMC SPI flash. Hashing the protected content would require CPLD to read beyond
 * the SPI AvMM memory space.
 *
 * After that, Nios authenticate the capsule signature. If that passes, Nios moves on to validate its content.
 * If this capsule is a key cancellation certificate, Nios cancels the key.
 * If this capsule is a decommission capsule, Nios erases UFM and then reconfig into CFM1 (Active Image).
 * If this capsule is a CPLD update capsule, this function returns 1 if its SVN is valid.
 * If this capsule is a firmware update capsule, Nios proceeds to check the signature of PFM. Once that passes,
 * SVN is then checked. The SVN checks here are more involved. The capsule PFM SVN is validated against the
 * SVN policy stored in UFM. Also, if this is an active firmware update, its SVN must be the same as the SVN
 * of recovery image. If user wishes to bump the SVN, a recovery firmware update, which update both active and
 * recovery firmware, must be triggered.
 *
 * If any check fails, Nios logs a major/minor error and increments number of failed update attempts counter.
 *
 * @param spi_flash_type indicate BMC or PCH SPI flash device
 * @param signed_capsule pointer to the start address of a signed capsule
 * @param update_intent The update intent value that triggered this update
 *
 * @return alt_u32 1 if Nios should proceed to perform the CPLD or FW update with the @p signed_capsule; 0, otherwise.
 *
 * @see act_on_update_intent
 * @see is_svn_valid
 * @see does_pc_type_match_update_intent
 * @see is_signature_valid
 */
static alt_u32 check_capsule_before_update(
        SPI_FLASH_TYPE_ENUM spi_flash_type, alt_u32* signed_capsule, alt_u32 update_intent)
{
    // This minor error code will be posted to the Mailbox, when any check failed
    alt_u32 update_event_minor_error = MINOR_ERROR_AUTHENTICATION_FAILED;

    // The PC type of the capsule must match the update intent
    KCH_BLOCK0* b0 = (KCH_BLOCK0*) signed_capsule;
    if (does_pc_type_match_update_intent(b0, update_intent))
    {
        // Validate Capsule signature
        if (is_signature_valid((KCH_SIGNATURE*) signed_capsule))
        {
            alt_u32* capsule_pc = incr_alt_u32_ptr(signed_capsule, SIGNATURE_SIZE);

            if (b0->pc_type & KCH_PC_TYPE_KEY_CAN_CERT_MASK)
            {
                // If this is a key cancellation certificate, proceed to cancel this key.
                cancel_key(get_kch_pc_type(b0), ((KCH_CAN_CERT*) capsule_pc)->csk_id);

                // There's no update or error logging to do.
                // Exit with 0, so Nios won't proceed to do update after calling this function.
                return 0;
            }
            else if (b0->pc_type & KCH_PC_TYPE_DECOMM_CAP_MASK)
            {
                // This is a valid decommission capsule; proceed.
                ufm_erase_page(UFM_PFR_DATA_OFFSET);
                perform_cfm_switch(CPLD_CFM1);
            }
            else if (b0->pc_type == KCH_PC_PFR_CPLD_UPDATE_CAPSULE)
            {
                // This is a CPLD update
                if (is_svn_valid(UFM_SVN_POLICY_CPLD, ((CPLD_UPDATE_PC*) capsule_pc)->svn))
                {
                    // Yes, this is a valid CPLD update capsule. Proceed with this CPLD update
                    return 1;
                }

                // Failed SVN check
                update_event_minor_error = MINOR_ERROR_INVALID_SVN;
            }
            else
            {
                // This is a firmware update

                // Validate PFM signature and fields in compression structure header
                if (is_signature_valid((KCH_SIGNATURE*) capsule_pc)
                        && is_pbc_valid(get_pbc_ptr_from_signed_capsule(signed_capsule)))
                {
                    // Validate SVN
                    alt_u8 new_svn = get_capsule_pfm(signed_capsule)->svn;

                    // Check whether this SVN is allowed by looking at the UFM SVN policy
                    if ((b0->pc_type == KCH_PC_PFR_PCH_UPDATE_CAPSULE) && is_svn_valid(UFM_SVN_POLICY_PCH, new_svn))
                    {
                        if ((update_intent & MB_UPDATE_INTENT_PCH_RECOVERY_MASK) || (read_from_mailbox(MB_PCH_PFM_RECOVERY_SVN) == new_svn))
                        {
                            // Yes, this is a valid firmware update capsule. Proceed with the update.
                            return 1;
                        }
                    }
                    else if ((b0->pc_type == KCH_PC_PFR_BMC_UPDATE_CAPSULE) && is_svn_valid(UFM_SVN_POLICY_BMC, new_svn))
                    {
                        if ((update_intent & MB_UPDATE_INTENT_BMC_RECOVERY_MASK) || (read_from_mailbox(MB_BMC_PFM_RECOVERY_SVN) == new_svn))
                        {
                            // Yes, this is a valid firmware update capsule. Proceed with the update.
                            return 1;
                        }
                    }

                    update_event_minor_error = MINOR_ERROR_INVALID_SVN;
                }
            }
        }
    }

    // If Nios reaches this point, it means that this incoming update capsule is rejected.
    // Update number of failed attempts and log the error
    incr_failed_update_attempts(spi_flash_type);
    log_update_failure(spi_flash_type, update_event_minor_error);

    return 0;
}

/**
 * @brief Authenticate staging capsule and also ensure it matches the active image,
 * before firmware recovery update.
 *
 * For the second check, Nios compares the hashes of PFM in staged capsule and active image. If
 * they match, Nios says staged image matches active image and returns 1.
 *
 * @param spi_flash_type indicate BMC or PCH SPI flash device
 * @param skip_hash_check allow user to skip the second check
 *
 * @return alt_u32 1 if staging capsule passed the two checks; 0, otherwise.
 */
static alt_u32 check_capsule_before_fw_recovery_update(SPI_FLASH_TYPE_ENUM spi_flash_type, alt_u32 skip_hash_check)
{
    switch_spi_flash(spi_flash_type);

    alt_u32* staging_capsule = get_spi_staging_region_ptr(spi_flash_type);
    if (is_signature_valid((KCH_SIGNATURE*) staging_capsule))
    {
        // Nios can trust this staging capsule now.
        // Next, check if the staged image is identical to the active image.
        if (skip_hash_check || does_staged_fw_image_match_active_fw_image(spi_flash_type))
        {
            return 1;
        }
    }
    return 0;
}

#endif /* WHITLEY_INC_CAPSULE_VALIDATION_H */
