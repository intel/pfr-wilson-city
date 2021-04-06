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
#include <openssl/ec.h>
#include <openssl/obj_mac.h>
#include <openssl/bn.h>
#include <openssl/objects.h>
#include <openssl/ecdsa.h>
#include <openssl/evp.h>

// Include the GTest headers
#include "gtest_headers.h"

// Include the SYSTEM MOCK and PFR headers
#include "ut_nios_wrapper.h"

class PFRCryptoTest : public testing::Test
{
public:
    virtual void SetUp() { SYSTEM_MOCK::get()->reset(); }

    virtual void TearDown() {}
};

TEST_F(PFRCryptoTest, test_crypto_block_ready)
{
    // After reset the CSR should be 0
    EXPECT_EQ(IORD(CRYPTO_CSR_ADDR, 0), (alt_u32) 0);
}

TEST_F(PFRCryptoTest, test_crypto_mock_read_all_addr)
{
    // Set asserts to throw as opposed to abort
    SYSTEM_MOCK::get()->set_assert_to_throw();

    EXPECT_NO_THROW(IORD(CRYPTO_CSR_ADDR, 0));

    EXPECT_ANY_THROW(IORD(CRYPTO_DATA_ADDR, 0));
    EXPECT_ANY_THROW(IORD(CRYPTO_DATA_LEN_ADDR, 0));

    EXPECT_NO_THROW(IORD(CRYPTO_DATA_SHA_ADDR(0), 0));
    EXPECT_NO_THROW(IORD(CRYPTO_DATA_SHA_ADDR(1), 0));
    EXPECT_NO_THROW(IORD(CRYPTO_DATA_SHA_ADDR(2), 0));
    EXPECT_NO_THROW(IORD(CRYPTO_DATA_SHA_ADDR(3), 0));
    EXPECT_NO_THROW(IORD(CRYPTO_DATA_SHA_ADDR(4), 0));
    EXPECT_NO_THROW(IORD(CRYPTO_DATA_SHA_ADDR(5), 0));
    EXPECT_NO_THROW(IORD(CRYPTO_DATA_SHA_ADDR(6), 0));
    EXPECT_NO_THROW(IORD(CRYPTO_DATA_SHA_ADDR(7), 0));
    EXPECT_ANY_THROW(IORD(CRYPTO_DATA_SHA_ADDR(8), 0));
}

TEST_F(PFRCryptoTest, test_64_bytes_data_sha_only)
{
    const alt_u32 td_sha_data_len = 64;

    const alt_u8 td_crypto_data[64] = {
        0xf4, 0x19, 0x76, 0xe9, 0xeb, 0xd2, 0xab, 0x72, 0x8a, 0x78, 0xe1, 0x38,
        0x81, 0x17, 0xb7, 0xf6, 0x8d, 0xd8, 0xa3, 0x6c, 0xbf, 0xfa, 0xd0, 0x7d,
        0xde, 0x60, 0x74, 0xc1, 0x65, 0xfa, 0x42, 0x65, 0x49, 0x29, 0x95, 0x1b,
        0xef, 0xb9, 0x83, 0xf0, 0xf6, 0xc1, 0x46, 0xd6, 0x15, 0x73, 0xfa, 0x09,
        0x75, 0xc6, 0x4e, 0x64, 0xf1, 0xde, 0x34, 0x5f, 0x89, 0xf0, 0x61, 0x5d,
        0x61, 0xb4, 0x08, 0x9d};

    const alt_u8 td_expected_hash[PFR_CRYPTO_LENGTH] = {
        0x6b, 0xa4, 0xa6, 0x98, 0x53, 0x63, 0xf0, 0xe7,
        0xe9, 0x98, 0x76, 0x62, 0x7d, 0xe7, 0x12, 0x41, 0xda, 0xab, 0x4b, 0x96,
        0xbd, 0x67, 0x99, 0x82, 0x81, 0x40, 0x27, 0x87, 0xa5, 0x10, 0x6e, 0x73,};

    alt_u32* td_crypto_data_ptr = (alt_u32*) td_crypto_data;
    alt_u32* td_expected_hash_ptr = (alt_u32*) td_expected_hash;

    EXPECT_EQ(alt_u32(1), verify_sha(td_expected_hash_ptr, td_crypto_data_ptr, td_sha_data_len));
}

