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

#ifndef UNITTEST_SYSTEM_UT_NIOS_WRAPPER_H_
#define UNITTEST_SYSTEM_UT_NIOS_WRAPPER_H_

// Always include the BSP mock first
#include "bsp_mock.h"

// Include the PFR headers
#include "authentication.h"
#include "capsule_validation.h"
#include "cpld_update.h"
#include "cpld_reconfig.h"
#include "cpld_recovery.h"
#include "crypto.h"
#include "decompression.h"
#include "firmware_recovery.h"
#include "firmware_update.h"
#include "flash_validation.h"
#include "gen_gpi_signals.h"
#include "gen_gpo_controls.h"
#include "gen_smbus_relay_config.h"
#include "global_state.h"
#include "keychain.h"
#include "keychain_utils.h"
#include "mailbox_enums.h"
#include "mailbox_utils.h"
#include "pbc.h"
#include "pbc_utils.h"
#include "pfm.h"
#include "pfm_utils.h"
#include "pfm_validation.h"
#include "pfr_main.h"
#include "pfr_pointers.h"
#include "pfr_sys.h"
#include "platform_log.h"
#include "recovery_main.h"
#include "rfnvram_utils.h"
#include "smbus_relay_utils.h"
#include "spi_common.h"
#include "spi_ctrl_utils.h"
#include "spi_flash_state.h"
#include "spi_rw_utils.h"
#include "status_enums.h"
#include "t0_provisioning.h"
#include "t0_routines.h"
#include "t0_update.h"
#include "t0_watchdog_handler.h"
#include "timer_utils.h"
#include "tmin1_routines.h"
#include "transition.h"
#include "ufm.h"
#include "ufm_rw_utils.h"
#include "ufm_utils.h"
#include "utils.h"
#include "watchdog_timers.h"

static void ut_prep_nios_gpi_signals()
{
    SYSTEM_MOCK::get()->register_read_write_callback(
            [](SYSTEM_MOCK::READ_OR_WRITE read_or_write, void* addr, alt_u32 data) {
        if (read_or_write == SYSTEM_MOCK::READ_OR_WRITE::READ && addr == U_GPI_1_ADDR)
        {
            // De-assert both PCH and BMC resets from common core (0b11)
            // Signals that ME firmware has booted (0b1000)
            // Keep FORCE_RECOVERY signal inactive (0b1000000)
            SYSTEM_MOCK::get()->set_mem_word(U_GPI_1_ADDR, alt_u32(0x4b), true);

            if ((read_from_mailbox(MB_PLATFORM_STATE) == PLATFORM_STATE_ENTER_T0) &&
                    check_bit(U_GPO_1_ADDR, GPO_1_RST_RSMRST_PLD_R_N))
            {
                // When Nios firmware just reaches T0 and PCH is out of reset, simulate the hw PLTRST# toggle
                // Set the GPI_1_PLTRST_DETECTED_REARM_ACM_TIMER bit (0b10000)
                SYSTEM_MOCK::get()->set_mem_word(U_GPI_1_ADDR, alt_u32(0x5b), true);
            }
        }
    });
}

static void ut_send_bmc_reset_detected_gpi_once_upon_boot_complete()
{
    SYSTEM_MOCK::get()->register_read_write_callback(
            [](SYSTEM_MOCK::READ_OR_WRITE read_or_write, void* addr, alt_u32 data) {
        if (read_or_write == SYSTEM_MOCK::READ_OR_WRITE::READ && addr == U_GPI_1_ADDR)
        {
            if ((read_from_mailbox(MB_PLATFORM_STATE) == PLATFORM_STATE_T0_BOOT_COMPLETE) &&
                    (read_from_mailbox(MB_PANIC_EVENT_COUNT) == 0))
            {
                // De-assert both PCH and BMC resets (0b11)
                // Keep FORCE_RECOVERY signal inactive (0b1000000)
                // BMC reset detected (0b100000)
                SYSTEM_MOCK::get()->set_mem_word(U_GPI_1_ADDR, alt_u32(0x63), true);
            }
        }
    });
}

