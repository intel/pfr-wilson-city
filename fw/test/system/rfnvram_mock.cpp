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

// Test headers
#include "bsp_mock.h"
#include "rfnvram_mock.h"

// Code headers

// Static data

// Type definitions

// Constructor/Destructor
RFNVRAM_MOCK::RFNVRAM_MOCK() {}
RFNVRAM_MOCK::~RFNVRAM_MOCK() {}

// Class methods

void RFNVRAM_MOCK::reset()
{
    m_ram.reset();
}

bool RFNVRAM_MOCK::is_addr_in_range(void* addr)
{
    return m_ram.is_addr_in_range(addr);
}

alt_u32 RFNVRAM_MOCK::get_mem_word(void* addr)
{
    if (addr == RFNVRAM_RX_FIFO)
    {
        alt_u32 ret_data = (alt_u32) m_read_fifo.front();
        m_read_fifo.pop();
        return ret_data;
    }
    else if (addr == RFNVRAM_TX_FIFO_BYTES_LEFT)
    {
        return m_cmd_fifo.size();
    }
    else if (addr == RFNVRAM_RX_FIFO_BYTES_LEFT)
    {
        return m_read_fifo.size();
    }
    return 0;
}

void RFNVRAM_MOCK::set_mem_word(void* addr, alt_u32 data)
{
    if (addr == U_RFNVRAM_SMBUS_MASTER_ADDR)
    {
        m_cmd_fifo.push(data);

        // simulate all the i2c commands sent to the IP
        // TODO move this function to a separate thread
        simulate_i2c();
    }
    // else do nothing
}

void RFNVRAM_MOCK::simulate_i2c()
{
    int cmd;

    while (!m_cmd_fifo.empty())
    {
        cmd = m_cmd_fifo.front();
        m_cmd_fifo.pop();
        switch (state)
        {
            case IDLE:
                // if it is start + write go into write internal addr state
                if ((cmd & (1 << 9)) && (cmd & 0xFF) == RFNVRAM_SMBUS_ADDR)
                {
                    state = WRITE_ADDR_UPPER;
                }
                else if ((cmd & (1 << 9)) && (cmd & 0xFF) == (RFNVRAM_SMBUS_ADDR | 0x1))
                {
                    state = READ_DATA;
                }
                break;
            case WRITE_ADDR_UPPER:
                internal_addr = (cmd & 0x3) << 8;
                // return to idle if stop is detected
                if (cmd & (1 << 8))
                    state = IDLE;
                else
                    state = WRTIE_ADDR_LOWER;
                break;
            case WRTIE_ADDR_LOWER:
                internal_addr = internal_addr | (cmd & 0xFF);
                if (cmd & (1 << 8))
                    state = IDLE;
                else if ((cmd & (1 << 9)) && (cmd & 0xFF) == (RFNVRAM_SMBUS_ADDR | 0x1))
                    state = READ_DATA;
                else
                    state = WRITE_DATA;
                break;
            case WRITE_DATA:
                if (cmd & (1 << 9))
                {
                    if ((cmd & 0xFF) == (RFNVRAM_SMBUS_ADDR | 0x1))
                    {
                        state = READ_DATA;
                    }
                    else
                    {
                        // start condition but no addr match
                        state = IDLE;
                    }
                }
                else if (cmd & (1 << 8))
                {
                    rfnvram_mem[internal_addr++] = cmd & 0xFF;
                    state = IDLE;
                }
                else
                {
                    rfnvram_mem[internal_addr++] = cmd & 0xFF;
                    state = WRITE_DATA;
                }
                break;

            case READ_DATA:
                m_read_fifo.push(rfnvram_mem[internal_addr++]);

                if (cmd & (1 << 8))
                    state = IDLE;
                else if ((cmd & (1 << 9)) && (cmd & 0xFF) == (RFNVRAM_SMBUS_ADDR))
                    state = WRITE_DATA;
                else
                    state = READ_DATA;
                break;

            default:
                state = IDLE;
                break;
        }
    }
}
