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
#include <iostream>

// Include the GTest headers
#include "gtest_headers.h"

// Include the SYSTEM MOCK and PFR headers
#include "ut_nios_wrapper.h"
#include "testdata_info.h"


/**
 * @brief The purpose of this suite of tests is to ensure the content of testdata binaries matches
 * the expectation of all unittests. If any test in this test suite fails, you will see various types
 * of unittest failures because their assumptions are no longer valid.
 */
class TestDataSanityTest : public testing::Test
{
public:
    alt_u32* m_flash_x86_ptr = nullptr;

    // For simplicity, use PCH flash for all tests.
    SPI_FLASH_TYPE_ENUM m_spi_flash_in_use = SPI_FLASH_PCH;

    virtual void SetUp()
    {
        SYSTEM_MOCK* sys = SYSTEM_MOCK::get();
        sys->reset();

        // Prepare SPI flash
        sys->reset_spi_flash(m_spi_flash_in_use);
        m_flash_x86_ptr = sys->get_x86_ptr_to_spi_flash(m_spi_flash_in_use);
        switch_spi_flash(m_spi_flash_in_use);

        // Perform provisioning
        SYSTEM_MOCK::get()->provision_ufm_data(UFM_PFR_DATA_EXAMPLE_KEY_FILE);
    }

    virtual void TearDown() {}
};

/*
 * This test creates a BMC PFR image with BMC/PCH/CPLD update capsule.
 */
TEST_F(TestDataSanityTest, test_create_image_with_staging_capsules)
{
    // Load the full image
    SYSTEM_MOCK::get()->load_to_flash(m_spi_flash_in_use, FULL_PFR_IMAGE_BMC_FILE, FULL_PFR_IMAGE_BMC_FILE_SIZE);

    alt_u32 bmc_staging_offset = get_ufm_pfr_data()->bmc_staging_region;
    // Load BMC FW update capsule
    SYSTEM_MOCK::get()->load_to_flash(
            m_spi_flash_in_use,
            SIGNED_CAPSULE_BMC_FILE,
            SIGNED_CAPSULE_BMC_FILE_SIZE,
            bmc_staging_offset + BMC_STAGING_REGION_BMC_UPDATE_CAPSULE_OFFSET);

    // Load PCH FW update capsule
    SYSTEM_MOCK::get()->load_to_flash(
            m_spi_flash_in_use,
            SIGNED_CAPSULE_PCH_FILE,
            SIGNED_CAPSULE_PCH_FILE_SIZE,
            bmc_staging_offset + BMC_STAGING_REGION_PCH_UPDATE_CAPSULE_OFFSET);

    // Load CPLD update capsule
    SYSTEM_MOCK::get()->load_to_flash(
            m_spi_flash_in_use,
            SIGNED_CAPSULE_CPLD_FILE,
            SIGNED_CAPSULE_CPLD_FILE_SIZE,
            bmc_staging_offset + BMC_STAGING_REGION_CPLD_UPDATE_CAPSULE_OFFSET);

    // Some quick authentication checks on the capsules.
    alt_u32* recovery_region_ptr = get_spi_flash_ptr_with_offset(get_ufm_pfr_data()->bmc_recovery_region);
    alt_u32* staging_region_bmc_capsule_ptr = get_spi_flash_ptr_with_offset(bmc_staging_offset + BMC_STAGING_REGION_BMC_UPDATE_CAPSULE_OFFSET);
    alt_u32* staging_region_pch_capsule_ptr = get_spi_flash_ptr_with_offset(bmc_staging_offset + BMC_STAGING_REGION_PCH_UPDATE_CAPSULE_OFFSET);
    alt_u32* staging_region_cpld_capsule_ptr = get_spi_flash_ptr_with_offset(bmc_staging_offset + BMC_STAGING_REGION_CPLD_UPDATE_CAPSULE_OFFSET);

    EXPECT_TRUE(is_signature_valid((KCH_SIGNATURE*) recovery_region_ptr));
    EXPECT_TRUE(is_signature_valid((KCH_SIGNATURE*) staging_region_bmc_capsule_ptr));
    EXPECT_TRUE(is_signature_valid((KCH_SIGNATURE*) staging_region_pch_capsule_ptr));
    EXPECT_TRUE(is_signature_valid((KCH_SIGNATURE*) staging_region_cpld_capsule_ptr));

    // Create the binary file.
    // Uncomment this line when needed
    //    SYSTEM_MOCK::get()->write_x86_mem_to_file(
    //            GENERATED_FULL_PFR_IMAGE_BMC_WITH_STAGING_FILE,
    //            (alt_u8*) m_flash_x86_ptr,
    //            FULL_PFR_IMAGE_BMC_FILE_SIZE);
}

