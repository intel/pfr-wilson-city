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


class SPIFlashRWTest : public testing::Test
{
public:
    virtual void SetUp()
    {
        SYSTEM_MOCK::get()->reset();
        SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, SIGNED_CAPSULE_BMC_FILE, SIGNED_CAPSULE_BMC_FILE_SIZE);
        switch_spi_flash(SPI_FLASH_BMC);
    }

    virtual void TearDown() {}
};

TEST_F(SPIFlashRWTest, test_basic_read)
{
    alt_u32* bmc_flash_ptr = get_spi_flash_ptr();
    EXPECT_EQ(*bmc_flash_ptr, (alt_u32) BLOCK0_MAGIC);
}

TEST_F(SPIFlashRWTest, test_basic_write)
{
    alt_u32* bmc_flash_ptr = get_spi_flash_ptr();
    *bmc_flash_ptr = 0x12345678;
    EXPECT_EQ(*bmc_flash_ptr, (alt_u32) 0x12345678);
}

TEST_F(SPIFlashRWTest, test_erase_4kb_in_spi_flash)
{
    // Get pointer to the SPI flash
    alt_u32* bmc_flash_ptr = get_spi_flash_ptr();

    // Fill the first three pages with 0xdeadbeef and verify it
    for (alt_u32 i = 0; i < (0x3000 >> 2); i++)
    {
        bmc_flash_ptr[i] = 0xdeadbeef;
    }
    for (alt_u32 i = 0; i < (0x3000 >> 2); i++)
    {
        EXPECT_EQ(bmc_flash_ptr[i], alt_u32(0xdeadbeef));
    }

    // Erase the second page
    execute_spi_erase_cmd(0x1000, SPI_CMD_4KB_SECTOR_ERASE);

    // The first page should remain untouched
    for (alt_u32 i = 0; i < (0x1000 >> 2); i++)
    {
        EXPECT_EQ(bmc_flash_ptr[i], alt_u32(0xdeadbeef));
    }
    // The second page should now be empty (filled with 0xffffffff)
    for (alt_u32 i = (0x1000 >> 2); i < (0x2000 >> 2); i++)
    {
        EXPECT_EQ(bmc_flash_ptr[i], alt_u32(0xffffffff));
    }
    // The third page should remain untouched
    for (alt_u32 i = (0x2000 >> 2); i < (0x3000 >> 2); i++)
    {
        EXPECT_EQ(bmc_flash_ptr[i], alt_u32(0xdeadbeef));
    }

    // Check SPI Erase command counts
    EXPECT_EQ(SYSTEM_MOCK::get()->get_spi_cmd_count(SPI_CMD_4KB_SECTOR_ERASE), alt_u32(1));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_spi_cmd_count(SPI_CMD_64KB_SECTOR_ERASE), alt_u32(0));
}

TEST_F(SPIFlashRWTest, test_erase_64kb_in_spi_flash)
{
    // Get pointer to the SPI flash
    alt_u32* bmc_flash_ptr = get_spi_flash_ptr();

    // Fill the thirty two pages with 0xdeadbeef and verify it
    for (alt_u32 i = 0; i < (0x20000 >> 2); i++)
    {
        bmc_flash_ptr[i] = 0xdeadbeef;
    }
    for (alt_u32 i = 0; i < (0x20000 >> 2); i++)
    {
        EXPECT_EQ(bmc_flash_ptr[i], alt_u32(0xdeadbeef));
    }

    // Erase from 9th to 24th page
    execute_spi_erase_cmd(0x8000, SPI_CMD_64KB_SECTOR_ERASE);

    // The first eight page should remain untouched
    for (alt_u32 i = 0; i < (0x8000 >> 2); i++)
    {
        EXPECT_EQ(bmc_flash_ptr[i], alt_u32(0xdeadbeef));
    }
    // The 9th to 24th pages should now be empty (filled with 0xffffffff)
    for (alt_u32 i = (0x8000 >> 2); i < (0x18000 >> 2); i++)
    {
        EXPECT_EQ(bmc_flash_ptr[i], alt_u32(0xffffffff));
    }
    // The 25th page and onwards should remain untouched
    for (alt_u32 i = (0x18000 >> 2); i < (0x20000 >> 2); i++)
    {
        EXPECT_EQ(bmc_flash_ptr[i], alt_u32(0xdeadbeef));
    }

    // Check SPI Erase command counts
    EXPECT_EQ(SYSTEM_MOCK::get()->get_spi_cmd_count(SPI_CMD_4KB_SECTOR_ERASE), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_spi_cmd_count(SPI_CMD_64KB_SECTOR_ERASE), alt_u32(1));
}

