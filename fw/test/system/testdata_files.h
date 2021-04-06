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

#ifndef INC_TESTDATA_FILES_H
#define INC_TESTDATA_FILES_H

/************************************************
 *
 * This header describes all the unittests binaries files under fw/test/testdata.
 * All BMC and PCH flash images are generated with gen_pfr_flash_images/... scripts.
 *
 ************************************************/

/*
 * PFR Images for SPI flash
 */
#define FULL_PFR_IMAGE_BMC_FILE "testdata/full_pfr_image_bmc.bin"
#define FULL_PFR_IMAGE_BMC_FILE_SIZE 134217728
// Steps
// sha256sum testdata/full_pfr_image_bmc.bin
// echo "<insert hash here>" | xxd -r -p | xxd -i
const alt_u8 full_pfr_image_bmc_file_sha256sum[32] = {
    0xef, 0xe0, 0xa8, 0xba, 0xfa, 0x4e, 0xa4, 0x2b, 0xce, 0x05, 0x36, 0x21, 
    0xad, 0x65, 0xf9, 0x5b, 0x8d, 0x30, 0xdf, 0xe5, 0x90, 0xbf, 0xc9, 0x91, 
    0x66, 0x0a, 0x94, 0x9c, 0xd2, 0x41, 0xc6, 0x8a    
};

#define FULL_PFR_IMAGE_PCH_FILE "testdata/full_pfr_image_pch.bin"
#define FULL_PFR_IMAGE_PCH_FILE_SIZE 67108864
const alt_u8 full_pfr_image_pch_file_sha256sum[32] = {
    0x86, 0x0c, 0x27, 0x74, 0xca, 0x9f, 0xc0, 0x51, 0x20, 0xea, 0xc8, 0x2c,
    0xc8, 0x15, 0x9b, 0xd2, 0x15, 0xc6, 0x3e, 0xfc, 0xaa, 0x6f, 0xb3, 0x17,
    0xca, 0x3d, 0x05, 0x5e, 0x85, 0xc4, 0xa7, 0x4f
};

#define GENERATED_FULL_PFR_IMAGE_BMC_WITH_STAGING_FILE \
    "testdata/generated_full_pfr_image_bmc_with_staging.bin"
#define GENERATED_FULL_PFR_IMAGE_BMC_WITH_STAGING_FILE_SIZE 134217728
/*
 * Signed firmware update capsule
 */
#define SIGNED_CAPSULE_BMC_FILE "testdata/signed_capsule_bmc.bin"
#define SIGNED_CAPSULE_BMC_FILE_SIZE 21965312

#define SIGNED_CAPSULE_PCH_FILE "testdata/signed_capsule_pch.bin"
#define SIGNED_CAPSULE_PCH_FILE_SIZE 12564992

/*
 * Signed PFM
 */
#define SIGNED_PFM_BMC_FILE "testdata/signed_pfm_bmc.bin"
#define SIGNED_PFM_BMC_FILE_SIZE 1408

#define SIGNED_PFM_BIOS_FILE "testdata/signed_pfm_pch.bin"
#define SIGNED_PFM_BIOS_FILE_SIZE 1408

/*
 * CPLD image & capsule
 */
#define CFM1_ACTIVE_IMAGE_FILE "testdata/cpld_cfm1_active_image.bin"
#define CFM1_ACTIVE_IMAGE_FILE_SIZE 270336

#define CFM0_RECOVERY_IMAGE_FILE "testdata/cpld_cfm0_recovery_image.bin"
#define CFM0_RECOVERY_IMAGE_FILE_SIZE 270336

#define SIGNED_CAPSULE_CPLD_FILE "testdata/signed_capsule_cpld.bin"
#define SIGNED_CAPSULE_CPLD_FILE_SIZE 271488

/*
 * Signed random binary
 */
#define SIGNED_BINARY_BLOCKSIGN_FILE "testdata/signed_binary_blocksign.bin"
#define SIGNED_BINARY_BLOCKSIGN_FILE_SIZE 1152

