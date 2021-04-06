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

module kt (
	hash_size,
        cnt,
        kt_out
        );

input hash_size;
input [6:0] cnt;
output [63:0] kt_out;

reg [31:0] kt_reg_256;
reg [63:0] kt_reg_512;

assign kt_out = (hash_size==1'b0) ? {32'h0,kt_reg_256} : kt_reg_512;

always_comb
case (cnt[5:0])

       6'd0  : kt_reg_256 = 32'h428a2f98; 
       6'd1  : kt_reg_256 = 32'h71374491;
       6'd2  : kt_reg_256 = 32'hb5c0fbcf;
       6'd3  : kt_reg_256 = 32'he9b5dba5;
       6'd4  : kt_reg_256 = 32'h3956c25b;
       6'd5  : kt_reg_256 = 32'h59f111f1;
       6'd6  : kt_reg_256 = 32'h923f82a4;
       6'd7  : kt_reg_256 = 32'hab1c5ed5;
       6'd8  : kt_reg_256 = 32'hd807aa98;
       6'd9  : kt_reg_256 = 32'h12835b01;
       6'd10 : kt_reg_256 = 32'h243185be;
       6'd11 : kt_reg_256 = 32'h550c7dc3;
       6'd12 : kt_reg_256 = 32'h72be5d74;
       6'd13 : kt_reg_256 = 32'h80deb1fe;
       6'd14 : kt_reg_256 = 32'h9bdc06a7;
       6'd15 : kt_reg_256 = 32'hc19bf174;
       6'd16 : kt_reg_256 = 32'he49b69c1;
       6'd17 : kt_reg_256 = 32'hefbe4786;
       6'd18 : kt_reg_256 = 32'h0fc19dc6;
       6'd19 : kt_reg_256 = 32'h240ca1cc;
       6'd20 : kt_reg_256 = 32'h2de92c6f;
       6'd21 : kt_reg_256 = 32'h4a7484aa;
       6'd22 : kt_reg_256 = 32'h5cb0a9dc;
       6'd23 : kt_reg_256 = 32'h76f988da;
       6'd24 : kt_reg_256 = 32'h983e5152;
       6'd25 : kt_reg_256 = 32'ha831c66d;
       6'd26 : kt_reg_256 = 32'hb00327c8;
       6'd27 : kt_reg_256 = 32'hbf597fc7;
       6'd28 : kt_reg_256 = 32'hc6e00bf3;
       6'd29 : kt_reg_256 = 32'hd5a79147;
       6'd30 : kt_reg_256 = 32'h06ca6351;
       6'd31 : kt_reg_256 = 32'h14292967;
       6'd32 : kt_reg_256 = 32'h27b70a85;
       6'd33 : kt_reg_256 = 32'h2e1b2138;
       6'd34 : kt_reg_256 = 32'h4d2c6dfc;
       6'd35 : kt_reg_256 = 32'h53380d13;
       6'd36 : kt_reg_256 = 32'h650a7354;
       6'd37 : kt_reg_256 = 32'h766a0abb;
       6'd38 : kt_reg_256 = 32'h81c2c92e;
       6'd39 : kt_reg_256 = 32'h92722c85;
       6'd40 : kt_reg_256 = 32'ha2bfe8a1;
       6'd41 : kt_reg_256 = 32'ha81a664b;
       6'd42 : kt_reg_256 = 32'hc24b8b70;
       6'd43 : kt_reg_256 = 32'hc76c51a3;
       6'd44 : kt_reg_256 = 32'hd192e819;
       6'd45 : kt_reg_256 = 32'hd6990624;
       6'd46 : kt_reg_256 = 32'hf40e3585;
       6'd47 : kt_reg_256 = 32'h106aa070;
       6'd48 : kt_reg_256 = 32'h19a4c116;
       6'd49 : kt_reg_256 = 32'h1e376c08;
       6'd50 : kt_reg_256 = 32'h2748774c;
       6'd51 : kt_reg_256 = 32'h34b0bcb5;
       6'd52 : kt_reg_256 = 32'h391c0cb3;
       6'd53 : kt_reg_256 = 32'h4ed8aa4a;
       6'd54 : kt_reg_256 = 32'h5b9cca4f;
       6'd55 : kt_reg_256 = 32'h682e6ff3;
       6'd56 : kt_reg_256 = 32'h748f82ee;
       6'd57 : kt_reg_256 = 32'h78a5636f;
       6'd58 : kt_reg_256 = 32'h84c87814;
       6'd59 : kt_reg_256 = 32'h8cc70208;
       6'd60 : kt_reg_256 = 32'h90befffa;
       6'd61 : kt_reg_256 = 32'ha4506ceb;
       6'd62 : kt_reg_256 = 32'hbef9a3f7;
       6'd63 : kt_reg_256 = 32'hc67178f2;
       default : kt_reg_256 = 32'h428a2f98;  

endcase

