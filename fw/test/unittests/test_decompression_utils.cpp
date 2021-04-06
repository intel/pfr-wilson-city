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


class DecompressionUtilsTest : public testing::Test
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
        switch_spi_flash(m_spi_flash_in_use);

        // Perform provisioning
        SYSTEM_MOCK::get()->provision_ufm_data(UFM_PFR_DATA_EXAMPLE_KEY_FILE);

        /*
         * Standard capsule setting for all test cases
         */
        // Get a pointer to an empty capsule location
        alt_u32* signed_capsule = get_spi_recovery_region_ptr(m_spi_flash_in_use);

        // Set PFM size so that we can get to PBC
        alt_u32* signed_pfm = incr_alt_u32_ptr(signed_capsule, SIGNATURE_SIZE);
        KCH_BLOCK0* pfm_block0 = (KCH_BLOCK0*) signed_pfm;
        pfm_block0->pc_length = 0x400;

        // Get pointers to various structure inside a capsule
        PBC_HEADER* pbc = get_pbc_ptr_from_signed_capsule(signed_capsule);
        // Set page/bitmap size
        pbc->bitmap_nbit = 0x4000;
        pbc->page_size = PBC_EXPECTED_PAGE_SIZE;
    }

    virtual void TearDown() {}
};

TEST_F(DecompressionUtilsTest, test_decompression_erase_function_case_1)
{
    // Get pointers to various structure inside a capsule
    alt_u32* signed_capsule = get_spi_recovery_region_ptr(m_spi_flash_in_use);
    PBC_HEADER* pbc = get_pbc_ptr_from_signed_capsule(signed_capsule);
    alt_u8* active_bitmap = (alt_u8*) get_active_bitmap(pbc);
    alt_u8* comp_bitmap = (alt_u8*) get_compression_bitmap(pbc);

    // Customize the active bitmap
    //   Erase the first 16 pages (64KB data)
    active_bitmap[0] = 0b11111111;
    active_bitmap[1] = 0b11111111;
    for (alt_u32 byte_i = 2; byte_i < (pbc->bitmap_nbit / 8); byte_i++)
    {
        // Erase everything else
        active_bitmap[byte_i] = 0b11111111;
    }

    // Customize the compression bitmap
    //   Don't copy anything
    for (alt_u32 byte_i = 0; byte_i < (pbc->bitmap_nbit / 8); byte_i++)
    {
        comp_bitmap[byte_i] = 0;
    }

    // Target SPI region
    alt_u32 target_region_num_pages = 16;
    alt_u32 target_start_addr = 0x0;
    alt_u32 target_end_addr = target_start_addr + target_region_num_pages * PBC_EXPECTED_PAGE_SIZE;

    // Write some data to the target SPI region
    for (alt_u32 page_id = 0; page_id < target_region_num_pages; page_id++)
    {
        alt_u32* spi_ptr = get_spi_flash_ptr_with_offset(target_start_addr + page_id * PBC_EXPECTED_PAGE_SIZE);

        // Write in word index in all words of this page
        for (alt_u32 word_i = 0; word_i < (PBC_EXPECTED_PAGE_SIZE >> 2); word_i++)
        {
            spi_ptr[word_i] = word_i;
        }
    }

    // Perform the decompression
    decompress_spi_region_from_capsule(target_start_addr, target_end_addr, signed_capsule);

    // Check the target SPI region
    for (alt_u32 page_id = 0; page_id < target_region_num_pages; page_id++)
    {
        alt_u32* spi_ptr = get_spi_flash_ptr_with_offset(target_start_addr + page_id * PBC_EXPECTED_PAGE_SIZE);

        // All data should be erased in this SPI Region
        for (alt_u32 word_i = 0; word_i < (PBC_EXPECTED_PAGE_SIZE >> 2); word_i++)
        {
            ASSERT_EQ(spi_ptr[word_i], alt_u32(0xFFFFFFFF));
        }
    }

    // Check SPI Erase command counts
    EXPECT_EQ(SYSTEM_MOCK::get()->get_spi_cmd_count(SPI_CMD_4KB_SECTOR_ERASE), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_spi_cmd_count(SPI_CMD_64KB_SECTOR_ERASE), alt_u32(1));
}

