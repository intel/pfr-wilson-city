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


// spi_control
//
// This module encapusulates the SPI Master and SPI Filter functionality for the Root of Trust CPLD design.
// It interfaces with two SPI busses, one for the BMC and one for the PCH.
//
// The SPI Master block is a modified version of the Generic Serial Flash Interface Intel FPGA IP Core provided
// in Quartus 18.1 Standard edition.  The modifications involve hard-coding certains parameters to save
// area, and modifying the I/O ports to make sharing the master with two SPI busses simpler.  The single SPI
// master block can be configured at runtime to interface to either the BMC or the PCH flash device.
//
// There are two SPI filter blocks, one for the BMC and one for the PCH SPI bus.  These blocks monitor traffic
// coming from the BMC/PCH and filter the CSn (chip select) signal to prevent illegal commands.  Each filter block
// has a 'write enable' memory.  Each bit in this memory represents whether write commands to a 4kB block of FLASH
// is enabled for program and erase commands. 

`timescale 1 ps / 1 ps
`default_nettype none

module spi_control #(
    parameter [31:16] BMC_IBB_ADDRESS_MSBS  = 16'h0000,                             // 16 msbs out of a 32 bit SPI address that indicate an access to the IBB sector
    parameter   BMC_FLASH_ADDRESS_BITS      = 26,                                   // number of BYTE-based address bits supported by the BMC FLASH device (26 bits = 64 MBytes = 512 Mbit)
    parameter   PCH_FLASH_ADDRESS_BITS      = 27,                                   // number of BYTE-based address bits supported by the PCH FLASH device (27 bits = 128 MBytes = 1 Gbit)

    // the following parameters must all be left at their default values, they should not be modified
    parameter   AVMM_FLASH_ADDRESS_BITS     = BMC_FLASH_ADDRESS_BITS > PCH_FLASH_ADDRESS_BITS ? BMC_FLASH_ADDRESS_BITS-2 : PCH_FLASH_ADDRESS_BITS-2,    // -2 to convert from bytes to 32-bit AVMM word addresses
    parameter   BMC_WE_AVMM_ADDRESS_BITS    = platform_defs_pkg::BMC_WE_AVMM_ADDRESS_BITS,
    parameter   PCH_WE_AVMM_ADDRESS_BITS    = platform_defs_pkg::PCH_WE_AVMM_ADDRESS_BITS
) (
    input  wire                                 sys_clock,                          // system clock
    input  wire                                 clock,                          // SPI clock frequency, unless otherwise specified all inputs and outputs are synchronous with this clock
    input  wire                                 i_resetn,
    input  wire                                 i_sys_resetn,
    
    // SPI signals for our internal SPI Master to interface with the BMC FLASH device (CSn signal is shared with the filter interface and defined elsewhere)
    output logic                                o_bmc_spi_master_sclk,          // SPI clock signal from the SPI master block
    input  wire  [3:0]                          i_bmc_spi_master_data,          // data signals from the SPI data pins
    output logic [3:0]                          o_bmc_spi_master_data,          // data signals driven to the SPI data pins
    output logic [3:0]                          o_bmc_spi_master_data_oe,       // when asserted, o_bmc_spi_data drives the BMC SPI pins

    // SPI signals for our internal SPI Master to interface with the PCH FLASH device (CSn signal is shared with the filter interface and defined elsewhere)
    output logic                                o_pch_spi_master_sclk,          // SPI clock signal from the SPI master block
    input  wire  [3:0]                          i_pch_spi_master_data,          // data signals from the SPI data pins
    output logic [3:0]                          o_pch_spi_master_data,          // data signals driven to the SPI data pins
    output logic [3:0]                          o_pch_spi_master_data_oe,       // when asserted, o_bmc_spi_data drives the BMC SPI pins

    // SPI signals for monitoring and filtering activity to the BMC FLASH device (input signals are synchronous with clk_bmc_spi_mon_sclk)
    input  wire                                 clk_bmc_spi_mon_sclk,           // clock from the SPI interface, used to clock internal SPI filtering logic
    input  wire                                 i_bmc_spi_mon_mosi,             // master out slave in data bit - BMC only supports a single data bit, and we have no need to monitor the MISO (data returned from slave) for filtering
    input  wire                                 i_bmc_spi_mon_csn,              // active low chip select from the BMC, we filter this signal then pass it through (when appropriate) to o_bmc_spi_csn

    // SPI signals for monitoring and filtering activity to the PCH FLASH device (input signals are synchronous with clk_pch_spi_mon_sclk)
    input  wire                                 clk_pch_spi_mon_sclk,           // clock from the SPI interface, used to clock internal SPI filtering logic
    input  wire  [3:0]                          i_pch_spi_mon_data,             // data bits to/from the PCH
    input  wire                                 i_pch_spi_mon_csn,              // active low chip select from the PCH, we filter this signal then pass it through (when appropriate) to o_pch_spi_csn
    
    // chip select signals to SPI FLASH devices
    // these signals can come from the internal master (in which case they are synchronous with clock) or from the bmc/pch interfaces (in which case they are synchronous with the relevant sclk input)
    output logic                                o_bmc_spi_csn,
    output logic                                o_pch_spi_csn,

    // control and status bits for the internal blocks
    input  wire                                 i_pfr_bmc_master_sel,           // 1 - internal SPI master drives the BMC CSn signal, 0 - BMC drives the CSn signal (through the SPI filter block)
    input  wire                                 i_pfr_pch_master_sel,           // 1 - internal SPI master drives the PCH CSn signal, 0 - PCH drives the CSn signal (through the SPI filter block)
    input  wire                                 i_spi_master_bmc_pchn,          // 1 - internal SPI master connects to the BMC SPI bus, 0 - internal SPI master connects tot he PCH SPI bus
    input  wire                                 i_bmc_filter_disable,           // set to 1 to put the BMC SPI filter into 'permissive mode' where all commands are allowed
    input  wire                                 i_pch_filter_disable,           // set to 1 to put the PCH SPI filter into 'permissive mode' where all commands are allowed
    output logic                                o_bmc_ibb_access_detected,      // indicates that the BMC has accessed an address in its Initial Boot Block range
    input  wire                                 i_bmc_clear_ibb_detected,       // assert to clear the above signal, then deassert to re-enable detection
    input  wire                                 i_bmc_addr_mode_set_3b,         // assert to tell the BMC filter that the SPI FLASH device is in 3B address mode (thus write commands are blocked), de-assert to allow the filter to detect the ENTER_4B_ADDR_MODE command
    output logic [31:0]                         o_bmc_filtered_command_info,    // 31:14 - address bits 31:14 of filtered command, 13 - 1=illegal command, 0=illegal write/erase region, 12:8- number of filtered commands, 7:0 - command that was filtered
    output logic [31:0]                         o_pch_filtered_command_info,    // 31:14 - address bits 31:14 of filtered command, 13 - 1=illegal command, 0=illegal write/erase region, 12:8- number of filtered commands, 7:0 - command that was filtered

    // AVMM slave interface to access the write enable mask memory for the BMC (note that reads from this memory are not supported)
    // This interface uses the sys_clock
    input  wire                                 i_avmm_bmc_we_write,
    input  wire  [BMC_WE_AVMM_ADDRESS_BITS-1:0] i_avmm_bmc_we_address,
    input  wire  [31:0]                         i_avmm_bmc_we_writedata,
    
    // AVMM slave interface to access the write enable mask memory for the PCH (note that reads from this memory are not supported)
    // This interface uses the sys_clock
    input  wire                                 i_avmm_pch_we_write,
    input  wire  [PCH_WE_AVMM_ADDRESS_BITS-1:0] i_avmm_pch_we_address,
    input  wire  [31:0]                         i_avmm_pch_we_writedata,

    // AVMM interface to connect the NIOS to the SPI Master CSR interface
    input  wire  [5:0]                          i_avmm_csr_address,
    input  wire                                 i_avmm_csr_read,
    input  wire                                 i_avmm_csr_write,
    output logic                                o_avmm_csr_waitrequest,
    input  wire  [31:0]                         i_avmm_csr_writedata,
    output logic [31:0]                         o_avmm_csr_readdata,
    output logic                                o_avmm_csr_readdatavalid,

    // AVMM interface to map the SPI master interface into the NIOS memory space
    input  wire  [AVMM_FLASH_ADDRESS_BITS-1:0]  i_avmm_mem_address,             // -2 to convert from byte addresses to 32-bit word addresses
    input  wire                                 i_avmm_mem_read,
    input  wire                                 i_avmm_mem_write,
    output logic                                o_avmm_mem_waitrequest,
    input  wire  [31:0]                         i_avmm_mem_writedata,
    output logic [31:0]                         o_avmm_mem_readdata,
    output logic                                o_avmm_mem_readdatavalid
);

    ///////////////////////////////////////
    // Parameter checking
    //
    // Generate an error if any illegal parameter settings or combinations are used
    ///////////////////////////////////////
    initial /* synthesis enable_verilog_initial_construct */
    begin
        if (AVMM_FLASH_ADDRESS_BITS != (BMC_FLASH_ADDRESS_BITS > PCH_FLASH_ADDRESS_BITS ? BMC_FLASH_ADDRESS_BITS-2 : PCH_FLASH_ADDRESS_BITS-2))
            $fatal(1, "Illegal parameterization: AVMM_FLASH_ADDRESS_BITS should always be left with the default assignment");
        if (BMC_WE_AVMM_ADDRESS_BITS != BMC_FLASH_ADDRESS_BITS-14-5)
            $fatal(1, "Illegal parameterization: BMC_WE_AVMM_ADDRESS_BITS should always be left with the default assignment");
        if (PCH_WE_AVMM_ADDRESS_BITS != PCH_FLASH_ADDRESS_BITS-14-5)
            $fatal(1, "Illegal parameterization: PCH_WE_AVMM_ADDRESS_BITS should always be left with the default assignment");
    end


    ///////////////////////////////////////
    // internal signals
    ///////////////////////////////////////
    logic           spi_master_sclk     ;       // SPI clock out of the internal SPI master
    logic [3:0]     spi_master_data_out ;       // output data from the internal SPI master
    logic [3:0]     spi_master_data_oe  ;       // output data enable from the internal SPI master
    logic [3:0]     spi_master_data_in  ;       // input data sent to the internal SPI master, comes from the SPI data pins
    logic           spi_master_csn      ;       // SPI Chip Select (active low) from the internal SPI master
    logic           bmc_spi_filter_csn  ;       // SPI Chip Select (active low) from the BMC after passing through the SPI filter
    logic           pch_spi_filter_csn  ;       // SPI Chip Select (active low) from the PCH after passing through the SPI filter


    ///////////////////////////////////////
    // data and control muxes for the SPI busses
    ///////////////////////////////////////
    always_comb begin

        if ( i_pfr_bmc_master_sel ) begin
            if (i_spi_master_bmc_pchn) begin
                o_bmc_spi_master_sclk       = spi_master_sclk       ;
                o_bmc_spi_master_data       = spi_master_data_out   ;
                o_bmc_spi_master_data_oe    = spi_master_data_oe    ;
                o_bmc_spi_csn               = spi_master_csn        ;
            end else begin
                o_bmc_spi_master_sclk       = '0                    ;
                o_bmc_spi_master_data       = '0                    ;
                o_bmc_spi_master_data_oe    = '0                    ;
                o_bmc_spi_csn               = '1                    ;
            end
        end else begin
            o_bmc_spi_master_sclk       = '0                    ;
            o_bmc_spi_master_data       = '0                    ;
            o_bmc_spi_master_data_oe    = '0                    ;
            o_bmc_spi_csn               = bmc_spi_filter_csn    ;
        end
        
        if ( i_pfr_pch_master_sel ) begin
            if (~i_spi_master_bmc_pchn) begin
                o_pch_spi_master_sclk       = spi_master_sclk       ;
                o_pch_spi_master_data       = spi_master_data_out   ;
                o_pch_spi_master_data_oe    = spi_master_data_oe    ;
                o_pch_spi_csn               = spi_master_csn        ;
            end else begin
                o_pch_spi_master_sclk       = '0                    ;
                o_pch_spi_master_data       = '0                    ;
                o_pch_spi_master_data_oe    = '0                    ;
                o_pch_spi_csn               = '1                    ;
            end
        end else begin
            o_pch_spi_master_sclk       = '0                    ;
            o_pch_spi_master_data       = '0                    ;
            o_pch_spi_master_data_oe    = '0                    ;
            o_pch_spi_csn               = pch_spi_filter_csn    ;
        end
        
        spi_master_data_in = i_spi_master_bmc_pchn ? i_bmc_spi_master_data : i_pch_spi_master_data;
        
    end



    ///////////////////////////////////////
    // instantiate the SPI master (modified version of an IP block from QSYS)
    ///////////////////////////////////////
    spi_master_spi_master spi_master_inst (
        .clk_clk                    ( clock                     ),
        .reset_reset                ( ~i_resetn                 ),
        .avl_csr_address            ( i_avmm_csr_address        ),
        .avl_csr_read               ( i_avmm_csr_read           ),
        .avl_csr_write              ( i_avmm_csr_write          ),
        .avl_csr_waitrequest        ( o_avmm_csr_waitrequest    ),
        .avl_csr_writedata          ( i_avmm_csr_writedata      ),
        .avl_csr_readdata           ( o_avmm_csr_readdata       ),
        .avl_csr_readdatavalid      ( o_avmm_csr_readdatavalid  ),
        .avl_mem_address            ( i_avmm_mem_address        ),
        .avl_mem_read               ( i_avmm_mem_read           ),
        .avl_mem_write              ( i_avmm_mem_write          ),
        .avl_mem_waitrequest        ( o_avmm_mem_waitrequest    ),
        .avl_mem_writedata          ( i_avmm_mem_writedata       ),
        .avl_mem_readdata           ( o_avmm_mem_readdata       ),
        .avl_mem_readdatavalid      ( o_avmm_mem_readdatavalid  ),
        .avl_mem_byteenable         ( 4'b1111                   ),      // byteenables not supported
        .avl_mem_burstcount         ( 7'b0000001                ),      // bursts not supported
        .qspi_pins_dclk             ( spi_master_sclk           ),
        .qspi_pins_ncs              ( spi_master_csn            ),
        .qspi_pins_data_out         ( spi_master_data_out       ),
        .qspi_pins_data_oe          ( spi_master_data_oe        ),
        .qspi_pins_data_in          ( spi_master_data_in        )
    );

    
    
    ///////////////////////////////////////
    // Instantiate the SPI filter blocks
    ///////////////////////////////////////
    logic bmc_filter_disable_cs;
    logic pch_filter_disable_cs;
    
    spi_filter #(
        .ENABLE_IBB_DETECT      ( 1                             ),
        .IBB_ADDRESS_MSBS       ( 16'h0000                      ),
        .FLASH_ADDRESS_BITS     ( BMC_FLASH_ADDRESS_BITS        ),
        .ENABLE_COMMAND_LOG     ( 0                             ),      // currently turned off to save area, set to 1 to re-enable command logging
        .ENABLE_3B_ADDR         ( 1                             ),      // BMC boots in 3B address mode, so 3B support is required
        .USE_BMC_SPECIFIC_COMMANDS ( 1                          )
    ) bmc_spi_filter_inst (
        .clock                  ( sys_clock                     ),
        .i_resetn               ( i_sys_resetn                  ),
        .i_avmm_write           ( i_avmm_bmc_we_write           ),
        .i_avmm_address         ( i_avmm_bmc_we_address         ),
        .i_avmm_writedata       ( i_avmm_bmc_we_writedata       ),
        .i_filter_disable       ( i_bmc_filter_disable          ),
        .o_ibb_access_detected  ( o_bmc_ibb_access_detected     ),
        .i_clear_ibb_detected   ( i_bmc_clear_ibb_detected      ),
        .i_addr_mode_set_3b     ( i_bmc_addr_mode_set_3b        ),
        .o_filtered_command_info( o_bmc_filtered_command_info   ),
        .clk_spi_sclk           ( clk_bmc_spi_mon_sclk          ),
        .i_spi_data             ( {{3'b0}, i_bmc_spi_mon_mosi}  ),      // TODO confirm bit ordering, mosi or miso = lsb?
        .i_spi_csn              ( i_bmc_spi_mon_csn             ),
        .o_spi_disable_cs       ( bmc_filter_disable_cs         )
    );

    spi_filter #(
        .ENABLE_IBB_DETECT      ( 0                             ),
        .IBB_ADDRESS_MSBS       ( 16'h0000                      ),
        .FLASH_ADDRESS_BITS     ( PCH_FLASH_ADDRESS_BITS        ),
        .ENABLE_COMMAND_LOG     ( 0                             ),      // currently turned off to save area, set to 1 to re-enable command logging
        .ENABLE_3B_ADDR         ( 0                             )       // PCH can boot in 4B addressing mode, NIOS must use the spi master to ensure the PCH FLASH is in 4B mode before leaving T-1 state
    ) pch_spi_filter_inst (
        .clock                  ( sys_clock                     ),
        .i_resetn               ( i_sys_resetn                  ),
        .i_avmm_write           ( i_avmm_pch_we_write           ),
        .i_avmm_address         ( i_avmm_pch_we_address         ),
        .i_avmm_writedata       ( i_avmm_pch_we_writedata       ),
        .i_filter_disable       ( i_pch_filter_disable          ),
        .o_ibb_access_detected  (                               ),
        .i_clear_ibb_detected   ( '0                            ),      // IBB Detect feature not used for PCH
        .i_addr_mode_set_3b     ( '0                            ),      // 3B addressing mode not used for PCH
        .o_filtered_command_info( o_pch_filtered_command_info   ),
        .clk_spi_sclk           ( clk_pch_spi_mon_sclk          ),
        .i_spi_data             ( i_pch_spi_mon_data            ),
        .i_spi_csn              ( i_pch_spi_mon_csn             ),
        .o_spi_disable_cs       ( pch_filter_disable_cs         )
    );

    assign bmc_spi_filter_csn = i_bmc_spi_mon_csn | bmc_filter_disable_cs;
    assign pch_spi_filter_csn = i_pch_spi_mon_csn | pch_filter_disable_cs;


endmodule
