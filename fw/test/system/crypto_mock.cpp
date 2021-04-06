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

// Standard headers
#include <string>
#include <unordered_map>
#include <openssl/ec.h>
#include <openssl/obj_mac.h>
#include <openssl/bn.h>
#include <openssl/objects.h>
#include <openssl/ecdsa.h>
#include <openssl/evp.h>

// Test headers
#include "bsp_mock.h"
#include "crypto_mock.h"

// Code headers

// Static data

// Type definitions

CRYPTO_MOCK::CRYPTO_MOCK() :
    m_crypto_state(CRYPTO_STATE::WAIT_CRYPTO_START),
    m_ec_or_sha(EC_OR_SHA_STATE::SHA_AND_EC),
    m_data_length(0),
    m_cur_transfer_size(0),
    m_crypto_data_idx(0),
    m_num_done_read_before_done(0),
    m_crypto_calc_pass(false)
{

    // Clear all vectors
    for (auto& it : m_crypto_data)
    {
        it.fill(0);
    }
    
    // Clear the calculated SHA array
    for (alt_u32 word_i = 0; word_i < (PFR_CRYPTO_LENGTH / 4); word_i++)
    {
        m_calculated_sha[word_i] = 0;
    }

    // OpenSSL Initialization
    OpenSSL_add_all_algorithms();

}

CRYPTO_MOCK::~CRYPTO_MOCK()
{
    EVP_cleanup();
}

alt_u32 CRYPTO_MOCK::get_mem_word(void* addr)
{
    alt_u32* addr_int = reinterpret_cast<alt_u32*>(addr);

    if (addr_int == CRYPTO_CSR_ADDR)
    {
        // CSR Interface
        // Word Address           | Description
        //----------------------------------------------------------------------
        // 0x01                   | 0: ECDSA + SHA start (WO)
        //                        | 1: ECDSA + SHA done (RO, cleared on go)
        //                        | 2: ECDSA signature good (RO, cleared on go)
        //                        | 3 : Reserved
        //                        | 4 : SHA-only start (WO)
        //                        | 5 : SHA done (Cleared on start)
        //                        | 31:6 : Reserved
        //----------------------------------------------------------------------
        // 0x02                   | Data (WO)
        //----------------------------------------------------------------------
        // 0x03                   | Data length in bytes, must be 64 byte aligned (lower 6 bits = 0) (WO)
        //----------------------------------------------------------------------
        // 0x08-0x0f              | 256 bit data register for SHA result (RO)
        //                        | address 0x08 is the lsbs (31:0), address 0x0f is the msbs (255:224)

        if (m_crypto_state == CRYPTO_STATE::CRYPTO_CALC_DONE)
        {
            alt_u32 ret = 0;

            // Wait a certain number of reads before returning done
            if (m_num_done_read_before_done == 0)
            {
                if (m_ec_or_sha == EC_OR_SHA_STATE::SHA_ONLY)
                {
                    ret |= CRYPTO_CSR_SHA_DONE_MSK;
                }
                else
                {
                    ret |= CRYPTO_CSR_EC_SHA_DONE_MSK;
                    if (m_crypto_calc_pass)
                    {
                        ret |= CRYPTO_CSR_EC_SHA_GOOD_MSK;
                    }
                }
            }
            else
            {
                m_num_done_read_before_done--;
                if (m_num_done_read_before_done == 0)
                {
                    compute_crypto_calculation();
                }
            }

            return ret;
        }
    }
    else if (addr_int == CRYPTO_DATA_ADDR)
    {
        PFR_INTERNAL_ERROR("It is illegal to read from CRYPTO_DATA_ADDR ");
    }
    else if (addr_int == CRYPTO_DATA_LEN_ADDR)
    {
        PFR_INTERNAL_ERROR("It is illegal to read from CRYPTO_DATA_LEN_ADDR ");
    }
    else if (addr_int == CRYPTO_DATA_SHA_ADDR(0))
    {
        return m_calculated_sha[7];
    }
    else if (addr_int == CRYPTO_DATA_SHA_ADDR(1))
    {
        return m_calculated_sha[6];
    }
    else if (addr_int == CRYPTO_DATA_SHA_ADDR(2))
    {
        return m_calculated_sha[5];
    }
    else if (addr_int == CRYPTO_DATA_SHA_ADDR(3))
    {
        return m_calculated_sha[4];
    }
    else if (addr_int == CRYPTO_DATA_SHA_ADDR(4))
    {
        return m_calculated_sha[3];
    }
    else if (addr_int == CRYPTO_DATA_SHA_ADDR(5))
    {
        return m_calculated_sha[2];
    }
    else if (addr_int == CRYPTO_DATA_SHA_ADDR(6))
    {
        return m_calculated_sha[1];
    }
    else if (addr_int == CRYPTO_DATA_SHA_ADDR(7))
    {
        return m_calculated_sha[0];
    }
    else
    {
        PFR_INTERNAL_ERROR("Undefined handler for address");
    }
    return 0;
}