TEST_F(DecompressionUtilsTest, test_decompression_erase_function_case_2)
{
    // Get pointers to various structure inside a capsule
    alt_u32* signed_capsule = get_spi_recovery_region_ptr(m_spi_flash_in_use);
    PBC_HEADER* pbc = get_pbc_ptr_from_signed_capsule(signed_capsule);
    alt_u8* active_bitmap = (alt_u8*) get_active_bitmap(pbc);
    alt_u8* comp_bitmap = (alt_u8*) get_compression_bitmap(pbc);

    // Customize the active bitmap
    //   Don't erase page 2 - page 10
    active_bitmap[0] = 0b11000000;
    active_bitmap[1] = 0b00111111;
    for (alt_u32 byte_i = 2; byte_i < (pbc->bitmap_nbit / 8); byte_i++)
    {
        // Erase everything else
        active_bitmap[byte_i] = 0b11111111;
    }

    // Customize the compression bitmap
    //   Don't copy anything
    for (alt_u32 byte_i = 0; byte_i < (pbc->bitmap_nbit / 8); byte_i++)
    {
        comp_bitmap[byte_i] = 0;
    }

    // Target SPI region
    alt_u32 target_region_num_pages = 40;
    alt_u32 target_start_addr = 0x0;
    alt_u32 target_end_addr = target_start_addr + target_region_num_pages * PBC_EXPECTED_PAGE_SIZE;

    // Save some data in the target SPI region
    for (alt_u32 page_id = 0; page_id < target_region_num_pages; page_id++)
    {
        alt_u32* spi_ptr = get_spi_flash_ptr_with_offset(target_start_addr + page_id * PBC_EXPECTED_PAGE_SIZE);
        spi_ptr[0] = page_id;
    }

    // Perform the decompression
    decompress_spi_region_from_capsule(target_start_addr, target_end_addr, signed_capsule);

    // Check the target SPI region
    for (alt_u32 page_id = 0; page_id < target_region_num_pages; page_id++)
    {
        alt_u32* spi_ptr = get_spi_flash_ptr_with_offset(target_start_addr + page_id * PBC_EXPECTED_PAGE_SIZE);

        if ((page_id >= 2) && (page_id < 10))
        {
            // These pages should not be erased
            EXPECT_EQ(spi_ptr[0], page_id);
        }
        else
        {
            // These pages should be erased
            EXPECT_EQ(spi_ptr[0], alt_u32(0xFFFFFFFF));
        }

        // Other than the first word, the rest of the page was initialized with 0xFFFFFFFF.
        for (alt_u32 word_i = 1; word_i < (PBC_EXPECTED_PAGE_SIZE >> 2); word_i++)
        {
            ASSERT_EQ(spi_ptr[word_i], alt_u32(0xFFFFFFFF));
        }
    }

    // Check SPI Erase command counts
    EXPECT_EQ(SYSTEM_MOCK::get()->get_spi_cmd_count(SPI_CMD_4KB_SECTOR_ERASE), alt_u32(16));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_spi_cmd_count(SPI_CMD_64KB_SECTOR_ERASE), alt_u32(1));
}

