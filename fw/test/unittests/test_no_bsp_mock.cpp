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
// Include standard headers
#include <iostream>
using namespace std;

// Include the GTest headers
#include "gtest/gtest.h"

// Include the PFR headers. Include the BSP mock but not the system mock
#define NO_SYSTEM_MOCK
#include "bsp_mock.h"

#include "pfr_sys.h"
// Other PFR headers
#include "utils.h"

// Raw pointer testing
class RawPtrTest : public testing::Test
{
public:
    alt_u32* m_memory;

    virtual void SetUp() { m_memory = new alt_u32[32](); }

    virtual void TearDown() { delete[] m_memory; }
};

TEST_F(RawPtrTest, test_basic)
{
    m_memory[0] = 1;
    EXPECT_EQ(m_memory[0], (alt_u32) 1);
    EXPECT_EQ(m_memory[1], (alt_u32) 0);
}

TEST_F(RawPtrTest, test_load_store_using_ptr)
{
    alt_u8 src_mem[4] = {0x01, 0x23, 0x45, 0x67};
    EXPECT_EQ(*((alt_u32*) src_mem), (alt_u32) 0x67452301);
    EXPECT_EQ(src_mem[0], (alt_u8) 0x01);
    EXPECT_EQ(src_mem[1], (alt_u8) 0x23);
    EXPECT_EQ(src_mem[2], (alt_u8) 0x45);
    EXPECT_EQ(src_mem[3], (alt_u8) 0x67);

    alt_u32 data = *((alt_u32*) src_mem);
    EXPECT_EQ(data, (alt_u32) 0x67452301);

    m_memory[0] = data;
    EXPECT_EQ(m_memory[0], (alt_u32) 0x67452301);

    alt_u8* memory_u8_ptr = (alt_u8*) m_memory;
    EXPECT_EQ(memory_u8_ptr[0], (alt_u8) 0x01);
    EXPECT_EQ(memory_u8_ptr[1], (alt_u8) 0x23);
    EXPECT_EQ(memory_u8_ptr[2], (alt_u8) 0x45);
    EXPECT_EQ(memory_u8_ptr[3], (alt_u8) 0x67);
}

TEST_F(RawPtrTest, test_load_store_using_array)
{
    alt_u8 src_mem[4] = {0x01, 0x23, 0x45, 0x67};
    alt_u32* src_mem_u32_ptr = (alt_u32*) src_mem;

    EXPECT_EQ(src_mem_u32_ptr[0], (alt_u32) 0x67452301);
    EXPECT_EQ(src_mem[0], (alt_u8) 0x01);
    EXPECT_EQ(src_mem[1], (alt_u8) 0x23);
    EXPECT_EQ(src_mem[2], (alt_u8) 0x45);
    EXPECT_EQ(src_mem[3], (alt_u8) 0x67);

    alt_u32 data = src_mem_u32_ptr[0];
    EXPECT_EQ(data, (alt_u32) 0x67452301);

    m_memory[0] = src_mem_u32_ptr[0];
    EXPECT_EQ(m_memory[0], (alt_u32) 0x67452301);

    alt_u8* memory_u8_ptr = (alt_u8*) m_memory;
    EXPECT_EQ(memory_u8_ptr[0], (alt_u8) 0x01);
    EXPECT_EQ(memory_u8_ptr[1], (alt_u8) 0x23);
    EXPECT_EQ(memory_u8_ptr[2], (alt_u8) 0x45);
    EXPECT_EQ(memory_u8_ptr[3], (alt_u8) 0x67);
}

TEST_F(RawPtrTest, test_load_store)
{
    alt_u8 src_mem[4] = {0x01, 0x23, 0x45, 0x67};
    EXPECT_EQ(*((alt_u32*) src_mem), (alt_u32) 0x67452301);

    alt_u32 data = IORD(src_mem, 0);
    EXPECT_EQ(data, (alt_u32) 0x67452301);

    IOWR(m_memory, 0, data);
    EXPECT_EQ(IORD(m_memory, 0), (alt_u32) 0x67452301);
}

TEST_F(RawPtrTest, test_set_bit)
{
    EXPECT_EQ(m_memory[0], (alt_u32) 0);
    // Set the second bit
    set_bit((alt_u32*) m_memory, 2);
    EXPECT_EQ(m_memory[0], (alt_u32) 4);
}

TEST_F(RawPtrTest, test_clear_bit)
{
    m_memory[3] = 7;
    EXPECT_EQ(m_memory[3], (alt_u32) 7);

    // Clear the third bit
    clear_bit((alt_u32*) &m_memory[3], 2);
    EXPECT_EQ(m_memory[3], (alt_u32) 3);
}