TEST_F(SPIFlashRWTest, test_erase_spi_region)
{
    // Get pointer to the SPI flash
    switch_spi_flash(SPI_FLASH_PCH);
    alt_u32* pch_flash_ptr = get_spi_flash_ptr();
    pch_flash_ptr[0] = 0x12345678;
    pch_flash_ptr[1] = 0xdeadbeef;
    // 4KB boundary
    pch_flash_ptr[1023] = 0x33336666;
    pch_flash_ptr[1024] = BLOCK0_MAGIC;

    // Erase the first page
    erase_spi_region(0, 0x1000);

    // The first page should be cleared now
    for (alt_u32 i = 0; i < (0x1000 >> 2); i++)
    {
        EXPECT_EQ(pch_flash_ptr[i], 0xFFFFFFFF);
    }

    EXPECT_EQ(pch_flash_ptr[1024], alt_u32(BLOCK0_MAGIC));
}

TEST_F(SPIFlashRWTest, test_erase_spi_region_with_4kb_erase)
{
    // Get pointer to the SPI flash
    switch_spi_flash(SPI_FLASH_BMC);
    alt_u32* bmc_flash_ptr = get_spi_flash_ptr();
    bmc_flash_ptr[0] = 0x12345678;
    bmc_flash_ptr[1] = 0xdeadbeef;
    // 32KB boundary
    bmc_flash_ptr[8191] = 0xabcdabcd;
    bmc_flash_ptr[8192] = BLOCK1_MAGIC;

    // Erase the first eight page
    erase_spi_region(0, 0x8000);

    // The first eight page should be cleared now
    for (alt_u32 i = 0; i < (0x8000 >> 2); i++)
    {
        EXPECT_EQ(bmc_flash_ptr[i], 0xFFFFFFFF);
    }
    // The 9th page should remain untouched
    EXPECT_EQ(bmc_flash_ptr[8192], alt_u32(BLOCK1_MAGIC));

    // Check SPI Erase command counts
    EXPECT_EQ(SYSTEM_MOCK::get()->get_spi_cmd_count(SPI_CMD_4KB_SECTOR_ERASE), alt_u32(8));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_spi_cmd_count(SPI_CMD_64KB_SECTOR_ERASE), alt_u32(0));
}

TEST_F(SPIFlashRWTest, test_erase_spi_region_with_64kb_erase)
{
    // Get pointer to the SPI flash
    switch_spi_flash(SPI_FLASH_BMC);
    alt_u32* bmc_flash_ptr = get_spi_flash_ptr();
    bmc_flash_ptr[0] = 0x12345678;
    bmc_flash_ptr[1] = 0xdeadbeef;
    // 64KB boundary
    bmc_flash_ptr[16383] = 0xabcdabcd;
    bmc_flash_ptr[16384] = BLOCK1_MAGIC;

    // Erase the first sixteenth page
    erase_spi_region(0, 0x10000);

    // The sixteen pages should be cleared now
    for (alt_u32 i = 0; i < (0x10000 >> 2); i++)
    {
        EXPECT_EQ(bmc_flash_ptr[i], 0xFFFFFFFF);
    }

    // The 17th page should remain untouched
    EXPECT_EQ(bmc_flash_ptr[16384], alt_u32(BLOCK1_MAGIC));

    // Check SPI Erase command counts
    EXPECT_EQ(SYSTEM_MOCK::get()->get_spi_cmd_count(SPI_CMD_4KB_SECTOR_ERASE), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_spi_cmd_count(SPI_CMD_64KB_SECTOR_ERASE), alt_u32(1));
}

