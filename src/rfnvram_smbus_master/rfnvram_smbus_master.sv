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



`timescale 1 ps / 1 ps
`default_nettype none

// SMBus address = 0xDC
module rfnvram_smbus_master #(
	parameter FIFO_DEPTH = 8
)(
	input wire clk,
	input wire resetn,

	// AVMM interface to connect the NIOS to the CSR interface
	input  wire [3:0]	csr_address,
	input  wire 	  	csr_read,
	output wire [31:0]	csr_readdata,
	input  wire 		csr_write,
	input  wire [31:0]  csr_writedata,
    output logic        csr_readdatavalid,

	input wire sda_in,
	output wire sda_oe,
	input wire scl_in,
	output wire scl_oe

);

    logic readdatavalid_dly;
    always_ff @(posedge clk or negedge resetn) begin
        if (!resetn) begin
            csr_readdatavalid <= 1'b0;
            readdatavalid_dly <= 1'b0;
        
        end else begin
            readdatavalid_dly <= csr_read;
            csr_readdatavalid <= readdatavalid_dly;
        end
    end

    localparam FIFO_DEPTH_LOG2 = $clog2(FIFO_DEPTH);
    i2c_master #(.FIFO_DEPTH(FIFO_DEPTH),
                 .FIFO_DEPTH_LOG2(FIFO_DEPTH_LOG2))
    u0 (
        .clk                     (clk),   
        .rst_n                   (resetn),
        .sda_in                  (sda_in),
        .scl_in                  (scl_in),
        .sda_oe                  (sda_oe),
        .scl_oe                  (scl_oe),
        .addr                    (csr_address),       
        .read                    (csr_read),          
        .write                   (csr_write),         
        .writedata               (csr_writedata),     
        .readdata                (csr_readdata)
    );



endmodule