TEST_F(PFRCryptoTest, test_single_block_pattern_sha_only)
{
    // ###################################################################################################################################
    // Generating pattern 0 of 128 bytes (1024 bits)
    // Test Data block 0:
    // 1024'h44BD5317988365A3924CD72EE0C8EC394024F6236D173B3BE4F3EFCB510B34CA9EF2F3BFF6C48E2AD772580FB27D4160AB8F26B1B61D876C6C73F75A1F789CFFE4D3DB20751AF87E9172E9A75BDC99FD964108B7E7B7D2F43F065AA5FEEEDE899CE34E4C4B136CB8F8ADF8B3538731231CA87A01425299B7DC7B7E0522C859EC
    // Digest : 256'hA37C4FD5F1F46D462091B9E92D22C6E9CE3097EFE9CD4B25E8F23A2DDD36DDFC
    // Generating curve
    // Using curve prime256v1
    // Generating private key
    // Private Key:
    //      256'hA07DF98A6BE1EA1E6B9C132E4E04A5AB652AEB7A660BC447943B56E7B8B62F19
    // Pubkey:
    //  CX : 256'hCD021AE9F1D8D74B4B11ABF9667CCE94
    //  CY : 256'h8F09B8B0EB68B4D9D447A39EDD1F3A0A

    // echo
    // "44BD5317988365A3924CD72EE0C8EC394024F6236D173B3BE4F3EFCB510B34CA9EF2F3BFF6C48E2AD772580FB27D4160AB8F26B1B61D876C6C73F75A1F789CFFE4D3DB20751AF87E9172E9A75BDC99FD964108B7E7B7D2F43F065AA5FEEEDE899CE34E4C4B136CB8F8ADF8B3538731231CA87A01425299B7DC7B7E0522C859EC"
    // | xxd -r -p | openssl dgst -sha256 (stdin)=
    // a37c4fd5f1f46d462091b9e92d22c6e9ce3097efe9cd4b25e8f23a2ddd36ddfc

    // Generate for C:
    // echo
    // "44BD5317988365A3924CD72EE0C8EC394024F6236D173B3BE4F3EFCB510B34CA9EF2F3BFF6C48E2AD772580FB27D4160AB8F26B1B61D876C6C73F75A1F789CFFE4D3DB20751AF87E9172E9A75BDC99FD964108B7E7B7D2F43F065AA5FEEEDE899CE34E4C4B136CB8F8ADF8B3538731231CA87A01425299B7DC7B7E0522C859EC"
    // | xxd -r -p | xxd -i echo "A37C4FD5F1F46D462091B9E92D22C6E9CE3097EFE9CD4B25E8F23A2DDD36DDFC"
    // | xxd -r -p | xxd -i

    const alt_u32 td_sha_data_len = 128;

    const alt_u8 td_crypto_data[128] = {
        0x44, 0xbd, 0x53, 0x17, 0x98, 0x83, 0x65, 0xa3, 0x92, 0x4c, 0xd7, 0x2e, 0xe0, 0xc8, 0xec,
        0x39, 0x40, 0x24, 0xf6, 0x23, 0x6d, 0x17, 0x3b, 0x3b, 0xe4, 0xf3, 0xef, 0xcb, 0x51, 0x0b,
        0x34, 0xca, 0x9e, 0xf2, 0xf3, 0xbf, 0xf6, 0xc4, 0x8e, 0x2a, 0xd7, 0x72, 0x58, 0x0f, 0xb2,
        0x7d, 0x41, 0x60, 0xab, 0x8f, 0x26, 0xb1, 0xb6, 0x1d, 0x87, 0x6c, 0x6c, 0x73, 0xf7, 0x5a,
        0x1f, 0x78, 0x9c, 0xff, 0xe4, 0xd3, 0xdb, 0x20, 0x75, 0x1a, 0xf8, 0x7e, 0x91, 0x72, 0xe9,
        0xa7, 0x5b, 0xdc, 0x99, 0xfd, 0x96, 0x41, 0x08, 0xb7, 0xe7, 0xb7, 0xd2, 0xf4, 0x3f, 0x06,
        0x5a, 0xa5, 0xfe, 0xee, 0xde, 0x89, 0x9c, 0xe3, 0x4e, 0x4c, 0x4b, 0x13, 0x6c, 0xb8, 0xf8,
        0xad, 0xf8, 0xb3, 0x53, 0x87, 0x31, 0x23, 0x1c, 0xa8, 0x7a, 0x01, 0x42, 0x52, 0x99, 0xb7,
        0xdc, 0x7b, 0x7e, 0x05, 0x22, 0xc8, 0x59, 0xec};

    const alt_u8 td_expected_hash[PFR_CRYPTO_LENGTH] = {
        0xa3, 0x7c, 0x4f, 0xd5, 0xf1, 0xf4, 0x6d, 0x46, 0x20, 0x91, 0xb9,
        0xe9, 0x2d, 0x22, 0xc6, 0xe9, 0xce, 0x30, 0x97, 0xef, 0xe9, 0xcd,
        0x4b, 0x25, 0xe8, 0xf2, 0x3a, 0x2d, 0xdd, 0x36, 0xdd, 0xfc};

    alt_u32* td_crypto_data_ptr = (alt_u32*) td_crypto_data;
    alt_u32* td_expected_hash_ptr = (alt_u32*) td_expected_hash;

    EXPECT_EQ(alt_u32(1), verify_sha(td_expected_hash_ptr, td_crypto_data_ptr, td_sha_data_len));
}

