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

/**********************************************************************/
// Description : SHA message digest top module including 8x64-bit state  
//		 registers. This module performs one rounds of SHA 
//		 operations per clock cyle.
/**********************************************************************/


module sha_round_top(
        clk,
        rst,
        start,
	input_valid,
	hash_size,
        win,
        kin,
        cnt,
        h_init,
        hout_a,
        hout_e
        );

input clk, rst, start;
input input_valid;
input [1:0] hash_size;
input [6:0] cnt;
input [63:0] win, kin;
input [511:0] h_init;
output [63:0] hout_a, hout_e;

logic [63:0] A_in,B_in,C_in,D_in,E_in,F_in,G_in,H_in;

logic [63:0] A;
logic [63:0] B;
logic [63:0] C;
logic [63:0] D;
logic [63:0] E;
logic [63:0] F;
logic [63:0] G;
logic [63:0] H;

logic [63:0] A_out;
logic [63:0] E_out;

//############### Combinational Logic ######

sha_round sha_rnd_0 (
	.hash_size(hash_size[1]),
	.state_in({A,B,C,D,E,F,G,H}),
	.w_in(win),
	.k_in(kin),
	.state_out_a(hout_a),
	.state_out_e(hout_e)
	);

assign A_in = h_init[511:448];
assign B_in = h_init[447:384];
assign C_in = h_init[383:320];
assign D_in = h_init[319:256];
assign E_in = h_init[255:192];
assign F_in = h_init[191:128];
assign G_in = h_init[127:64 ];
assign H_in = h_init[63:0   ];

//############### Registers ################

always @(posedge clk or negedge rst)
begin

        if (rst==1'b0) 
        begin
        
                A <= 64'h0; 
                B <= 64'h0;
                C <= 64'h0;
                D <= 64'h0;
                E <= 64'h0;
                F <= 64'h0;
                G <= 64'h0;
                H <= 64'h0;
                
        end
        
        else
        begin
           
                if (start==1'b1)
                begin
                
                        A  <= (hash_size == 2'b01) ? {32'h0,32'h6A09E667} : ((hash_size == 2'b10) ? 64'hcbbb9d5dc1059ed8 : 64'h6a09e667f3bcc908); 
                        B  <= (hash_size == 2'b01) ? {32'h0,32'hBB67AE85} : ((hash_size == 2'b10) ? 64'h629a292a367cd507 : 64'hbb67ae8584caa73b);
                        C  <= (hash_size == 2'b01) ? {32'h0,32'h3C6EF372} : ((hash_size == 2'b10) ? 64'h9159015a3070dd17 : 64'h3c6ef372fe94f82b);
                        D  <= (hash_size == 2'b01) ? {32'h0,32'hA54FF53A} : ((hash_size == 2'b10) ? 64'h152fecd8f70e5939 : 64'ha54ff53a5f1d36f1);
                        E  <= (hash_size == 2'b01) ? {32'h0,32'h510E527F} : ((hash_size == 2'b10) ? 64'h67332667ffc00b31 : 64'h510e527fade682d1);
                        F  <= (hash_size == 2'b01) ? {32'h0,32'h9B05688C} : ((hash_size == 2'b10) ? 64'h8eb44a8768581511 : 64'h9b05688c2b3e6c1f);
                        G  <= (hash_size == 2'b01) ? {32'h0,32'h1F83D9AB} : ((hash_size == 2'b10) ? 64'hdb0c2e0d64f98fa7 : 64'h1f83d9abfb41bd6b);
                        H  <= (hash_size == 2'b01) ? {32'h0,32'h5BE0CD19} : ((hash_size == 2'b10) ? 64'h47b5481dbefa4fa4 : 64'h5be0cd19137e2179);

                end
                
                else
                begin
                
                        A <=  (input_valid==1'b1) ? A_in : hout_a; 
                        B <=  (input_valid==1'b1) ? B_in : A;
                        C <=  (input_valid==1'b1) ? C_in : B;
                        D <=  (input_valid==1'b1) ? D_in : C;
                        E <=  (input_valid==1'b1) ? E_in : hout_e; 
                        F <=  (input_valid==1'b1) ? F_in : E;                                 
                        G <=  (input_valid==1'b1) ? G_in : F;                                 
                        H <=  (input_valid==1'b1) ? H_in : G;                                 
                
                end
                
        end
        
end

endmodule   
      