TEST_F(DecompressionUtilsTest, test_decompression_erase_function_case_3)
{
    // Get pointers to various structure inside a capsule
    alt_u32* signed_capsule = get_spi_recovery_region_ptr(m_spi_flash_in_use);
    PBC_HEADER* pbc = get_pbc_ptr_from_signed_capsule(signed_capsule);
    alt_u8* active_bitmap = (alt_u8*) get_active_bitmap(pbc);
    alt_u8* comp_bitmap = (alt_u8*) get_compression_bitmap(pbc);

    // Customize the active bitmap
    //   Only erase page 0-7 and 31
    active_bitmap[0] = 0b11111111;
    active_bitmap[1] = 0;
    active_bitmap[2] = 0;
    active_bitmap[3] = 0b00000001;
    for (alt_u32 byte_i = 4; byte_i < (pbc->bitmap_nbit / 8); byte_i++)
    {
        // Don't erase any more page
        active_bitmap[byte_i] = 0;
    }

    // Customize the compression bitmap
    //   Don't copy anything
    for (alt_u32 byte_i = 0; byte_i < (pbc->bitmap_nbit / 8); byte_i++)
    {
        comp_bitmap[byte_i] = 0;
    }

    // Target SPI region
    alt_u32 target_region_num_pages = 20;
    alt_u32 target_start_addr = 0x10000;
    alt_u32 target_end_addr = target_start_addr + target_region_num_pages * PBC_EXPECTED_PAGE_SIZE;

    // Save some data in the target SPI region
    for (alt_u32 page_id = 0; page_id < 60; page_id++)
    {
        alt_u32* spi_ptr = get_spi_flash_ptr_with_offset(page_id * PBC_EXPECTED_PAGE_SIZE);
        spi_ptr[0] = page_id;
    }

    // Perform the decompression
    decompress_spi_region_from_capsule(target_start_addr, target_end_addr, signed_capsule);

    // Check the target SPI region
    for (alt_u32 page_id = 0; page_id < 60; page_id++)
    {
        alt_u32* spi_ptr = get_spi_flash_ptr_with_offset(page_id * PBC_EXPECTED_PAGE_SIZE);

        if ((page_id != 31))
        {
            // These pages should not be erased
            EXPECT_EQ(spi_ptr[0], page_id);
        }
        else
        {
            // These pages should be erased
            EXPECT_EQ(spi_ptr[0], alt_u32(0xFFFFFFFF));
        }

        // Other than the first word, the rest of the page was initialized with 0xFFFFFFFF.
        for (alt_u32 word_i = 1; word_i < (PBC_EXPECTED_PAGE_SIZE >> 2); word_i++)
        {
            ASSERT_EQ(spi_ptr[word_i], alt_u32(0xFFFFFFFF));
        }
    }

    // Check SPI Erase command counts
    EXPECT_EQ(SYSTEM_MOCK::get()->get_spi_cmd_count(SPI_CMD_4KB_SECTOR_ERASE), alt_u32(1));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_spi_cmd_count(SPI_CMD_64KB_SECTOR_ERASE), alt_u32(0));
}

TEST_F(DecompressionUtilsTest, test_decompression_copy_function_case_1)
{
    // Get pointers to various structure inside a capsule
    alt_u32* signed_capsule = get_spi_recovery_region_ptr(m_spi_flash_in_use);
    PBC_HEADER* pbc = get_pbc_ptr_from_signed_capsule(signed_capsule);
    alt_u8* active_bitmap = (alt_u8*) get_active_bitmap(pbc);
    alt_u8* comp_bitmap = (alt_u8*) get_compression_bitmap(pbc);
    alt_u32* compressed_payload = get_compressed_payload(pbc);

    // Customize the active bitmap
    //   Erase page 2 - page 10
    active_bitmap[0] = 0b00111111;
    active_bitmap[1] = 0b11000000;
    for (alt_u32 byte_i = 2; byte_i < (pbc->bitmap_nbit / 8); byte_i++)
    {
        // Don't erase anything else
        active_bitmap[byte_i] = 0;
    }

    // Customize the compression bitmap
    //   Copy to page 2 - page 9
    comp_bitmap[0] = 0b00111111;
    comp_bitmap[1] = 0b11000000;
    for (alt_u32 byte_i = 2; byte_i < (pbc->bitmap_nbit / 8); byte_i++)
    {
        // Don't copy anything else
        comp_bitmap[byte_i] = 0;
    }

    // Customize the compressed payload (20 pages)
    //   Write the page ID on the first word of each page.
    //   Leave the rest blank (i.e. 0xFFFFFFFF)
    for (alt_u32 page_i = 0; page_i < 20; page_i++)
    {
        compressed_payload[page_i * PBC_EXPECTED_PAGE_SIZE / 4] = page_i;
    }

    // Target SPI region
    alt_u32 target_region_num_pages = 40;
    alt_u32 target_start_addr = 0x0;
    alt_u32 target_end_addr = target_start_addr + target_region_num_pages * PBC_EXPECTED_PAGE_SIZE;

    // Perform the decompression
    decompress_spi_region_from_capsule(target_start_addr, target_end_addr, signed_capsule);

    // Compare against expected data
    for (alt_u32 page_id = 0; page_id < target_region_num_pages; page_id++)
    {
        alt_u32* spi_ptr = get_spi_flash_ptr_with_offset(target_start_addr + page_id * PBC_EXPECTED_PAGE_SIZE);

        if ((page_id >= 2) && (page_id < 10))
        {
            // Expect to see data from the compressed payload
            alt_u32 expected_word = page_id - 2;
            EXPECT_EQ(spi_ptr[0], expected_word);
        }
        else
        {
            // Expect nothing to be written to here
            EXPECT_EQ(spi_ptr[0], alt_u32(0xFFFFFFFF));
        }

        // The rest of the words in this page should be blank (i.e. 0xFFFFFFFF)
        for (alt_u32 word_i = 1; word_i < (PBC_EXPECTED_PAGE_SIZE >> 2); word_i++)
        {
            ASSERT_EQ(spi_ptr[word_i], alt_u32(0xFFFFFFFF));
        }
    }
}

