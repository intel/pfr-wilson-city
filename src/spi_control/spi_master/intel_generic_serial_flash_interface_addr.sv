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


`timescale 1ns / 1ns

module intel_generic_serial_flash_interface_addr #(
    parameter ADDR_WIDTH    = 22
)(
    input                  clk,
    input                  reset,
                                                        
    // ports to access memory                   
    input                  avl_mem_write,
    input                  avl_mem_read,
    input [ADDR_WIDTH-1:0] avl_mem_addr,
    input [31:0]           avl_mem_wrdata,
    input [3:0]            avl_mem_byteenable,
    input [6:0]            avl_mem_burstcount,
    output [31:0]          avl_mem_rddata,
    output logic           avl_mem_rddata_valid,
    output logic           avl_mem_waitrequest,
    
    
    // ASMI PARALLEL interface
    output logic [31:0]    gen_qspi_mem_addr, 
    output logic           gen_qspi_mem_read, 
    input [31:0]           gen_qspi_mem_rddata, 
    output logic           gen_qspi_mem_write, 
    output logic [31:0]    gen_qspi_mem_wrdata, 
    output logic [3:0]     gen_qspi_mem_byteenable, 
    output logic [6:0]     gen_qspi_mem_burstcount, 
    input                  gen_qspi_mem_waitrequest, 
    input                  gen_qspi_mem_rddata_valid

);

    // Do nothing, except adjust the address
     
    assign gen_qspi_mem_addr        = {{31-ADDR_WIDTH{1'b0}}, avl_mem_addr};
    assign gen_qspi_mem_read        = avl_mem_read;
    assign gen_qspi_mem_write       = avl_mem_write;
    assign gen_qspi_mem_wrdata      = avl_mem_wrdata;
    assign gen_qspi_mem_byteenable  = avl_mem_byteenable;
    assign gen_qspi_mem_burstcount  = avl_mem_burstcount;
    assign avl_mem_rddata           = gen_qspi_mem_rddata;
    assign avl_mem_rddata_valid     = gen_qspi_mem_rddata_valid;
    assign avl_mem_waitrequest      = gen_qspi_mem_waitrequest;

endmodule