TEST_F(RawPtrTest, test_check_bit)
{
    m_memory[1] = 7;
    EXPECT_EQ(m_memory[1], (alt_u32) 7);

    // Verify the first 4 bits
    EXPECT_EQ(check_bit((alt_u32*) &m_memory[1], 0), (alt_u32) 1);
    EXPECT_EQ(check_bit((alt_u32*) &m_memory[1], 1), (alt_u32) 1);
    EXPECT_EQ(check_bit((alt_u32*) &m_memory[1], 2), (alt_u32) 1);
    EXPECT_EQ(check_bit((alt_u32*) &m_memory[1], 3), (alt_u32) 0);
}

TEST_F(RawPtrTest, test_alt_u32_memcpy_non_incr)
{
    // Set up source memory blocks
    alt_u8 src_mem[8] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF};

    // Perform memcpy
    alt_u32* dest_ptr = &m_memory[10];
    alt_u32_memcpy_non_incr(dest_ptr, (alt_u32*) src_mem, 8);

    // Verify memcpy results
    EXPECT_EQ(m_memory[9], (alt_u32) 0);
    EXPECT_EQ(m_memory[10], (alt_u32) 0xEFCDAB89);
    EXPECT_EQ(m_memory[11], (alt_u32) 0);
}

TEST_F(RawPtrTest, test_alt_u32_memcpy_non_incr_ordering)
{
    // Set up source memory blocks
    alt_u8 src_mem[8] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF};

    // Perform memcpy
    alt_u32* dest_ptr = &m_memory[10];
    alt_u32_memcpy_non_incr(dest_ptr, (alt_u32*) src_mem, 8);

    // Verify memcpy results
    EXPECT_EQ(m_memory[9], (alt_u32) 0);
    EXPECT_EQ(m_memory[10], (alt_u32) 0xEFCDAB89);
    EXPECT_EQ(m_memory[11], (alt_u32) 0);

    // Verify memcpy results
    alt_u8* dest_u8_ptr = (alt_u8*) dest_ptr;
    EXPECT_EQ(dest_u8_ptr[0], (alt_u8) 0x89);
    EXPECT_EQ(dest_u8_ptr[1], (alt_u8) 0xAB);
    EXPECT_EQ(dest_u8_ptr[2], (alt_u8) 0xCD);
    EXPECT_EQ(dest_u8_ptr[3], (alt_u8) 0xEF);
}

TEST_F(RawPtrTest, test_alt_u32_memcpy)
{
    // Set up source memory blocks
    alt_u8 src_mem[8] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF};

    // Perform memcpy
    alt_u32* dest_ptr = &m_memory[10];
    alt_u32_memcpy(dest_ptr, (alt_u32*) src_mem, 8);

    // Verify memcpy results
    EXPECT_EQ(m_memory[9], (alt_u32) 0);
    EXPECT_EQ(m_memory[10], (alt_u32) 0x67452301);
    EXPECT_EQ(m_memory[11], (alt_u32) 0xEFCDAB89);
    EXPECT_EQ(m_memory[12], (alt_u32) 0);
}

TEST_F(RawPtrTest, test_alt_u32_memcpy_ordering)
{
    // Set up source memory blocks
    alt_u8 src_mem[8] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF};

    // Perform memcpy
    alt_u32* dest_ptr = &m_memory[10];
    alt_u32_memcpy(dest_ptr, (alt_u32*) src_mem, 8);

    // Verify memcpy results
    alt_u8* dest_u8_ptr = (alt_u8*) dest_ptr;
    EXPECT_EQ(dest_u8_ptr[0], (alt_u8) 0x01);
    EXPECT_EQ(dest_u8_ptr[1], (alt_u8) 0x23);
    EXPECT_EQ(dest_u8_ptr[2], (alt_u8) 0x45);
    EXPECT_EQ(dest_u8_ptr[3], (alt_u8) 0x67);
    EXPECT_EQ(dest_u8_ptr[4], (alt_u8) 0x89);
    EXPECT_EQ(dest_u8_ptr[5], (alt_u8) 0xAB);
    EXPECT_EQ(dest_u8_ptr[6], (alt_u8) 0xCD);
    EXPECT_EQ(dest_u8_ptr[7], (alt_u8) 0xEF);
}

TEST_F(RawPtrTest, test_alt_u32_memcpy_non_incr_bad_num_words)
{
    EXPECT_ANY_THROW({ alt_u32_memcpy_non_incr(m_memory, m_memory, 3); });
}

TEST_F(RawPtrTest, testalt_u32_memcpy_bad_num_words)
{
    EXPECT_ANY_THROW({ alt_u32_memcpy(m_memory, m_memory, 3); });
}