TEST_F(SPIFlashRWTest, test_erase_spi_region_with_mix_of_4kb_and_64kb_erase)
{
    // Test setting
    //   Erasing 47 pages starting at a certain offset
    alt_u32 region_start_addr = 0x2000;
    alt_u32 erase_nbytes = 0x2F000;
    alt_u32 region_end_addr = region_start_addr + erase_nbytes;
    // word index
    alt_u32 region_start_word_i = region_start_addr >> 2;
    alt_u32 region_end_word_i = region_end_addr >> 2;

    // Get pointer to the SPI flash
    switch_spi_flash(SPI_FLASH_BMC);
    alt_u32* bmc_flash_ptr = get_spi_flash_ptr();

    // Leave some data around the target SPI region
    for (alt_u32 i = region_start_word_i - 2; i < region_end_word_i + 2; i++)
    {
        bmc_flash_ptr[i] = 0xDEADBEEF;
    }

    // Erasing 16 pages starting at a certain offset
    erase_spi_region(region_start_addr, erase_nbytes);

    // The targeted sector should be cleared now
    for (alt_u32 i = region_start_word_i; i < region_end_word_i; i++)
    {
        ASSERT_EQ(bmc_flash_ptr[i], 0xFFFFFFFF);
    }
    // Words near the region should not be erased
    EXPECT_EQ(bmc_flash_ptr[region_start_word_i - 1], 0xDEADBEEF);
    EXPECT_EQ(bmc_flash_ptr[region_start_word_i - 2], 0xDEADBEEF);
    EXPECT_EQ(bmc_flash_ptr[region_end_word_i], 0xDEADBEEF);
    EXPECT_EQ(bmc_flash_ptr[region_end_word_i + 1], 0xDEADBEEF);

    // Check SPI Erase command counts
    EXPECT_EQ(SYSTEM_MOCK::get()->get_spi_cmd_count(SPI_CMD_4KB_SECTOR_ERASE), alt_u32(15));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_spi_cmd_count(SPI_CMD_64KB_SECTOR_ERASE), alt_u32(2));
}

TEST_F(SPIFlashRWTest, test_erase_64kb_spi_region_which_is_not_64kb_aligned)
{
    // Test setting
    //   Erasing 16 pages starting at a certain offset
    alt_u32 region_start_addr = 0x6000;
    alt_u32 erase_nbytes = 0x10000;
    alt_u32 region_end_addr = region_start_addr + erase_nbytes;
    // word index
    alt_u32 region_start_word_i = region_start_addr >> 2;
    alt_u32 region_end_word_i = region_end_addr >> 2;

    // Get pointer to the SPI flash
    switch_spi_flash(SPI_FLASH_BMC);
    alt_u32* bmc_flash_ptr = get_spi_flash_ptr();

    // Leave some data around the target SPI region
    for (alt_u32 i = region_start_word_i - 2; i < region_end_word_i + 2; i++)
    {
        bmc_flash_ptr[i] = 0xDEADBEEF;
    }

    // Erasing 16 pages starting at a certain offset
    erase_spi_region(region_start_addr, erase_nbytes);

    // The targeted sector should be cleared now
    for (alt_u32 i = region_start_word_i; i < region_end_word_i; i++)
    {
        ASSERT_EQ(bmc_flash_ptr[i], 0xFFFFFFFF);
    }
    // Words near the region should not be erased
    EXPECT_EQ(bmc_flash_ptr[region_start_word_i - 1], 0xDEADBEEF);
    EXPECT_EQ(bmc_flash_ptr[region_start_word_i - 2], 0xDEADBEEF);
    EXPECT_EQ(bmc_flash_ptr[region_end_word_i], 0xDEADBEEF);
    EXPECT_EQ(bmc_flash_ptr[region_end_word_i + 1], 0xDEADBEEF);

    // Check SPI Erase command counts
    EXPECT_EQ(SYSTEM_MOCK::get()->get_spi_cmd_count(SPI_CMD_4KB_SECTOR_ERASE), alt_u32(16));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_spi_cmd_count(SPI_CMD_64KB_SECTOR_ERASE), alt_u32(0));
}

