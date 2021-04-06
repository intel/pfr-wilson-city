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

class PFMTest : public testing::Test
{
public:
    alt_u32 m_raw_pfm_nbytes = GEN_256B_PFM_FILE_SIZE;
    alt_u8 m_raw_pfm_x86[GEN_256B_PFM_FILE_SIZE];

    virtual void SetUp()
    {
        SYSTEM_MOCK* sys = SYSTEM_MOCK::get();
        sys->reset();

        // Load PFM
        sys->init_x86_mem_from_file(GEN_256B_PFM_FILE, (alt_u32*) m_raw_pfm_x86);
    }

    virtual void TearDown() {}
};

TEST_F(PFMTest, test_basic)
{
    alt_u32* m_raw_pfm_x86_u32_ptr = (alt_u32*) m_raw_pfm_x86;
    EXPECT_EQ(*m_raw_pfm_x86_u32_ptr, alt_u32(PFM_MAGIC));
}

TEST_F(PFMTest, test_pfm_struct)
{
    PFM* pfm = (PFM*) m_raw_pfm_x86;

    EXPECT_EQ(pfm->tag, alt_u32(PFM_MAGIC));
    EXPECT_EQ(pfm->svn, (alt_u8) 0x03);
    EXPECT_EQ(pfm->bkc_version, (alt_u8) 0x01);
    EXPECT_EQ(pfm->major_rev, (alt_u8) 0x01);
    EXPECT_EQ(pfm->minor_rev, (alt_u8) 0x01);
    EXPECT_EQ(pfm->length, (alt_u32) m_raw_pfm_nbytes);
}

TEST_F(PFMTest, test_pfm_spi_region_definition)
{
    PFM* pfm = (PFM*) m_raw_pfm_x86;
    PFM_SPI_REGION_DEF* rule_def = (PFM_SPI_REGION_DEF*) (pfm->pfm_body);

    EXPECT_EQ(rule_def->def_type, alt_u32(SPI_REGION_DEF_TYPE));
    EXPECT_EQ(rule_def->hash_algorithm, alt_u32(PFM_HASH_ALGO_SHA256_MASK));
    EXPECT_EQ(rule_def->start_addr, alt_u32(0x4000));
    EXPECT_EQ(rule_def->end_addr, alt_u32(0x5C000));
}
