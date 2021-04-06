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

class RFNVRamUtilsTest : public testing::Test
{
public:
    alt_u32* m_rfnvram_memory = nullptr;

    virtual void SetUp() {
        SYSTEM_MOCK::get()->reset();
        m_rfnvram_memory = U_RFNVRAM_SMBUS_MASTER_ADDR;
    }

    virtual void TearDown() {  }
};

TEST_F(RFNVRamUtilsTest, test_basic_rw)
{
    // write to addr 0
    IOWR(m_rfnvram_memory, 0, 0x2DC);
    IOWR(m_rfnvram_memory, 0, 0x00);
    IOWR(m_rfnvram_memory, 0, 0x00);
    IOWR(m_rfnvram_memory, 0, 0xAB);
    IOWR(m_rfnvram_memory, 0, 0x1DC);
    // read from addr 0
    IOWR(m_rfnvram_memory, 0, 0x2DC);
    IOWR(m_rfnvram_memory, 0, 0x00);
    IOWR(m_rfnvram_memory, 0, 0x00);
    IOWR(m_rfnvram_memory, 0, 0x2DD);
    IOWR(m_rfnvram_memory, 0, 0x00);
    IOWR(m_rfnvram_memory, 0, 0x100);

    while (IORD_32DIRECT(RFNVRAM_RX_FIFO_BYTES_LEFT, 0) != 2) {}

    EXPECT_EQ(IORD(m_rfnvram_memory, 1), (alt_u32) 0xAB);
    EXPECT_EQ(IORD(m_rfnvram_memory, 1), (alt_u32) 0xDC);
}

TEST_F(RFNVRamUtilsTest, test_read_pit_id)
{
    alt_u8 example_pit_id[RFNVRAM_PIT_ID_LENGTH] = {0x1b, 0x2a, 0xef, 0xa8, 0x11, 0x5c, 0x80, 0x37};
    alt_u8 read_pit_id[RFNVRAM_PIT_ID_LENGTH] = {};

    // Write PIT ID
    IOWR(m_rfnvram_memory, 0, 0x2DC);
    IOWR(m_rfnvram_memory, 0, RFNVRAM_PIT_ID_MSB_OFFSET);
    IOWR(m_rfnvram_memory, 0, RFNVRAM_PIT_ID_LSB_OFFSET);
    for (alt_u32 byte_i = 0; byte_i < RFNVRAM_PIT_ID_LENGTH - 1; byte_i++)
    {
        IOWR(m_rfnvram_memory, 0, example_pit_id[byte_i]);
    }
    IOWR(m_rfnvram_memory, 0, 0x100 | example_pit_id[RFNVRAM_PIT_ID_LENGTH - 1]);

    // Read PIT ID
    read_from_rfnvram((alt_u8*) read_pit_id, RFNVRAM_PIT_ID_OFFSET, RFNVRAM_PIT_ID_LENGTH);

    // Check results
    for (alt_u32 byte_i = 0; byte_i < RFNVRAM_PIT_ID_LENGTH; byte_i++)
    {
        EXPECT_EQ(example_pit_id[byte_i], read_pit_id[byte_i]);
    }
}

TEST_F(RFNVRamUtilsTest, test_write_then_read_pit_id)
{
    alt_u8 example_pit_id[RFNVRAM_PIT_ID_LENGTH] = {0x71, 0xb9, 0xef, 0xa8, 0x11, 0x54, 0x80, 0x33};
    alt_u8 read_pit_id[RFNVRAM_PIT_ID_LENGTH] = {};

    // Save PIT ID to UFM
    alt_u8* ufm_pit_id = (alt_u8*) get_ufm_pfr_data()->pit_id;
    for (alt_u32 byte_i = 0; byte_i < RFNVRAM_PIT_ID_LENGTH; byte_i++)
    {
        ufm_pit_id[byte_i] = example_pit_id[byte_i];
    }

    // Write PIT ID from UFM to RFNVRAM
    write_ufm_pit_id_to_rfnvram();

    // Read PIT ID
    read_from_rfnvram((alt_u8*) read_pit_id, RFNVRAM_PIT_ID_OFFSET, RFNVRAM_PIT_ID_LENGTH);

    // Check results
    for (alt_u32 byte_i = 0; byte_i < RFNVRAM_PIT_ID_LENGTH; byte_i++)
    {
        EXPECT_EQ(example_pit_id[byte_i], read_pit_id[byte_i]);
    }
}
