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
// Description : Top level SHA module instantiating the message digest  
//		 and message scheduler logics.
/**********************************************************************/


module sha_top (
        clk,
        rst,
        start,
	input_valid,
        win,
	hash_size,
        output_valid,
	h_init_256,
        hout
        );

input clk, rst, start;
input input_valid;
input [1:0] hash_size;
input [1023:0] win;

output output_valid;
output [255:0] h_init_256;
output [511:0] hout;

reg hash_en;

reg [6:0] cnt;
wire [63:0] kt_out;

wire [511:0] h_init;
wire [255:0] h_init_256;

wire [63:0] hout_a, hout_e, hout_rnd_a, hout_rnd_e; 

wire [63:0] hin_init_a, hin_init_e;
wire [63:0] hout_add_a, hout_add_e;

wire [63:0] wout;

//############### Combinational Logic ######

assign hout_add_a = hout_rnd_a + hin_init_a;
assign hout_add_e = hout_rnd_e + hin_init_e;

assign hout_a = {hout_add_a[63:33], hash_size[1] & hout_add_a[32], hout_add_a[31:0]} ;
assign hout_e = {hout_add_e[63:33], hash_size[1] & hout_add_e[32], hout_add_e[31:0]} ;

assign hout = {hout_a,h_init[511:320],hout_e,h_init[255:64]};

assign output_valid = (hash_en == 1'b0) ? 1'b0 :
	       (hash_size[1] == 1'b0 && cnt == 7'd63) ? 1'b1 :
	       (hash_size[1] == 1'b1 && cnt == 7'd79) ? 1'b1 :
		1'b0;

//############### Registers ################

always @(posedge clk or negedge rst)
begin

   if (rst==1'b0)
   begin

        hash_en <= 1'b0; 
        cnt <= 7'd0;

   end

   else
   begin

        hash_en <= (start == 1'b1 || input_valid==1'b1) ? 1'b1 : 
		   ((hash_size[1]==1'b0 && cnt== 7'd63) || (hash_size[1]==1'b1 && cnt== 7'd79)) ? 1'b0 :
		   hash_en;
        cnt <= (hash_en == 1'b0)? 7'h0 :
	       ((hash_size[1]==1'b0 && cnt<= 7'd62) || (hash_size[1]==1'b1 && cnt<= 7'd78))? cnt+1'b1 : 7'h0 ;

   end

end

//################## Module Instantiation #####

kt kt_inst (
	.hash_size(hash_size[1]),
	.cnt(cnt),
	.kt_out(kt_out)
	);

sha_scheduler_top msg_schedule_top (
	.clk(clk),
	.rst(rst),
	.start(start),
	.input_valid(input_valid),
	.cnt(cnt),
	.hash_size(hash_size),
	.win(win),
	.w_out(wout)
	);

hin_init init_state (
	.clk(clk),
	.rst(rst),
	.start(start),
	.cnt(cnt),
	.hash_size(hash_size),
	.hin_init_a_new(hout_a),
	.hin_init_e_new(hout_e),
	.hin_init_a(hin_init_a),
	.hin_init_e(hin_init_e),
	.h_init(h_init),
	.h_init_256(h_init_256)
	);

sha_round_top sha_inst_top (
	.clk(clk),
	.rst(rst),
	.start(start),
	.input_valid(input_valid),
	.hash_size(hash_size),
	.win(wout),
	.kin(kt_out),
	.cnt(cnt),
	.h_init(h_init),
	.hout_a(hout_rnd_a),
	.hout_e(hout_rnd_e)
	);

endmodule