always_comb
case (cnt[6:0])

       7'd0  : kt_reg_512 = 64'h428a2f98d728ae22; 
       7'd1  : kt_reg_512 = 64'h7137449123ef65cd;
       7'd2  : kt_reg_512 = 64'hb5c0fbcfec4d3b2f;
       7'd3  : kt_reg_512 = 64'he9b5dba58189dbbc;
       7'd4  : kt_reg_512 = 64'h3956c25bf348b538;
       7'd5  : kt_reg_512 = 64'h59f111f1b605d019;
       7'd6  : kt_reg_512 = 64'h923f82a4af194f9b;
       7'd7  : kt_reg_512 = 64'hab1c5ed5da6d8118;
       7'd8  : kt_reg_512 = 64'hd807aa98a3030242;
       7'd9  : kt_reg_512 = 64'h12835b0145706fbe;
       7'd10 : kt_reg_512 = 64'h243185be4ee4b28c;
       7'd11 : kt_reg_512 = 64'h550c7dc3d5ffb4e2;
       7'd12 : kt_reg_512 = 64'h72be5d74f27b896f;
       7'd13 : kt_reg_512 = 64'h80deb1fe3b1696b1;
       7'd14 : kt_reg_512 = 64'h9bdc06a725c71235;
       7'd15 : kt_reg_512 = 64'hc19bf174cf692694;
       7'd16 : kt_reg_512 = 64'he49b69c19ef14ad2;
       7'd17 : kt_reg_512 = 64'hefbe4786384f25e3;
       7'd18 : kt_reg_512 = 64'h0fc19dc68b8cd5b5;
       7'd19 : kt_reg_512 = 64'h240ca1cc77ac9c65;
       7'd20 : kt_reg_512 = 64'h2de92c6f592b0275;
       7'd21 : kt_reg_512 = 64'h4a7484aa6ea6e483;
       7'd22 : kt_reg_512 = 64'h5cb0a9dcbd41fbd4;
       7'd23 : kt_reg_512 = 64'h76f988da831153b5;
       7'd24 : kt_reg_512 = 64'h983e5152ee66dfab;
       7'd25 : kt_reg_512 = 64'ha831c66d2db43210;
       7'd26 : kt_reg_512 = 64'hb00327c898fb213f;
       7'd27 : kt_reg_512 = 64'hbf597fc7beef0ee4;
       7'd28 : kt_reg_512 = 64'hc6e00bf33da88fc2;
       7'd29 : kt_reg_512 = 64'hd5a79147930aa725;
       7'd30 : kt_reg_512 = 64'h06ca6351e003826f;
       7'd31 : kt_reg_512 = 64'h142929670a0e6e70;
       7'd32 : kt_reg_512 = 64'h27b70a8546d22ffc;
       7'd33 : kt_reg_512 = 64'h2e1b21385c26c926;
       7'd34 : kt_reg_512 = 64'h4d2c6dfc5ac42aed;
       7'd35 : kt_reg_512 = 64'h53380d139d95b3df;
       7'd36 : kt_reg_512 = 64'h650a73548baf63de;
       7'd37 : kt_reg_512 = 64'h766a0abb3c77b2a8;
       7'd38 : kt_reg_512 = 64'h81c2c92e47edaee6;
       7'd39 : kt_reg_512 = 64'h92722c851482353b;
       7'd40 : kt_reg_512 = 64'ha2bfe8a14cf10364;
       7'd41 : kt_reg_512 = 64'ha81a664bbc423001;
       7'd42 : kt_reg_512 = 64'hc24b8b70d0f89791;
       7'd43 : kt_reg_512 = 64'hc76c51a30654be30;
       7'd44 : kt_reg_512 = 64'hd192e819d6ef5218;
       7'd45 : kt_reg_512 = 64'hd69906245565a910;
       7'd46 : kt_reg_512 = 64'hf40e35855771202a;
       7'd47 : kt_reg_512 = 64'h106aa07032bbd1b8;
       7'd48 : kt_reg_512 = 64'h19a4c116b8d2d0c8;
       7'd49 : kt_reg_512 = 64'h1e376c085141ab53;
       7'd50 : kt_reg_512 = 64'h2748774cdf8eeb99;
       7'd51 : kt_reg_512 = 64'h34b0bcb5e19b48a8;
       7'd52 : kt_reg_512 = 64'h391c0cb3c5c95a63;
       7'd53 : kt_reg_512 = 64'h4ed8aa4ae3418acb;
       7'd54 : kt_reg_512 = 64'h5b9cca4f7763e373;
       7'd55 : kt_reg_512 = 64'h682e6ff3d6b2b8a3;
       7'd56 : kt_reg_512 = 64'h748f82ee5defb2fc;
       7'd57 : kt_reg_512 = 64'h78a5636f43172f60;
       7'd58 : kt_reg_512 = 64'h84c87814a1f0ab72;
       7'd59 : kt_reg_512 = 64'h8cc702081a6439ec;
       7'd60 : kt_reg_512 = 64'h90befffa23631e28;
       7'd61 : kt_reg_512 = 64'ha4506cebde82bde9;
       7'd62 : kt_reg_512 = 64'hbef9a3f7b2c67915;
       7'd63 : kt_reg_512 = 64'hc67178f2e372532b;
       7'd64 : kt_reg_512 = 64'hca273eceea26619c;
       7'd65 : kt_reg_512 = 64'hd186b8c721c0c207;
       7'd66 : kt_reg_512 = 64'heada7dd6cde0eb1e;
       7'd67 : kt_reg_512 = 64'hf57d4f7fee6ed178;
       7'd68 : kt_reg_512 = 64'h06f067aa72176fba;
       7'd69 : kt_reg_512 = 64'h0a637dc5a2c898a6;
       7'd70 : kt_reg_512 = 64'h113f9804bef90dae;
       7'd71 : kt_reg_512 = 64'h1b710b35131c471b;
       7'd72 : kt_reg_512 = 64'h28db77f523047d84;
       7'd73 : kt_reg_512 = 64'h32caab7b40c72493;
       7'd74 : kt_reg_512 = 64'h3c9ebe0a15c9bebc;
       7'd75 : kt_reg_512 = 64'h431d67c49c100d4c;
       7'd76 : kt_reg_512 = 64'h4cc5d4becb3e42b6;
       7'd77 : kt_reg_512 = 64'h597f299cfc657e2a;
       7'd78 : kt_reg_512 = 64'h5fcb6fab3ad6faec;
       7'd79 : kt_reg_512 = 64'h6c44198c4a475817;
       default : kt_reg_512 = 64'h428a2f98d728ae22;

endcase

endmodule