TEST_F(DecompressionUtilsTest, test_decompression_copy_function_case_2)
{
    // Get pointers to various structure inside a capsule
    alt_u32* signed_capsule = get_spi_recovery_region_ptr(m_spi_flash_in_use);
    PBC_HEADER* pbc = get_pbc_ptr_from_signed_capsule(signed_capsule);
    alt_u8* active_bitmap = (alt_u8*) get_active_bitmap(pbc);
    alt_u8* comp_bitmap = (alt_u8*) get_compression_bitmap(pbc);
    alt_u32* compressed_payload = get_compressed_payload(pbc);

    // Customize the active bitmap
    //   Erase page 0, page 2, page 4, ..., page 14
    active_bitmap[0] = 0b10101010;
    active_bitmap[1] = 0b10101010;
    for (alt_u32 byte_i = 2; byte_i < (pbc->bitmap_nbit / 8); byte_i++)
    {
        // Don't erase anything else
        active_bitmap[byte_i] = 0;
    }

    // Customize the compression bitmap
    //   Copy to page 0, page 2, page 4, ..., page 14
    comp_bitmap[0] = 0b10101010;
    comp_bitmap[1] = 0b10101010;
    for (alt_u32 byte_i = 2; byte_i < (pbc->bitmap_nbit / 8); byte_i++)
    {
        // Don't copy anything else
        comp_bitmap[byte_i] = 0;
    }

    // Customize the compressed payload (20 pages)
    //   Write the page ID on the first word of each page.
    //   Leave the rest blank (i.e. 0xFFFFFFFF)
    for (alt_u32 page_i = 0; page_i < 20; page_i++)
    {
        compressed_payload[page_i * PBC_EXPECTED_PAGE_SIZE / 4] = page_i;
    }

    // Target SPI region
    alt_u32 target_region_num_pages = 100;
    alt_u32 target_start_addr = 10 * PBC_EXPECTED_PAGE_SIZE;
    alt_u32 target_end_addr = target_start_addr + target_region_num_pages * PBC_EXPECTED_PAGE_SIZE;

    // Perform the decompression
    decompress_spi_region_from_capsule(target_start_addr, target_end_addr, signed_capsule);

    // Compare against expected data
    for (alt_u32 page_id = 0; page_id < target_region_num_pages; page_id++)
    {
        alt_u32* spi_ptr = get_spi_flash_ptr_with_offset(target_start_addr + page_id * PBC_EXPECTED_PAGE_SIZE);

        if ((page_id < 5) && (page_id % 2 == 0))
        {
            // Expect to see data from the compressed payload
            alt_u32 expected_word = page_id / 2 + 5;
            EXPECT_EQ(spi_ptr[0], expected_word);
        }
        else
        {
            // Expect nothing to be written to here
            EXPECT_EQ(spi_ptr[0], alt_u32(0xFFFFFFFF));
        }

        // The rest of the words in this page should be blank (i.e. 0xFFFFFFFF)
        for (alt_u32 word_i = 1; word_i < (PBC_EXPECTED_PAGE_SIZE >> 2); word_i++)
        {
            ASSERT_EQ(spi_ptr[word_i], alt_u32(0xFFFFFFFF));
        }
    }
}