static void ut_toggle_pltrst_gpi_once_upon_boot_complete()
{
    SYSTEM_MOCK::get()->register_read_write_callback(
            [](SYSTEM_MOCK::READ_OR_WRITE read_or_write, void* addr, alt_u32 data) {
        if (read_or_write == SYSTEM_MOCK::READ_OR_WRITE::READ && addr == U_GPI_1_ADDR)
        {
            if ((read_from_mailbox(MB_PLATFORM_STATE) == PLATFORM_STATE_T0_BOOT_COMPLETE) &&
                    (read_from_mailbox(MB_PANIC_EVENT_COUNT) == 0))
            {
                // De-assert both PCH and BMC resets from common core (0b11)
                // Signals that ME firmware has booted (0b1000)
                // Keep FORCE_RECOVERY signal inactive (0b1000000)
                // Set the GPI_1_PLTRST_DETECTED_REARM_ACM_TIMER bit (0b10000)

                // When Nios firmware reaches T0 boot complete, simulate the hw PLTRST# toggle
                SYSTEM_MOCK::get()->set_mem_word(U_GPI_1_ADDR, alt_u32(0x5b), true);
            }
        }
    });
}

/**
 * @brief Return the value in the global state register
 *
 * @return alt_u32 global state
 */
static alt_u32 ut_get_global_state()
{
    return IORD_32DIRECT(U_GLOBAL_STATE_REG_ADDR, 0);
}

static void ut_send_block_complete_chkpt_msg()
{
    // Signals that BMC/ACM/BIOS have all booted after one check
    SYSTEM_MOCK::get()->register_read_write_callback(
            [](SYSTEM_MOCK::READ_OR_WRITE read_or_write, void* addr, alt_u32 data) {
        if (read_or_write == SYSTEM_MOCK::READ_OR_WRITE::READ)
        {
            alt_u32* bmc_ckpt_addr = U_MAILBOX_AVMM_BRIDGE_ADDR + MB_BMC_CHECKPOINT;
            alt_u32* acm_ckpt_addr = U_MAILBOX_AVMM_BRIDGE_ADDR + MB_ACM_CHECKPOINT;
            alt_u32* bios_ckpt_addr = U_MAILBOX_AVMM_BRIDGE_ADDR + MB_BIOS_CHECKPOINT;

            if (addr == bmc_ckpt_addr)
            {
                SYSTEM_MOCK::get()->set_mem_word(bmc_ckpt_addr, MB_CHKPT_COMPLETE, true);
            }
            else if (addr == acm_ckpt_addr)
            {
                // Nios is supposed to ignore the BOOT_START/BOOT_DONE checkpoint messages from ACM.
                // Sending this shouldn't hurt.
                SYSTEM_MOCK::get()->set_mem_word(acm_ckpt_addr, MB_CHKPT_COMPLETE, true);
            }
            else if (addr == bios_ckpt_addr)
            {
                if (wdt_boot_status & WDT_ACM_BOOT_DONE_MASK)
                {
                    // Once ACM is booted, keep sending BOOT_DONE checkpoint to BIOS checkpoint register.
                    // With this, Nios is supposed to turn off IBB and OBB WDT.
                    SYSTEM_MOCK::get()->set_mem_word(bios_ckpt_addr, MB_CHKPT_COMPLETE, true);
                }
                else
                {
                    // Sends BOOT_START to BIOS checkpoint, when ACM is booting.
                    // Nios is supposed to turn off ACM WDT when IBB starts to boot.
                    SYSTEM_MOCK::get()->set_mem_word(bios_ckpt_addr, MB_CHKPT_START, true);
                }
            }
        }
    });
}

static void ut_send_in_update_intent(MB_REGFILE_OFFSET_ENUM update_intent_offset, alt_u32 update_intent_value)
{
    SYSTEM_MOCK::get()->register_read_write_callback(
            [update_intent_offset, update_intent_value](SYSTEM_MOCK::READ_OR_WRITE read_or_write, void* addr, alt_u32 data) {
        if (read_or_write == SYSTEM_MOCK::READ_OR_WRITE::READ)
        {
            alt_u32* update_intent_addr = U_MAILBOX_AVMM_BRIDGE_ADDR + update_intent_offset;
            if (addr == update_intent_addr)
            {
                if (read_from_mailbox(MB_PLATFORM_STATE) == PLATFORM_STATE_T0_BOOT_COMPLETE)
                {
                    SYSTEM_MOCK::get()->set_mem_word(update_intent_addr, update_intent_value, true);
                }
            }
        }
    });
}

