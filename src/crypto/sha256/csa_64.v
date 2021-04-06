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
// Description : This module performs carry save addition
/**********************************************************************/

module csa_64 (
        input hash_size,       
        input [63:0]  a,
        input [63:0]  b,
        input [63:0]  c,
        output [63:0] sum_out,
        output [63:0] carry_out
);


wire [63:0] a_and_b;
wire [63:0] b_and_c;
wire [63:0] a_and_c;
wire [63:0] cout_prior_to_shift;

assign  carry_out[63:0]   = ({ cout_prior_to_shift[62:32], hash_size & cout_prior_to_shift[31], cout_prior_to_shift[30:0], 1'b0});
assign  a_and_b[63:0] = b[63:0] & a[63:0];
assign  b_and_c[63:0] = c[63:0] & b[63:0];
assign  a_and_c[63:0] = c[63:0] & a[63:0];
assign  cout_prior_to_shift[63:0] = ( a_and_c[63:0] | b_and_c[63:0] | a_and_b[63:0] );
assign  sum_out[63:0]     = ( a[63:0] ^ b[63:0] ^ c[63:0] );

endmodule
