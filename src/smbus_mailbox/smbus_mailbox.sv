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

// SMBus mailbox top level
// this module instanciates the i2c/SMBus slaves and the register file
// the BMC and PCH slaves both have different read and write access to the register file
// the AVMM interface has full access to the entire register file

`timescale 1 ps / 1 ps
`default_nettype none

module smbus_mailbox #(
                       // SMBus Address, both the BMC and PCH use different addresses but can be set to the same in the top level
                       parameter PCH_SMBUS_ADDRESS = 7'h55,
                       parameter BMC_SMBUS_ADDRESS = 7'h55
                       ) (
                        // Clock input
                        input  logic          clk,

                        // Asyncronous active low reset
                        input  logic          i_resetn,

                        // SMBus signals for the BMC slave 
                        // the input and output enable signals should be combinded in a tristate buffer in the top level of a project using this
                        input  logic          ia_bmc_slave_sda_in,
                        output logic          o_bmc_slave_sda_oe, 
                        input  logic          ia_bmc_slave_scl_in,
                        output logic          o_bmc_slave_scl_oe,

                        // SMBus signals for the PCH/CPU slave
                        input  logic          ia_pch_slave_sda_in,
                        output logic          o_pch_slave_sda_oe, 
                        input  logic          ia_pch_slave_scl_in,
                        output logic          o_pch_slave_scl_oe,


                          // AVMM master interface for the Nios Controller
                        input  logic          m0_read,
                        input  logic          m0_write,
                        input  logic [31:0]   m0_writedata,
                        output logic [31:0]   m0_readdata,
                        input  logic [7:0]    m0_address,
                        output logic          m0_readdatavalid
                        );
    
    // AVMM like bus, used to connect the i2c_slaves to the register file
    logic [31:0]                               pch_address;
    logic                                     pch_read;
    logic [31:0]                              pch_readdata;
    logic                                     pch_readdatavalid;
    logic                                     pch_write;
    logic [31:0]                              pch_writedata;
    logic                                     pch_waitrequest;
    // used to signify when a transaction is blocked by the register file. 
    logic                                     pch_invalid_cmd;

    // AVMM like bus, used to connect the i2c_slaves to the register file
    logic [31:0]                               bmc_address;
    logic                                     bmc_read;
    logic [31:0]                              bmc_readdata;
    logic                                     bmc_readdatavalid;
    logic                                     bmc_write;
    logic [31:0]                              bmc_writedata;
    logic                                     bmc_waitrequest;
    // used to signify when a transaction is blocked by the register file.
    logic                                     bmc_invalid_cmd;

    // PCH SMBus slave
    // Takes SMBus/I2C transactions and translates them into AVMM transactions
    // This module is based off of the quartus i2c to avmm master bridge
    // But it was modified to support 8 bit reads and writes instead of the 32 bits that are expected in AVMM
    i2c_slave #(
                .BYTE_ADDRESSING(1),
                .I2C_SLAVE_ADDRESS(PCH_SMBUS_ADDRESS)
                ) pch_slave (     
                                .clk           (clk),                           
                                .invalid_cmd   (pch_invalid_cmd),
                                .address       (pch_address),                   
                                .read          (pch_read),                      
                                .readdata      (pch_readdata),     
                                .readdatavalid (pch_readdatavalid),
                                .waitrequest   (pch_waitrequest),                         
                                .write         (pch_write),                                               
                                .writedata     (pch_writedata),                
                                .rst_n         (i_resetn),                     
                                .i2c_data_in   (ia_pch_slave_sda_in),             
                                .i2c_clk_in    (ia_pch_slave_scl_in),             
                                .i2c_data_oe   (o_pch_slave_sda_oe),                  
                                .i2c_clk_oe    (o_pch_slave_scl_oe)                    
                                );

    // BMC SMBus slave
    i2c_slave #(
                .BYTE_ADDRESSING(1),
                .I2C_SLAVE_ADDRESS(BMC_SMBUS_ADDRESS)
                ) bmc_slave (
                                .clk           (clk),               
                                .invalid_cmd   (bmc_invalid_cmd),
                                .address       (bmc_address),           
                                .read          (bmc_read),           
                                .readdata      (bmc_readdata),           
                                .readdatavalid (bmc_readdatavalid),           
                                .waitrequest   (bmc_waitrequest),                      
                                .write         (bmc_write),                                  
                                .writedata     (bmc_writedata),           
                                .rst_n         (i_resetn),              
                                .i2c_data_in   (ia_bmc_slave_sda_in),              
                                .i2c_clk_in    (ia_bmc_slave_scl_in),             
                                .i2c_data_oe   (o_bmc_slave_sda_oe),                   
                                .i2c_clk_oe    (o_bmc_slave_scl_oe)                    
                                );
    // Register file
    // module contains the register map as well as the write protection circuitry 
    // this module currently is hardcoded to support 2 SMBus slave ports but more can be added later
    reg_file reg_file_0 (
                         .clk               (clk),   
                         .resetn            (i_resetn),     

                         .m0_read           (m0_read),
                         .m0_write          (m0_write),
                         .m0_writedata      (m0_writedata[7:0]),
                         .m0_readdata       (m0_readdata),
                         .m0_address        (m0_address), 
                         .m0_readdatavalid  (m0_readdatavalid),

                         .pch_read          (pch_read),
                         .pch_write         (pch_write),
                         .pch_writedata     (pch_writedata[7:0]),
                         .pch_readdata      (pch_readdata),
                         .pch_address       (pch_address[7:0]), 
                         .pch_readdatavalid (pch_readdatavalid),
                         .pch_waitrequest   (pch_waitrequest),
                         .pch_invalid_cmd   (pch_invalid_cmd),

                         .bmc_read          (bmc_read),
                         .bmc_write         (bmc_write),
                         .bmc_writedata     (bmc_writedata[7:0]),
                         .bmc_readdata      (bmc_readdata),
                         .bmc_address       (bmc_address[7:0]), 
                         .bmc_readdatavalid (bmc_readdatavalid),
                         .bmc_waitrequest   (bmc_waitrequest),
                         .bmc_invalid_cmd   (bmc_invalid_cmd)
                         );

endmodule
