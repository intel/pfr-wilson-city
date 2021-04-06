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

module ecdsa256_top (ecc_done, ecc_busy, dout_valid, cx_out, clk, resetn, sw_reset, ecc_start, data_in, din_valid, ecc_ins, ins_valid);

output reg         ecc_done;
output reg         ecc_busy;
output reg         dout_valid;
output     [255:0] cx_out;
input              clk;
input              resetn;
input              sw_reset;
input              ecc_start;
input              din_valid;
input              ins_valid;
input      [255:0] data_in;
input      [4:0]   ecc_ins;

reg [4:0]     ecc_ins_reg;
reg           epm_start;
wire [255:0]  mas_in1, mas_in2, mas_out;
wire          mas_cin, mas_cout;
wire          mas_done;
wire          mas_start_top;
wire [1:0]    mas_mode_top;
reg           mas_start;
reg [1:0]     mas_mode;
reg           eaj_ad;
reg           eaj_start;
wire          eaj_done;
wire          count_zero;
reg [8:0]     count;
reg           fins;
reg [16:0]    fsav;
reg [41:0]    frsav;
reg [9:0]     fepm;
reg [6:0]    frepm;
reg [11:0]    fdsc;
reg [23:0]    fexp;
reg [255:0]   dr;
wire[255:0]   np;
reg           exp_start;
reg [9:0]     fead;
reg           sel_n;
reg           dsav_start;
reg           ecc_sdone;
reg           keep_eaj_jpc1;
reg           keep_eaj_jpc2;
reg           rd_nonzero;
reg           s_nonzero;
	
wire [1:0]    eaj_mas_mode;
wire          eaj_mas_start;
wire          write_mas_out;

wire[255:0]   mem_in; 
wire[255:0]   mem_out1; 
wire[255:0]   mem_out2;
reg [4:0]     raddr_mem1; 
reg [4:0]     raddr_mem2; 
wire[4:0]     raddr_mem1w;
wire[4:0]     raddr_mem1ww;
wire[4:0]     raddr_mem2w;
wire[4:0]     waddr_mem;
wire          wren_mem;

wire [4:0]    raddr_to_extn1;
wire [4:0]    raddr_to_extn2;
wire [4:0]    waddr_to_extn;
wire          wren_init;
wire          wren_fadd54;
wire          y3_div2;  

wire          wren_mem4 ;
wire          waddr_mem4;
wire          raddr_mem4;
wire          write_n;
wire[255:0]   mem_out4;
reg [1:0]     read_count;
reg           read_count_start;
wire          read_done;
reg           s_range_check;

//mas_righttoleft_gf256
multr_all_256x256  i_ma256 (
 .cout(mas_cout),
 .out(mas_out), 
 .done(mas_done), 
 .mode(mas_mode_top),
 .in1(mas_in1), 
 .in2(mas_in2), 
 .inp(np),
 .cin(mas_cin), 
 .clk(clk), 
 .resetn(resetn),
 .sw_reset(sw_reset), 
 .start(mas_start_top)
); 

ocs_ecp256_ad_jpc i_jpc(
 .clk              (clk)           , 
 .resetn           (resetn)        , 
 .start            (eaj_start)     , 
 .ad               (eaj_ad)        , 
 .mas_done         (mas_done)      , 
 .mas_cout         (mas_cout)      , 
 .mem_out0         (mem_out1[0])   , 
 .raddr_to_extn1   (raddr_to_extn1), 
 .raddr_to_extn2   (raddr_to_extn2), 
 .waddr_to_extn    (waddr_to_extn) , 
 .wren_init        (wren_init)     , 
 .wren_fadd54      (wren_fadd54)   , 
 .y3_div2          (y3_div2)       , 
 .done             (eaj_done)      , 
 .mas_mode         (eaj_mas_mode)  , 
 .mas_start        (eaj_mas_start)
); 

crypto_mem2 i_mem212( .clock(clk), .data(mem_in[255:128]), .rdaddress(raddr_mem1w), .wraddress(waddr_mem), .wren(wren_mem), .q(mem_out1[255:128]));
crypto_mem2 i_mem211( .clock(clk), .data(mem_in[127:0]), .rdaddress(raddr_mem1w), .wraddress(waddr_mem), .wren(wren_mem), .q(mem_out1[127:0]));
	
crypto_mem2 i_mem222( .clock(clk), .data(mem_in[255:128]), .rdaddress(raddr_mem2w), .wraddress(waddr_mem), .wren(wren_mem), .q(mem_out2[255:128]));
crypto_mem2 i_mem221( .clock(clk), .data(mem_in[127:0]), .rdaddress(raddr_mem2w), .wraddress(waddr_mem), .wren(wren_mem), .q(mem_out2[127:0]));

crypto_mem4 i_mem412( .clock(clk), .data(data_in[255:128]), .rdaddress(raddr_mem4), .wraddress(waddr_mem4), .wren(wren_mem4), .q(mem_out4[255:128]));
crypto_mem4 i_mem411( .clock(clk), .data(data_in[127:0]), .rdaddress(raddr_mem4), .wraddress(waddr_mem4), .wren(wren_mem4), .q(mem_out4[127:0]));

assign cx_out = mem_out1;