TEST_F(SPIFlashRWTest, test_erase_120kb_spi_region_which_is_not_64kb_aligned)
{
    // Test setting
    //   Erasing 30 pages starting at a certain offset
    alt_u32 region_start_addr = 0x21000;
    alt_u32 erase_nbytes = 0x1E000;
    alt_u32 region_end_addr = region_start_addr + erase_nbytes;
    // word index
    alt_u32 region_start_word_i = region_start_addr >> 2;
    alt_u32 region_end_word_i = region_end_addr >> 2;

    // Get pointer to the SPI flash
    switch_spi_flash(SPI_FLASH_BMC);
    alt_u32* bmc_flash_ptr = get_spi_flash_ptr();

    // Leave some data around the target SPI region
    for (alt_u32 i = region_start_word_i - 2; i < region_end_word_i + 2; i++)
    {
        bmc_flash_ptr[i] = 0xDEADBEEF;
    }

    // Erasing 16 pages starting at a certain offset
    erase_spi_region(region_start_addr, erase_nbytes);

    // The targeted sector should be cleared now
    for (alt_u32 i = region_start_word_i; i < region_end_word_i; i++)
    {
        ASSERT_EQ(bmc_flash_ptr[i], 0xFFFFFFFF);
    }
    // Words near the region should not be erased
    EXPECT_EQ(bmc_flash_ptr[region_start_word_i - 1], 0xDEADBEEF);
    EXPECT_EQ(bmc_flash_ptr[region_start_word_i - 2], 0xDEADBEEF);
    EXPECT_EQ(bmc_flash_ptr[region_end_word_i], 0xDEADBEEF);
    EXPECT_EQ(bmc_flash_ptr[region_end_word_i + 1], 0xDEADBEEF);

    // Check SPI Erase command counts
    EXPECT_EQ(SYSTEM_MOCK::get()->get_spi_cmd_count(SPI_CMD_4KB_SECTOR_ERASE), alt_u32(30));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_spi_cmd_count(SPI_CMD_64KB_SECTOR_ERASE), alt_u32(0));
}

TEST_F(SPIFlashRWTest, test_memcpy_signed_payload_with_bmc_capsule)
{
    // A valid capsule is available from the beginning of the BMC flash
    alt_u32* bmc_flash_ptr = get_spi_flash_ptr();
    EXPECT_EQ(*bmc_flash_ptr, alt_u32(BLOCK0_MAGIC));

    // Copy this capsule to another SPI region
    bmc_flash_ptr[(0x2a00000 - 4) >> 2] = 0xdeadbeef;
    memcpy_signed_payload(0x2a00000, bmc_flash_ptr);

    // Ensure the whole capsule has been copied over.
    for (alt_u32 word_i = 0; word_i < (SIGNED_CAPSULE_BMC_FILE_SIZE >> 2); word_i++)
    {
        EXPECT_EQ(bmc_flash_ptr[word_i], bmc_flash_ptr[word_i + (0x2a00000 >> 2)]);
    }

    // Make sure the word before the target SPI region are not affected
    // The word after the region may be erased, if it's part of the page that capsule would occupy.
    EXPECT_EQ(bmc_flash_ptr[(0x2a00000 - 4) >> 2], 0xdeadbeef);

    // Calculate expected number of erases
    alt_u32 expected_num_4kb_erases = (SIGNED_CAPSULE_BMC_FILE_SIZE % 0x10000) / 0x1000;
    if (SIGNED_CAPSULE_BMC_FILE_SIZE % 0x1000)
    {
        // if the capsule is not 4KB aligned, an additional erase would be executed.
        expected_num_4kb_erases++;
    }
    alt_u32 expected_num_64kb_erases = SIGNED_CAPSULE_BMC_FILE_SIZE / 0x10000;

    // Check SPI Erase command counts
    EXPECT_EQ(SYSTEM_MOCK::get()->get_spi_cmd_count(SPI_CMD_4KB_SECTOR_ERASE),
              expected_num_4kb_erases);
    EXPECT_EQ(SYSTEM_MOCK::get()->get_spi_cmd_count(SPI_CMD_64KB_SECTOR_ERASE),
              expected_num_64kb_erases);
}