/*
 * This test ensures that the update capsule we stored matches the
 * update capsule in the recovery region of the full image we stored.
 *
 * If the size of the capsule changed, this test also fails out.
 */
TEST_F(TestDataSanityTest, test_capsule_and_full_image_consistency_bmc)
{
    SYSTEM_MOCK::get()->load_to_flash(m_spi_flash_in_use, FULL_PFR_IMAGE_BMC_FILE, FULL_PFR_IMAGE_BMC_FILE_SIZE);
    alt_u32* recovery_region_ptr = get_spi_recovery_region_ptr(SPI_FLASH_BMC);

    alt_u32 *signed_capsule = new alt_u32[SIGNED_CAPSULE_BMC_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(SIGNED_CAPSULE_BMC_FILE, signed_capsule);

    for (alt_u32 word_i = 0; word_i < SIGNED_CAPSULE_BMC_FILE_SIZE/4; word_i++)
    {
        ASSERT_EQ(signed_capsule[word_i], recovery_region_ptr[word_i]);
    }

    delete[] signed_capsule;
}

/*
 * This test ensures that the update capsule we stored matches the
 * update capsule in the recovery region of the full image we stored.
 *
 * If the size of the capsule changed, this test also fails out.
 */
TEST_F(TestDataSanityTest, test_capsule_and_full_image_consistency_pch)
{
    SYSTEM_MOCK::get()->load_to_flash(m_spi_flash_in_use, FULL_PFR_IMAGE_PCH_FILE, FULL_PFR_IMAGE_PCH_FILE_SIZE);
    alt_u32* recovery_region_ptr = get_spi_recovery_region_ptr(SPI_FLASH_PCH);

    alt_u32 *signed_capsule = new alt_u32[SIGNED_CAPSULE_PCH_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(SIGNED_CAPSULE_PCH_FILE, signed_capsule);

    for (alt_u32 word_i = 0; word_i < SIGNED_CAPSULE_PCH_FILE_SIZE/4; word_i++)
    {
        ASSERT_EQ(signed_capsule[word_i], recovery_region_ptr[word_i]);
    }

    delete[] signed_capsule;
}

TEST_F(TestDataSanityTest, test_pfm_svn_and_version)
{
    // Prepare the flashes
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, FULL_PFR_IMAGE_BMC_FILE, FULL_PFR_IMAGE_BMC_FILE_SIZE);
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_PCH, FULL_PFR_IMAGE_PCH_FILE, FULL_PFR_IMAGE_PCH_FILE_SIZE);

    // Read the PFMs from BMC image
    switch_spi_flash(SPI_FLASH_BMC);
    PFM* pfm = get_active_pfm(SPI_FLASH_BMC);
    EXPECT_EQ(pfm->svn, alt_u8(BMC_ACTIVE_PFM_SVN));
    EXPECT_EQ(pfm->major_rev, alt_u8(BMC_ACTIVE_PFM_MAJOR_VER));
    EXPECT_EQ(pfm->minor_rev, alt_u8(BMC_ACTIVE_PFM_MINOR_VER));

    alt_u32* recovery_region_ptr = get_spi_recovery_region_ptr(SPI_FLASH_BMC);
    pfm = get_capsule_pfm(recovery_region_ptr);
    EXPECT_EQ(pfm->svn, alt_u8(BMC_RECOVERY_PFM_SVN));
    EXPECT_EQ(pfm->major_rev, alt_u8(BMC_RECOVERY_PFM_MAJOR_VER));
    EXPECT_EQ(pfm->minor_rev, alt_u8(BMC_RECOVERY_PFM_MINOR_VER));

    // Read the PFMs from PCH image
    switch_spi_flash(SPI_FLASH_PCH);
    pfm = get_active_pfm(SPI_FLASH_PCH);
    EXPECT_EQ(pfm->svn, alt_u8(PCH_ACTIVE_PFM_SVN));
    EXPECT_EQ(pfm->major_rev, alt_u8(PCH_ACTIVE_PFM_MAJOR_VER));
    EXPECT_EQ(pfm->minor_rev, alt_u8(PCH_ACTIVE_PFM_MINOR_VER));

    recovery_region_ptr = get_spi_recovery_region_ptr(SPI_FLASH_PCH);
    pfm = get_capsule_pfm(recovery_region_ptr);
    EXPECT_EQ(pfm->svn, alt_u8(PCH_RECOVERY_PFM_SVN));
    EXPECT_EQ(pfm->major_rev, alt_u8(PCH_RECOVERY_PFM_MAJOR_VER));
    EXPECT_EQ(pfm->minor_rev, alt_u8(PCH_RECOVERY_PFM_MINOR_VER));
}