void CRYPTO_MOCK::set_mem_word(void* addr, alt_u32 data)
{
    alt_u32* addr_int = reinterpret_cast<alt_u32*>(addr);

    if (addr_int == CRYPTO_CSR_ADDR)
    {
        if ((m_crypto_state != CRYPTO_STATE::WAIT_CRYPTO_START) && (m_crypto_state != CRYPTO_STATE::CRYPTO_CALC_DONE))
        {
            PFR_INTERNAL_ERROR("Illegal state when writing to CRYPTO_CSR_ADDR");
        }
        m_crypto_calc_pass = false;

        if (data & CRYPTO_CSR_EC_SHA_START_MSK)
        {
            // Start EC and SHA
            m_crypto_state = CRYPTO_STATE::ACCEPT_SHA_DATA;
            m_ec_or_sha = EC_OR_SHA_STATE::SHA_AND_EC;
            m_cur_transfer_size = m_data_length;
        }
        else if (data & CRYPTO_CSR_SHA_START_MSK)
        {
            // SHA only
            m_crypto_state = CRYPTO_STATE::ACCEPT_SHA_DATA;
            m_ec_or_sha = EC_OR_SHA_STATE::SHA_ONLY;
            m_cur_transfer_size = m_data_length;
        }
    }
    else if (addr_int == CRYPTO_DATA_LEN_ADDR)
    {
        if ((m_crypto_state != CRYPTO_STATE::WAIT_CRYPTO_START) && (m_crypto_state != CRYPTO_STATE::CRYPTO_CALC_DONE))
        {
            PFR_INTERNAL_ERROR("Illegal state when writing to CRYPTO_DATA_LEN_ADDR");
        }

        m_data_length = data;
        m_sha_data.resize(0);
        m_sha_data.reserve(m_data_length);
        for (auto& it : m_crypto_data)
        {
            it.fill(0);
        }
    }
    else if (addr_int == CRYPTO_DATA_ADDR)
    {
        if (m_crypto_state == CRYPTO_STATE::ACCEPT_SHA_DATA)
        {
            m_cur_transfer_size -= 4;

            m_sha_data.push_back((data >> 0) & 0xFF);
            m_sha_data.push_back((data >> 8) & 0xFF);
            m_sha_data.push_back((data >> 16) & 0xFF);
            m_sha_data.push_back((data >> 24) & 0xFF);

            if (m_cur_transfer_size == 0)
            {
                m_cur_transfer_size = 0;
                m_crypto_data_idx = 0;

                if (m_ec_or_sha == EC_OR_SHA_STATE::SHA_ONLY)
                {
                    m_crypto_state = CRYPTO_STATE::CRYPTO_CALC_DONE;

                   // Require 10 reads before returning done
                   m_num_done_read_before_done = 10;
                }
                else
                {
                    m_crypto_state = CRYPTO_STATE::ACCEPT_EC_DATA;
                }
            }
        }
        else if (m_crypto_state == CRYPTO_STATE::ACCEPT_EC_DATA)
        {
            m_crypto_data[m_crypto_data_idx][m_cur_transfer_size++] = ((data >> 0) & 0xFF);
            m_crypto_data[m_crypto_data_idx][m_cur_transfer_size++] = ((data >> 8) & 0xFF);
            m_crypto_data[m_crypto_data_idx][m_cur_transfer_size++] = ((data >> 16) & 0xFF);
            m_crypto_data[m_crypto_data_idx][m_cur_transfer_size++] = ((data >> 24) & 0xFF);

            if (m_cur_transfer_size == PFR_CRYPTO_LENGTH)
            {
                m_cur_transfer_size = 0;
                m_crypto_data_idx++;

                if (m_crypto_data_idx == m_crypto_data_elem)
                {
                    m_crypto_state = CRYPTO_STATE::CRYPTO_CALC_DONE;

                    // Require 10 reads before returning done
                    m_num_done_read_before_done = 10;
                }
            }
        }
        else
        {
            PFR_INTERNAL_ERROR("Unhandled SHA state for write to CRYPTO_DATA_ADDR");
        }
    }
    else
    {
        PFR_INTERNAL_ERROR("Undefined handler for address");
    }
}

