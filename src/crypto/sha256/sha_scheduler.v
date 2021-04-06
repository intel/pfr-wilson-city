///////////////////////////////////////////////////////////////////////////////
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
///////////////////////////////////////////////////////////////////////////////

/**********************************************************************/
// Description : This module performs one round of SHA-256/384/512
//		 message expansion
/**********************************************************************/


module sha_scheduler (
        W_t_2,
        W_t_7,
        W_t_15,
        W_t_16,
	hash_size,
        W_t
        );

input hash_size;
input [63:0] W_t_2;
input [63:0] W_t_7;
input [63:0] W_t_15;
input [63:0] W_t_16;

output [63:0] W_t;

wire [63:0] sigma0, sigma1;

wire [63:0] a_rotr7, a_rotr18, a_shr3; //Rotate and shift for SHA-256 sigma0 
wire [63:0] a_rotr1, a_rotr8, a_shr7; //Rotate and shift for SHA-384/512 sigma0 

wire [63:0] a_rotr17, a_rotr19_0, a_shr10; //Rotate and shift for SHA-256 sigma1 
wire [63:0] a_rotr19_1, a_rotr61, a_shr6; //Rotate and shift for SHA-384/512 sigma1 

wire [63:0] sigma1_w7_sigma0_sum, sigma1_w7_sigma0_carry;
wire [63:0] sigma1_w7_sigma0_w16_sum, sigma1_w7_sigma0_w16_carry;
wire [63:0] W_t_sum;

assign a_rotr7  = {32'h0, W_t_15[6:0], W_t_15[31:7]};
assign a_rotr18 = {32'h0, W_t_15[17:0], W_t_15[31:18]};
assign a_shr3 = {32'h0, 3'b0, W_t_15[31:3]};

assign a_rotr1  = {W_t_15[0], W_t_15[63:1]};
assign a_rotr8 = {W_t_15[7:0], W_t_15[63:8]};
assign a_shr7 = {7'b0, W_t_15[63:7]};

assign a_rotr17  = {32'h0, W_t_2[16:0], W_t_2[31:17]};
assign a_rotr19_0 = {32'h0, W_t_2[18:0], W_t_2[31:19]};
assign a_shr10 = {32'h0, 10'b0, W_t_2[31:10]};

assign a_rotr19_1  = {W_t_2[18:0], W_t_2[63:19]};
assign a_rotr61 = {W_t_2[60:0], W_t_2[63:61]};
assign a_shr6 = {6'b0, W_t_2[63:6]};

assign sigma0 = (hash_size == 1'b0) ? (a_rotr7 ^ a_rotr18 ^ a_shr3) : (a_rotr1 ^ a_rotr8 ^ a_shr7);
assign sigma1 = (hash_size == 1'b0) ? (a_rotr17 ^ a_rotr19_0 ^ a_shr10) : (a_rotr19_1 ^ a_rotr61 ^ a_shr6);

assign W_t_sum = sigma1_w7_sigma0_w16_sum + sigma1_w7_sigma0_w16_carry;
assign W_t = {W_t_sum[63:33], hash_size & W_t_sum[32], W_t_sum[31:0]}; 


csa_64 csa0 (
	.hash_size(hash_size),
	.a(sigma1),
	.b(W_t_7),
	.c(sigma0),
	.sum_out(sigma1_w7_sigma0_sum),
	.carry_out(sigma1_w7_sigma0_carry)
	);

csa_64 csa1 (
	.hash_size(hash_size),
	.a(sigma1_w7_sigma0_sum),
	.b(sigma1_w7_sigma0_carry),
	.c(W_t_16),
	.sum_out(sigma1_w7_sigma0_w16_sum),
	.carry_out(sigma1_w7_sigma0_w16_carry)
	);


endmodule

