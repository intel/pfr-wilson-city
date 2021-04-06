// (C) 2001-2018 Intel Corporation. All rights reserved.
// Your use of Intel Corporation's design tools, logic functions and other 
// software and tools, and its AMPP partner logic functions, and any output 
// files from any of the foregoing (including device programming or simulation 
// files), and any associated documentation or information are expressly subject 
// to the terms and conditions of the Intel Program License Subscription 
// Agreement, Intel FPGA IP License Agreement, or other applicable 
// license agreement, including, without limitation, that your use is for the 
// sole purpose of programming logic devices manufactured by Intel and sold by 
// Intel or its authorized distributors.  Please refer to the applicable 
// agreement for further details.


`timescale 1ps / 1ps
`default_nettype none

module altera_avalon_i2c_csr #(
    parameter USE_AV_ST = 0,
    parameter FIFO_DEPTH = 8,
    parameter FIFO_DEPTH_LOG2 = 3
) (
    input                       clk,
    input                       rst_n,
    input [3:0]                 addr,
    input                       read,
    input                       write,
    input [31:0]                writedata,
    input                       arb_lost,
    input                       txfifo_empty,
    input                       txfifo_full,
    input                       mstfsm_idle_state,
    input                       abrt_txdata_noack,
    input                       abrt_7b_addr_noack,
    input                       rxfifo_full,
    input                       rxfifo_empty,
    input [7:0]                 rx_fifo_data_out,
    input                       push_rxfifo,
    input [FIFO_DEPTH_LOG2:0]   txfifo_navail,
    input [FIFO_DEPTH_LOG2:0]   rxfifo_navail,
    
    // Remove AVALON_ST feature and interrupt interface signal since it is not used
    output [31:0]       readdata,
    output              ctrl_en,
    output              flush_txfifo,
    output              write_txfifo,
    output              read_rxfifo,
    output [9:0]        txfifo_writedata,
    output [15:0]       scl_lcnt,
    output [15:0]       scl_hcnt,
    output reg [15:0]   sda_hold,
    output              speed_mode


);


reg [5:0]   ctrl;
reg         arblost_det;
reg         nack_det;
reg [15:0]  scl_low;
reg [15:0]  scl_high;
reg [31:0]  readdata_dly2;

reg         rx_data_rden_dly;
reg         rx_data_rden_dly2;

// 1. Remove some register access from AvMM Bus as runtime setting modification is not required
// 2. Disabled interrupt since this feature is not used
// Modified registers are:
//  - Control register, set to 6'b11: Core is enabled, BUS_SPEED is fast mode
//  - Interrupt status enable register, removed
//  - Interrupt status register, removed roll over register
//  - SCL Low Counter register, set to 16'd130
//  - SCL High Counter register, set to 16'd130
//  - SDA Hold Counter register, set to 16'h1
//  - Status register read access is removed

reg         tfr_cmd_fifo_lvl_rden_dly;
reg         rx_data_fifo_lvl_rden_dly;

wire    tfr_cmd_addr;
wire    rx_data_addr;
wire    tfr_cmd_fifo_lvl_addr;
wire    rx_data_fifo_lvl_addr;

wire    tfr_cmd_wren;
wire    rx_data_rden;

wire    tfr_cmd_fifo_lvl_rden;
wire    rx_data_fifo_lvl_rden;


wire [31:0] rx_data_internal;
wire [31:0] tfr_cmd_fifo_lvl_internal;
wire [31:0] rx_data_fifo_lvl_internal;

wire [31:0] readdata_nxt;

localparam FIFO_DEPTH_LOG2_MASK = 32 - (FIFO_DEPTH_LOG2 + 1);

// Address Decode
assign tfr_cmd_addr             = (addr == 4'h0);
assign rx_data_addr             = (addr == 4'h1);
assign tfr_cmd_fifo_lvl_addr    = (addr == 4'h6);
assign rx_data_fifo_lvl_addr    = (addr == 4'h7);

// Write access
assign tfr_cmd_wren     = tfr_cmd_addr & write;

// Read access
assign rx_data_rden             = rx_data_addr & read;
assign tfr_cmd_fifo_lvl_rden    = tfr_cmd_fifo_lvl_addr & read;
assign rx_data_fifo_lvl_rden    = rx_data_fifo_lvl_addr & read;

// CTRL Register
assign ctrl = 6'b11;

// ARBLOST_DET bit
always @(posedge clk or negedge rst_n) begin
    if (!rst_n)
        arblost_det     <= 1'b0;
    else if (arb_lost) 
        arblost_det     <= 1'b1;
    else
        arblost_det     <= 1'b0;
end

// NACK_DET bit
always @(posedge clk or negedge rst_n) begin
    if (!rst_n)
        nack_det    <= 1'b0;
    else if (abrt_txdata_noack | abrt_7b_addr_noack) 
        nack_det    <= 1'b1;
    else
        nack_det    <= 1'b0;
end

// SCL_LOW Register
assign scl_low = 16'd130;

// SCL_HIGH Register
assign scl_high = 16'd130;

// SDA_HOLD Register
assign sda_hold = 16'd1;

// AVMM read data path
assign rx_data_internal             = {24'h0, rx_fifo_data_out};
assign tfr_cmd_fifo_lvl_internal    = {{FIFO_DEPTH_LOG2_MASK{1'b0}}, txfifo_navail};
assign rx_data_fifo_lvl_internal    = {{FIFO_DEPTH_LOG2_MASK{1'b0}}, rxfifo_navail};

assign readdata_nxt     =   (tfr_cmd_fifo_lvl_internal & {32{tfr_cmd_fifo_lvl_rden_dly}})   |
                            (rx_data_fifo_lvl_internal & {32{rx_data_fifo_lvl_rden_dly}});
                            
// AVMM readdata register
always @(posedge clk or negedge rst_n) begin
    if (!rst_n)
        readdata_dly2 <= 32'h0;
    else
        readdata_dly2 <= readdata_nxt;
end

assign readdata = (rx_data_rden_dly2) ? rx_data_internal : readdata_dly2;

always @(posedge clk or negedge rst_n) begin
    if (!rst_n) begin
        rx_data_rden_dly            <= 1'b0;
        rx_data_rden_dly2           <= 1'b0;
        tfr_cmd_fifo_lvl_rden_dly   <= 1'b0;
        rx_data_fifo_lvl_rden_dly   <= 1'b0;
    end
    else begin
        rx_data_rden_dly            <= rx_data_rden;
        rx_data_rden_dly2           <= rx_data_rden_dly;
        tfr_cmd_fifo_lvl_rden_dly   <= tfr_cmd_fifo_lvl_rden;
        rx_data_fifo_lvl_rden_dly   <= rx_data_fifo_lvl_rden;
    end
end

// output assignment
assign write_txfifo     = tfr_cmd_wren;
assign read_rxfifo      = rx_data_rden;
assign ctrl_en          = ctrl[0];
assign speed_mode       = ctrl[1];
assign flush_txfifo     = arblost_det | nack_det;
assign scl_lcnt         = scl_low;
assign scl_hcnt         = scl_high;
assign txfifo_writedata = writedata[9:0];

endmodule
