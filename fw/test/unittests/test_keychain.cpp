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

class KeychainTest : public testing::Test
{
public:
    alt_u32 raw_keychain_nbytes = 1024;
    alt_u8 m_raw_data_x86[SIGNED_BINARY_BLOCKSIGN_FILE_SIZE];
    alt_u8 m_raw_can_cert_keychain_x86[KEY_CAN_CERT_FILE_SIZE];

    virtual void SetUp()
    {
        SYSTEM_MOCK* sys = SYSTEM_MOCK::get();
        sys->reset();

        // Load Blocksign tool signed payload to memory
        sys->init_x86_mem_from_file(SIGNED_BINARY_BLOCKSIGN_FILE, (alt_u32*) m_raw_data_x86);
        sys->init_x86_mem_from_file(KEY_CAN_CERT_PCH_PFM_KEY2, (alt_u32*) m_raw_can_cert_keychain_x86);
    }

    virtual void TearDown() {}
};

TEST_F(KeychainTest, test_sanity)
{
    EXPECT_EQ(SIGNATURE_SIZE, 1024);
    EXPECT_EQ(SIGNATURE_SIZE, BLOCK0_SIZE + BLOCK1_SIZE);
    EXPECT_EQ(sizeof(KCH_SIGNATURE), (unsigned int) 1024);

    EXPECT_EQ(sizeof(KCH_BLOCK1), (unsigned int) BLOCK1_SIZE);
    EXPECT_EQ(sizeof(KCH_BLOCK1_ROOT_ENTRY), (unsigned int) BLOCK1_ROOT_ENTRY_SIZE);
    EXPECT_EQ(sizeof(KCH_BLOCK1_CSK_ENTRY), (unsigned int) BLOCK1_CSK_ENTRY_SIZE);
    EXPECT_EQ(sizeof(KCH_BLOCK1_B0_ENTRY), (unsigned int) BLOCK1_B0_ENTRY_SIZE);

    // key chaining structure to authenticate PFM/update capsule payload
    int calc_b1_size = BLOCK1_HEADER_SIZE + BLOCK1_ROOT_ENTRY_SIZE + BLOCK1_CSK_ENTRY_SIZE +
                       BLOCK1_B0_ENTRY_SIZE + BLOCK1_RESERVED_SIZE;
    EXPECT_EQ(calc_b1_size, BLOCK1_SIZE);

    EXPECT_TRUE(BLOCK1_CSK_ENTRY_HASH_REGION_SIZE < BLOCK1_CSK_ENTRY_SIZE);
}

TEST_F(KeychainTest, test_keychain)
{
    KCH_SIGNATURE* kc = (KCH_SIGNATURE*) m_raw_data_x86;
    KCH_BLOCK0* b0 = &kc->b0;
    EXPECT_EQ(b0->magic, (alt_u32) BLOCK0_MAGIC);
    EXPECT_FALSE(b0->pc_type & KCH_PC_TYPE_KEY_CAN_CERT_MASK);

    KCH_BLOCK1* b1 = &kc->b1;
    EXPECT_EQ(b1->magic, (alt_u32) BLOCK1_MAGIC);

    KCH_BLOCK1_ROOT_ENTRY* root_entry = &b1->root_entry;
    EXPECT_EQ(root_entry->magic, (alt_u32) BLOCK1_ROOT_ENTRY_MAGIC);
    EXPECT_EQ(root_entry->curve_magic, (alt_u32) CURVE_SECP256R1_MAGIC);

    KCH_BLOCK1_CSK_ENTRY* csk_entry = &b1->csk_entry;
    EXPECT_EQ(csk_entry->magic, (alt_u32) BLOCK1_CSK_ENTRY_MAGIC);
    EXPECT_EQ(csk_entry->curve_magic, (alt_u32) CURVE_SECP256R1_MAGIC);

    KCH_BLOCK1_B0_ENTRY* b0_entry = &b1->b0_entry;
    EXPECT_EQ(b0_entry->magic, (alt_u32) BLOCK1_B0_ENTRY_MAGIC);
    EXPECT_EQ(b0_entry->sig_magic, (alt_u32) SIG_SECP256R1_MAGIC);
}