TEST_F(TestDataSanityTest, test_sanity_on_pch_recovery_capsule)
{
    // Check the recovery region location
    EXPECT_EQ(get_ufm_pfr_data()->pch_recovery_region, alt_u32(PCH_RECOVERY_REGION_ADDR));

    alt_u32 *full_image = new alt_u32[FULL_PFR_IMAGE_PCH_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(FULL_PFR_IMAGE_PCH_FILE, full_image);

    // Make sure all the SPI regions match the macros exactly
    // This check also ensure the SPI regions are in ascending order, in terms of address.
    alt_u32* signed_capsule = incr_alt_u32_ptr(full_image, PCH_RECOVERY_REGION_ADDR);
    PFM* pfm = get_capsule_pfm(signed_capsule);
    alt_u32* pfm_body_ptr = pfm->pfm_body;

    alt_u32 spi_region_counter = 0;
    alt_u32 static_spi_region_counter = 0;

    // Go through the PFM Body
    while (1)
    {
        alt_u8* tag = (alt_u8*) pfm_body_ptr;

        if (*tag == SMBUS_RULE_DEF_TYPE)
        {
            // Skip SMBus rule definition
            pfm_body_ptr = incr_alt_u32_ptr(pfm_body_ptr, SMBUS_RULE_DEF_SIZE);
        }
        else if (*tag == SPI_REGION_DEF_TYPE)
        {
            PFM_SPI_REGION_DEF* region_def = (PFM_SPI_REGION_DEF*) pfm_body_ptr;

            EXPECT_EQ(region_def->start_addr, testdata_pch_spi_regions_start_addr[spi_region_counter]);
            EXPECT_EQ(region_def->end_addr, testdata_pch_spi_regions_end_addr[spi_region_counter]);

            if (is_spi_region_static(region_def))
            {
                EXPECT_EQ(region_def->start_addr, testdata_pch_static_regions_start_addr[static_spi_region_counter]);
                EXPECT_EQ(region_def->end_addr, testdata_pch_static_regions_end_addr[static_spi_region_counter]);
                static_spi_region_counter++;
            }

            pfm_body_ptr = get_end_of_spi_region_def(region_def);
            spi_region_counter++;
        }
        else
        {
            // Exit if there is no more SMBus rule definition in PFM body.
            break;
        }
    }

    EXPECT_TRUE(spi_region_counter == PCH_NUM_SPI_REGIONS);
    EXPECT_TRUE(static_spi_region_counter == PCH_NUM_STATIC_REGIONS);

    delete[] full_image;
}

TEST_F(TestDataSanityTest, test_sanity_on_bmc_recovery_capsule)
{
    // Check the recovery region location
    EXPECT_EQ(get_ufm_pfr_data()->bmc_recovery_region, alt_u32(BMC_RECOVERY_REGION_ADDR));

    alt_u32 *full_image = new alt_u32[FULL_PFR_IMAGE_BMC_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(FULL_PFR_IMAGE_BMC_FILE, full_image);

    // Make sure all the SPI regions match the macros exactly
    // This check also ensure the SPI regions are in ascending order, in terms of address.
    alt_u32* signed_capsule = incr_alt_u32_ptr(full_image, BMC_RECOVERY_REGION_ADDR);
    PFM* pfm = get_capsule_pfm(signed_capsule);
    alt_u32* pfm_body_ptr = pfm->pfm_body;

    alt_u32 spi_region_counter = 0;
    alt_u32 static_spi_region_counter = 0;

    // Go through the PFM Body
    while (1)
    {
        alt_u8* tag = (alt_u8*) pfm_body_ptr;

        if (*tag == SMBUS_RULE_DEF_TYPE)
        {
            // Skip SMBus rule definition
            pfm_body_ptr = incr_alt_u32_ptr(pfm_body_ptr, SMBUS_RULE_DEF_SIZE);
        }
        else if (*tag == SPI_REGION_DEF_TYPE)
        {
            PFM_SPI_REGION_DEF* region_def = (PFM_SPI_REGION_DEF*) pfm_body_ptr;

            EXPECT_EQ(region_def->start_addr, testdata_bmc_spi_regions_start_addr[spi_region_counter]);
            EXPECT_EQ(region_def->end_addr, testdata_bmc_spi_regions_end_addr[spi_region_counter]);

            if (is_spi_region_static(region_def))
            {
                EXPECT_EQ(region_def->start_addr, testdata_bmc_static_regions_start_addr[static_spi_region_counter]);
                EXPECT_EQ(region_def->end_addr, testdata_bmc_static_regions_end_addr[static_spi_region_counter]);
                static_spi_region_counter++;
            }

            pfm_body_ptr = get_end_of_spi_region_def(region_def);
            spi_region_counter++;
        }
        else
        {
            // Exit if there is no more SMBus rule definition in PFM body.
            break;
        }
    }

    EXPECT_TRUE(spi_region_counter == BMC_NUM_SPI_REGIONS);
    EXPECT_TRUE(static_spi_region_counter == BMC_NUM_STATIC_REGIONS);

    delete[] full_image;
}

TEST_F(TestDataSanityTest, test_sanity_on_cpld_update_capsule)
{
    EXPECT_EQ(CFM1_ACTIVE_IMAGE_FILE_SIZE, UFM_CPLD_ACTIVE_IMAGE_LENGTH);
    EXPECT_EQ(CFM0_RECOVERY_IMAGE_FILE_SIZE, UFM_CPLD_ROM_IMAGE_LENGTH);
    EXPECT_EQ(alt_u32(SIGNED_CAPSULE_CPLD_FILE_SIZE), SIGNATURE_SIZE + alt_u32(sizeof(CPLD_UPDATE_PC)));

    // Expect both CFM images have the same size as well
    EXPECT_EQ(UFM_CPLD_ACTIVE_IMAGE_LENGTH, UFM_CPLD_ROM_IMAGE_LENGTH);

    // Check update capsule content
    // The image embedded in the capsule is expected to match what we would load to CFM1
    alt_u32* cfm1_active_image = new alt_u32[CFM1_ACTIVE_IMAGE_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(CFM1_ACTIVE_IMAGE_FILE, cfm1_active_image);

    alt_u32* signed_cpld_capsule = new alt_u32[SIGNED_CAPSULE_CPLD_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(SIGNED_CAPSULE_CPLD_FILE, signed_cpld_capsule);
    CPLD_UPDATE_PC* cpld_capsule_protected_content = (CPLD_UPDATE_PC*) incr_alt_u32_ptr(signed_cpld_capsule, SIGNATURE_SIZE);
    alt_u32* cpld_bitstream_ptr = cpld_capsule_protected_content->cpld_bitstream;

    for (int i = 0; i < (CFM1_ACTIVE_IMAGE_FILE_SIZE/4); i++)
    {
        ASSERT_EQ(cpld_bitstream_ptr[i], cfm1_active_image[i]);
    }

    delete[] cfm1_active_image;
    delete[] signed_cpld_capsule;
}

/*
 * This test ensures that the expected hashes for PCH/BMC firmware matches the calculated hashes
 */
TEST_F(TestDataSanityTest, test_expected_fw_hashes)
{
    SYSTEM_MOCK::get()->load_to_flash(m_spi_flash_in_use, FULL_PFR_IMAGE_PCH_FILE, FULL_PFR_IMAGE_PCH_FILE_SIZE);
    EXPECT_TRUE(verify_sha((alt_u32*) full_pfr_image_pch_file_sha256sum, get_spi_flash_ptr(),PCH_SPI_FLASH_SIZE));

    SYSTEM_MOCK::get()->load_to_flash(m_spi_flash_in_use, FULL_PFR_IMAGE_BMC_FILE, FULL_PFR_IMAGE_BMC_FILE_SIZE);
    EXPECT_TRUE(verify_sha((alt_u32*) full_pfr_image_bmc_file_sha256sum, get_spi_flash_ptr(),BMC_SPI_FLASH_SIZE));
}

/*
 * This test checks if there's any static region in BMC PFM that has any RPLM Bit 2-4 set.
 * This check is necessary to guarantee test_wdt_recovery_on_bmc test from test_fw_recovery.cpp works as expected.
 */
TEST_F(TestDataSanityTest, check_static_region_in_bmc)
{
    SYSTEM_MOCK::get()->load_to_flash(m_spi_flash_in_use, SIGNED_PFM_BMC_FILE, SIGNED_PFM_BMC_FILE_SIZE);
    PFM* pfm = (PFM*) get_spi_flash_ptr_with_offset(SIGNATURE_SIZE);
    alt_u32* pfm_body_ptr = pfm->pfm_body;

    alt_u32 test_passed = 0;

    // Go through the PFM Body
    while (1)
    {
        alt_u8* tag = (alt_u8*) pfm_body_ptr;

        if (*tag == SMBUS_RULE_DEF_TYPE)
        {
            // Skip the rule definition
            pfm_body_ptr = incr_alt_u32_ptr(pfm_body_ptr, SMBUS_RULE_DEF_SIZE);
        }
        else if (*tag == SPI_REGION_DEF_TYPE)
        {
            PFM_SPI_REGION_DEF* region_def = (PFM_SPI_REGION_DEF*) pfm_body_ptr;

            if (is_spi_region_static(region_def) &&
                    (region_def->protection_mask & SPI_REGION_PROTECT_MASK_RECOVER_BITS))
            {
                test_passed = 1;
            }

            // Increment the pointer in PFM body appropriately
            pfm_body_ptr = get_end_of_spi_region_def(region_def);
        }
        else
        {
            // Break when there is no more region/rule definition in PFM body
            break;
        }
    }

    EXPECT_TRUE(test_passed);
}

/**
 * @brief Make sure all cpld update capsules have the correct SVN.
 * This is crucial for the CPLD update/recovery unittests to function properly.
 */
TEST_F(TestDataSanityTest, check_svn_in_cpld_update_capsules)
{
    alt_u32 *cpld_capsule = new alt_u32[SIGNED_CAPSULE_CPLD_FILE_SIZE/4];
    CPLD_UPDATE_PC* cpld_capsule_pc = (CPLD_UPDATE_PC*) incr_alt_u32_ptr(cpld_capsule, SIGNATURE_SIZE);

    SYSTEM_MOCK::get()->init_x86_mem_from_file(SIGNED_CAPSULE_CPLD_FILE, cpld_capsule);
    EXPECT_EQ(cpld_capsule_pc->svn, alt_u32(0));

    SYSTEM_MOCK::get()->init_x86_mem_from_file(SIGNED_CAPSULE_CPLD_WITH_SVN1_FILE, cpld_capsule);
    EXPECT_EQ(cpld_capsule_pc->svn, alt_u32(1));

    SYSTEM_MOCK::get()->init_x86_mem_from_file(SIGNED_CAPSULE_CPLD_WITH_SVN64_FILE, cpld_capsule);
    EXPECT_EQ(cpld_capsule_pc->svn, alt_u32(64));

    SYSTEM_MOCK::get()->init_x86_mem_from_file(SIGNED_CAPSULE_CPLD_WITH_SVN65_FILE, cpld_capsule);
    EXPECT_EQ(cpld_capsule_pc->svn, alt_u32(65));

    delete[] cpld_capsule;
}
