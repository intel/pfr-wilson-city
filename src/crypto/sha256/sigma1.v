/////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Intel Corporation
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the
//"Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included
//in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
//OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
//THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
///////////////////////////////////////////////////////////////////////////////////

module Sigma1 (
	input hash_size,
	input [63:0] in,
	output [63:0] out
	);

wire [63:0] in_rotr6, in_rotr11, in_rotr25; // Rotated input for SHA-256
wire [63:0] in_rotr14, in_rotr18, in_rotr41; // Rotated input for SHA-384/512

assign in_rotr6  = {32'h0, in[5:0], in[31:6]};
assign in_rotr11 = {32'h0, in[10:0], in[31:11]};
assign in_rotr25 = {32'h0, in[24:0], in[31:25]};

assign in_rotr14 = {in[13:0], in[63:14]}; 
assign in_rotr18 = {in[17:0], in[63:18]}; 
assign in_rotr41 = {in[40:0], in[63:41]}; 

assign out = (hash_size==1'b0) ? (in_rotr6 ^ in_rotr11 ^ in_rotr25) : (in_rotr14 ^ in_rotr18 ^ in_rotr41);

endmodule