void CRYPTO_MOCK::reset()
{
    m_data_length = 0;
    m_cur_transfer_size = 0;

    // Resize the sha data to reallocate
    m_sha_data.resize(0);

    // Clear all vectors
    for (auto it : m_crypto_data)
    {
        it.fill(0);
    }
}

bool CRYPTO_MOCK::is_addr_in_range(void* addr)
{
    return MEMORY_MOCK_IF::is_addr_in_range(addr, __IO_CALC_ADDRESS_NATIVE_ALT_U32(U_CRYPTO_AVMM_BRIDGE_BASE, 0), U_CRYPTO_AVMM_BRIDGE_SPAN);
}

void CRYPTO_MOCK::compute_crypto_calculation()
{
    m_crypto_calc_pass = false;
    if (m_ec_or_sha == EC_OR_SHA_STATE::SHA_ONLY)
    {
        unsigned int md_len;
        unsigned char md_value[256];

        EVP_MD_CTX* mdctx = EVP_MD_CTX_create();
        const EVP_MD* md = EVP_sha256();

        // Initialize the SHA
        EVP_DigestInit_ex(mdctx, md, nullptr);

        // Create the SHA on the block
        EVP_DigestUpdate(mdctx, m_sha_data.data(), m_sha_data.size());

        // Finalize the digest
        EVP_DigestFinal_ex(mdctx, md_value, &md_len);
        PFR_ASSERT(md_len == PFR_CRYPTO_LENGTH);

        // Compare to the expected value
        alt_u8* m_calculated_sha_ptr = (alt_u8*) m_calculated_sha;
        for (int i = 0; i < PFR_CRYPTO_LENGTH; i++)
        {
            m_calculated_sha_ptr[i] = md_value[i];
        }

        EVP_MD_CTX_destroy(mdctx);
    }
    else
    {
        BIGNUM* bn_bx;
        BIGNUM* bn_by;
        BIGNUM* bn_r;
        BIGNUM* bn_s;
        unsigned int md_len;
        unsigned char md_value[256];

        EVP_MD_CTX* mdctx = EVP_MD_CTX_create();
        const EVP_MD* md = EVP_sha256();

        // Initialize the SHA
        EVP_DigestInit_ex(mdctx, md, nullptr);

        // Create the SHA on the block
        EVP_DigestUpdate(mdctx, m_sha_data.data(), m_sha_data.size());

        // Finalize the digest
        EVP_DigestFinal_ex(mdctx, md_value, &md_len);
        PFR_ASSERT(md_len == PFR_CRYPTO_LENGTH);
        EVP_MD_CTX_destroy(mdctx);

        // Set the public key in the EC key
        bn_bx = BN_bin2bn(m_crypto_data[2].data(), PFR_CRYPTO_LENGTH, nullptr);
        bn_by = BN_bin2bn(m_crypto_data[3].data(), PFR_CRYPTO_LENGTH, nullptr);
        bn_r = BN_bin2bn(m_crypto_data[7].data(), PFR_CRYPTO_LENGTH, nullptr);
        bn_s = BN_bin2bn(m_crypto_data[8].data(), PFR_CRYPTO_LENGTH, nullptr);

        // Set the public key components in EC_KEY
        EC_KEY* eckey = EC_KEY_new_by_curve_name(NID_X9_62_prime256v1);
        EC_KEY_set_public_key_affine_coordinates(eckey, bn_bx, bn_by);

        // Set the R/S in the signature struct
        ECDSA_SIG sig;
        sig.r = bn_r;
        sig.s = bn_s;

        // Test the signature
        m_crypto_calc_pass = (ECDSA_do_verify(md_value, PFR_CRYPTO_LENGTH, &sig, eckey) == 1);

        // Clean up
        EC_KEY_free(eckey);
        BN_free(bn_bx);
        BN_free(bn_by);
        BN_free(bn_r);
        BN_free(bn_s);
    }
}
