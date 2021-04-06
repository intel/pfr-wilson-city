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
/////////////////////////////////////////////////////////////////////////////////

module kt_st0 (
	hash_size,
        cnt,
        kt_out
        );

input hash_size;
input [6:0] cnt;
output [63:0] kt_out;

wire [31:0] kt_out;
reg [31:0] kt_reg_256;
reg [63:0] kt_reg_512;

assign kt_out = (hash_size==1'b0) ? {32'h0,kt_reg_256} : kt_reg_512;

always_comb
case (cnt[5:0])

       6'd0  : kt_reg_256 = 32'h428a2f98; 
       6'd2  : kt_reg_256 = 32'hb5c0fbcf;
       6'd4  : kt_reg_256 = 32'h3956c25b;
       6'd6  : kt_reg_256 = 32'h923f82a4;
       6'd8  : kt_reg_256 = 32'hd807aa98;
       6'd10 : kt_reg_256 = 32'h243185be;
       6'd12 : kt_reg_256 = 32'h72be5d74;
       6'd14 : kt_reg_256 = 32'h9bdc06a7;
       6'd16 : kt_reg_256 = 32'he49b69c1;
       6'd18 : kt_reg_256 = 32'h0fc19dc6;
       6'd20 : kt_reg_256 = 32'h2de92c6f;
       6'd22 : kt_reg_256 = 32'h5cb0a9dc;
       6'd24 : kt_reg_256 = 32'h983e5152;
       6'd26 : kt_reg_256 = 32'hb00327c8;
       6'd28 : kt_reg_256 = 32'hc6e00bf3;
       6'd30 : kt_reg_256 = 32'h06ca6351;
       6'd32 : kt_reg_256 = 32'h27b70a85;
       6'd34 : kt_reg_256 = 32'h4d2c6dfc;
       6'd36 : kt_reg_256 = 32'h650a7354;
       6'd38 : kt_reg_256 = 32'h81c2c92e;
       6'd40 : kt_reg_256 = 32'ha2bfe8a1;
       6'd42 : kt_reg_256 = 32'hc24b8b70;
       6'd44 : kt_reg_256 = 32'hd192e819;
       6'd46 : kt_reg_256 = 32'hf40e3585;
       6'd48 : kt_reg_256 = 32'h19a4c116;
       6'd50 : kt_reg_256 = 32'h2748774c;
       6'd52 : kt_reg_256 = 32'h391c0cb3;
       6'd54 : kt_reg_256 = 32'h5b9cca4f;
       6'd56 : kt_reg_256 = 32'h748f82ee;
       6'd58 : kt_reg_256 = 32'h84c87814;
       6'd60 : kt_reg_256 = 32'h90befffa;
       6'd62 : kt_reg_256 = 32'hbef9a3f7;
       default : kt_reg_256 = 32'h428a2f98;  

endcase

always_comb
case (cnt[6:0])

       7'd0  : kt_reg_512 = 64'h428a2f98d728ae22; 
       7'd2  : kt_reg_512 = 64'hb5c0fbcfec4d3b2f;
       7'd4  : kt_reg_512 = 64'h3956c25bf348b538;
       7'd6  : kt_reg_512 = 64'h923f82a4af194f9b;
       7'd8  : kt_reg_512 = 64'hd807aa98a3030242;
       7'd10 : kt_reg_512 = 64'h243185be4ee4b28c;
       7'd12 : kt_reg_512 = 64'h72be5d74f27b896f;
       7'd14 : kt_reg_512 = 64'h9bdc06a725c71235;
       7'd16 : kt_reg_512 = 64'he49b69c19ef14ad2;
       7'd18 : kt_reg_512 = 64'h0fc19dc68b8cd5b5;
       7'd20 : kt_reg_512 = 64'h2de92c6f592b0275;
       7'd22 : kt_reg_512 = 64'h5cb0a9dcbd41fbd4;
       7'd24 : kt_reg_512 = 64'h983e5152ee66dfab;
       7'd26 : kt_reg_512 = 64'hb00327c898fb213f;
       7'd28 : kt_reg_512 = 64'hc6e00bf33da88fc2;
       7'd30 : kt_reg_512 = 64'h06ca6351e003826f;
       7'd32 : kt_reg_512 = 64'h27b70a8546d22ffc;
       7'd34 : kt_reg_512 = 64'h4d2c6dfc5ac42aed;
       7'd36 : kt_reg_512 = 64'h650a73548baf63de;
       7'd38 : kt_reg_512 = 64'h81c2c92e47edaee6;
       7'd40 : kt_reg_512 = 64'ha2bfe8a14cf10364;
       7'd42 : kt_reg_512 = 64'hc24b8b70d0f89791;
       7'd44 : kt_reg_512 = 64'hd192e819d6ef5218;
       7'd46 : kt_reg_512 = 64'hf40e35855771202a;
       7'd48 : kt_reg_512 = 64'h19a4c116b8d2d0c8;
       7'd50 : kt_reg_512 = 64'h2748774cdf8eeb99;
       7'd52 : kt_reg_512 = 64'h391c0cb3c5c95a63;
       7'd54 : kt_reg_512 = 64'h5b9cca4f7763e373;
       7'd56 : kt_reg_512 = 64'h748f82ee5defb2fc;
       7'd58 : kt_reg_512 = 64'h84c87814a1f0ab72;
       7'd60 : kt_reg_512 = 64'h90befffa23631e28;
       7'd62 : kt_reg_512 = 64'hbef9a3f7b2c67915;
       7'd64 : kt_reg_512 = 64'hca273eceea26619c;
       7'd66 : kt_reg_512 = 64'heada7dd6cde0eb1e;
       7'd68 : kt_reg_512 = 64'h06f067aa72176fba;
       7'd70 : kt_reg_512 = 64'h113f9804bef90dae;
       7'd72 : kt_reg_512 = 64'h28db77f523047d84;
       7'd74 : kt_reg_512 = 64'h3c9ebe0a15c9bebc;
       7'd76 : kt_reg_512 = 64'h4cc5d4becb3e42b6;
       7'd78 : kt_reg_512 = 64'h5fcb6fab3ad6faec;
       default : kt_reg_512 = 64'h428a2f98d728ae22;

endcase

endmodule