/*
 * UFM data for provisioning
 */
#define UFM_PFR_DATA_EXAMPLE_KEY_FILE "testdata/ufm_data_example_key.hex"
#define UFM_PFR_DATA_EXAMPLE_KEY_FILE_SIZE 236

/*
 * Key cancellation certificate
 */
#define KEY_CAN_CERT_FILE_SIZE 1152

#define KEY_CAN_CERT_PCH_PFM_KEY2 "testdata/key_cancellation_certs/signed_pch_pfm_key2_can_cert.bin"
#define KEY_CAN_CERT_PCH_PFM_KEY255 "testdata/key_cancellation_certs/signed_pch_pfm_key255_can_cert.bin"

#define KEY_CAN_CERT_MODIFIED_RESERVED_AREA "testdata/key_cancellation_certs/signed_key_can_cert_modified_reserved_area.bin"
#define KEY_CAN_CERT_BAD_PCH_LEGNTH "testdata/key_cancellation_certs/signed_key_can_cert_bad_pc_length.bin"
#define KEY_CAN_CERT_BAD_PCH_LEGNTH_FILE_SIZE 1280

#define KEY_CAN_CERT_PCH_CAPSULE_KEY2 "testdata/key_cancellation_certs/signed_pch_capsule_key2_can_cert.bin"
#define KEY_CAN_CERT_PCH_CAPSULE_KEY10 "testdata/key_cancellation_certs/signed_pch_capsule_key10_can_cert.bin"

#define KEY_CAN_CERT_CPLD_CAPSULE_KEY2 "testdata/key_cancellation_certs/signed_cpld_capsule_key2_can_cert.bin"
#define KEY_CAN_CERT_CPLD_CAPSULE_KEY10 "testdata/key_cancellation_certs/signed_cpld_capsule_key10_can_cert.bin"

#define KEY_CAN_CERT_BMC_CAPSULE_KEY1 "testdata/key_cancellation_certs/signed_bmc_capsule_key1_can_cert.bin"
#define KEY_CAN_CERT_BMC_CAPSULE_KEY10 "testdata/key_cancellation_certs/signed_bmc_capsule_key10_can_cert.bin"

#define KEY_CAN_CERT_BMC_PFM_KEY2 "testdata/key_cancellation_certs/signed_bmc_pfm_key2_can_cert.bin"
#define KEY_CAN_CERT_BMC_PFM_KEY10 "testdata/key_cancellation_certs/signed_bmc_pfm_key10_can_cert.bin"

/*
 * Generated PFM
 */
#define GEN_256B_PFM_FILE "testdata/gen_256B_pfm.dat"
#define GEN_256B_PFM_FILE_SIZE 256

/*
 * BMC firmware update flow
 */
#define FULL_PFR_IMAGE_BMC_V14_FILE "testdata/bmc_firmware_update/full_pfr_image_bmc.bin"
#define FULL_PFR_IMAGE_BMC_V14_FILE_SIZE 134217728

#define SIGNED_CAPSULE_BMC_V14_FILE "testdata/bmc_firmware_update/signed_capsule_bmc.bin"
#define SIGNED_CAPSULE_BMC_V14_FILE_SIZE 17582592

/*
 * PCH firmware update flow
 * Generated file with Major version 0x3 and Minor version 0xC
 */
#define FULL_PFR_IMAGE_PCH_V03P12_FILE "testdata/pch_firmware_update/full_pfr_image_pch.bin"
#define FULL_PFR_IMAGE_PCH_V03P12_FILE_SIZE 67108864

#define SIGNED_CAPSULE_PCH_V03P12_FILE "testdata/pch_firmware_update/signed_capsule_pch.bin"
#define SIGNED_CAPSULE_PCH_V03P12_FILE_SIZE 15084032

/*
 * CPLD firmware update flow
 */
