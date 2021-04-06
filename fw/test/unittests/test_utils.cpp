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

class UtilsTest : public testing::Test
{
public:
    alt_u32* m_memory = nullptr;

    virtual void SetUp()
    {
        SYSTEM_MOCK::get()->reset();
        m_memory = NIOS_SCRATCHPAD_ADDR;
    }

    virtual void TearDown() {}
};

TEST_F(UtilsTest, test_basic)
{
    IOWR(m_memory, 0, 1);
    EXPECT_EQ(IORD(m_memory, 0), (alt_u32) 1);
    EXPECT_EQ(IORD(m_memory, 1), (alt_u32) 0);
}

TEST_F(UtilsTest, test_load_store)
{
    alt_u8 src_mem[4] = {0x01, 0x23, 0x45, 0x67};
    EXPECT_EQ(*((alt_u32*) src_mem), (alt_u32) 0x67452301);

    alt_u32 data = *((alt_u32*) src_mem);
    EXPECT_EQ(data, (alt_u32) 0x67452301);

    IOWR(m_memory, 0, data);
    EXPECT_EQ(IORD(m_memory, 0), (alt_u32) 0x67452301);
}

TEST_F(UtilsTest, test_set_bit)
{
    EXPECT_EQ(IORD(m_memory, 0), (alt_u32) 0);
    // Set the second bit
    set_bit(&m_memory[0], 2);
    EXPECT_EQ(IORD(m_memory, 0), (alt_u32) 4);
}

TEST_F(UtilsTest, test_clear_bit)
{
    IOWR(m_memory, 3, 7);
    EXPECT_EQ(IORD(m_memory, 3), (alt_u32) 7);

    // Clear the third bit
    clear_bit(&m_memory[3], 2);
    EXPECT_EQ(IORD(m_memory, 3), (alt_u32) 3);
}

TEST_F(UtilsTest, test_check_bit)
{
    IOWR(m_memory, 1, 7);
    EXPECT_EQ(IORD(m_memory, 1), (alt_u32) 7);

    // Verify the first 4 bits
    EXPECT_EQ(check_bit(&m_memory[1], 0), (alt_u32) 1);
    EXPECT_EQ(check_bit(&m_memory[1], 1), (alt_u32) 1);
    EXPECT_EQ(check_bit(&m_memory[1], 2), (alt_u32) 1);
    EXPECT_EQ(check_bit(&m_memory[1], 3), (alt_u32) 0);
}

TEST_F(UtilsTest, test_alt_u32_memcpy_non_incr_1word)
{
    // Set up source memory blocks
    alt_u8 src_mem[4] = {0x01, 0x23, 0x45, 0x67};
    EXPECT_EQ(*((alt_u32*) src_mem), (alt_u32) 0x67452301);

    // Perform memcpy
    alt_u32_memcpy_non_incr(m_memory, (alt_u32*) src_mem, 4);

    // Verify memcpy results
    EXPECT_EQ(IORD(m_memory, 0), (alt_u32) 0x67452301);
}

TEST_F(UtilsTest, test_alt_u32_memcpy_non_incr)
{
    // Set up source m_memory blocks
    alt_u8 src_mem[8] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF};

    // Perform memcpy
    alt_u32* dest_ptr = &m_memory[10];
    alt_u32_memcpy_non_incr(dest_ptr, (alt_u32*) src_mem, 8);

    // Verify memcpy results
    EXPECT_EQ(IORD(m_memory, 9), (alt_u32) 0);
    EXPECT_EQ(IORD(m_memory, 10), (alt_u32) 0xEFCDAB89);
    EXPECT_EQ(IORD(m_memory, 11), (alt_u32) 0);
}

TEST_F(UtilsTest, test_alt_u32_memcpy)
{
    // Set up source m_memory blocks
    alt_u8 src_mem[8] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF};

    // Perform memcpy
    alt_u32 dest_ptr[10] = {0};
    alt_u32_memcpy(dest_ptr, (alt_u32*) src_mem, 8);

    // Verify memcpy results
    EXPECT_EQ(dest_ptr[0], (alt_u32) 0x67452301);
    EXPECT_EQ(dest_ptr[1], (alt_u32) 0xEFCDAB89);
    EXPECT_EQ(dest_ptr[2], (alt_u32) 0);
}

TEST(SanityTest, test_alt_u32_memcpy_non_incr_bad_num_words)
{
    // Set asserts to throw as opposed to abort
    SYSTEM_MOCK::get()->set_assert_to_throw();
    EXPECT_ANY_THROW({
        alt_u32_memcpy_non_incr(NIOS_SCRATCHPAD_ADDR, NIOS_SCRATCHPAD_ADDR, 3);
    });
}

TEST_F(UtilsTest, testalt_u32_memcpy_bad_num_words)
{
    // Set asserts to throw as opposed to abort
    SYSTEM_MOCK::get()->set_assert_to_throw();
    EXPECT_ANY_THROW(
        { alt_u32_memcpy(NIOS_SCRATCHPAD_ADDR, NIOS_SCRATCHPAD_ADDR, 3); });
}