assign write_n = (ecc_ins_reg[4:0] == 5'b10000) ? 1'b1 : 1'b0;
assign wren_mem4 = fins & din_valid & ((ecc_ins_reg[4:0] == 5'b00101) | write_n);
assign waddr_mem4 = (write_n)? 1'b1 : 1'b0;
assign raddr_mem4 = (sel_n) ? 1'b1 : 1'b0;
assign np = mem_out4; //address 0 holds p and 1 holds n

assign raddr_mem1w = (fead[3]) ? raddr_to_extn1 : raddr_mem1;
assign raddr_mem2w = (fead[3]) ? raddr_to_extn2 : raddr_mem2;

assign mas_in1 = mem_out1;
assign mas_in2 = mem_out2;
assign mas_cin = 1'b0;
assign mas_mode_top = (fead[3] | fepm[1] | fepm[3] | fepm[5]) ? eaj_mas_mode : mas_mode;
assign mas_start_top = (fead[3] | fepm[1] | fepm[3] | fepm[5]) ? eaj_mas_start : mas_start;

//Mem read/write
assign write_mas_out = (mas_done & (fead[3] | fdsc[2] | fdsc[5] | fdsc[8] | fdsc[11] | fsav[2] | fsav[4] | fsav[6] | fsav[12] | fsav[14] | fexp[3] | fexp[14] | (fexp[18]&dr[255]) | frsav[39]));

assign wren_mem = (fins & din_valid) | write_mas_out | frsav[36] | frsav[1] | frsav[2] | frsav[4] | frsav[5] | frsav[7] | frsav[15] | frsav[16] | frsav[17] | frsav[20] | frsav[21] | frsav[27] | frsav[28] | frsav[29] | frsav[32] | frsav[33] | frsav[22] | frsav[24] | frsav[25] | wren_init | wren_fadd54 | fead[0] | fead[1] | fead[6] | fead[7] | fead[8] | fexp[0] | (read_done & (fexp[5] | fexp[7]))  | fexp[8] | fexp[21] | fexp[22] | fexp[23] | frepm[0] | frepm[1] | frepm[3] | frepm[4] | frepm[5] | frepm[6];

assign waddr_mem = ((ecc_ins_reg[4:0] == 5'b00001) | (frsav[1] | frsav[28] | fsav[12] | fsav[14] | fexp[21] | fead[6] | fdsc[5] | frepm[5]))? 5'd0  : ((ecc_ins_reg[4:0] == 5'b00010) | frsav[29] | fexp[22] | fead[7] | fdsc[11] | frepm[6] | frsav[25])? 5'd1 : (frsav[2] | fexp[5] | fexp[23] | fead[8] | fead[0] | frepm[0])? 5'd2: ((frsav[7] | frsav[17] | frsav[27] | frsav[32] | fexp[0] | fdsc[2] | fdsc[8] | frepm[3]) | (ecc_ins_reg[4:0] == 5'b00011))? 5'd3  : (frsav[33] | frepm[4] | (ecc_ins_reg[4:0] == 5'b00100))? 5'd4  : (fead[1] | fexp[3]| fexp[7] | frepm[1])? 5'd5: (ecc_ins_reg[4:0] == 5'b00101)? 5'd6 : (ecc_ins_reg[4:0] == 5'b00110)? 5'd7 : (ecc_ins_reg[4:0] == 5'b10000)? 5'd8  : ((ecc_ins_reg[4:0] == 5'b10001) | (frsav[39] & ~s_range_check))? 5'd9 : (fsav[2] | fsav[4] | frsav[20] | (ecc_ins_reg[4:0] == 5'b10010))? 5'd10 : (frsav[21] | (ecc_ins_reg[4:0] == 5'b10011) | (frsav[39] & s_range_check))? 5'd11 : (frsav[4] | fsav[6])? 5'd12 : (frsav[15]) ? 5'd13 : (frsav[16])? 5'd14 : (frsav[36] | frsav[5] | frsav[22] | fexp[8] | fexp[14] | fexp[18])? 5'd15 :  (fead[3])? waddr_to_extn : 5'd31;

assign mem_in[255:2] = (fins & din_valid)? data_in[255:2] : (write_mas_out) ? mas_out[255:2] : (wren_fadd54)? {y3_div2,mem_out1[255:3]} : (frsav[36] | frsav[1] | frsav[5] | frsav[22] | frsav[25] | fead[0] | fead[1] | fexp[0] | fexp[8] | fexp[22] | frepm[0] | frepm[1] | frepm[5] | frepm[6])? 254'd0 : mem_out1[255:2];

assign mem_in[1:0] = (fins & din_valid)? data_in[1:0] : (write_mas_out) ? mas_out[1:0] : (wren_fadd54)? mem_out1[2:1] : (fexp[22] | frepm[5] | frsav[5] | frsav[22] | frsav[25] | frsav[36])? 2'd0 : (frsav[1] | fead[0] | fead[1] | fexp[8] | frepm[0] | frepm[1] | frepm[6])? 2'd1 : (fexp[0])? 2'd2 : mem_out1[1:0];

always@(posedge clk) begin
   if(frsav[41] | frepm[2] | exp_start) 
    count[8:0] <= 9'd255;
   else if(~count_zero & (frsav[0] | frsav[35] | fepm[2] | (fepm[0] & ~dr[255]) | (fexp[18] & mas_done) )) 
    count[8:0] <= count[8:0] - 1'b1;
   else 
    count[8:0] <= count[8:0];	  
end
assign count_zero = ~(count[8] | count[7] | count[6] | count[5] | count[4] | count[3] | count[2] | count[1] | count[0]);

always@(posedge clk) begin
   if(read_count_start)     read_count[1:0] <= 2'd0;
   else if(~read_count[1])  read_count[1:0] <= read_count[1:0] + 1'b1;
   else                     read_count[1:0] <= read_count[1:0];	  
end
assign read_done = (~read_count[1]&read_count[0])? 1'b1 : 1'b0;

always@(posedge clk) begin
   if(frsav[41] | frepm[2] | exp_start) 
    dr[255:0] <= mem_out1;                       
   else if(frsav[0] | frsav[35] | fepm[2] | (fepm[0] & ~dr[255]) | (fexp[18] & mas_done)) 
    dr[255:0] <= {dr[254:0],1'b0};	 
end 

always@(posedge clk or negedge resetn) begin
  if(~resetn) begin
    fead[9:0] <= 10'd0;
    fins <= 1'b0;
    fsav[16:0] <= 17'd0;
	frsav[41:0] <= 42'd0;
    fepm[9:0] <= 10'd0;
	frepm[6:0] <= 7'd0;
    fdsc[11:0] <= 12'd0;
    fexp[23:0] <= 24'd0;
    mas_mode <= 2'b00;
    mas_start <= 1'b0;
    eaj_ad <= 1'b0;
    eaj_start <= 1'b0;
    epm_start <= 1'b0;
    dsav_start <= 1'b0;
    ecc_done <= 1'b0;
    ecc_busy <= 1'b0;
    dout_valid <= 1'b0;
    exp_start <= 1'b0;
    ecc_ins_reg <= 5'd0;
    sel_n <= 1'b0;
    ecc_sdone <= 1'b0;
	raddr_mem1 <= 5'd0;
	raddr_mem2 <= 5'd0;
	read_count_start <= 1'b0;
    keep_eaj_jpc1 <= 1'b0;
    keep_eaj_jpc2 <= 1'b0;
    rd_nonzero <= 1'b0;
    s_nonzero <= 1'b0;
	s_range_check <= 1'b0;
 end
 else if(sw_reset | (ecc_start & ~ecc_busy)) begin
   ecc_busy <= 1'b1;
   dout_valid <= 1'b0;
   epm_start <= 1'b0;
   exp_start <= 1'b0;
   dsav_start <= 1'b0;
   fead[9:0] <= 10'd0;
   fins <= 1'b1;
   fsav[16:0] <= 17'd0;
   frsav[41:0] <= 42'd0;
   fepm[9:0] <= 10'd0;
   frepm[6:0] <= 7'd0;
   fdsc[11:0] <= 12'd0;
   fexp[23:0] <= 24'd0;
   ecc_ins_reg <= 5'd0;
   ecc_sdone <= 1'b0;
   sel_n <= 1'b0;
   raddr_mem1 <= 5'd0;
   raddr_mem2 <= 5'd0;
   read_count_start <= 1'b0;
   keep_eaj_jpc1 <= 1'b0;
   keep_eaj_jpc2 <= 1'b0;
   rd_nonzero <= 1'b0;
   s_nonzero <= 1'b0;
   s_range_check <= 1'b0;
 end
 else if(fins) begin
   if(ins_valid) 
     ecc_ins_reg[4:0] <= ecc_ins[4:0];	
   else if(ecc_ins_reg[4:0] == 5'b01111) begin
     fins <= 1'b0;
     dsav_start <= 1'b1;
     raddr_mem1 <= 5'd9;  //rd
     read_count_start <= 1'b1;
   end
 end
 else if(fead[0]) begin //write 1 to r0z
  fead[0] <= 1'b0;
  fead[1] <= 1'b1;
 end
 else if(fead[1]) begin //write 1 to r1z
  fead[1] <= 1'b0;
  fead[2] <= 1'b1;
 end
 else if(fead[2]) begin 
  fead[2] <= 1'b0;
  fead[3] <= 1'b1;
  eaj_start <= 1'b1;
 end
 else if(fead[3]) begin
   eaj_start <= 1'b0;
   if(eaj_done) begin
     fead[3] <= 1'b0;
     fead[4] <= 1'b1;
     raddr_mem1 <= 5'd16;  //read cx
   end	
 end
 else if(fead[4]) begin
  fead[4] <= 1'b0;
  fead[5] <= 1'b1;
  raddr_mem1 <= 5'd17;    //read cy
 end
 else if(fead[5]) begin
  fead[5] <= 1'b0;
  fead[6] <= 1'b1;
  raddr_mem1 <= 5'd18;    //read cz,
 end
 else if(fead[6]) begin   //write cx to r0x
  fead[6] <= 1'b0;
  fead[7] <= 1'b1;
 end 
 else if(fead[7]) begin   //write cy to r0y
  fead[7] <= 1'b0;
  fead[8] <= 1'b1;
 end 
 else if(fead[8]) begin   //write cz to r0z
  fead[8] <= 1'b0;
  fead[9] <= 1'b1;
 end 
 else if(fead[9]) begin
  fead[9] <= 1'b0;
  if(keep_eaj_jpc1)      fepm[3] <= 1'b1; 
  else if(keep_eaj_jpc2) fepm[5] <= 1'b1; 
  else                   fexp[0] <= 1'b1;
 end
  /*START ECDSA Verify*/
 else if(dsav_start) begin
  read_count_start <= 1'b0;
  sel_n <= 1'b1;
  if(read_done) begin
   dsav_start <= 1'b0;
   frsav[36] <= 1'b1;
  end
 end
 
 else if(frsav[36]) begin //write 0 to 15
  frsav[36] <= 1'b0;
  frsav[37] <= 1'b1;
 end
 else if(frsav[37]) begin //write 0 to 15
  frsav[37] <= 1'b0;
  frsav[38] <= 1'b1;
  raddr_mem2 <= 5'd15;  
  read_count_start <= 1'b1;
 end
 else if(frsav[38]) begin
  read_count_start <= 1'b0;
  if(read_done) begin
   mas_mode <= 2'b10; //r + 0 
   mas_start <= 1'b1;
   frsav[38] <= 1'b0;
   frsav[39] <= 1'b1;
  end
 end
  else if(frsav[39]) begin
  mas_start <= 1'b0;
  if(mas_done) begin //update r or s //if s_range_check write to 11 else write to 9
    frsav[39] <= 1'b0;
    frsav[40] <= 1'b1;
  end
 end
 else if(frsav[40]) begin 
  frsav[40] <= 1'b0;
  frsav[41] <= 1'b1;
  read_count_start <= 1'b1;
 end
 else if(frsav[41]) begin
  read_count_start <= 1'b0;
  if(read_done) begin
   frsav[41] <= 1'b0;
   if(s_range_check) frsav[35] <= 1'b1;
   else frsav[0] <= 1'b1;
  end
 end
 
 else if(frsav[0]) begin
   if(dr[255]) rd_nonzero <= 1'b1;
   if(count_zero) begin
    if(rd_nonzero) begin 
     raddr_mem1 <= 5'd11;  //s
     read_count_start <= 1'b1;
	 frsav[0] <= 1'b0;
	 frsav[34] <= 1'b1;  //move for ecdsa verify
    end
    else begin
     frsav[0] <= 1'b0;
     frsav[1] <= 1'b1; //FAIL! set r0x to 1 and return
    end
   end	
 end
 
 else if(frsav[34]) begin
  read_count_start <= 1'b0;
  if(read_done) begin
   frsav[34] <= 1'b0;
   frsav[36] <= 1'b1;
   s_range_check <= 1'b1;
  end
 end
 
 else if(frsav[35]) begin
   s_range_check <= 1'b0;
   if(dr[255]) s_nonzero <= 1'b1;
   if(count_zero) begin
    if(s_nonzero) begin 
     raddr_mem1 <= 5'd3;    //read r1x,
	 frsav[35] <= 1'b0;
	 frsav[2] <= 1'b1;  //move for ecdsa verify
    end
    else begin
     frsav[35] <= 1'b0;
     frsav[1] <= 1'b1; //FAIL! set r0x to 1 and return
    end
   end	
 end
 
 else if(frsav[1]) begin //FAIL write 1 to r0x
   frsav[1] <= 1'b0;
   fsav[15] <= 1'b1;
   raddr_mem1 <= 5'd0;  //r0x
   read_count_start <= 1'b1;
 end
 else if(frsav[2]) begin  //write s to r0z
   frsav[2] <= 1'b0;
   frsav[3] <= 1'b1;
 end 
 else if(frsav[3]) begin  
   frsav[3] <= 1'b0;
   frsav[4] <= 1'b1;
 end
 else if(frsav[4]) begin  //write r1x to t
   frsav[4] <= 1'b0;
   fsav[0] <= 1'b1;
   fexp[0] <= 1'b1;
 end
 else if(fsav[0] & ecc_sdone) begin //r0z = s^-1 mod n = s^n-2 mod n
  ecc_sdone <= 1'b0;
  fsav[0] <= 1'b0;
  frsav[5] <= 1'b1;
  raddr_mem1 <= 5'd12; //read t
 end  
 else if(frsav[5]) begin //write 0 to address 15 for e + 0 mod n
  frsav[5] <= 1'b0;
  frsav[6] <= 1'b1;
 end
 else if(frsav[6]) begin //read e and 0
  raddr_mem1 <= 5'd10;
  raddr_mem2 <= 5'd15;
  frsav[6] <= 1'b0;
  frsav[7] <= 1'b1;
 end
 else if(frsav[7]) begin //write t to r1x back
  frsav[7] <= 1'b0;
  frsav[8] <= 1'b1;
 end
 else if(frsav[8]) begin 
  frsav[8] <= 1'b0;
  fsav[1] <= 1'b1;
 end
 else if(fsav[1]) begin
  mas_mode <= 2'b10; //e + 0 
  mas_start <= 1'b1;
  fsav[1] <= 1'b0;
  fsav[2] <= 1'b1;
 end
 else if(fsav[2]) begin
  mas_start <= 1'b0;
  if(mas_done) begin //update e
    fsav[2] <= 1'b0;
    frsav[9] <= 1'b1;
  end
 end
 else if(frsav[9]) begin 
  raddr_mem1 <= 5'd10;
  raddr_mem2 <= 5'd2;
  read_count_start <= 1'b1;
  frsav[9] <= 1'b0;
  fsav[3] <= 1'b1;
 end
 else if(fsav[3]) begin //u1 = e * r0z (s^-1)
  read_count_start <= 1'b0;
  if(read_done) begin
   mas_mode <= 2'b11;
   mas_start <= 1'b1;
   fsav[3] <= 1'b0;
   fsav[4] <= 1'b1;
  end 
 end
 else if(fsav[4]) begin
  mas_start <= 1'b0;
  if(mas_done) begin //r3x(24) <= mas_out; //u1 10
   fsav[4] <= 1'b0;
   frsav[10] <= 1'b1;
  end
 end
 else if(frsav[10]) begin 
  raddr_mem1 <= 5'd9;  //rd
  raddr_mem2 <= 5'd2;   //r0z
  read_count_start <= 1'b1;
  frsav[10] <= 1'b0;
  fsav[5] <= 1'b1;
 end 
 else if(fsav[5]) begin
  read_count_start <= 1'b0;
  if(read_done) begin 
   mas_mode <= 2'b11;
   mas_start <= 1'b1;
   fsav[5] <= 1'b0;
   fsav[6] <= 1'b1;
  end 
 end
 else if(fsav[6]) begin
  mas_start <= 1'b0;
  if(mas_done) begin //t <= mas_out; //u2 == rd * s^-1
   fsav[6] <= 1'b0;
   frsav[11] <= 1'b1;
  end
 end
 else if(frsav[11]) begin //copy r0x, r0y, r1x and r1y
  frsav[11] <= 1'b0;
  frsav[12] <= 1'b1;
 end
 else if(frsav[12]) begin 
  raddr_mem1 <= 5'd3;
  frsav[12] <= 1'b0;
  frsav[13] <= 1'b1;
 end
 else if(frsav[13]) begin 
  raddr_mem1 <= 5'd4;
  frsav[13] <= 1'b0;
  frsav[14] <= 1'b1;
 end
 else if(frsav[14]) begin 
  raddr_mem1 <= 5'd10;   //read u1(24) //24
  frsav[14] <= 1'b0;
  frsav[15] <= 1'b1;
 end
 else if(frsav[15]) begin //write at r2x(13) to copy r1x
  frsav[15] <= 1'b0;
  frsav[16] <= 1'b1;
 end
 else if(frsav[16]) begin //write at r2y(14) to copy r1y
  frsav[16] <= 1'b0;
  frsav[17] <= 1'b1; 
 end
 else if(frsav[17]) begin //write u1(24) to r1x //10
  frsav[17] <= 1'b0;
  fsav[7] <= 1'b1;
 end
 else if(fsav[7]) begin
  fsav[7] <= 1'b0;
  fsav[8] <= 1'b1;
  sel_n <= 1'b0;
  epm_start <= 1'b1; //[u1]G
 end
 else if(fsav[8] & ecc_sdone) begin
  ecc_sdone <= 1'b0;
  fsav[8] <= 1'b0;
  frsav[18] <= 1'b1;
  raddr_mem1 <= 5'd0;     //read r0x (0)
 end
 else if(frsav[18]) begin //read r0y (1)
  raddr_mem1 <= 5'd1;
  frsav[18]  <= 1'b0;
  frsav[19]  <= 1'b1;
 end
 else if(frsav[19]) begin 
  raddr_mem1 <= 5'd12;  //read t (u2 from 12)
  frsav[19] <= 1'b0;
  frsav[20] <= 1'b1;
 end
 else if(frsav[20]) begin //write r0x to r3x(24)
  raddr_mem1 <= 5'd13;  //read r1x
  frsav[20] <= 1'b0;
  frsav[21] <= 1'b1;
 end
 else if(frsav[21]) begin //write r0y to r3y(25)
  raddr_mem1 <= 5'd14;  //read r1y
  frsav[21] <= 1'b0;
  frsav[27] <= 1'b1;
 end
 else if(frsav[27]) begin //write u2 to r1x(3)
  frsav[27] <= 1'b0;
  frsav[28] <= 1'b1;
 end
 else if(frsav[28]) begin //write r1x (13) to r0x(0)
  frsav[28] <= 1'b0;
  frsav[29] <= 1'b1;
 end
  else if(frsav[29]) begin //write r1y (14) to r0y(1)
  frsav[29] <= 1'b0;
  fsav[9] <= 1'b1;
  epm_start <= 1'b1;
 end
 else if(fsav[9] & ecc_sdone) begin
  ecc_sdone <= 1'b0;
  fsav[9] <= 1'b0;
  frsav[30] <= 1'b1;
  raddr_mem1 <= 5'd10;     //read r3x (24)
 end
 else if(frsav[30]) begin 
  raddr_mem1 <= 5'd11;     //read r3y (25)
  frsav[30] <= 1'b0;
  frsav[31] <= 1'b1;
 end
 else if(frsav[31]) begin 
  frsav[31] <= 1'b0;
  frsav[32] <= 1'b1;
 end
 else if(frsav[32]) begin //write r3x to r1x (3)
  frsav[32] <= 1'b0;
  frsav[33] <= 1'b1;
 end
 else if(frsav[33]) begin //write r3y to r1y (4)
  frsav[33] <= 1'b0;
  fsav[10] <= 1'b1;
  eaj_ad <= 1'b1;
  fead[0] <= 1'b1;
 end
 else if(fsav[10] & ecc_sdone) begin
  ecc_sdone <= 1'b0;
  fsav[10] <= 1'b0;
  frsav[22] <= 1'b1;
  sel_n <= 1'b1;
 end
 else if(frsav[22]) begin //write 0 to 15
  frsav[22] <= 1'b0;
  frsav[23] <= 1'b1;
 end
 else if(frsav[23]) begin //read from r0x and 15
  read_count_start <= 1'b1;
  raddr_mem1 <= 5'd0;   
  raddr_mem2 <= 5'd15;  
  frsav[23] <= 1'b0;
  fsav[11] <= 1'b1;
 end
 else if(fsav[11]) begin
  read_count_start <= 1'b0;
  if(read_done) begin
   mas_mode <= 2'b10; //x1+0 mod n 
   mas_start <= 1'b1;
   fsav[11] <= 1'b0;
   fsav[12] <= 1'b1;
  end 
 end
 else if(fsav[12]) begin
  mas_start <= 1'b0;
  if(mas_done) begin //write mas_out to r0x
    fsav[12] <= 1'b0;
    frsav[24] <= 1'b1;
  end
 end
 else if(frsav[24]) begin //read from r0x and r (9)
  read_count_start <= 1'b1;
  raddr_mem1 <= 5'd0;   
  raddr_mem2 <= 5'd9;  
  frsav[24] <= 1'b0;
  fsav[13] <= 1'b1;
 end
 else if(fsav[13]) begin
  read_count_start <= 1'b0;
  if(read_done) begin
   mas_mode <= 2'b01; //doing x1 - r mod n 
   mas_start <= 1'b1;
   fsav[13] <= 1'b0;
   fsav[14] <= 1'b1;
  end  
 end
 else if(fsav[14]) begin
  mas_start <= 1'b0;
  if(mas_done) begin //write mas_out to r0x
   fsav[14] <= 1'b0;
   frsav[25] <= 1'b1;
  end
 end
 else if(frsav[25]) begin //write 0 to r0y
  frsav[25] <= 1'b0;
  frsav[26] <= 1'b1;
 end
 else if(frsav[26]) begin //read r0x and r0y for output
  read_count_start <= 1'b1;
  raddr_mem1 <= 5'd0;   
  raddr_mem2 <= 5'd1;  
  frsav[26] <= 1'b0;
  fsav[15] <= 1'b1;
 end
 else if(fsav[15]) begin
  read_count_start <= 1'b0;
  if(read_done) begin 
   ecc_done <= 1'b1;
   dout_valid <= 1'b1;
   fsav[15] <= 1'b0;
   fsav[16] <= 1'b1;
  end 
 end
 else if(fsav[16]) begin
   ecc_done <= 1'b0;
   ecc_busy <= 1'b0;
   fsav[16] <= 1'b0;
 end
 /*END ECDSA Verify*/
 
 else if(fdsc[0]) begin
   raddr_mem1 <= 5'd2;  //r0z
   raddr_mem2 <= 5'd2;  //r0z
   read_count_start <= 1'b1;
   fdsc[0] <= 1'b0;
   fdsc[1] <= 1'b1;
 end
 else if(fdsc[1]) begin
  read_count_start <= 1'b0;
  if(read_done) begin   
   mas_start <= 1'b1;
   mas_mode <= 2'b11; // r1x = r0z . r0z
   fdsc[1] <= 1'b0;
   fdsc[2] <= 1'b1;
  end 
 end
 else if(fdsc[2]) begin
   mas_start <= 1'b0;
   if(mas_done) begin  //write mas_out to r1x;
     fdsc[2] <= 1'b0;
     fdsc[3] <= 1'b1; 
   end	 
 end
 else if(fdsc[3]) begin
   raddr_mem1 <= 5'd0;  //r0x
   raddr_mem2 <= 5'd3;  //r1x
   read_count_start <= 1'b1;
   fdsc[3] <= 1'b0;
   fdsc[4] <= 1'b1;
 end
 else if(fdsc[4]) begin
  read_count_start <= 1'b0;
  if(read_done) begin   
   mas_start <= 1'b1;
   mas_mode <= 2'b11; // r0x = r0x . r1x
   fdsc[4] <= 1'b0;
   fdsc[5] <= 1'b1;
  end 
 end
 else if(fdsc[5]) begin
    mas_start <= 1'b0;
    if(mas_done) begin //write mas_out to r0x
      fdsc[5] <= 1'b0;
      fdsc[6] <= 1'b1; 
    end	 
 end
 else if(fdsc[6]) begin
   raddr_mem1 <= 5'd2;  //r0z
   raddr_mem2 <= 5'd3;  //r1x
   read_count_start <= 1'b1;
   fdsc[6] <= 1'b0;
   fdsc[7] <= 1'b1;
 end
 else if(fdsc[7]) begin
  read_count_start <= 1'b0;
  if(read_done) begin   
   mas_start <= 1'b1;
   mas_mode <= 2'b11; // r1x = r0z . r1x
   fdsc[7] <= 1'b0;
   fdsc[8] <= 1'b1;
  end 
 end
 else if(fdsc[8]) begin
    mas_start <= 1'b0;
    if(mas_done) begin //write mas_out to r1x
      fdsc[8] <= 1'b0;
      fdsc[9] <= 1'b1; 
    end	 
 end
 else if(fdsc[9]) begin
   raddr_mem1 <= 5'd1;  //r0y
   raddr_mem2 <= 5'd3;  //r1x
   read_count_start <= 1'b1;
   fdsc[9] <= 1'b0;
   fdsc[10] <= 1'b1;
 end
 else if(fdsc[10]) begin
  read_count_start <= 1'b0;
  if(read_done) begin   
   mas_start <= 1'b1;
   mas_mode <= 2'b11; // r0y = r0y . r1x
   fdsc[10] <= 1'b0;
   fdsc[11] <= 1'b1;
  end 
 end
 else if(fdsc[11]) begin
    mas_start <= 1'b0;
    if(mas_done) begin //write mas_out to r0y
      fdsc[11] <= 1'b0;
      fepm[6] <= 1'b1;
    end	 
 end
 //start point multiplication flow
 else if(epm_start) begin
  raddr_mem1 <= 5'd3;  //read r1x //3
  raddr_mem2 <= 5'd12;  //read t //12
  epm_start <= 1'b0;
  frepm[0] <= 1'b1;
 end
 else if(frepm[0]) begin  //read r0x write 1 to r0z
  raddr_mem1 <= 5'd0;  //r0x
  frepm[0] <= 1'b0;
  frepm[1] <= 1'b1;
 end
 else if(frepm[1]) begin //read r0y write 1 to r1z
  raddr_mem1 <= 5'd1;  //r0y
  frepm[1] <= 1'b0;
  frepm[2] <= 1'b1;
 end
 else if(frepm[2]) begin //initialize dr by r1x and dr2 by t
  frepm[2] <= 1'b0;
  frepm[3] <= 1'b1;
 end
 else if(frepm[3]) begin //write r0x to r1x
  frepm[3] <= 1'b0;
  frepm[4] <= 1'b1;
 end
 else if(frepm[4]) begin //write r0y to r1y
  frepm[4] <= 1'b0;
  fepm[0] <= 1'b1;
 end
 else if(frepm[5]) begin //write r0x <= 256'd0
  frepm[5] <= 1'b0;
  frepm[6] <= 1'b1;
 end
 else if(frepm[6]) begin //write r0y <= 256'd1
  frepm[6] <= 1'b0;
  fepm[6] <= 1'b1;
 end
 else if(fepm[0]) begin
   if(count_zero) begin
     fepm[0] <= 1'b0;
	 if(~dr[255])   
	  frepm[5] <= 1'b1;  //write r0x <= 256'd0; r0y <= 256'd1;
     else
      fepm[6] <= 1'b1;
   end
   else if(dr[255]) begin //added this condition to skip leading zeros in the scaler dr 
     fepm[0] <= 1'b0;  
     fepm[2] <= 1'b1;
   end	 
 end 
 else if(fepm[2]) begin
   eaj_start <= 1'b1;
   eaj_ad <= 1'b0; // r1 = 2r1 simple binary left_to_right
   keep_eaj_jpc1 <= 1'b1;
   fepm[2] <= 1'b0;
   fead[3] <= 1'b1;
 end
 else if(fepm[3]) begin
   keep_eaj_jpc1 <= 1'b0;
   fepm[3] <= 1'b0;
   fepm[4] <= 1'b1; 
 end 
 else if(fepm[4]) begin
   if(~dr[255]) begin
     if(count_zero) begin
       fepm[4] <= 1'b0;
       fexp[0] <= 1'b1;
     end  
     else begin
       fepm[4] <= 1'b0;
       fepm[2] <= 1'b1;
     end
   end	
   else begin
     eaj_start <= 1'b1;
     eaj_ad <= 1'b1; // r0 = r0 + r1
	 keep_eaj_jpc2 <= 1'b1;
     fepm[4] <= 1'b0;
     fead[3] <= 1'b1;
   end	
 end
 else if(fepm[5]) begin
   keep_eaj_jpc2 <= 1'b0;
   if(count_zero) begin
    fepm[5] <= 1'b0;
    fexp[0] <= 1'b1;
   end  
   else begin
     fepm[5] <= 1'b0;
     fepm[2] <= 1'b1;
   end  
 end
 else if(fexp[0]) begin //write 2 to r1x
   fexp[0] <= 1'b0;
   fexp[1] <= 1'b1;
 end
 else if(fexp[1]) begin 
   if(fsav[0]) raddr_mem1 <= 5'd8;  //n
   else raddr_mem1 <= 5'd6;         //p
   raddr_mem2 <= 5'd3;              //r1x
   read_count_start <= 1'b1;
   fexp[1] <= 1'b0;
   fexp[2] <= 1'b1;
 end
 else if(fexp[2]) begin 
  read_count_start <= 1'b0;
  if (read_done) begin
   fexp[2] <= 1'b0;
   fexp[3] <= 1'b1;
   mas_start <= 1'b1;
   mas_mode <= 2'b01; // r1z = p - r1x or p-2   
  end
 end
 else if(fexp[3]) begin //write mas_out to r1z
   mas_start <= 1'b0;
   if(mas_done) begin
     fexp[3] <= 1'b0;
     fexp[8] <= 1'b1;
	 raddr_mem1 <= 5'd5; //initialize dr with r1z
   end	 
 end
 else if(fexp[5]) begin             /*States 5 to 7 are used for direct exp call from outside*/
    read_count_start <= 1'b0;
	if(read_done) begin       //write (ina) r0x to r0z
	 fexp[5] <= 1'b0;
     fexp[6] <= 1'b1;
	end 
 end
 else if(fexp[6]) begin //read ine from r0y
    fexp[6] <= 1'b0;
    fexp[7] <= 1'b1;
	raddr_mem1 <= 5'd1;  
    read_count_start <= 1'b1;
 end
 else if(fexp[7]) begin 
    read_count_start <= 1'b0; 
    if(read_done) begin      //write (ine) r0y to r1z	
     fexp[7] <= 1'b0;
     fexp[8] <= 1'b1;
	 raddr_mem1 <= 5'd5; //initialize dr with r1z
	end
 end
 else if(fexp[8]) begin //write 1 to temp
    fexp[8] <= 1'b0;
    fexp[9] <= 1'b1;
 end
 else if(fexp[9]) begin 
	fexp[9] <= 1'b0;
    fexp[10] <= 1'b1;
	exp_start <= 1'b1;
 end
 else if(fexp[10]) begin //exp_start
    exp_start <= 1'b0;
    fexp[10] <= 1'b0;
    fexp[11] <= 1'b1;
	raddr_mem1 <= 5'd15;
	raddr_mem2 <= 5'd15;
 end
 else if(fexp[11]) begin  //read temp 
	fexp[11] <= 1'b0;
    fexp[12] <= 1'b1;
 end
 else if(fexp[12]) begin  //read temp delay
	fexp[12] <= 1'b0;
	fexp[13] <= 1'b1;
 end
 else if(fexp[13]) begin //mas start for square temp x temp
    mas_start <= 1'b1;
	mas_mode <= 2'b11;
	fexp[13] <= 1'b0;
    fexp[14] <= 1'b1;
 end
 else if(fexp[14]) begin //write enable with mas_done from mas_out to temp
    mas_start <= 1'b0;
	if(mas_done) begin
	 fexp[14] <= 1'b0;
     fexp[15] <= 1'b1;
	 raddr_mem2 <= 5'd2;
	end
 end
 else if(fexp[15]) begin //read from temp and r0z
  fexp[15] <= 1'b0;
  fexp[16] <= 1'b1;
 end
 else if(fexp[16]) begin //read from temp and r0z
  fexp[16] <= 1'b0;
  fexp[17] <= 1'b1;
 end
 else if(fexp[17]) begin //mas_start for mult temp * r0z
  mas_start <= 1'b1;
  fexp[17] <= 1'b0;
  fexp[18] <= 1'b1;
 end
 else if(fexp[18]) begin //write enable with mas_done & dr[383] from mas_out to temp
    mas_start <= 1'b0;
	if(mas_done) begin
     fexp[18] <= 1'b0;
     if(count_zero) fexp[19] <= 1'b1;
	 else fexp[11] <= 1'b1;
	 raddr_mem2 <= 5'd15;
	end
 end
 else if(fexp[19]) begin //mas_start for mult temp * r0z
  fexp[19] <= 1'b0;
  fexp[20] <= 1'b1;
 end
 else if(fexp[20]) begin
  fexp[20] <= 1'b0;
  if(ecc_ins_reg[4:0] == 5'b01011) fexp[21] <= 1'b1; 
  else                             fexp[23] <= 1'b1;
 end
 else if(fexp[21]) begin //write temp to r0x             
  fexp[21] <= 1'b0;
  fexp[22] <= 1'b1; 
 end 
 else if(fexp[22]) begin //write 0 to r0y to make cy_out = 0
  fexp[22] <= 1'b0;
  fepm[6] <= 1'b1; 
 end		
 else if(fexp[23]) begin //write temp to r0z
  fexp[23] <= 1'b0;
  if(fsav[0]) fepm[6] <= 1'b1; 
  else        fdsc[0] <= 1'b1; 
 end  
 else if(fepm[6]) begin
   if((ecc_ins_reg[4:0] == 5'b01111)) begin
     ecc_sdone <= 1'b1;
     fepm[6] <= 1'b0;
   end
   else begin //read from r0x and r0y as output
      fepm[6] <= 1'b0;
      fepm[7] <= 1'b1;
	  raddr_mem1 <= 5'd0;
	  raddr_mem2 <= 5'd1;
   end 
 end
 else if(fepm[7]) begin
   fepm[7] <= 1'b0;
   fepm[8] <= 1'b1;
 end
 else if(fepm[8]) begin //r0x and r0y will be at the memory port
   ecc_done <= 1'b1;
   dout_valid <= 1'b1;
   fepm[8] <= 1'b0;
   fepm[9] <= 1'b1;  
 end
 else if(fepm[9]) begin
   ecc_done <= 1'b0;
   ecc_busy <= 1'b0;
   fepm[9] <= 1'b0;
 end
end

endmodule
