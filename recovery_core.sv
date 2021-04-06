/////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Intel Corporation
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
/////////////////////////////////////////////////////////////////////////////////



// recovery_core
//
// This module implements the toplevel of the PFR IP. It instantiates the
// Nios II system along with all of the other toplevel IPs such as SMBus
// relay, SPI filter, etc.
//

`timescale 1 ps / 1 ps
`default_nettype none

module recovery_core (
    // Clocks and resets
    input wire clk2M,
    input wire clk50M,
    input wire sys_clk,
    input wire spi_clk,
    input wire clk2M_reset_sync_n,
    input wire clk50M_reset_sync_n,
    input wire sys_clk_reset_sync_n,
    input wire spi_clk_reset_sync_n,
    
    // BMC and PCH resets from common core
    input wire cc_RST_RSMRST_PLD_R_N,
    input wire cc_RST_SRST_BMC_PLD_R_N,
    
    // 20ms pulse generator timer
    input wire i20mSCE,
    
    // 1s pulse generator timer
    input wire i1SCE,

    // SMBus slave device inside the common core
    input wire ccSMB_PCH_PMBUS2_STBY_LVC3_SDA_OEn,
    
    // LED Control from the common core
    input wire ccLED_CONTROL_0,
    input wire ccLED_CONTROL_1,
    input wire ccLED_CONTROL_2,
    input wire ccLED_CONTROL_3,
    input wire ccLED_CONTROL_4,
    input wire ccLED_CONTROL_5,
    input wire ccLED_CONTROL_6,
    input wire ccLED_CONTROL_7,
    input wire ccFM_POST_7SEG1_SEL_N,
    input wire ccFM_POST_7SEG2_SEL_N,
    input wire ccFM_POSTLED_SEL,

    // LED Output 
    output logic LED_CONTROL_0,
    output logic LED_CONTROL_1,
    output logic LED_CONTROL_2,
    output logic LED_CONTROL_3,
    output logic LED_CONTROL_4,
    output logic LED_CONTROL_5,
    output logic LED_CONTROL_6,
    output logic LED_CONTROL_7,
    output wire FM_POST_7SEG1_SEL_N,
    output wire FM_POST_7SEG2_SEL_N,
    output wire FM_POSTLED_SEL,

    output wire FAN_BMC_PWM_R,
    input wire FM_BMC_BMCINIT,
    input wire FM_ME_PFR_1,
    input wire FM_ME_PFR_2,
    output wire FM_PFR_CLK_MUX_SEL,
    output wire FM_PFR_CPU1_BMCINIT,
    output wire FM_PFR_CPU1_FRMAGENT,
    output wire FM_PFR_CPU2_BMCINIT,
    input wire FM_PFR_DEBUG_MODE_N,
    input wire FM_PFR_FORCE_RECOVERY_N,
    output wire FM_PFR_LEGACY_CPU1,
    input wire FM_PFR_PROV_UPDATE_N,
    output wire FM_SPI_PFR_BMC_BT_MASTER_SEL,
    output wire FM_SPI_PFR_PCH_MASTER_SEL,
    input wire FP_ID_BTN_N,
    output wire FP_ID_BTN_PFR_N,
    input wire FP_ID_LED_N,
    output wire FP_ID_LED_PFR_N,
    input wire FP_LED_STATUS_AMBER_N,
    output wire FP_LED_STATUS_AMBER_PFR_N,
    input wire FP_LED_STATUS_GREEN_N,
    output wire FP_LED_STATUS_GREEN_PFR_N,
    output wire PWRGD_DSW_PWROK_R,
    output wire RST_PFR_EXTRST_N,
    output wire RST_PFR_OVR_RTC_N,
    output wire RST_PFR_OVR_SRTC_N,
    input wire RST_PLTRST_PLD_N,
    output wire RST_RSMRST_PLD_R_N,
    output wire RST_SPI_PFR_BMC_BOOT_N,
    output wire RST_SPI_PFR_PCH_N,
    output wire RST_SRST_BMC_PLD_R_N,
    inout wire SMB_BMC_HSBP_STBY_LVC3_SCL,
    inout wire SMB_BMC_HSBP_STBY_LVC3_SDA,
    input wire SMB_BMC_SPD_ACCESS_STBY_LVC3_SCL,
    inout wire SMB_BMC_SPD_ACCESS_STBY_LVC3_SDA,
    input wire SMB_CPU_PIROM_SCL,
    inout wire SMB_CPU_PIROM_SDA,
    inout wire SMB_PCH_PMBUS2_STBY_LVC3_SCL,
    inout wire SMB_PCH_PMBUS2_STBY_LVC3_SDA,
    input wire SMB_PFR_DDRABCD_CPU1_LVC2_SCL,
    inout wire SMB_PFR_DDRABCD_CPU1_LVC2_SDA,
    input wire SMB_PFR_DDRABCD_CPU2_LVC2_SCL,
    inout wire SMB_PFR_DDRABCD_CPU2_LVC2_SDA,
    input wire SMB_PFR_DDREFGH_CPU1_LVC2_SCL,
    inout wire SMB_PFR_DDREFGH_CPU1_LVC2_SDA,
    input wire SMB_PFR_DDREFGH_CPU2_LVC2_SCL,
    inout wire SMB_PFR_DDREFGH_CPU2_LVC2_SDA,
    inout wire SMB_PFR_HSBP_STBY_LVC3_SCL,
    inout wire SMB_PFR_HSBP_STBY_LVC3_SDA,
    inout wire SMB_PFR_PMB1_STBY_LVC3_SCL,
    inout wire SMB_PFR_PMB1_STBY_LVC3_SDA,
    inout wire SMB_PFR_PMBUS2_STBY_LVC3_SCL,
    inout wire SMB_PFR_PMBUS2_STBY_LVC3_SDA,
    output wire SMB_PFR_RFID_STBY_LVC3_SCL,
    inout wire SMB_PFR_RFID_STBY_LVC3_SDA,
    inout wire SMB_PMBUS_SML1_STBY_LVC3_SCL,
    inout wire SMB_PMBUS_SML1_STBY_LVC3_SDA,
    input wire SPI_BMC_BOOT_CS_N,
    input wire SPI_BMC_BOOT_R_CS1_N,
    input wire SPI_BMC_BT_MUXED_MON_CLK,
    input wire SPI_BMC_BT_MUXED_MON_MISO,
    input wire SPI_BMC_BT_MUXED_MON_MOSI,
    input wire SPI_CPU1_PFR_CLK_PLD_R,
    input wire SPI_CPU1_PFR_CS_PLD_R,
    output wire SPI_CPU1_PFR_MISO_PLD_R,
    input wire SPI_CPU1_PFR_MOSI_PLD_R,
    input wire SPI_CPU2_PFR_CLK_PLD_R,
    input wire SPI_CPU2_PFR_CS_PLD_R,
    output wire SPI_CPU2_PFR_MISO_PLD_R,
    input wire SPI_CPU2_PFR_MOSI_PLD_R,
    input wire SPI_PCH_BMC_PFR_CS0_N,
    input wire SPI_PCH_BMC_SAFS_MUXED_MON_CLK,
    input wire SPI_PCH_BMC_SAFS_MUXED_MON_IO0,
    input wire SPI_PCH_BMC_SAFS_MUXED_MON_IO1,
    input wire SPI_PCH_BMC_SAFS_MUXED_MON_IO2,
    input wire SPI_PCH_BMC_SAFS_MUXED_MON_IO3,
    input wire SPI_PCH_CS1_N,
    inout wire SPI_PFR_BMC_BOOT_R_IO2,
    inout wire SPI_PFR_BMC_BOOT_R_IO3,
    output wire SPI_PFR_BMC_BT_SECURE_CS_N,
    output wire SPI_PFR_BMC_FLASH1_BT_CLK,
    inout wire SPI_PFR_BMC_FLASH1_BT_MISO,
    inout wire SPI_PFR_BMC_FLASH1_BT_MOSI,
    output wire SPI_PFR_BOOT_CS1_N,
    output wire SPI_PFR_PCH_BMC_SECURE_CS0_N,
    output wire SPI_PFR_PCH_R_CLK,
    inout wire SPI_PFR_PCH_R_IO0,
    inout wire SPI_PFR_PCH_R_IO1,
    inout wire SPI_PFR_PCH_R_IO2,
    inout wire SPI_PFR_PCH_R_IO3,
    output wire SPI_PFR_PCH_SECURE_CS1_N,
    
    output wire FM_PFR_SLP_SUS_N
    
);

    // Import the GPI names
    import gen_gpi_signals_pkg::*;

    //Import the platform defs names
    import platform_defs_pkg::*;

    // Nios general purpose outputs
    wire [31:0] gpo_1;
    
    // Nios general purpose inputs
    wire [31:0] gpi_1;

    // Nios global state broadcast
    wire [31:0] global_state;

    // SBus Mailbox AVMM interface
    wire        mailbox_avmm_clk;
    wire        mailbox_avmm_areset;
    wire [7:0]  mailbox_avmm_address;
    wire        mailbox_avmm_waitrequest;
    wire        mailbox_avmm_read;
    wire        mailbox_avmm_write;
    wire [31:0] mailbox_avmm_readdata;
    wire [31:0] mailbox_avmm_writedata;
    wire        mailbox_avmm_readdatavalid;

    // RFNVRAM master AVMM interface
    wire        rfnvram_avmm_clk;
    wire        rfnvram_avmm_areset;
    wire [3:0]  rfnvram_avmm_address;
    wire        rfnvram_avmm_waitrequest;
    wire        rfnvram_avmm_read;
    wire        rfnvram_avmm_write;
    wire [31:0] rfnvram_avmm_readdata;
    wire [31:0] rfnvram_avmm_writedata;
    wire        rfnvram_avmm_readdatavalid;
    
    // SMBUS relay AVMM interfaces
    wire [7:0]  relay1_avmm_address;
    wire        relay1_avmm_write;
    wire [31:0] relay1_avmm_writedata;
    wire [7:0]  relay2_avmm_address;
    wire        relay2_avmm_write;
    wire [31:0] relay2_avmm_writedata;
    wire [7:0]  relay3_avmm_address;
    wire        relay3_avmm_write;
    wire [31:0] relay3_avmm_writedata;

    // Crypto block AVMM interface
    wire        crypto_avmm_clk;
    wire        crypto_avmm_areset;
    wire [6:0]  crypto_avmm_address;
    wire        crypto_avmm_waitrequest;
    wire        crypto_avmm_read;
    wire        crypto_avmm_write;
    wire [31:0] crypto_avmm_readdata;
    wire [31:0] crypto_avmm_writedata;

    // Timer bank AVMM interface
    wire        timer_bank_avmm_clk;
    wire        timer_bank_avmm_areset;
    wire [2:0]  timer_bank_avmm_address;
    wire        timer_bank_avmm_waitrequest;
    wire        timer_bank_avmm_read;
    wire        timer_bank_avmm_write;
    wire [31:0] timer_bank_avmm_readdata;
    wire [31:0] timer_bank_avmm_writedata;


    // SPI Master CSR AVMM interface
    wire [5:0]  spi_master_csr_avmm_address;
    wire        spi_master_csr_avmm_waitrequest;
    wire        spi_master_csr_avmm_read;
    wire        spi_master_csr_avmm_write;
    wire [31:0] spi_master_csr_avmm_readdata;
    wire        spi_master_csr_avmm_readdatavalid;
    wire [31:0] spi_master_csr_avmm_writedata;

    // SPI Master AVMM interface to SPI memory
    wire [24:0] spi_master_avmm_address;
    wire        spi_master_avmm_waitrequest;
    wire        spi_master_avmm_read;
    wire        spi_master_avmm_write;
    wire [31:0] spi_master_avmm_readdata;
    wire [31:0] spi_master_avmm_writedata;
    wire        spi_master_avmm_readdatavalid;

    // SPI Filter AVMM interface to BMC and PCH SPI filters to control which sections of each FLASH device suppor write/erase commands (to be configured based on the PFM)
    wire        spi_filter_bmc_we_avmm_clk;
    wire        spi_filter_bmc_we_avmm_areset;
    wire [10:0] spi_filter_bmc_we_avmm_address;     // address window is larger than actually required, some top address bits will be ignored
    wire        spi_filter_bmc_we_avmm_write;
    wire [31:0] spi_filter_bmc_we_avmm_writedata;
    wire [10:0] spi_filter_pch_we_avmm_address;     // address window is larger than actually required, some top address bits will be ignored
    wire        spi_filter_pch_we_avmm_write;
    wire [31:0] spi_filter_pch_we_avmm_writedata;

    // signals for logging blocked SPI commands
    logic [31:0]                         bmc_spi_filtered_command_info; // 31 - 1=4-byte addressing mode, 0=3-byte addressing mode; 30:14 - address bits 30:14 of filtered command (invalid when Bit 13 == 1); 13 - 1=illegal command, 0=illegal write/erase region; 12:8- number of filtered commands; 7:0 - command that was filtered
    logic [31:0]                         pch_spi_filtered_command_info; // 31 - 1=4-byte addressing mode, 0=3-byte addressing mode; 30:14 - address bits 30:14 of filtered command (invalid when Bit 13 == 1); 13 - 1=illegal command, 0=illegal write/erase region; 12:8- number of filtered commands; 7:0 - command that was filtered

   ///////////////////////////////////////////////////////////////////////////
    // Nios IIe system
    ///////////////////////////////////////////////////////////////////////////
    recovery_sys u_pfr_sys(
        // System clocks and synchronized resets. Note that sys_clk must be
        // less than 80MHz as it is connected to the dual config IP, and that
        // IP has a max frequency of 80MHz. 
        .sys_clk_clk(sys_clk),
        .spi_clk_clk(spi_clk),

        .sys_clk_reset_reset(!sys_clk_reset_sync_n),
        .spi_clk_reset_reset(!spi_clk_reset_sync_n),

        .global_state_export(global_state),

        .gpo_1_export(gpo_1),

        .gpi_1_export(gpi_1),

        // AVMM interface to the timer bank
        .timer_bank_avmm_clk(timer_bank_avmm_clk),
        .timer_bank_avmm_areset(timer_bank_avmm_areset),
        .timer_bank_avmm_address(timer_bank_avmm_address),
        .timer_bank_avmm_waitrequest(timer_bank_avmm_waitrequest),
        .timer_bank_avmm_read(timer_bank_avmm_read),
        .timer_bank_avmm_write(timer_bank_avmm_write),
        .timer_bank_avmm_readdata(timer_bank_avmm_readdata),
        .timer_bank_avmm_writedata(timer_bank_avmm_writedata),

        // AVMM interface to SMBus Mailbox         
        .mailbox_avmm_clk(mailbox_avmm_clk),
        .mailbox_avmm_areset(mailbox_avmm_areset),
        .mailbox_avmm_address(mailbox_avmm_address),
        .mailbox_avmm_waitrequest(mailbox_avmm_waitrequest),
        .mailbox_avmm_read(mailbox_avmm_read),
        .mailbox_avmm_write(mailbox_avmm_write),
        .mailbox_avmm_readdata(mailbox_avmm_readdata),
        .mailbox_avmm_writedata(mailbox_avmm_writedata),
        .mailbox_avmm_readdatavalid(mailbox_avmm_readdatavalid),

        .rfnvram_avmm_clk                  (rfnvram_avmm_clk),
        .rfnvram_avmm_areset               (rfnvram_avmm_areset),
        .rfnvram_avmm_address              (rfnvram_avmm_address),
        .rfnvram_avmm_waitrequest          (rfnvram_avmm_waitrequest),
        .rfnvram_avmm_read                 (rfnvram_avmm_read),
        .rfnvram_avmm_write                (rfnvram_avmm_write),
        .rfnvram_avmm_readdata             (rfnvram_avmm_readdata),
        .rfnvram_avmm_writedata            (rfnvram_avmm_writedata),
        .rfnvram_avmm_readdatavalid        (rfnvram_avmm_readdatavalid),
        // AVMM interfaces to SMBus relay command whitelist memory
        .relay1_avmm_clk        (  ),
        .relay1_avmm_areset     (  ),
        .relay1_avmm_address    ( relay1_avmm_address ),
        .relay1_avmm_waitrequest( '0 ),
        .relay1_avmm_read       (  ),
        .relay1_avmm_write      ( relay1_avmm_write ),
        .relay1_avmm_readdata   ( '0 ),
        .relay1_avmm_writedata  ( relay1_avmm_writedata ),
        .relay2_avmm_clk        (  ),
        .relay2_avmm_areset     (  ),
        .relay2_avmm_address    ( relay2_avmm_address ),
        .relay2_avmm_waitrequest( '0 ),
        .relay2_avmm_read       (  ),
        .relay2_avmm_write      ( relay2_avmm_write ),
        .relay2_avmm_readdata   ( '0 ),
        .relay2_avmm_writedata  ( relay2_avmm_writedata ),
        .relay3_avmm_clk        (  ),
        .relay3_avmm_areset     (  ),
        .relay3_avmm_address    ( relay3_avmm_address ),
        .relay3_avmm_waitrequest( '0 ),
        .relay3_avmm_read       (  ),
        .relay3_avmm_write      ( relay3_avmm_write ),
        .relay3_avmm_readdata   ( '0 ),
        .relay3_avmm_writedata  ( relay3_avmm_writedata ),

        // AVMM interface to the crypto block
        .crypto_avmm_clk(crypto_avmm_clk),
        .crypto_avmm_areset(crypto_avmm_areset),
        .crypto_avmm_address(crypto_avmm_address),
        .crypto_avmm_waitrequest(crypto_avmm_waitrequest),
        .crypto_avmm_read(crypto_avmm_read),
        .crypto_avmm_write(crypto_avmm_write),
        .crypto_avmm_readdata(crypto_avmm_readdata),
        .crypto_avmm_writedata(crypto_avmm_writedata),

        // AVMM interface to the SPI Filter write-enable memories
        .spi_filter_bmc_we_avmm_clk        ( spi_filter_bmc_we_avmm_clk ),
        .spi_filter_bmc_we_avmm_areset     ( spi_filter_bmc_we_avmm_areset ),
        .spi_filter_bmc_we_avmm_address    ( spi_filter_bmc_we_avmm_address ),
        .spi_filter_bmc_we_avmm_waitrequest( '0 ),
        .spi_filter_bmc_we_avmm_read       (  ),
        .spi_filter_bmc_we_avmm_write      ( spi_filter_bmc_we_avmm_write ),
        .spi_filter_bmc_we_avmm_readdata   ( bmc_spi_filtered_command_info ),       // this data is NOT synchronized to the system clock, NIOS must read this multiple times and get identical results before using this data
        .spi_filter_bmc_we_avmm_writedata  ( spi_filter_bmc_we_avmm_writedata ),
        .spi_filter_pch_we_avmm_clk        (  ),
        .spi_filter_pch_we_avmm_areset     (  ),
        .spi_filter_pch_we_avmm_address    ( spi_filter_pch_we_avmm_address ),
        .spi_filter_pch_we_avmm_waitrequest( '0 ),
        .spi_filter_pch_we_avmm_read       (  ),
        .spi_filter_pch_we_avmm_write      ( spi_filter_pch_we_avmm_write ),
        .spi_filter_pch_we_avmm_readdata   ( pch_spi_filtered_command_info ),       // this data is NOT synchronized to the system clock, NIOS must read this multiple times and get identical results before using this data
        .spi_filter_pch_we_avmm_writedata  ( spi_filter_pch_we_avmm_writedata ),

        // AVMM interface to the SPI filter CSR
        .spi_filter_csr_avmm_clk            (  ),
        .spi_filter_csr_avmm_areset         (  ),
        .spi_filter_csr_avmm_address        (spi_master_csr_avmm_address),
        .spi_filter_csr_avmm_waitrequest    (spi_master_csr_avmm_waitrequest),
        .spi_filter_csr_avmm_read           (spi_master_csr_avmm_read),
        .spi_filter_csr_avmm_write          (spi_master_csr_avmm_write),
        .spi_filter_csr_avmm_readdata       (spi_master_csr_avmm_readdata),
        .spi_filter_csr_avmm_readdatavalid  (spi_master_csr_avmm_readdatavalid),
        .spi_filter_csr_avmm_writedata      (spi_master_csr_avmm_writedata),

        // AVMM interface to the SPI filter
        .spi_filter_avmm_clk                (  ),
        .spi_filter_avmm_areset             (  ),
        .spi_filter_avmm_address            (spi_master_avmm_address),
        .spi_filter_avmm_waitrequest        (spi_master_avmm_waitrequest),
        .spi_filter_avmm_read               (spi_master_avmm_read),
        .spi_filter_avmm_write              (spi_master_avmm_write),
        .spi_filter_avmm_readdata           (spi_master_avmm_readdata),
        .spi_filter_avmm_readdatavalid      (spi_master_avmm_readdatavalid),
        .spi_filter_avmm_writedata          (spi_master_avmm_writedata)
    );
	
    ///////////////////////////////////////////////////////////////////////////
    // GPI Connectivity from Nios
    ///////////////////////////////////////////////////////////////////////////
    // PCH and BMC resets from common core
    assign gpi_1[gen_gpi_signals_pkg::GPI_1_cc_RST_RSMRST_PLD_R_N_BIT_POS] = cc_RST_RSMRST_PLD_R_N;
    assign gpi_1[gen_gpi_signals_pkg::GPI_1_cc_RST_SRST_BMC_PLD_R_N_BIT_POS] = cc_RST_SRST_BMC_PLD_R_N;
    // ME firmware uses these two GPIOs to signal its security status to PFR RoT 
    assign gpi_1[gen_gpi_signals_pkg::GPI_1_FM_ME_PFR_1_BIT_POS] = FM_ME_PFR_1;
    assign gpi_1[gen_gpi_signals_pkg::GPI_1_FM_ME_PFR_2_BIT_POS] = FM_ME_PFR_2;
    // Signal to re-arm the ACM timer
    reg rearm_acm_timer;
    assign gpi_1[gen_gpi_signals_pkg::GPI_1_PLTRST_DETECTED_REARM_ACM_TIMER_BIT_POS] = rearm_acm_timer;
    // status bit from the SPI filter block
    logic spi_control_bmc_ibb_access_detected;
    assign gpi_1[gen_gpi_signals_pkg::GPI_1_BMC_SPI_IBB_ACCESS_DETECTED_BIT_POS] = spi_control_bmc_ibb_access_detected;
    // Forces recovery in manual mode
    // We repurposed the FM_PFR_PROV_UPDATE pin because the FORCE_RECOVERY pin could not be used
    assign gpi_1[gen_gpi_signals_pkg::GPI_1_FM_PFR_FORCE_RECOVERY_N_BIT_POS] = FM_PFR_PROV_UPDATE_N;
    // unused GPI signals
    assign gpi_1[31:gen_gpi_signals_pkg::GPI_1_UNUSED_BITS_START_BIT_POS] = '0;
    
    ///////////////////////////////////////////////////////////////////////////
    // GPO Connectivity from Nios	
    ///////////////////////////////////////////////////////////////////////////
    // BMC Reset. Active low
    assign RST_SRST_BMC_PLD_R_N = gpo_1[gen_gpo_controls_pkg::GPO_1_RST_SRST_BMC_PLD_R_N_BIT_POS] & cc_RST_RSMRST_PLD_R_N;
    // PCH Reset. Active low
    // When Nios drives the GPO high, let common core drives the RSMRST#. 
    // The common core will have code that waits for this SLP_SUS signal before asserting RSMRST#.
    assign RST_RSMRST_PLD_R_N = (gpo_1[gen_gpo_controls_pkg::GPO_1_RST_RSMRST_PLD_R_N_BIT_POS]) ? cc_RST_RSMRST_PLD_R_N : 1'b0;
    
    // Mux select pins
    // PCH SPI Mux. 0-PCH, 1-CPLD
    assign FM_SPI_PFR_PCH_MASTER_SEL = gpo_1[gen_gpo_controls_pkg::GPO_1_FM_SPI_PFR_PCH_MASTER_SEL_BIT_POS];
    // BMC SPI Mux. 0-BMC, 1-CPLD
    assign FM_SPI_PFR_BMC_BT_MASTER_SEL = gpo_1[gen_gpo_controls_pkg::GPO_1_FM_SPI_PFR_BMC_BT_MASTER_SEL_BIT_POS];

    // Deep sleep power-ok to PCH
    // This signal is only supported on Wilson City fab 2 and beyond
    assign PWRGD_DSW_PWROK_R = (gpo_1[gen_gpo_controls_pkg::GPO_1_PWRGD_DSW_PWROK_R_BIT_POS]) ? 1'b0 : 1'bz;
    
    // BMC external reset. Used for BMC only update
    // Going away in fab2
    assign RST_PFR_EXTRST_N = gpo_1[gen_gpo_controls_pkg::GPO_1_RST_PFR_EXTRST_N_BIT_POS];
    
    // pin to allow the Nios to clear the pltrest detect register
    wire clear_pltrst_detect_flag;
    assign clear_pltrst_detect_flag = gpo_1[gen_gpo_controls_pkg::GPO_1_CLEAR_PLTRST_DETECT_FLAG_BIT_POS];

    ///////////////////////////////////////////////////////////////////////////
    // PLTRST_N edge detect   
    ///////////////////////////////////////////////////////////////////////////
    reg pltrst_reg;
    always_ff @(posedge sys_clk or posedge timer_bank_avmm_areset) begin
        if (timer_bank_avmm_areset) begin
            pltrst_reg <= 1'b0;
            rearm_acm_timer <=1'b0;
        end else begin 
            pltrst_reg <= RST_PLTRST_PLD_N;
            if (!pltrst_reg & RST_PLTRST_PLD_N) begin
                rearm_acm_timer <= 1'b1;
            end
            if (clear_pltrst_detect_flag) begin
                rearm_acm_timer <= 1'b0;
            end
        end
    end

    // SMBus Relay control pins
    logic relay1_block_disable;
    logic relay1_filter_disable;
    assign relay1_block_disable  = gpo_1[gen_gpo_controls_pkg::GPO_1_RELAY1_BLOCK_DISABLE_BIT_POS] ;
    assign relay1_filter_disable = gpo_1[gen_gpo_controls_pkg::GPO_1_RELAY1_FILTER_DISABLE_BIT_POS];
    logic relay2_block_disable;
    logic relay2_filter_disable;
    assign relay2_block_disable  = gpo_1[gen_gpo_controls_pkg::GPO_1_RELAY2_BLOCK_DISABLE_BIT_POS] ;
    assign relay2_filter_disable = gpo_1[gen_gpo_controls_pkg::GPO_1_RELAY2_FILTER_DISABLE_BIT_POS];
    logic relay3_block_disable;
    logic relay3_filter_disable;
    assign relay3_block_disable  = gpo_1[gen_gpo_controls_pkg::GPO_1_RELAY3_BLOCK_DISABLE_BIT_POS] ;
    assign relay3_filter_disable = gpo_1[gen_gpo_controls_pkg::GPO_1_RELAY3_FILTER_DISABLE_BIT_POS];

    // SPI control block control pins
    logic spi_control_pfr_bmc_master_sel;
    logic spi_control_pfr_pch_master_sel;
    logic spi_control_spi_master_bmc_pchn;
    logic spi_control_bmc_filter_disable;
    logic spi_control_pch_filter_disable;
    logic spi_control_bmc_clear_ibb_detected;
    logic spi_control_bmc_addr_mode_set_3b;
    assign spi_control_pfr_bmc_master_sel       = gpo_1[gen_gpo_controls_pkg::GPO_1_FM_SPI_PFR_BMC_BT_MASTER_SEL_BIT_POS];
    assign spi_control_pfr_pch_master_sel       = gpo_1[gen_gpo_controls_pkg::GPO_1_FM_SPI_PFR_PCH_MASTER_SEL_BIT_POS];
    assign spi_control_spi_master_bmc_pchn      = gpo_1[gen_gpo_controls_pkg::GPO_1_SPI_MASTER_BMC_PCHN_BIT_POS];
    assign spi_control_bmc_filter_disable       = gpo_1[gen_gpo_controls_pkg::GPO_1_BMC_SPI_FILTER_DISABLE_BIT_POS];
    assign spi_control_pch_filter_disable       = gpo_1[gen_gpo_controls_pkg::GPO_1_PCH_SPI_FILTER_DISABLE_BIT_POS];
    assign spi_control_bmc_clear_ibb_detected   = gpo_1[gen_gpo_controls_pkg::GPO_1_BMC_SPI_CLEAR_IBB_DETECTED_BIT_POS];
    assign spi_control_bmc_addr_mode_set_3b     = gpo_1[gen_gpo_controls_pkg::GPO_1_BMC_SPI_ADDR_MODE_SET_3B_BIT_POS];
    
    // PCH Resets. They are used to trigger Top Swap Reset during the last stage of PCH recovery. 
    assign RST_PFR_OVR_RTC_N = (gpo_1[gen_gpo_controls_pkg::GPO_1_TRIGGER_TOP_SWAP_RESET_BIT_POS]) ? 1'b0 : 1'bZ; 
    assign RST_PFR_OVR_SRTC_N = (gpo_1[gen_gpo_controls_pkg::GPO_1_TRIGGER_TOP_SWAP_RESET_BIT_POS]) ? 1'b0 : 1'bZ; 

    // Sleep suspend signal
    assign FM_PFR_SLP_SUS_N = gpo_1[gen_gpo_controls_pkg::GPO_1_FM_PFR_SLP_SUS_N_BIT_POS];
    
    ///////////////////////////////////////////////////////////////////////////
    // Unused toplevel pins. Drive to Z
    ///////////////////////////////////////////////////////////////////////////
    assign FAN_BMC_PWM_R = 'Z;
    assign SMB_CPU_PIROM_SDA = 'Z;
    assign SMB_BMC_SPD_ACCESS_STBY_LVC3_SDA = 'Z;
    assign SPI_CPU1_PFR_MISO_PLD_R = 'Z; 
    assign SPI_CPU2_PFR_MISO_PLD_R = 'Z; 
    
    ///////////////////////////////////////////////////////////////////////////
    // Unused toplevel pins that are going away in Wilson City FAB2
    ///////////////////////////////////////////////////////////////////////////
    assign FM_PFR_CPU1_FRMAGENT = 'Z; 
    assign FM_PFR_LEGACY_CPU1 = 'Z;
    assign SMB_PFR_DDRABCD_CPU2_LVC2_SDA = 'Z;
    assign SMB_PFR_DDREFGH_CPU2_LVC2_SDA = 'Z;
    
    // Drive to 0 for now. Going away in Fab2
    assign FM_PFR_CLK_MUX_SEL = 1'b0;
    // Drive to 0 for now. Going away in Fab2
    assign FM_PFR_CPU1_BMCINIT = 1'b0;
    // Drive to 0 for now. Going away in Fab2
    assign FM_PFR_CPU2_BMCINIT = 1'b0;

    /////////////////////////////////////////
    // REFACTOR THESE. CONNECT TO IPs AS REQUIRED
    /////////////////////////////////////////

    // TODO: PCH Mailbox
    assign SMB_PFR_DDREFGH_CPU1_LVC2_SDA = 'Z;
    
    // TODO: LED signaling
    assign FP_ID_BTN_PFR_N = FP_ID_BTN_N;
    assign FP_ID_LED_PFR_N = FP_ID_LED_N;
    assign FP_LED_STATUS_AMBER_PFR_N = FP_LED_STATUS_AMBER_N;
    assign FP_LED_STATUS_GREEN_PFR_N = FP_LED_STATUS_GREEN_N;
    
    // SPI 
    assign RST_SPI_PFR_BMC_BOOT_N = gpo_1[gen_gpo_controls_pkg::GPO_1_RST_SPI_PFR_BMC_BOOT_N_BIT_POS];
    assign RST_SPI_PFR_PCH_N = gpo_1[gen_gpo_controls_pkg::GPO_1_RST_SPI_PFR_PCH_N_BIT_POS];
     
    assign SPI_PFR_BMC_BT_SECURE_CS_N = 1'b1;
    assign SPI_PFR_PCH_SECURE_CS1_N = SPI_PCH_CS1_N;

    /////////////////////////////////////////
    /////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////////////
    // Timer bank
    ///////////////////////////////////////////////////////////////////////////
    timer_bank u_timer_bank (
        .clk(timer_bank_avmm_clk),
        .areset(timer_bank_avmm_areset),
        .i20msCE(i20mSCE),
        
        .avmm_address(timer_bank_avmm_address),
        .avmm_read(timer_bank_avmm_read),
        .avmm_write(timer_bank_avmm_write),
        .avmm_readdata(timer_bank_avmm_readdata),
        .avmm_writedata(timer_bank_avmm_writedata)
    );
    assign timer_bank_avmm_waitrequest = 1'b0;

    ///////////////////////////////////////////////////////////////////////////
    // Crypto block
    ///////////////////////////////////////////////////////////////////////////
    // Note that waitrequest can be tied low, and thus reduce the area of the
    // fabric, if the master guarantees to poll for data-ready after delivering
    // 128-bytes (32 words)
    crypto256_top #
    (
        .USE_ECDSA_BLOCK(1),
        .ECDSA_AUTHENTICATION_RESULT (0)
    ) u_crypto (
        .clk(crypto_avmm_clk),
        .areset(crypto_avmm_areset),
        
        .csr_address(crypto_avmm_address[3:0]),
        .csr_waitrequest(crypto_avmm_waitrequest),
        .csr_read(crypto_avmm_read),
        .csr_write(crypto_avmm_write),
        .csr_readdata(crypto_avmm_readdata),
        .csr_writedata(crypto_avmm_writedata)
    );
	 
	 
    ///////////////////////////////////////////////////////////////////////////
    // SMBus mailbox
    ///////////////////////////////////////////////////////////////////////////

    logic bmc_mailbox_slave_scl_in;
    logic bmc_mailbox_slave_scl_oe;
    logic bmc_mailbox_slave_sda_in;
    logic bmc_mailbox_slave_sda_oe;

    logic pch_mailbox_slave_scl_in;
    logic pch_mailbox_slave_scl_oe;
    logic pch_mailbox_slave_sda_in;
    logic pch_mailbox_slave_sda_oe;

    smbus_mailbox #(
            .PCH_SMBUS_ADDRESS(platform_defs_pkg::PCH_SMBUS_MAILBOX_ADDR),
            .BMC_SMBUS_ADDRESS(platform_defs_pkg::BMC_SMBUS_MAILBOX_ADDR)
    ) u_smbus_mailbox (
        .clk(mailbox_avmm_clk),
        .i_resetn(!mailbox_avmm_areset),

        .ia_bmc_slave_sda_in(bmc_mailbox_slave_sda_in),
        .o_bmc_slave_sda_oe(bmc_mailbox_slave_sda_oe), 
        .ia_bmc_slave_scl_in(bmc_mailbox_slave_scl_in),
        .o_bmc_slave_scl_oe(bmc_mailbox_slave_scl_oe),
        .ia_pch_slave_sda_in(pch_mailbox_slave_sda_in),
        .o_pch_slave_sda_oe(pch_mailbox_slave_sda_oe), 
        .ia_pch_slave_scl_in(pch_mailbox_slave_scl_in),
        .o_pch_slave_scl_oe(pch_mailbox_slave_scl_oe),

        .m0_read(mailbox_avmm_read),
        .m0_write(mailbox_avmm_write),
        .m0_writedata(mailbox_avmm_writedata),
        .m0_readdata(mailbox_avmm_readdata),
        .m0_address(mailbox_avmm_address),
        .m0_readdatavalid(mailbox_avmm_readdatavalid)
    );
    // Remove tristate buffer because SMB_PFR_DDRABCD_CPU1_LVC2_SCL & SMB_BMC_HSBP_STBY_LVC3_SCL is changed to input port
    // Not expecting clock stretching from SMBus mailbox
    //assign SMB_PFR_DDRABCD_CPU1_LVC2_SCL = pch_mailbox_slave_scl_oe ? 1'b0 : 1'bz;
    assign SMB_PFR_DDRABCD_CPU1_LVC2_SDA = pch_mailbox_slave_sda_oe ? 1'b0 : 1'bz;
    assign pch_mailbox_slave_scl_in = SMB_PFR_DDRABCD_CPU1_LVC2_SCL;
    assign pch_mailbox_slave_sda_in = SMB_PFR_DDRABCD_CPU1_LVC2_SDA;

    //assign SMB_BMC_HSBP_STBY_LVC3_SCL = bmc_mailbox_slave_scl_oe ? 1'b0 : 1'bz;
    assign SMB_BMC_HSBP_STBY_LVC3_SDA = bmc_mailbox_slave_sda_oe ? 1'b0 : 1'bz;
    assign bmc_mailbox_slave_scl_in = SMB_BMC_HSBP_STBY_LVC3_SCL;
    assign bmc_mailbox_slave_sda_in = SMB_BMC_HSBP_STBY_LVC3_SDA;

    assign mailbox_avmm_waitrequest =1'b0;

    ///////////////////////////////////////////////////////////////////////////
    // RFNVRAM Master
    ///////////////////////////////////////////////////////////////////////////
    logic rfnvram_sda_in;
    logic rfnvram_sda_oe;
    logic rfnvram_scl_in;
    logic rfnvram_scl_oe;

    rfnvram_smbus_master #(.FIFO_DEPTH(platform_defs_pkg::RFNVRAM_FIFO_SIZE))
    u_rfnvram_master (
        .clk(rfnvram_avmm_clk),
        .resetn(!rfnvram_avmm_areset),

        // AVMM interface to connect the NIOS to the CSR interface
        .csr_address(rfnvram_avmm_address),
        .csr_read(rfnvram_avmm_read),
        .csr_readdata(rfnvram_avmm_readdata),
        .csr_write(rfnvram_avmm_write),
        .csr_writedata(rfnvram_avmm_writedata),
        .csr_readdatavalid(rfnvram_avmm_readdatavalid),

        .sda_in(rfnvram_sda_in),
        .sda_oe(rfnvram_sda_oe),
        .scl_in(rfnvram_scl_in),
        .scl_oe(rfnvram_scl_oe)

    );

    assign SMB_PFR_RFID_STBY_LVC3_SCL = rfnvram_scl_oe ? 1'b0 : 1'bz;
    assign SMB_PFR_RFID_STBY_LVC3_SDA = rfnvram_sda_oe ? 1'b0 : 1'bz;
    assign rfnvram_scl_in = SMB_PFR_RFID_STBY_LVC3_SCL;
    assign rfnvram_sda_in = SMB_PFR_RFID_STBY_LVC3_SDA;
    assign rfnvram_avmm_waitrequest = 1'b0;

    ///////////////////////////////////////////////////////////////////////////
    // SPI Filter and master
    ///////////////////////////////////////////////////////////////////////////

    // signals for acting as SPI bus master on the BMC SPI bus
    logic [3:0]                          bmc_spi_master_data_in;        // data signals from the SPI data pins
    logic [3:0]                          bmc_spi_master_data_out;       // data signals driven to the SPI data pins
    logic [3:0]                          bmc_spi_master_data_oe;        // when asserted, bmc_spi_data drives the respective BMC SPI data pin

    // signals for acting as SPI bus master on the PCH SPI bus
    logic [3:0]                          pch_spi_master_data_in;        // data signals from the SPI data pins
    logic [3:0]                          pch_spi_master_data_out;       // data signals driven to the SPI data pins
    logic [3:0]                          pch_spi_master_data_oe;        // when asserted, pch_spi_data drives the respective PCH SPI data pin

    spi_control #(
        .BMC_IBB_ADDRESS_MSBS       ( platform_defs_pkg::BMC_IBB_ADDRESS_MSBS   ),
        .BMC_FLASH_ADDRESS_BITS     ( platform_defs_pkg::BMC_FLASH_ADDRESS_BITS ),
        .PCH_FLASH_ADDRESS_BITS     ( platform_defs_pkg::PCH_FLASH_ADDRESS_BITS )
    ) u_spi_control (
        .clock                      ( spi_clk                               ),
        .i_resetn                   ( spi_clk_reset_sync_n                  ),
        .sys_clock                  ( spi_filter_bmc_we_avmm_clk            ),
        .i_sys_resetn               ( !spi_filter_bmc_we_avmm_areset        ),
        .o_bmc_spi_master_sclk      ( SPI_PFR_BMC_FLASH1_BT_CLK             ),  // connect directly to output pin
        .i_bmc_spi_master_data      ( bmc_spi_master_data_in                ),
        .o_bmc_spi_master_data      ( bmc_spi_master_data_out               ),
        .o_bmc_spi_master_data_oe   ( bmc_spi_master_data_oe                ),
        .o_pch_spi_master_sclk      ( SPI_PFR_PCH_R_CLK                     ),  // connect directly to output pin
        .i_pch_spi_master_data      ( pch_spi_master_data_in                ),
        .o_pch_spi_master_data      ( pch_spi_master_data_out               ),
        .o_pch_spi_master_data_oe   ( pch_spi_master_data_oe                ),
        .clk_bmc_spi_mon_sclk       ( SPI_BMC_BT_MUXED_MON_CLK              ),
        .i_bmc_spi_mon_mosi         ( SPI_BMC_BT_MUXED_MON_MOSI             ),
        .i_bmc_spi_mon_csn          ( SPI_BMC_BOOT_CS_N                     ),
        .clk_pch_spi_mon_sclk       ( SPI_PCH_BMC_SAFS_MUXED_MON_CLK        ),
        .i_pch_spi_mon_data         ( {SPI_PCH_BMC_SAFS_MUXED_MON_IO3, SPI_PCH_BMC_SAFS_MUXED_MON_IO2, SPI_PCH_BMC_SAFS_MUXED_MON_IO1, SPI_PCH_BMC_SAFS_MUXED_MON_IO0} ),
        .i_pch_spi_mon_csn          ( SPI_PCH_BMC_PFR_CS0_N                 ),
        .o_bmc_spi_csn              ( SPI_PFR_BOOT_CS1_N                    ),
        .o_pch_spi_csn              ( SPI_PFR_PCH_BMC_SECURE_CS0_N          ),
        .i_pfr_bmc_master_sel       ( spi_control_pfr_bmc_master_sel        ),
        .i_pfr_pch_master_sel       ( spi_control_pfr_pch_master_sel        ),
        .i_spi_master_bmc_pchn      ( spi_control_spi_master_bmc_pchn       ),
        .i_bmc_filter_disable       ( spi_control_bmc_filter_disable        ),
        .i_pch_filter_disable       ( spi_control_pch_filter_disable        ),
        .o_bmc_ibb_access_detected  ( spi_control_bmc_ibb_access_detected   ),
        .i_bmc_clear_ibb_detected   ( spi_control_bmc_clear_ibb_detected    ),
        .i_bmc_addr_mode_set_3b     ( spi_control_bmc_addr_mode_set_3b      ),
        .o_bmc_filtered_command_info( bmc_spi_filtered_command_info         ),
        .o_pch_filtered_command_info( pch_spi_filtered_command_info         ),
        .i_avmm_bmc_we_write        ( spi_filter_bmc_we_avmm_write          ),
        .i_avmm_bmc_we_address      ( spi_filter_bmc_we_avmm_address[platform_defs_pkg::BMC_WE_AVMM_ADDRESS_BITS-1:0]),
        .i_avmm_bmc_we_writedata    ( spi_filter_bmc_we_avmm_writedata      ),
        .i_avmm_pch_we_write        ( spi_filter_pch_we_avmm_write          ),
        .i_avmm_pch_we_address      ( spi_filter_pch_we_avmm_address[platform_defs_pkg::PCH_WE_AVMM_ADDRESS_BITS-1:0]),
        .i_avmm_pch_we_writedata    ( spi_filter_pch_we_avmm_writedata      ),
        .i_avmm_csr_address         ( spi_master_csr_avmm_address           ),
        .i_avmm_csr_read            ( spi_master_csr_avmm_read              ),
        .i_avmm_csr_write           ( spi_master_csr_avmm_write             ),
        .o_avmm_csr_waitrequest     ( spi_master_csr_avmm_waitrequest       ),
        .i_avmm_csr_writedata       ( spi_master_csr_avmm_writedata         ),
        .o_avmm_csr_readdata        ( spi_master_csr_avmm_readdata          ),
        .o_avmm_csr_readdatavalid   ( spi_master_csr_avmm_readdatavalid     ),
        .i_avmm_mem_address         ( spi_master_avmm_address               ),
        .i_avmm_mem_read            ( spi_master_avmm_read                  ),
        .i_avmm_mem_write           ( spi_master_avmm_write                 ),
        .o_avmm_mem_waitrequest     ( spi_master_avmm_waitrequest           ),
        .i_avmm_mem_writedata       ( spi_master_avmm_writedata             ),
        .o_avmm_mem_readdata        ( spi_master_avmm_readdata              ),
        .o_avmm_mem_readdatavalid   ( spi_master_avmm_readdatavalid         )
    );
    
    // implement tri-state drivers and input connections for SPI master data pins
    assign bmc_spi_master_data_in       = {SPI_PFR_BMC_BOOT_R_IO3, SPI_PFR_BMC_BOOT_R_IO2, SPI_PFR_BMC_FLASH1_BT_MISO, SPI_PFR_BMC_FLASH1_BT_MOSI};
    assign SPI_PFR_BMC_BOOT_R_IO3       = bmc_spi_master_data_oe[3] ? bmc_spi_master_data_out[3] : 1'bz;
    assign SPI_PFR_BMC_BOOT_R_IO2       = bmc_spi_master_data_oe[2] ? bmc_spi_master_data_out[2] : 1'bz;
    assign SPI_PFR_BMC_FLASH1_BT_MISO   = bmc_spi_master_data_oe[1] ? bmc_spi_master_data_out[1] : 1'bz;
    assign SPI_PFR_BMC_FLASH1_BT_MOSI   = bmc_spi_master_data_oe[0] ? bmc_spi_master_data_out[0] : 1'bz;
    assign pch_spi_master_data_in       = {SPI_PFR_PCH_R_IO3, SPI_PFR_PCH_R_IO2, SPI_PFR_PCH_R_IO1, SPI_PFR_PCH_R_IO0};
    assign SPI_PFR_PCH_R_IO3            = pch_spi_master_data_oe[3] ? pch_spi_master_data_out[3] : 1'bz;
    assign SPI_PFR_PCH_R_IO2            = pch_spi_master_data_oe[2] ? pch_spi_master_data_out[2] : 1'bz;
    assign SPI_PFR_PCH_R_IO1            = pch_spi_master_data_oe[1] ? pch_spi_master_data_out[1] : 1'bz;
    assign SPI_PFR_PCH_R_IO0            = pch_spi_master_data_oe[0] ? pch_spi_master_data_out[0] : 1'bz;

    //#########################################################################
    // SMBus Relays
    //#########################################################################

    
    ///////////////////////////////
    // Relay 1:
    // SMB_PMBUS_SML1_STBY_LVC3_SCL => SMB_PFR_PMB1_STBY_LVC3_SCL
    // SMB_PMBUS_SML1_STBY_LVC3_SDA => SMB_PFR_PMB1_STBY_LVC3_SDA

    // logic to implement the open-drain pins for scl/sda on the slave and master busses
    logic smbus_relay1_master_scl_in;
    logic smbus_relay1_master_scl_oe;
    logic smbus_relay1_master_sda_in;
    logic smbus_relay1_master_sda_oe;
    logic smbus_relay1_slave_scl_in;
    logic smbus_relay1_slave_scl_oe;
    logic smbus_relay1_slave_sda_in;
    logic smbus_relay1_slave_sda_oe;
    assign smbus_relay1_master_scl_in = SMB_PMBUS_SML1_STBY_LVC3_SCL;
    assign smbus_relay1_master_sda_in = SMB_PMBUS_SML1_STBY_LVC3_SDA;
    assign smbus_relay1_slave_scl_in  = SMB_PFR_PMB1_STBY_LVC3_SCL;
    assign smbus_relay1_slave_sda_in  = SMB_PFR_PMB1_STBY_LVC3_SDA;
    assign SMB_PMBUS_SML1_STBY_LVC3_SCL = smbus_relay1_master_scl_oe ? 1'b0 : 1'bz;
    assign SMB_PMBUS_SML1_STBY_LVC3_SDA = smbus_relay1_master_sda_oe ? 1'b0 : 1'bz;
    assign SMB_PFR_PMB1_STBY_LVC3_SCL   = smbus_relay1_slave_scl_oe ? 1'b0 : 1'bz;
    assign SMB_PFR_PMB1_STBY_LVC3_SDA   = smbus_relay1_slave_sda_oe ? 1'b0 : 1'bz;
    
    // Instantiate the relay block with no address or command filtering enabled
    smbus_filtered_relay #(
        .FILTER_ENABLE      ( 1                                                 ),      // enable command filtering
        .RELAY_ALL_ADDRESSES( 0                                                 ),      // only pass transactions to recognized SMBus addresses
        .CLOCK_PERIOD_PS    ( platform_defs_pkg::SYS_CLOCK_PERIOD_PS            ),
        .BUS_SPEED_KHZ      ( platform_defs_pkg::RELAY1_BUS_SPEED_KHZ           ),
        .NUM_RELAY_ADDRESSES( gen_smbus_relay_config_pkg::RELAY1_NUM_ADDRESSES  ),
        .SMBUS_RELAY_ADDRESS( gen_smbus_relay_config_pkg::RELAY1_I2C_ADDRESSES  )
    ) u_smbus_filtered_relay_1 (
        .clock              ( sys_clk                       ),
        .i_resetn           ( sys_clk_reset_sync_n          ),
        .i_block_disable    ( relay1_block_disable          ),
        .i_filter_disable   ( relay1_filter_disable         ),
        .ia_master_scl      ( smbus_relay1_master_scl_in    ),
        .o_master_scl_oe    ( smbus_relay1_master_scl_oe    ),
        .ia_master_sda      ( smbus_relay1_master_sda_in    ),
        .o_master_sda_oe    ( smbus_relay1_master_sda_oe    ),
        .ia_slave_scl       ( smbus_relay1_slave_scl_in     ),
        .o_slave_scl_oe     ( smbus_relay1_slave_scl_oe     ),
        .ia_slave_sda       ( smbus_relay1_slave_sda_in     ),
        .o_slave_sda_oe     ( smbus_relay1_slave_sda_oe     ),
        .i_avmm_write       ( relay1_avmm_write             ),
        .i_avmm_address     ( relay1_avmm_address           ),
        .i_avmm_writedata   ( relay1_avmm_writedata         )
    );
    

    ///////////////////////////////
    // Relay 2:
    // SMB_PFR_PMBUS2_STBY_LVC3_SCL => SMB_PCH_PMBUS2_STBY_LVC3_SCL
    // SMB_PFR_PMBUS2_STBY_LVC3_SDA => SMB_PCH_PMBUS2_STBY_LVC3_SDA
    // Note the Master SMBus pins are shared with the interface to the Common Core register file

    // logic to implement the open-drain pins for scl/sda on the slave and master busses
    logic smbus_relay2_master_scl_in;
    logic smbus_relay2_master_scl_oe;
    logic smbus_relay2_master_sda_in;
    logic smbus_relay2_master_sda_oe;
    logic smbus_relay2_slave_scl_in;
    logic smbus_relay2_slave_scl_oe;
    logic smbus_relay2_slave_sda_in;
    logic smbus_relay2_slave_sda_oe;
    assign smbus_relay2_master_scl_in = SMB_PCH_PMBUS2_STBY_LVC3_SCL;
    assign smbus_relay2_master_sda_in = SMB_PCH_PMBUS2_STBY_LVC3_SDA;
    assign smbus_relay2_slave_scl_in  = SMB_PFR_PMBUS2_STBY_LVC3_SCL;
    assign smbus_relay2_slave_sda_in  = SMB_PFR_PMBUS2_STBY_LVC3_SDA;
    assign SMB_PCH_PMBUS2_STBY_LVC3_SCL = smbus_relay2_master_scl_oe ? 1'b0 : 1'bz;
    assign SMB_PCH_PMBUS2_STBY_LVC3_SDA = (smbus_relay2_master_sda_oe || !ccSMB_PCH_PMBUS2_STBY_LVC3_SDA_OEn) ? 1'b0 : 1'bz;  // common core SMBus register file needs to be able to drive this open-drain signal as well
    assign SMB_PFR_PMBUS2_STBY_LVC3_SCL = smbus_relay2_slave_scl_oe ? 1'b0 : 1'bz;
    assign SMB_PFR_PMBUS2_STBY_LVC3_SDA = smbus_relay2_slave_sda_oe ? 1'b0 : 1'bz;
    
    // Instantiate the relay block with no address or command filtering enabled
    smbus_filtered_relay #(
        .FILTER_ENABLE      ( 1                                                 ),      // enable command filtering
        .RELAY_ALL_ADDRESSES( 0                                                 ),      // only pass transactions to recognized SMBus addresses
        .CLOCK_PERIOD_PS    ( platform_defs_pkg::SYS_CLOCK_PERIOD_PS            ),
        .BUS_SPEED_KHZ      ( platform_defs_pkg::RELAY2_BUS_SPEED_KHZ           ),
        .NUM_RELAY_ADDRESSES( gen_smbus_relay_config_pkg::RELAY2_NUM_ADDRESSES  ),
        .SMBUS_RELAY_ADDRESS( gen_smbus_relay_config_pkg::RELAY2_I2C_ADDRESSES  )
    ) u_smbus_filtered_relay_2 (
        .clock              ( sys_clk                       ),
        .i_resetn           ( sys_clk_reset_sync_n          ),
        .i_block_disable    ( relay2_block_disable          ),
        .i_filter_disable   ( relay2_filter_disable         ),
        .ia_master_scl      ( smbus_relay2_master_scl_in    ),
        .o_master_scl_oe    ( smbus_relay2_master_scl_oe    ),
        .ia_master_sda      ( smbus_relay2_master_sda_in    ),
        .o_master_sda_oe    ( smbus_relay2_master_sda_oe    ),
        .ia_slave_scl       ( smbus_relay2_slave_scl_in     ),
        .o_slave_scl_oe     ( smbus_relay2_slave_scl_oe     ),
        .ia_slave_sda       ( smbus_relay2_slave_sda_in     ),
        .o_slave_sda_oe     ( smbus_relay2_slave_sda_oe     ),
        .i_avmm_write       ( relay2_avmm_write             ),
        .i_avmm_address     ( relay2_avmm_address           ),
        .i_avmm_writedata   ( relay2_avmm_writedata         )
    );

    ///////////////////////////////
    // Relay 3:
    //SMB_PFR_HSBP_STBY_LVC3_SCL => SMB_BMC_HSBP_STBY_LVC3_SCL
    //SMB_PFR_HSBP_STBY_LVC3_SDA => SMB_BMC_HSBP_STBY_LVC3_SDA

    // logic to implement the open-drain pins for scl/sda on the slave and master busses
    logic smbus_relay3_master_scl_in;
    logic smbus_relay3_master_scl_oe;
    logic smbus_relay3_master_sda_in;
    logic smbus_relay3_master_sda_oe;
    logic smbus_relay3_slave_scl_in;
    logic smbus_relay3_slave_scl_oe;
    logic smbus_relay3_slave_sda_in;
    logic smbus_relay3_slave_sda_oe;
    assign smbus_relay3_master_scl_in = SMB_BMC_HSBP_STBY_LVC3_SCL;
    assign smbus_relay3_master_sda_in = SMB_BMC_HSBP_STBY_LVC3_SDA;
    assign smbus_relay3_slave_scl_in  = SMB_PFR_HSBP_STBY_LVC3_SCL;
    assign smbus_relay3_slave_sda_in  = SMB_PFR_HSBP_STBY_LVC3_SDA;
    assign SMB_BMC_HSBP_STBY_LVC3_SCL = smbus_relay3_master_scl_oe ? 1'b0 : 1'bz;
    assign SMB_BMC_HSBP_STBY_LVC3_SDA = smbus_relay3_master_sda_oe ? 1'b0 : 1'bz;
    assign SMB_PFR_HSBP_STBY_LVC3_SCL = smbus_relay3_slave_scl_oe ? 1'b0 : 1'bz;
    assign SMB_PFR_HSBP_STBY_LVC3_SDA   = smbus_relay3_slave_sda_oe ? 1'b0 : 1'bz;
    
    // Instantiate the relay block with no address or command filtering enabled
    smbus_filtered_relay #(
        .FILTER_ENABLE      ( 1                                                 ),      // enable command filtering
        .RELAY_ALL_ADDRESSES( 0                                                 ),      // only pass transactions to recognized SMBus addresses
        .CLOCK_PERIOD_PS    ( platform_defs_pkg::SYS_CLOCK_PERIOD_PS            ),
        .BUS_SPEED_KHZ      ( platform_defs_pkg::RELAY3_BUS_SPEED_KHZ           ),
        .NUM_RELAY_ADDRESSES( gen_smbus_relay_config_pkg::RELAY3_NUM_ADDRESSES  ),
        .SMBUS_RELAY_ADDRESS( gen_smbus_relay_config_pkg::RELAY3_I2C_ADDRESSES  )
    ) u_smbus_filtered_relay_3 (
        .clock              ( sys_clk                       ),
        .i_resetn           ( sys_clk_reset_sync_n          ),
        .i_block_disable    ( relay3_block_disable          ),
        .i_filter_disable   ( relay3_filter_disable         ),
        .ia_master_scl      ( smbus_relay3_master_scl_in    ),
        .o_master_scl_oe    ( smbus_relay3_master_scl_oe    ),
        .ia_master_sda      ( smbus_relay3_master_sda_in    ),
        .o_master_sda_oe    ( smbus_relay3_master_sda_oe    ),
        .ia_slave_scl       ( smbus_relay3_slave_scl_in     ),
        .o_slave_scl_oe     ( smbus_relay3_slave_scl_oe     ),
        .ia_slave_sda       ( smbus_relay3_slave_sda_in     ),
        .o_slave_sda_oe     ( smbus_relay3_slave_sda_oe     ),
        .i_avmm_write       ( relay3_avmm_write             ),
        .i_avmm_address     ( relay3_avmm_address           ),
        .i_avmm_writedata   ( relay3_avmm_writedata         )
    );


    //#########################################################################
    // LED Control
    //#########################################################################
    logic [6:0] pfr_seven_seg;
    always_comb begin
        if (!sys_clk_reset_sync_n)
           pfr_seven_seg = 7'b1000000;
        else
            case (ccFM_POST_7SEG1_SEL_N ? global_state[3:0] : global_state[7:4])
                7'd0:  pfr_seven_seg = 7'b1000000; //0 
                7'd1:  pfr_seven_seg = 7'b1111001; //1 
                7'd2:  pfr_seven_seg = 7'b0100100; //2
                7'd3:  pfr_seven_seg = 7'b0110000; //3 
                7'd4:  pfr_seven_seg = 7'b0011001; //4
                7'd5:  pfr_seven_seg = 7'b0010010; //5 .
                7'd6:  pfr_seven_seg = 7'b0000010; //6 
                7'd7:  pfr_seven_seg = 7'b1111000; //7 
                7'd8:  pfr_seven_seg = 7'b0000000; //8 
                7'd9:  pfr_seven_seg = 7'b0011000; //9 
                7'd10: pfr_seven_seg = 7'b0001000; //A 
                7'd11: pfr_seven_seg = 7'b0000011; //B 
                7'd12: pfr_seven_seg = 7'b1000110; //C 
                7'd13: pfr_seven_seg = 7'b0100001; //D 
                7'd14: pfr_seven_seg = 7'b0000110; //E  
                7'd15: pfr_seven_seg = 7'b0001110; //F 
                default: 
                       pfr_seven_seg = 7'b1000000; //0 
            endcase
    end 

    always_comb begin
        // Defaults
        LED_CONTROL_0 = ccLED_CONTROL_0;
        LED_CONTROL_1 = ccLED_CONTROL_1;
        LED_CONTROL_2 = ccLED_CONTROL_2;
        LED_CONTROL_3 = ccLED_CONTROL_3;
        LED_CONTROL_4 = ccLED_CONTROL_4;
        LED_CONTROL_5 = ccLED_CONTROL_5;
        LED_CONTROL_6 = ccLED_CONTROL_6;
        LED_CONTROL_7 = ccLED_CONTROL_7;

        // Is CPLD Debug enabled?
        if (!FM_PFR_DEBUG_MODE_N) begin
            if (!ccFM_POST_7SEG1_SEL_N) begin
                // Display on the first 7-seg
                LED_CONTROL_0 = pfr_seven_seg[0];
                LED_CONTROL_1 = pfr_seven_seg[1];
                LED_CONTROL_2 = pfr_seven_seg[2];
                LED_CONTROL_3 = pfr_seven_seg[3];
                LED_CONTROL_4 = pfr_seven_seg[4];
                LED_CONTROL_5 = pfr_seven_seg[5];
                LED_CONTROL_6 = pfr_seven_seg[6];
                // Decimal point
                LED_CONTROL_7 = 1'b0;
            end
            else if (!ccFM_POST_7SEG2_SEL_N) begin
                // Display on the second 7-seg
                LED_CONTROL_0 = pfr_seven_seg[0];
                LED_CONTROL_1 = pfr_seven_seg[1];
                LED_CONTROL_2 = pfr_seven_seg[2];
                LED_CONTROL_3 = pfr_seven_seg[3];
                LED_CONTROL_4 = pfr_seven_seg[4];
                LED_CONTROL_5 = pfr_seven_seg[5];
                LED_CONTROL_6 = pfr_seven_seg[6];
                // Decimal point
                LED_CONTROL_7 = 1'b0;
            end
            else if (ccFM_POSTLED_SEL) begin
                // Display on the post code LEDs
                LED_CONTROL_0 = 1'b1; // Green
                LED_CONTROL_1 = 1'b1; // Green
                LED_CONTROL_2 = 1'b1; // Green
                LED_CONTROL_3 = 1'b1; // Green 
                LED_CONTROL_4 = 1'b0; // Amber
                LED_CONTROL_5 = 1'b0; // Amber
                LED_CONTROL_6 = 1'b0; // Amber
                LED_CONTROL_7 = 1'b0; // Amber 
            end
        end
    end
    
    // Pass through the control signals
    assign FM_POST_7SEG1_SEL_N = ccFM_POST_7SEG1_SEL_N;
    assign FM_POST_7SEG2_SEL_N = ccFM_POST_7SEG2_SEL_N;
    assign FM_POSTLED_SEL = ccFM_POSTLED_SEL;


endmodule
    
