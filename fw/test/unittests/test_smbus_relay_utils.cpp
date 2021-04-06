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


class SMBusUtilsTest : public testing::Test
{
public:
    alt_u32* m_relay1 = nullptr;
    alt_u32* m_relay2 = nullptr;
    alt_u32* m_relay3 = nullptr;

    const alt_u8 m_expected_cmd_whitelist_bus1_rule2[32] = {
        0xb0, 0x90, 0xee, 0x15, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    };
    const alt_u8 m_expected_cmd_whitelist_bus2_rule8[32] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5a,
    };
    const alt_u8 m_expected_cmd_whitelist_bus3_rule1[32] = {
        0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xa8,
    };

    const alt_u8 m_raw_pfm_x86[256] = {
        // PFM header
        0x1d, 0xce, 0xb3, 0x02, 0x03, 0x01, 0x01, 0x00, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0x00, 0x01, 0x00, 0x00,
        // SMBus Rule
        0x02, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02, 0xac,
        0xb0, 0x90, 0xee, 0x15, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        // SMBus Rule
        0x02, 0x00, 0x00, 0x00, 0x00, 0x02, 0x0b, 0xe4,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5a,
        // SPI Region
        0x01, 0x03, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0xb0, 0x00, 0x00, 0x00,
        0xbe, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        // SMBus Rule
        0x02, 0x00, 0x00, 0x00, 0x00, 0x03, 0x01, 0x98,
        0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xa8,
        // SPI Region (with no hash)
        0x01, 0x13, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x62, 0x00, 0x00, 0x00,
        0x78, 0x00, 0x00, 0x00,
        // Padding
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    };

    virtual void SetUp()
    {
        SYSTEM_MOCK* sys = SYSTEM_MOCK::get();
        sys->reset();

        m_relay1 = sys->smbus_relay_mock_ptr->get_cmd_enable_memory_for_smbus(1);
        m_relay2 = sys->smbus_relay_mock_ptr->get_cmd_enable_memory_for_smbus(2);
        m_relay3 = sys->smbus_relay_mock_ptr->get_cmd_enable_memory_for_smbus(3);
    }

    virtual void TearDown() {}
};

TEST_F(SMBusUtilsTest, test_basic)
{
    // Relays are not supposed to respond to read.
    // Read response is enabled for convenience in testing.
    m_relay1[0] = 3;
    EXPECT_EQ(m_relay1[0], (alt_u32) 3);

    m_relay2[1] = 0xDEADBEEF;
    EXPECT_EQ(m_relay2[1], (alt_u32) 0xDEADBEEF);

    m_relay3[2] = 0xDE000ABC;
    EXPECT_EQ(m_relay3[2], (alt_u32) 0xDE000ABC);
}

TEST_F(SMBusUtilsTest, test_empty)
{
    // When the command enable memory is initialized, it's filled with 0s.
    for (int i = 0; i < U_RELAY1_AVMM_BRIDGE_SPAN/4; i++)
    {
        EXPECT_EQ(m_relay1[i], alt_u32(0));
    }
    for (int i = 0; i < U_RELAY2_AVMM_BRIDGE_SPAN/4; i++)
    {
        EXPECT_EQ(m_relay2[i], alt_u32(0));
    }
    for (int i = 0; i < U_RELAY3_AVMM_BRIDGE_SPAN/4; i++)
    {
        EXPECT_EQ(m_relay3[i], alt_u32(0));
    }
}

TEST_F(SMBusUtilsTest, test_init_smbus_relays)
{
    // Fill some values in the smbus relays
    m_relay1[0] = 0xffab23cd;
    m_relay1[2] = 0x002301b0;
    m_relay2[1] = 0xdeadbeef;
    m_relay2[7] = 0x12345678;
    m_relay3[5] = 0x10010010;

    init_smbus_relays();

    // When the command enable memory is initialized, it's filled with 0s.
    for (int i = 0; i < U_RELAY1_AVMM_BRIDGE_SPAN/4; i++)
    {
        EXPECT_EQ(m_relay1[i], alt_u32(0));
    }
    for (int i = 0; i < U_RELAY2_AVMM_BRIDGE_SPAN/4; i++)
    {
        EXPECT_EQ(m_relay2[i], alt_u32(0));
    }
    for (int i = 0; i < U_RELAY3_AVMM_BRIDGE_SPAN/4; i++)
    {
        EXPECT_EQ(m_relay3[i], alt_u32(0));
    }
}