static void ut_send_in_update_intent_tmin1(MB_REGFILE_OFFSET_ENUM update_intent_offset, alt_u32 update_intent_value)
{
    SYSTEM_MOCK::get()->register_read_write_callback(
            [update_intent_offset, update_intent_value](SYSTEM_MOCK::READ_OR_WRITE read_or_write, void* addr, alt_u32 data) {
        if (read_or_write == SYSTEM_MOCK::READ_OR_WRITE::READ)
        {
            alt_u32* update_intent_addr = U_MAILBOX_AVMM_BRIDGE_ADDR + update_intent_offset;
            if (addr == update_intent_addr)
            {
                    SYSTEM_MOCK::get()->set_mem_word(update_intent_addr, update_intent_value, true);
            }
        }
    });
}

static void ut_send_in_update_intent_once_upon_entry_to_t0(MB_REGFILE_OFFSET_ENUM update_intent_offset, alt_u32 update_intent_value)
{
    SYSTEM_MOCK::get()->register_read_write_callback(
            [update_intent_offset, update_intent_value](SYSTEM_MOCK::READ_OR_WRITE read_or_write, void* addr, alt_u32 data) {
        if (read_or_write == SYSTEM_MOCK::READ_OR_WRITE::READ)
        {
            alt_u32* update_intent_addr = U_MAILBOX_AVMM_BRIDGE_ADDR + update_intent_offset;
            if (addr == update_intent_addr)
            {
                if ((read_from_mailbox(MB_PLATFORM_STATE) == PLATFORM_STATE_ENTER_T0) && (read_from_mailbox(MB_PANIC_EVENT_COUNT) == 0))
                {
                    SYSTEM_MOCK::get()->set_mem_word(update_intent_addr, update_intent_value, true);
                }
            }
        }
    });
}

static void ut_send_in_update_intent_once_upon_boot_complete(MB_REGFILE_OFFSET_ENUM update_intent_offset, alt_u32 update_intent_value)
{
    SYSTEM_MOCK::get()->register_read_write_callback(
            [update_intent_offset, update_intent_value](SYSTEM_MOCK::READ_OR_WRITE read_or_write, void* addr, alt_u32 data) {
        if (read_or_write == SYSTEM_MOCK::READ_OR_WRITE::READ)
        {
            alt_u32* update_intent_addr = U_MAILBOX_AVMM_BRIDGE_ADDR + update_intent_offset;
            if (addr == update_intent_addr)
            {
                if ((read_from_mailbox(MB_PLATFORM_STATE) == PLATFORM_STATE_T0_BOOT_COMPLETE) && (read_from_mailbox(MB_PANIC_EVENT_COUNT) == 0))
                {
                    SYSTEM_MOCK::get()->set_mem_word(update_intent_addr, update_intent_value, true);
                }
            }
        }
    });
}

static alt_u32 ut_check_ufm_prov_status(MB_UFM_PROV_STATUS_MASK_ENUM status_mask)
{
    return (IORD(U_MAILBOX_AVMM_BRIDGE_ADDR, MB_PROVISION_STATUS) & status_mask) == status_mask;
}

static void ut_send_in_ufm_command(MB_UFM_PROV_CMD_ENUM ufm_prov_cmd)
{
    alt_u32* mb_ufm_prov_cmd_addr = U_MAILBOX_AVMM_BRIDGE_ADDR + MB_PROVISION_CMD;
    alt_u32* mb_ufm_cmd_trigger_addr = U_MAILBOX_AVMM_BRIDGE_ADDR + MB_UFM_CMD_TRIGGER;

    SYSTEM_MOCK::get()->set_mem_word(mb_ufm_prov_cmd_addr, ufm_prov_cmd, true);
    SYSTEM_MOCK::get()->set_mem_word(mb_ufm_cmd_trigger_addr, MB_UFM_CMD_EXECUTE_MASK, true);
}

static void ut_wait_for_ufm_prov_cmd_done()
{
    while ((IORD(U_MAILBOX_AVMM_BRIDGE_ADDR, MB_PROVISION_STATUS) & MB_UFM_PROV_CMD_DONE_MASK) == 0) {}
}

static void ut_disable_watchdog_timers()
{
    wdt_enable_status = 0;
}

static alt_u32 ut_is_bmc_out_of_reset()
{
    return check_bit(U_GPO_1_ADDR, GPO_1_RST_SRST_BMC_PLD_R_N);
}

static alt_u32 ut_is_pch_out_of_reset()
{
    return check_bit(U_GPO_1_ADDR, GPO_1_RST_RSMRST_PLD_R_N);
}

