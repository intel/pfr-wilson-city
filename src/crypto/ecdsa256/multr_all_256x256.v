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
/////////////////////////////////////////////////////////////////////////////////

module multr_all_256x256(cout, out, done, mode, in1, in2, inp, cin, clk, resetn, sw_reset, start);
input [255:0]   in1, in2, inp;
input [1:0]     mode;
input           cin;
input           clk, resetn, sw_reset, start;
output [255:0]  out;
output          cout;
output reg      done;

reg  [511:0]  out_m;
wire [255:0]  in_t1;
wire [264:0]  out_w;
reg  [264:0]  out_w_out;
wire [7:0]    in_t21, in_t22;
wire [8:0]    in_t2;
reg           mult_in_progress, active_acc, active_red_mult, active_red_sub;
wire          next_mult, mult_done, red_done;

wire [4:0]    raddr_in_t2; 
reg  [1:0]    in_t2_sel;
reg  [9:0]    rflag;
reg  [1:0]    aflag;
reg  [4:0]    mflag;
reg  [3:0]    count;
reg c1;
wire c2;
wire next_t2_mem;

wire [264:0]  as_out; 
wire [264:0]  as_in1, as_in2;
wire c_in;
wire c_out;
reg additional_sub;

assign out = out_m[511:256];

crypto_mem3 i_cm31(.clock(clk), .data(in2[127:0]  ), .rdaddress(raddr_in_t2), .wraddress(1'b0), .wren(start), .q(in_t21));
crypto_mem3 i_cm32(.clock(clk), .data(in2[255:128]), .rdaddress(raddr_in_t2), .wraddress(1'b0), .wren(start), .q(in_t22));

assign raddr_in_t2 = {1'b0,count[3:0]};
assign in_t2 = (in_t2_sel == 2'b00) ? {1'b0,in_t21} : (in_t2_sel == 2'b01) ? {1'b0,in_t22} : {1'b0,out_m[511:504]};
assign in_t1 = (mult_in_progress) ? in1 : inp;

mult_256x9 i_m256_9(.out(out_w), .in1(in_t1), .in2(in_t2));

always @(posedge clk) begin
 out_w_out[255:0] <= (start & ~(mode[1]&mode[0]))? in2 : out_w[255:0];
 out_w_out[264:256] <= (start & ~mode[0])? 9'd0 : (start & ~mode[1]& mode[0])? 9'h1ff : out_w[264:256];
 out_m <= (start) ? 512'd0 : (active_acc)? {as_out[263:0],out_m[255:8]} : (active_red_sub  | (rflag[8] & c2) | aflag[0] | (aflag[1] & ((~mode[1] & mode[0] & ~c1) | (mode[1] & ~mode[0] &(c1|c2)))))? {as_out[255:0],out_m[247:0],8'd0} : (additional_sub)? {as_out[255:0],out_m[255:0]} : out_m;
 c1 <= (aflag[0]) ? as_out[256] : c1;
end

assign c2 = as_out[256];
assign as_in1 = (active_acc | aflag[1] | rflag[8] | additional_sub)? {9'd0,out_m[511:256]} : (active_red_sub)? {1'b0,out_m[511:248]} : {9'd0,in1};  
assign as_in2 = (active_red_sub | (aflag[0] & ~mode[1]&mode[0]))? ~out_w_out[264:0] : ((aflag[1] & (mode[1]&~mode[0])) | rflag[8] | additional_sub) ? {9'd0,~inp} : (aflag[1] & (~mode[1]&mode[0])) ? {9'd0,inp} : out_w_out[264:0]; 
assign c_in = (mode == 2'd0) ? cin : ((aflag[0] & ~mode[1]&mode[0]) | (aflag[1] & mode[1]&~mode[0]) | active_red_sub | rflag[8] | additional_sub) ? 1'b1 : 1'b0;
assign cout = c1;


csl_add_sub_265 i_cs411(.c_out(c_out), .out(as_out), .in1(as_in1), .in2(as_in2), .c_in(c_in));

always @(posedge clk) begin
 if(mflag[0] | rflag[0]) count <= 4'd0;
 else if(~additional_sub) count <= count + 1'd1;
end

assign next_mult = (count == 4'hf) ? 1'b1 : 1'b0;   //short counter is used to generate in_t2 from three 128-bit memory
assign next_t2_mem = (count == 4'h1) ? 1'b1 : 1'b0;
assign mult_done = next_t2_mem & mflag[4];
assign red_done = next_mult & rflag[6];

always @(posedge clk or negedge resetn) begin
 if(~resetn) begin
  rflag[9:0] <= 10'd0;
  mflag[4:0] <= 5'd0;
  aflag[1:0] <= 2'd0;
  mult_in_progress <= 1'b0;
  active_acc <= 1'b0;
  active_red_mult <= 1'b0;
  active_red_sub <= 1'b0;
  done <= 1'b0;
  additional_sub <= 1'b0;
 end
 else if(sw_reset) begin
  rflag[9:0] <= 10'd0;
  mflag[4:0] <= 5'd0;
  aflag[1:0] <= 2'd0;
  mult_in_progress <= 1'b0;
  active_acc <= 1'b0;
  active_red_mult <= 1'b0;
  active_red_sub <= 1'b0;
  done <= 1'b0;
  additional_sub <= 1'b0;
 end
 else if(start) begin
  mflag[4:0] <= {4'd0,(mode[1]&mode[0])};
  rflag[9:0] <= 10'd0;
  aflag[1:0] <= {1'd0,~(mode[1]&mode[0])};
  mult_in_progress <= 1'b0;
  active_acc <= 1'b0;
  active_red_mult <= 1'b0;
  active_red_sub <= 1'b0;
  done <= 1'b0;
  additional_sub <= 1'b0;
 end 
 else if(mflag[0]) begin
  mflag[0] <= 1'b0;
  mflag[1] <= 1'b1;
  in_t2_sel <= 2'b00;
  mult_in_progress <= 1'b1;
 end
 else if(mflag[1]) begin
  if(count == 4'h2) active_acc <= 1'b1;
  if(next_mult) begin
   mflag[1] <= 1'b0;
   mflag[2] <= 1'b1;
  end
 end
 else if(mflag[2]) begin
  if(next_t2_mem) in_t2_sel <= 2'b01;
  if(next_mult) begin
   mflag[2] <= 1'b0;
   mflag[4] <= 1'b1;
  end
 end
 else if(mflag[4]) begin
  if(mult_done) begin
   mflag[4] <= 1'b0;
   rflag[0] <= 1'b1;
   mult_in_progress <= 1'b0;
   in_t2_sel <= 2'b11;  //select out_m as in_t2 during reduction
  end
 end 
 else if(rflag[0]) begin
  active_acc <= 1'b0;
  rflag[0] <= 1'b0;
  rflag[1] <= 1'b1; 
  active_red_mult <= 1'b1;
  active_red_sub <= 1'b0;
 end
 else if(rflag[1]) begin
  if(active_red_sub & c2) begin 
    additional_sub <= 1'b1;
    active_red_mult <= 1'b0;
    active_red_sub  <= 1'b0;
  end
  else if(additional_sub) begin 
    additional_sub  <= 1'b0;
    active_red_mult <= 1'b1;
    active_red_sub  <= 1'b0;
  end
  else begin 
   active_red_mult <= ~active_red_mult;
   active_red_sub <= ~active_red_sub; 
   additional_sub <= 1'b0;
  end
  if(next_mult) begin
   rflag[1] <= 1'b0;
   rflag[2] <= 1'b1;  
  end	
 end
 else if(rflag[2]) begin
  if(active_red_sub & c2) begin
    additional_sub <= 1'b1;
    active_red_mult <= 1'b0;
    active_red_sub  <= 1'b0;
  end
  else if(additional_sub) begin
    additional_sub  <= 1'b0;
    active_red_mult <= 1'b1;
    active_red_sub  <= 1'b0;
  end
  else begin
   active_red_mult <= ~active_red_mult;
   active_red_sub <= ~active_red_sub;
   additional_sub <= 1'b0;
  end
  if(next_mult) begin
   rflag[2] <= 1'b0;
   rflag[3] <= 1'b1;  
  end  
 end
 else if(rflag[3]) begin
  if(active_red_sub & c2) begin
    additional_sub <= 1'b1;
    active_red_mult <= 1'b0;
    active_red_sub  <= 1'b0;
  end
  else if(additional_sub) begin
    additional_sub  <= 1'b0;
    active_red_mult <= 1'b1;
    active_red_sub  <= 1'b0;
  end
  else begin
   active_red_mult <= ~active_red_mult;
   active_red_sub <= ~active_red_sub;
   additional_sub <= 1'b0;
  end
  if(next_mult) begin
   rflag[3] <= 1'b0;
   rflag[6] <= 1'b1; 
  end  
 end
 else if(rflag[6]) begin
  if(active_red_sub & c2) begin
    additional_sub <= 1'b1;
    active_red_mult <= 1'b0;
    active_red_sub  <= 1'b0;
  end
  else if(additional_sub) begin
    additional_sub  <= 1'b0;
    active_red_mult <= 1'b1;
    active_red_sub  <= 1'b0;
  end
  else begin
   active_red_mult <= ~active_red_mult;
   active_red_sub <= ~active_red_sub;
   additional_sub <= 1'b0;
  end
  if(red_done) begin
   rflag[6] <= 1'b0;
   rflag[7] <= 1'b1; 
  end  
 end
else if(rflag[7]) begin
  rflag[7] <= 1'b0;
  rflag[8] <= 1'b1;
  active_red_mult <= 1'b0;
  active_red_sub <= 1'b0;
 end
 else if(rflag[8]) begin
  rflag[8] <= 1'b0;
  rflag[9] <= 1'b1;
 end
 else if(rflag[9]) begin
  rflag[9] <= 1'b0;
  done <= 1'b1;
 end
 else if(aflag[0]) begin
  aflag[0] <= 1'b0;
  aflag[1] <= 1'b1;
 end
 else if(aflag[1]) begin
  aflag[1] <= 1'b0;
  done <= 1'b1; 
  end
 else begin
  done <= 1'b0; 
 end
end

endmodule