#define SIGNED_CAPSULE_CPLD_B393_6_FILE "testdata/cpld_update/signed_capsule_cpld_b393_6.bin"
#define SIGNED_CAPSULE_CPLD_B393_6_FILE_SIZE SIGNED_CAPSULE_CPLD_FILE_SIZE

/*
 * Anti Rollback Tests
 */
#define SIGNED_CAPSULE_PCH_WITH_CSK_ID10_FILE "testdata/anti_rollback_with_csk_id/signed_capsule_pch_with_csk_id10.bin"
#define SIGNED_CAPSULE_PCH_WITH_CSK_ID10_FILE_SIZE 12564992

/*
 * PCH firmware update capsule with different SVNs
 */
#define SIGNED_CAPSULE_PCH_PFM_WITH_SVN255_FILE "testdata/bad_firmware_capsule/signed_capsule_pch_pfm_with_svn_0xFF.bin"
#define SIGNED_CAPSULE_PCH_PFM_WITH_SVN255_FILE_SIZE 12564992

#define SIGNED_CAPSULE_PCH_PFM_WITH_SVN65_FILE "testdata/bad_firmware_capsule/signed_capsule_pch_pfm_with_svn65.bin"
#define SIGNED_CAPSULE_PCH_PFM_WITH_SVN65_FILE_SIZE 12564992

#define SIGNED_CAPSULE_PCH_PFM_WITH_SVN64_FILE "testdata/anti_rollback_with_svn/signed_capsule_pch_pfm_with_svn64.bin"
#define SIGNED_CAPSULE_PCH_PFM_WITH_SVN64_FILE_SIZE 12564992

#define SIGNED_CAPSULE_PCH_PFM_WITH_SVN9_FILE "testdata/anti_rollback_with_svn/signed_capsule_pch_pfm_with_svn9.bin"
#define SIGNED_CAPSULE_PCH_PFM_WITH_SVN9_FILE_SIZE 12564992

#define SIGNED_CAPSULE_PCH_PFM_WITH_SVN7_FILE "testdata/anti_rollback_with_svn/signed_capsule_pch_pfm_with_svn7.bin"
#define SIGNED_CAPSULE_PCH_PFM_WITH_SVN7_FILE_SIZE 12564992

/*
 * PCH firmware update capsules with invalid configurations
 */
#define SIGNED_CAPSULE_PCH_WITH_BAD_BITMAP_NBIT_IN_PBC_HEADER_FILE "testdata/bad_firmware_capsule/signed_capsule_pch_with_bad_bitmap_nbit_in_pbc_header.bin"
#define SIGNED_CAPSULE_PCH_WITH_BAD_BITMAP_NBIT_IN_PBC_HEADER_FILE_SIZE 12564992

#define SIGNED_CAPSULE_PCH_WITH_BAD_PAGE_SIZE_IN_PBC_HEADER_FILE "testdata/bad_firmware_capsule/signed_capsule_pch_with_bad_page_size_in_pbc_header.bin"
#define SIGNED_CAPSULE_PCH_WITH_BAD_PAGE_SIZE_IN_PBC_HEADER_FILE_SIZE 12564992

#define SIGNED_CAPSULE_PCH_WITH_BAD_PATT_IN_PBC_HEADER_FILE "testdata/bad_firmware_capsule/signed_capsule_pch_with_bad_pattern_in_pbc_header.bin"
#define SIGNED_CAPSULE_PCH_WITH_BAD_PATT_IN_PBC_HEADER_FILE_SIZE 12564992

#define SIGNED_CAPSULE_PCH_WITH_BAD_TAG_IN_PBC_HEADER_FILE "testdata/bad_firmware_capsule/signed_capsule_pch_with_bad_tag_in_pbc_header.bin"
#define SIGNED_CAPSULE_PCH_WITH_BAD_TAG_IN_PBC_HEADER_FILE_SIZE 12564992

