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

#ifndef INC_SYSTEM_CRYPTO_MOCK_H
#define INC_SYSTEM_CRYPTO_MOCK_H

// Standard headers
#include <memory>
#include <vector>
#include <array>

// Mock headers
#include "alt_types_mock.h"
#include "memory_mock.h"

// BSP headers
#include "pfr_sys.h"
#include "crypto.h"

class CRYPTO_MOCK : public MEMORY_MOCK_IF
{
public:
    CRYPTO_MOCK();
    virtual ~CRYPTO_MOCK();

    alt_u32 get_mem_word(void* addr) override;
    void set_mem_word(void* addr, alt_u32 data) override;

    void reset() override;

    bool is_addr_in_range(void* addr) override;

private:
    enum class CRYPTO_STATE
    {
        WAIT_CRYPTO_START,

        ACCEPT_SHA_DATA,
        ACCEPT_EC_DATA,

        CRYPTO_CALC_DONE
    };

    enum class EC_OR_SHA_STATE
    {
        SHA_ONLY,
        SHA_AND_EC
    };

    void compute_crypto_calculation();

    CRYPTO_STATE m_crypto_state;
    EC_OR_SHA_STATE m_ec_or_sha;

    std::vector<alt_u8> m_sha_data;
    const alt_u32 m_crypto_data_elem = 9;
    std::array<std::array<alt_u8, PFR_CRYPTO_LENGTH>, 9> m_crypto_data;

    alt_u32 m_data_length;
    alt_u32 m_cur_transfer_size;
    alt_u32 m_crypto_data_idx;
    alt_u32 m_num_done_read_before_done;
    alt_u32 m_calculated_sha[PFR_CRYPTO_LENGTH / 4];
    bool m_crypto_calc_pass;
};

#endif /* INC_SYSTEM_CRYPTO_MOCK_H */
