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

class PFRMailboxTest : public testing::Test
{
public:
    alt_u32* m_memory = nullptr;

    virtual void SetUp()
    {
        SYSTEM_MOCK::get()->reset();
        m_memory = U_MAILBOX_AVMM_BRIDGE_ADDR;
    }

    virtual void TearDown() {}
};

TEST_F(PFRMailboxTest, test_basic_rw)
{
    initialize_mailbox();
    EXPECT_EQ(IORD(m_memory, MB_CPLD_STATIC_ID), (alt_u32) 0xDE);
    IOWR(m_memory, MB_RECOVERY_COUNT, 5);
    EXPECT_EQ(IORD(m_memory, MB_RECOVERY_COUNT), (alt_u32) 5);
    EXPECT_EQ(IORD(m_memory, MB_PANIC_EVENT_COUNT), (alt_u32) 0);
}

TEST_F(PFRMailboxTest, basic_fifo_test)
{
    IOWR(m_memory, MB_UFM_WRITE_FIFO, 1);
    EXPECT_EQ(IORD(m_memory, MB_UFM_WRITE_FIFO), (alt_u32) 1);
    EXPECT_EQ(IORD(m_memory, MB_UFM_READ_FIFO), (alt_u32) 1);
    EXPECT_EQ(IORD(m_memory, MB_UFM_WRITE_FIFO), (alt_u32) 0);
    EXPECT_EQ(IORD(m_memory, MB_UFM_READ_FIFO), (alt_u32) 0);
}

TEST_F(PFRMailboxTest, fifo_test_width)
{
    IOWR(m_memory, MB_UFM_WRITE_FIFO, 0xDEADBEEF);
    EXPECT_EQ(IORD(m_memory, MB_UFM_WRITE_FIFO), (alt_u32) 0xEF);
    EXPECT_EQ(IORD(m_memory, MB_UFM_READ_FIFO), (alt_u32) 0xEF);
}

TEST_F(PFRMailboxTest, fifo_test)
{
    for (int i = 1; i < 6; i++)
    {
        IOWR(m_memory, MB_UFM_WRITE_FIFO, i);
        EXPECT_EQ(IORD(m_memory, MB_UFM_WRITE_FIFO), (alt_u32) i);
    }
    IOWR(m_memory, MB_UFM_CMD_TRIGGER, 1 << 3);
    EXPECT_EQ(IORD(m_memory, MB_UFM_READ_FIFO), (alt_u32) 1);
    EXPECT_EQ(IORD(m_memory, MB_UFM_READ_FIFO), (alt_u32) 2);
    EXPECT_EQ(IORD(m_memory, MB_UFM_READ_FIFO), (alt_u32) 3);
    IOWR(m_memory, MB_UFM_CMD_TRIGGER, 1 << 1);
    EXPECT_EQ(IORD(m_memory, MB_UFM_WRITE_FIFO), (alt_u32) 0);
    EXPECT_EQ(IORD(m_memory, MB_UFM_READ_FIFO), (alt_u32) 0);
}

TEST_F(PFRMailboxTest, test_flush_mailbox_fifo)
{
    for (int i = 1; i < 6; i++)
    {
        IOWR(m_memory, MB_UFM_WRITE_FIFO, i);
        EXPECT_EQ(IORD(m_memory, MB_UFM_WRITE_FIFO), (alt_u32) i);
    }
    EXPECT_EQ(IORD(m_memory, MB_UFM_READ_FIFO), (alt_u32) 1);
    flush_mailbox_fifo();
    EXPECT_EQ(IORD(m_memory, MB_UFM_WRITE_FIFO), (alt_u32) 0);
    EXPECT_EQ(IORD(m_memory, MB_UFM_READ_FIFO), (alt_u32) 0);
}

TEST_F(PFRMailboxTest, test_read_from_mailbox_fifo)
{
    IOWR(m_memory, MB_UFM_WRITE_FIFO, 0xDEADBEEF);
    IOWR(m_memory, MB_UFM_WRITE_FIFO, 0xABCDEFFF);
    IOWR(m_memory, MB_UFM_WRITE_FIFO, 0x16DC8B90);

    EXPECT_EQ(read_from_mailbox_fifo(), (alt_u32) 0xEF);
    EXPECT_EQ(read_from_mailbox_fifo(), (alt_u32) 0xFF);
    EXPECT_EQ(read_from_mailbox_fifo(), (alt_u32) 0x90);
}

TEST_F(PFRMailboxTest, test_write_to_mailbox_fifo)
{
    write_to_mailbox_fifo(0xDEADBEEF);
    write_to_mailbox_fifo(0xABCDEFFF);
    write_to_mailbox_fifo(0x16DC8B90);

    EXPECT_EQ(IORD(m_memory, MB_UFM_READ_FIFO), (alt_u32) 0xEF);
    EXPECT_EQ(IORD(m_memory, MB_UFM_READ_FIFO), (alt_u32) 0xFF);
    EXPECT_EQ(IORD(m_memory, MB_UFM_READ_FIFO), (alt_u32) 0x90);
}
