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


module basic_avmm_bridge #(
    ADDRESS_WIDTH = 1
) (
    input wire clk,
    input wire areset,
    
    input wire [ADDRESS_WIDTH-1:0] avs_address,
    output wire avs_waitrequest,
    input wire avs_read,
    input wire avs_write,
    output wire [31:0] avs_readdata,
    input wire [31:0] avs_writedata,
    
    output wire avm_clk,
    output wire avm_areset,
    output wire [ADDRESS_WIDTH-1:0] avm_address,
    input wire avm_waitrequest,
    output wire avm_read,
    output wire avm_write,
    input wire [31:0] avm_readdata,
    output wire [31:0] avm_writedata

);


    assign avm_clk = clk;
    assign avm_areset = areset;
    assign avm_address = avs_address;
    assign avs_waitrequest = avm_waitrequest;
    assign avm_read = avs_read;
    assign avm_write = avs_write;
    assign avs_readdata = avm_readdata;
    assign avm_writedata = avs_writedata;


endmodule