TEST_F(SPIFlashRWTest, test_memcpy_signed_payload_with_pch_pfm)
{
    alt_u32* bmc_flash_ptr = get_spi_flash_ptr();

    // Upload a signed PCH PFM for this test
    alt_u32 signed_payload_addr = 0x2a00000;
    SYSTEM_MOCK::get()->load_to_flash(
        SPI_FLASH_BMC, SIGNED_PFM_BIOS_FILE, SIGNED_PFM_BIOS_FILE_SIZE, signed_payload_addr);
    alt_u32* signed_payload = incr_alt_u32_ptr(bmc_flash_ptr, signed_payload_addr);

    // Destination address
    alt_u32 dest_addr = 0x4;
    alt_u32 dest_word_addr = dest_addr >> 2;

    // Some data to check near the boundaries of source/destination
    EXPECT_EQ(bmc_flash_ptr[0], alt_u32(BLOCK0_MAGIC));
    bmc_flash_ptr[(signed_payload_addr + SIGNED_PFM_BIOS_FILE_SIZE - 4) >> 2] = 0xdecaf369;
    bmc_flash_ptr[(signed_payload_addr + SIGNED_PFM_BIOS_FILE_SIZE) >> 2] = 0xdeadbeef;

    // Copy the signed payload
    memcpy_signed_payload(dest_addr, signed_payload);

    // Ensure the whole capsule has been copied over.
    // Note the last word of the capsule has overwritten with special word
    for (alt_u32 word_i = 0; word_i < ((SIGNED_PFM_BIOS_FILE_SIZE - 4) >> 2); word_i++)
    {
        ASSERT_EQ(bmc_flash_ptr[dest_word_addr + word_i], signed_payload[word_i]);
    }

    // Check data near the boundaries of destination location
    EXPECT_EQ(bmc_flash_ptr[0], alt_u32(BLOCK0_MAGIC));
    EXPECT_EQ(bmc_flash_ptr[(dest_addr + SIGNED_PFM_BIOS_FILE_SIZE - 4) >> 2], alt_u32(0xdecaf369));
    EXPECT_NE(bmc_flash_ptr[(dest_addr + SIGNED_PFM_BIOS_FILE_SIZE) >> 2], alt_u32(0xdeadbeef));

    // as sanity, make sure the source location is not change in the process.
    EXPECT_EQ(bmc_flash_ptr[(signed_payload_addr + SIGNED_PFM_BIOS_FILE_SIZE - 4) >> 2],
              alt_u32(0xdecaf369));
    EXPECT_EQ(bmc_flash_ptr[(signed_payload_addr + SIGNED_PFM_BIOS_FILE_SIZE) >> 2],
              alt_u32(0xdeadbeef));

    // Calculate expected number of erases
    // Since the destination address doesn't align with 64KB boundary, 64KB erase won't be used.
    alt_u32 expected_num_4kb_erases = SIGNED_PFM_BIOS_FILE_SIZE / 0x1000;
    if (SIGNED_PFM_BIOS_FILE_SIZE % 0x1000)
    {
        // if the capsule is not 4KB aligned, an additional erase would be executed.
        expected_num_4kb_erases++;
    }
    alt_u32 expected_num_64kb_erases = 0;

    // Check SPI Erase command counts
    EXPECT_EQ(SYSTEM_MOCK::get()->get_spi_cmd_count(SPI_CMD_4KB_SECTOR_ERASE),
              expected_num_4kb_erases);
    EXPECT_EQ(SYSTEM_MOCK::get()->get_spi_cmd_count(SPI_CMD_64KB_SECTOR_ERASE),
              expected_num_64kb_erases);
}