/**
 * @brief Check whether the SMBus rules from BMC PFM are applied as expected.
 */
TEST_F(SMBusUtilsTest, test_apply_smbus_rules_from_bmc)
{
    // Provision the system
    SYSTEM_MOCK::get()->provision_ufm_data(UFM_PFR_DATA_EXAMPLE_KEY_FILE);

    // Save the PFM in the SPI flash
    alt_u32_memcpy((alt_u32*) get_active_pfm(SPI_FLASH_BMC), (alt_u32*) m_raw_pfm_x86, 256);

    // Configure relays with white-lists from the SMBus rules
    apply_spi_write_protection_and_smbus_rules(SPI_FLASH_BMC);

    // Bus1 Rule2 command enable whitelist
    //   Skip the whitelist of the rule 1. That is 256/32 * 1 words.
    alt_u32* relay1_ptr = m_relay1 + 8;
    for (int i = 0; i < 8; i++)
    {
        // Expect
        EXPECT_EQ(relay1_ptr[i], ((alt_u32*) m_expected_cmd_whitelist_bus1_rule2)[i]);
    }

    // Bus2 Rule11 command enable whitelist
    //   Skip the whitelist of previous rules. That is 256/32 * 10 words.
    alt_u32* relay2_ptr = m_relay2 + 8 * 10;
    for (int i = 0; i < 8; i++)
    {
        EXPECT_EQ(relay2_ptr[i], ((alt_u32*) m_expected_cmd_whitelist_bus2_rule8)[i]);
    }

    // Bus3 Rule1 command enable whitelist
    //   No need to skip bytes because it's the first rule.
    for (int i = 0; i < 8; i++)
    {
        EXPECT_EQ(m_relay3[i], ((alt_u32*) m_expected_cmd_whitelist_bus3_rule1)[i]);
    }
}

/**
 * @brief Check whether the SMBus rules from PCH PFM are ignored as expected.
 */
TEST_F(SMBusUtilsTest, test_apply_smbus_rules_from_pch)
{
    // Provision the system
    SYSTEM_MOCK::get()->provision_ufm_data(UFM_PFR_DATA_EXAMPLE_KEY_FILE);

    // Save the PFM in the SPI flash
    alt_u32_memcpy((alt_u32*) get_active_pfm(SPI_FLASH_PCH), (alt_u32*) m_raw_pfm_x86, 256);

    // Configure relays with white-lists from the SMBus rules
    apply_spi_write_protection_and_smbus_rules(SPI_FLASH_PCH);

    // Bus1 Rule2 command enable whitelist
    //   Skip the whitelist of the rule 1. That is 256/32 * 1 words.
    alt_u32* relay1_ptr = m_relay1 + 8;
    for (int i = 0; i < 8; i++)
    {
        // Expect no rules have been applied
        EXPECT_EQ(relay1_ptr[i], alt_u32(0));
    }

    // Bus2 Rule11 command enable whitelist
    //   Skip the whitelist of previous rules. That is 256/32 * 10 words.
    alt_u32* relay2_ptr = m_relay2 + 8 * 10;
    for (int i = 0; i < 8; i++)
    {
        // Expect no rules have been applied
        EXPECT_EQ(relay2_ptr[i], alt_u32(0));
    }

    // Bus3 Rule1 command enable whitelist
    //   No need to skip bytes because it's the first rule.
    for (int i = 0; i < 8; i++)
    {
        // Expect no rules have been applied
        EXPECT_EQ(m_relay3[i], alt_u32(0));
    }
}

TEST_F(SMBusUtilsTest, test_filter_disabled_in_permissive_mode)
{
    ut_prep_nios_gpi_signals();

    // Insert the T0_OPERATIONS code block (break out of T0 loop)
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS_END_AFTER_50_ITERS);

    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(1, pfr_main());

    // Check observed vs expected global_state
    EXPECT_EQ(ut_get_global_state(), (alt_u32) PLATFORM_STATE_ENTER_T0);

    // Make sure filters are disabled
    EXPECT_TRUE(check_bit(U_GPO_1_ADDR, GPO_1_RELAY1_FILTER_DISABLE));
    EXPECT_TRUE(check_bit(U_GPO_1_ADDR, GPO_1_RELAY2_FILTER_DISABLE));
    EXPECT_TRUE(check_bit(U_GPO_1_ADDR, GPO_1_RELAY3_FILTER_DISABLE));
}
