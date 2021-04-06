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

module kt_st1 (
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

       6'd1  : kt_reg_256 = 32'h71374491;
       6'd3  : kt_reg_256 = 32'he9b5dba5;
       6'd5  : kt_reg_256 = 32'h59f111f1;
       6'd7  : kt_reg_256 = 32'hab1c5ed5;
       6'd9  : kt_reg_256 = 32'h12835b01;
       6'd11 : kt_reg_256 = 32'h550c7dc3;
       6'd13 : kt_reg_256 = 32'h80deb1fe;
       6'd15 : kt_reg_256 = 32'hc19bf174;
       6'd17 : kt_reg_256 = 32'hefbe4786;
       6'd19 : kt_reg_256 = 32'h240ca1cc;
       6'd21 : kt_reg_256 = 32'h4a7484aa;
       6'd23 : kt_reg_256 = 32'h76f988da;
       6'd25 : kt_reg_256 = 32'ha831c66d;
       6'd27 : kt_reg_256 = 32'hbf597fc7;
       6'd29 : kt_reg_256 = 32'hd5a79147;
       6'd31 : kt_reg_256 = 32'h14292967;
       6'd33 : kt_reg_256 = 32'h2e1b2138;
       6'd35 : kt_reg_256 = 32'h53380d13;
       6'd37 : kt_reg_256 = 32'h766a0abb;
       6'd39 : kt_reg_256 = 32'h92722c85;
       6'd41 : kt_reg_256 = 32'ha81a664b;
       6'd43 : kt_reg_256 = 32'hc76c51a3;
       6'd45 : kt_reg_256 = 32'hd6990624;
       6'd47 : kt_reg_256 = 32'h106aa070;
       6'd49 : kt_reg_256 = 32'h1e376c08;
       6'd51 : kt_reg_256 = 32'h34b0bcb5;
       6'd53 : kt_reg_256 = 32'h4ed8aa4a;
       6'd55 : kt_reg_256 = 32'h682e6ff3;
       6'd57 : kt_reg_256 = 32'h78a5636f;
       6'd59 : kt_reg_256 = 32'h8cc70208;
       6'd61 : kt_reg_256 = 32'ha4506ceb;
       6'd63 : kt_reg_256 = 32'hc67178f2;
       default : kt_reg_256 = 32'h71374491;  

endcase

always_comb
case (cnt[6:0])

       7'd1  : kt_reg_512 = 64'h7137449123ef65cd;
       7'd3  : kt_reg_512 = 64'he9b5dba58189dbbc;
       7'd5  : kt_reg_512 = 64'h59f111f1b605d019;
       7'd7  : kt_reg_512 = 64'hab1c5ed5da6d8118;
       7'd9  : kt_reg_512 = 64'h12835b0145706fbe;
       7'd11 : kt_reg_512 = 64'h550c7dc3d5ffb4e2;
       7'd13 : kt_reg_512 = 64'h80deb1fe3b1696b1;
       7'd15 : kt_reg_512 = 64'hc19bf174cf692694;
       7'd17 : kt_reg_512 = 64'hefbe4786384f25e3;
       7'd19 : kt_reg_512 = 64'h240ca1cc77ac9c65;
       7'd21 : kt_reg_512 = 64'h4a7484aa6ea6e483;
       7'd23 : kt_reg_512 = 64'h76f988da831153b5;
       7'd25 : kt_reg_512 = 64'ha831c66d2db43210;
       7'd27 : kt_reg_512 = 64'hbf597fc7beef0ee4;
       7'd29 : kt_reg_512 = 64'hd5a79147930aa725;
       7'd31 : kt_reg_512 = 64'h142929670a0e6e70;
       7'd33 : kt_reg_512 = 64'h2e1b21385c26c926;
       7'd35 : kt_reg_512 = 64'h53380d139d95b3df;
       7'd37 : kt_reg_512 = 64'h766a0abb3c77b2a8;
       7'd39 : kt_reg_512 = 64'h92722c851482353b;
       7'd41 : kt_reg_512 = 64'ha81a664bbc423001;
       7'd43 : kt_reg_512 = 64'hc76c51a30654be30;
       7'd45 : kt_reg_512 = 64'hd69906245565a910;
       7'd47 : kt_reg_512 = 64'h106aa07032bbd1b8;
       7'd49 : kt_reg_512 = 64'h1e376c085141ab53;
       7'd51 : kt_reg_512 = 64'h34b0bcb5e19b48a8;
       7'd53 : kt_reg_512 = 64'h4ed8aa4ae3418acb;
       7'd55 : kt_reg_512 = 64'h682e6ff3d6b2b8a3;
       7'd57 : kt_reg_512 = 64'h78a5636f43172f60;
       7'd59 : kt_reg_512 = 64'h8cc702081a6439ec;
       7'd61 : kt_reg_512 = 64'ha4506cebde82bde9;
       7'd63 : kt_reg_512 = 64'hc67178f2e372532b;
       7'd65 : kt_reg_512 = 64'hd186b8c721c0c207;
       7'd67 : kt_reg_512 = 64'hf57d4f7fee6ed178;
       7'd69 : kt_reg_512 = 64'h0a637dc5a2c898a6;
       7'd71 : kt_reg_512 = 64'h1b710b35131c471b;
       7'd73 : kt_reg_512 = 64'h32caab7b40c72493;
       7'd75 : kt_reg_512 = 64'h431d67c49c100d4c;
       7'd77 : kt_reg_512 = 64'h597f299cfc657e2a;
       7'd79 : kt_reg_512 = 64'h6c44198c4a475817;
       default : kt_reg_512 = 64'h7137449123ef65cd;

endcase

endmodule