TEST_F(PFRCryptoTest, test_single_block_pattern_sha_and_ec)
{
    /*
     * Test Data block :
     1024'h863A608FDC3B3965A80A8227439BA56CA04B0E7ECE832B29A58880CF8D08275AEFB121D3B7EC96C3CE5C1B63D198D3397206D7A08185148DE4468D113EC8D938F2EB8EB6056F99CA325012E3BCC5AF27082BECD8B63DAF37412F941BAC089930E4C0D901DFC6B3DA87029FDA4D3B258DCCB80B962E0350ECBD517F28DAB6DD09
     * Digest : 256'hFB62DED71EE9009AD54A9885DE6EE6CB5C7D261DB13320DC919FAA1DCEAD89AF
     * Generating curve
     * Using curve prime256v1
     * Private Key:
         256'hC51E4753AFDEC1E6B6C6A5B992F43F8DD0C7A8933072708B6522468B2FFB06FD
     * Pubkey:
     *   CX : 256'h942C9F408EAD9D82D34A1B9A6A827EBE3E2DDF782B448D23BE1B6143988CCEF4
     *   CY : 256'h8C9EAF6C0D14D992FC63BAD3E2496BE2EEE61CB5B97F65F428CA94A5D0EE19A1
     * Hash:
     *   E :  256'hFB62DED71EE9009AD54A9885DE6EE6CB5C7D261DB13320DC919FAA1DCEAD89AF
     * Signature:
     *   R :  256'h6BD9606BFF8142C667531604E15AA03911474514DFDFDF84EBEB2DA857C83377
     *   S :  256'h4FD46C71183C2BB7CA47030CEE81E2EEC9E317B912243325F273855A66C76D5E
     *
     * Generate for C:
     * % echo "4FD46C71183C2BB7CA47030CEE81E2EEC9E317B912243325F273855A66C76D5E" | xxd -r -p | xxd
     -i
     */
    const alt_u32 test_data_size = 128;

    const alt_u8 test_data[128] = {
        0x86, 0x3a, 0x60, 0x8f, 0xdc, 0x3b, 0x39, 0x65, 0xa8, 0x0a, 0x82, 0x27, 0x43, 0x9b, 0xa5,
        0x6c, 0xa0, 0x4b, 0x0e, 0x7e, 0xce, 0x83, 0x2b, 0x29, 0xa5, 0x88, 0x80, 0xcf, 0x8d, 0x08,
        0x27, 0x5a, 0xef, 0xb1, 0x21, 0xd3, 0xb7, 0xec, 0x96, 0xc3, 0xce, 0x5c, 0x1b, 0x63, 0xd1,
        0x98, 0xd3, 0x39, 0x72, 0x06, 0xd7, 0xa0, 0x81, 0x85, 0x14, 0x8d, 0xe4, 0x46, 0x8d, 0x11,
        0x3e, 0xc8, 0xd9, 0x38, 0xf2, 0xeb, 0x8e, 0xb6, 0x05, 0x6f, 0x99, 0xca, 0x32, 0x50, 0x12,
        0xe3, 0xbc, 0xc5, 0xaf, 0x27, 0x08, 0x2b, 0xec, 0xd8, 0xb6, 0x3d, 0xaf, 0x37, 0x41, 0x2f,
        0x94, 0x1b, 0xac, 0x08, 0x99, 0x30, 0xe4, 0xc0, 0xd9, 0x01, 0xdf, 0xc6, 0xb3, 0xda, 0x87,
        0x02, 0x9f, 0xda, 0x4d, 0x3b, 0x25, 0x8d, 0xcc, 0xb8, 0x0b, 0x96, 0x2e, 0x03, 0x50, 0xec,
        0xbd, 0x51, 0x7f, 0x28, 0xda, 0xb6, 0xdd, 0x09};

    const alt_u8 test_pubkey_cx[PFR_CRYPTO_LENGTH] = {
        0x94, 0x2c, 0x9f, 0x40, 0x8e, 0xad, 0x9d, 0x82, 0xd3, 0x4a, 0x1b,
        0x9a, 0x6a, 0x82, 0x7e, 0xbe, 0x3e, 0x2d, 0xdf, 0x78, 0x2b, 0x44,
        0x8d, 0x23, 0xbe, 0x1b, 0x61, 0x43, 0x98, 0x8c, 0xce, 0xf4};

    const alt_u8 test_pubkey_cy[PFR_CRYPTO_LENGTH] = {
        0x8c, 0x9e, 0xaf, 0x6c, 0x0d, 0x14, 0xd9, 0x92, 0xfc, 0x63, 0xba,
        0xd3, 0xe2, 0x49, 0x6b, 0xe2, 0xee, 0xe6, 0x1c, 0xb5, 0xb9, 0x7f,
        0x65, 0xf4, 0x28, 0xca, 0x94, 0xa5, 0xd0, 0xee, 0x19, 0xa1};

    const alt_u8 test_sig_r[PFR_CRYPTO_LENGTH] = {0x6b, 0xd9, 0x60, 0x6b, 0xff, 0x81, 0x42, 0xc6,
                                                  0x67, 0x53, 0x16, 0x04, 0xe1, 0x5a, 0xa0, 0x39,
                                                  0x11, 0x47, 0x45, 0x14, 0xdf, 0xdf, 0xdf, 0x84,
                                                  0xeb, 0xeb, 0x2d, 0xa8, 0x57, 0xc8, 0x33, 0x77};

    const alt_u8 test_sig_s[PFR_CRYPTO_LENGTH] = {0x4f, 0xd4, 0x6c, 0x71, 0x18, 0x3c, 0x2b, 0xb7,
                                                  0xca, 0x47, 0x03, 0x0c, 0xee, 0x81, 0xe2, 0xee,
                                                  0xc9, 0xe3, 0x17, 0xb9, 0x12, 0x24, 0x33, 0x25,
                                                  0xf2, 0x73, 0x85, 0x5a, 0x66, 0xc7, 0x6d, 0x5e};

    EXPECT_EQ(alt_u32(1),
              verify_ecdsa_and_sha((alt_u32*) test_pubkey_cx,
                                   (alt_u32*) test_pubkey_cy,
                                   (alt_u32*) test_sig_r,
                                   (alt_u32*) test_sig_s,
                                   (alt_u32*) test_data,
                                   test_data_size));
}