static void ut_reset_watchdog_timers()
{
    wdt_boot_status = 0;
    wdt_enable_status = WDT_ENABLE_ALL_TIMERS_MASK;
}

static void ut_reset_failed_update_attempts()
{
    reset_failed_update_attempts(SPI_FLASH_PCH);
    reset_failed_update_attempts(SPI_FLASH_BMC);
}

static alt_u32 ut_is_blocking_update_from(SPI_FLASH_TYPE_ENUM spi_flash_type)
{
    if (spi_flash_type == SPI_FLASH_PCH)
    {
        return num_failed_update_attempts_from_pch >= MAX_FAILED_UPDATE_ATTEMPTS_FROM_PCH;
    }
    return num_failed_update_attempts_from_bmc >= MAX_FAILED_UPDATE_ATTEMPTS_FROM_BMC;
}

/**
 * @brief Return 1 if the given 16KB page in the given SPI flash is writable, according to the
 * Write Enable memory inside its SPI filter.
 *
 * @see apply_spi_write_protection
 */
static alt_u32 ut_is_16kb_page_writable(SPI_FLASH_TYPE_ENUM spi_flash_type, alt_u32 addr)
{
    alt_u32* we_mem = __IO_CALC_ADDRESS_NATIVE_ALT_U32(U_SPI_FILTER_PCH_WE_AVMM_BRIDGE_BASE, 0);
    if (spi_flash_type == SPI_FLASH_BMC)
    {
        we_mem = __IO_CALC_ADDRESS_NATIVE_ALT_U32(U_SPI_FILTER_BMC_WE_AVMM_BRIDGE_BASE, 0);
    }

    // 14 bits to get to 16kB chunks, 5 more bits because we have 32 (=2^5) bits in each word in the
    // memory
    alt_u32 word_pos = addr >> (14 + 5);
    // Grab the 5 address bits that tell us the starting bit position within the 32 bit word
    alt_u32 bit_pos = (addr >> 14) & 0x0000001f;

    // If bit == 1 in WE memory, it means that this page is writable.
    return check_bit(we_mem + word_pos, bit_pos);
}

static void ut_reset_fw_recovery_levels()
{
    reset_fw_recovery_level(SPI_FLASH_PCH);
    reset_fw_recovery_level(SPI_FLASH_BMC);
}

static void ut_reset_fw_spi_flash_state()
{
    bmc_flash_state = 0;
    pch_flash_state = 0;
}

static void ut_reset_nios_fw()
{
    ut_reset_watchdog_timers();
    ut_reset_failed_update_attempts();
    ut_reset_fw_recovery_levels();
    ut_reset_fw_spi_flash_state();
}

static void ut_setup_for_recovery_main()
{
    ut_prep_nios_gpi_signals();
}

static void ut_setup_for_pfr_main()
{
    ut_prep_nios_gpi_signals();

    // Send checkpoint messages
    ut_send_block_complete_chkpt_msg();
}

/**
 * @brief This function runs pfr_main() or recovery_main() based on the
 * ConfigSelect register in the Dual-Config IP.
 */
static void ut_run_main(CPLD_CFM_TYPE_ENUM expect_cfm_one_or_zero, bool expect_throw)
{
    // Read ConfigSelect register; bit[1] represent CFM selection
    alt_u32 config_sel = IORD(U_DUAL_CONFIG_BASE, 1);

    if (config_sel & 0b10)
    {
        // CFM1: Active Image
        EXPECT_EQ(CPLD_CFM1, expect_cfm_one_or_zero);

        ut_setup_for_pfr_main();

        // Always run pfr_main with the timeout
        if (expect_throw)
        {
            ASSERT_DURATION_LE(500, EXPECT_ANY_THROW({ pfr_main(); }));
        }
        else
        {
            ASSERT_DURATION_LE(500, EXPECT_NO_THROW({ pfr_main(); }));
        }
    }
    else
    {
        // CFM0: ROM Image
        EXPECT_EQ(CPLD_CFM0, expect_cfm_one_or_zero);

        ut_setup_for_recovery_main();

        // Always run recovery_main with the timeout
        if (expect_throw)
        {
            ASSERT_DURATION_LE(500, EXPECT_ANY_THROW({ recovery_main(); }));
        }
        else
        {
            ASSERT_DURATION_LE(500, EXPECT_NO_THROW({ recovery_main(); }));
        }
    }
}

#endif /* UNITTEST_SYSTEM_UT_NIOS_WRAPPER_H_ */