#define SIGNED_CAPSULE_PCH_WITH_BAD_VERSION_IN_PBC_HEADER_FILE "testdata/bad_firmware_capsule/signed_capsule_pch_with_bad_version_in_pbc_header.bin"
#define SIGNED_CAPSULE_PCH_WITH_BAD_VERSION_IN_PBC_HEADER_FILE_SIZE 12564992

#define SIGNED_CAPSULE_PCH_WITH_PC_TYPE_10_FILE "testdata/bad_firmware_capsule/signed_capsule_pch_with_pc_type_10.bin"
#define SIGNED_CAPSULE_PCH_WITH_PC_TYPE_10_FILE_SIZE 12564992

#define SIGNED_CAPSULE_PCH_WITH_WRONG_CSK_PERMISSION_FILE "testdata/bad_firmware_capsule/signed_capsule_pch_with_wrong_csk_permission.bin"
#define SIGNED_CAPSULE_PCH_WITH_WRONG_CSK_PERMISSION_FILE_SIZE 12564992

/*
 * BMC firmware update capsule with different SVNs
 */
#define SIGNED_CAPSULE_BMC_WITH_SVN2_FILE "testdata/anti_rollback_with_svn/signed_capsule_bmc_with_svn2.bin"
#define SIGNED_CAPSULE_BMC_WITH_SVN2_FILE_SIZE 21965312

/*
 * CPLD update capsule with different SVNs
 */
#define SIGNED_CAPSULE_CPLD_WITH_SVN1_FILE "testdata/anti_rollback_with_svn/signed_capsule_cpld_with_svn1.bin"
#define SIGNED_CAPSULE_CPLD_WITH_SVN1_FILE_SIZE 271488

#define SIGNED_CAPSULE_CPLD_WITH_SVN64_FILE "testdata/anti_rollback_with_svn/signed_capsule_cpld_with_svn64.bin"
#define SIGNED_CAPSULE_CPLD_WITH_SVN64_FILE_SIZE 271488

#define SIGNED_CAPSULE_CPLD_WITH_SVN65_FILE "testdata/anti_rollback_with_svn/signed_capsule_cpld_with_svn65.bin"
#define SIGNED_CAPSULE_CPLD_WITH_SVN65_FILE_SIZE 271488

/*
 * PCH firmware recovery
 * This generated image is based on the IFWI image taken from HSD case 1507855229.
 * In this image, the BIOS regions (0x37F0000 - 0x3800000; 0x3850000 - 0x3880000; 0x3880000 - 0x3900000)
 * are configured to only be recovered on second recovery.
 */
#define FULL_PFR_IMAGE_PCH_ONLY_RECOVER_BIOS_REGIONS_ON_2ND_LEVEL_FILE "testdata/pch_firmware_recovery/full_pfr_image_pch_only_recover_bios_regions_on_2nd_level.bin"
#define FULL_PFR_IMAGE_PCH_ONLY_RECOVER_BIOS_REGIONS_ON_2ND_LEVEL_FILE_SIZE 67108864

/*
 * Decommission capsule
 */
#define SIGNED_DECOMMISSION_CAPSULE_FILE "testdata/signed_decommission_capsule.bin"
#define SIGNED_DECOMMISSION_CAPSULE_FILE_SIZE 1152

#define SIGNED_DECOMMISSION_CAPSULE_WITH_CSK_ID3_FILE "testdata/anti_rollback_with_csk_id/signed_decomm_capsule_with_csk_id3.bin"
#define SIGNED_DECOMMISSION_CAPSULE_WITH_CSK_ID3_FILE_SIZE SIGNED_DECOMMISSION_CAPSULE_FILE_SIZE

#define SIGNED_DECOMMISSION_CAPSULE_WITH_INVALID_PC_FILE "testdata/bad_firmware_capsule/signed_decomm_capsule_with_invalid_pc.bin"
#define SIGNED_DECOMMISSION_CAPSULE_WITH_INVALID_PC_FILE_SIZE 1280

#endif /* INC_TESTDATA_FILES_H_ */