TEST_F(SPIFlashRWTest, test_memcpy_signed_payload_with_cpld_capsule)
{
    alt_u32* bmc_flash_ptr = get_spi_flash_ptr();

    // Upload a signed PCH PFM for this test
    alt_u32 signed_payload_addr = 0x2a00000;
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC,
                                      SIGNED_CAPSULE_CPLD_FILE,
                                      SIGNED_CAPSULE_CPLD_FILE_SIZE,
                                      signed_payload_addr);
    alt_u32* signed_payload = incr_alt_u32_ptr(bmc_flash_ptr, signed_payload_addr);

    // Destination address
    alt_u32 dest_addr = 0x4;
    alt_u32 dest_word_addr = dest_addr >> 2;

    // Some data to check near the boundaries of source/destination
    EXPECT_EQ(bmc_flash_ptr[0], alt_u32(BLOCK0_MAGIC));
    bmc_flash_ptr[(signed_payload_addr + SIGNED_CAPSULE_CPLD_FILE_SIZE - 4) >> 2] = 0xdecaf369;
    bmc_flash_ptr[(signed_payload_addr + SIGNED_CAPSULE_CPLD_FILE_SIZE) >> 2] = 0xdeadbeef;

    // Copy the signed payload
    memcpy_signed_payload(dest_addr, signed_payload);

    // Ensure the whole capsule has been copied over.
    // Note the last word of the capsule has overwritten with special word
    for (alt_u32 word_i = 0; word_i < ((SIGNED_CAPSULE_CPLD_FILE_SIZE - 4) >> 2); word_i++)
    {
        ASSERT_EQ(bmc_flash_ptr[dest_word_addr + word_i], signed_payload[word_i]);
    }

    // Check data near the boundaries of destination location
    EXPECT_EQ(bmc_flash_ptr[0], alt_u32(BLOCK0_MAGIC));
    EXPECT_EQ(bmc_flash_ptr[(dest_addr + SIGNED_CAPSULE_CPLD_FILE_SIZE - 4) >> 2],
              alt_u32(0xdecaf369));
    EXPECT_NE(bmc_flash_ptr[(dest_addr + SIGNED_CAPSULE_CPLD_FILE_SIZE) >> 2], alt_u32(0xdeadbeef));

    // as sanity, make sure the source location is not change in the process.
    EXPECT_EQ(bmc_flash_ptr[(signed_payload_addr + SIGNED_CAPSULE_CPLD_FILE_SIZE - 4) >> 2],
              alt_u32(0xdecaf369));
    EXPECT_EQ(bmc_flash_ptr[(signed_payload_addr + SIGNED_CAPSULE_CPLD_FILE_SIZE) >> 2],
              alt_u32(0xdeadbeef));

    // Calculate expected number of erases
    // Since the destination address doesn't align with 64KB boundary, 64KB erase won't be used.
    alt_u32 expected_num_4kb_erases = SIGNED_CAPSULE_CPLD_FILE_SIZE / 0x1000;
    if (SIGNED_CAPSULE_CPLD_FILE_SIZE % 0x1000)
    {
        // if the capsule is not 4KB aligned, an additional erase would be executed.
        expected_num_4kb_erases++;
    }
    alt_u32 expected_num_64kb_erases = 0;

    // Check SPI Erase command counts
    EXPECT_EQ(SYSTEM_MOCK::get()->get_spi_cmd_count(SPI_CMD_4KB_SECTOR_ERASE),
              expected_num_4kb_erases);
    EXPECT_EQ(SYSTEM_MOCK::get()->get_spi_cmd_count(SPI_CMD_64KB_SECTOR_ERASE),
              expected_num_64kb_erases);
}
