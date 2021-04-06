// (C) 2019 Intel Corporation. All rights reserved.
// Your use of Intel Corporation's design tools, logic functions and other 
// software and tools, and its AMPP partner logic functions, and any output 
// files from any of the foregoing (including device programming or simulation 
// files), and any associated documentation or information are expressly subject 
// to the terms and conditions of the Intel Program License Subscription 
// Agreement, Intel FPGA IP License Agreement, or other applicable 
// license agreement, including, without limitation, that your use is for the 
// sole purpose of programming logic devices manufactured by Intel and sold by 
// Intel or its authorized distributors.  Please refer to the applicable 
// agreement for further details.

// Suppress Quartus Warning Messages
// altera message_off 10036
module ocs_ecp256_ad_jpc (clk, resetn, start, ad, mas_done, mas_cout, mem_out0, raddr_to_extn1, raddr_to_extn2, waddr_to_extn, wren_init, wren_fadd54, y3_div2, done, mas_mode, mas_start); 
  input             clk; 
  input             resetn;
  input             start;
  input             ad;
  input             mas_done;
  input             mas_cout;
  input             mem_out0;
  output [4:0]      raddr_to_extn1;
  output [4:0]      raddr_to_extn2;
  output [4:0]      waddr_to_extn;
  output            wren_init;
  output            wren_fadd54;
  output            y3_div2;  
  output  reg       done;
  output  reg [1:0] mas_mode; // 2'b11 -> GF(p) mult,  2'b10 -> GF(p) add, 2'b01 -> GF(p) sub, 2'b00 -> integer add/sub
  output  reg       mas_start; 

reg [45:0] fdbl;
reg [56:0] fadd;
reg [14:0] fread;
reg [28:0] fradd;
reg [21:0] frdbl;

wire [2:0]  waddr_mem, raddr_mem1_int, raddr_mem2_int; 
wire mas_in1_t1, mas_in1_t2, mas_in1_t3, mas_in1_t4, mas_in1_t5, mas_in1_t6, mas_in1_t7;
wire mas_in2_t1, mas_in2_t2, mas_in2_t3, mas_in2_t4, mas_in2_t5, mas_in2_t6, mas_in2_t7, mas_in2_t8;

reg y3_odd;
wire y3_div2;
reg mas_coutr;

assign mas_in1_t1 = frdbl[1] | fdbl[2] | fdbl[3] | frdbl[2] | fdbl[4] | fdbl[5] | frdbl[9] | fdbl[18] | fdbl[19] | frdbl[13] | fdbl[26] | fdbl[27] | frdbl[14] | fdbl[28] | fdbl[29] | fradd[1] | fadd[2] | fadd[3] | fradd[8] | fadd[16] | fadd[17] | fradd[10] | fadd[20] | fadd[21] | fradd[11] | fadd[22] | fadd[23] | fradd[18] | fadd[36] | fadd[37] | fradd[20] | fadd[40] | fadd[41];
assign mas_in1_t2 = frdbl[8] | fdbl[16] | fdbl[17] | frdbl[15] | fdbl[30] | fdbl[31] | frdbl[16] | fdbl[32] | fdbl[33] | frdbl[17] | fdbl[34] | fdbl[35] | frdbl[18] | fdbl[36] | fdbl[37] | fradd[3] | fadd[6] | fadd[7] | fradd[9] | fadd[18] | fadd[19] | fradd[12] | fadd[24] | fadd[25] | fradd[13] | fadd[26] | fadd[27] | fradd[24] | fadd[48] | fadd[49] | fradd[26] | fadd[52] | fadd[53] | (fradd[27] & ~y3_odd);
assign mas_in1_t3 = frdbl[0] | fdbl[0] | fdbl[1] | frdbl[6] | fdbl[12] | fdbl[13] | frdbl[7] | fdbl[14] | fdbl[15] | fradd[4] | fadd[8] | fadd[9] | fradd[6] | fadd[12] | fadd[13] | fradd[14] | fadd[28] | fadd[29] | fradd[15] | fadd[30] | fadd[31];
assign mas_in1_t4 = frdbl[3] | fdbl[6] | fdbl[7] | frdbl[5] | fdbl[10] | fdbl[11] | frdbl[12] | fdbl[24] | fdbl[25] | fradd[5] | fadd[10] | fadd[11] | fradd[16] | fadd[32] | fadd[33] | fradd[17] | fadd[34] | fadd[35];
assign mas_in1_t5 = frdbl[4] | fdbl[8] | fdbl[9] | frdbl[10] | fdbl[20] | fdbl[21] | frdbl[11] | fdbl[22] | fdbl[23] | frdbl[19] | fdbl[38] | fdbl[39] | frdbl[20] | fdbl[40] | fdbl[41] | frdbl[21] | fdbl[42] | fdbl[43] | fradd[7] | fadd[14] | fadd[15] | fradd[19] | fadd[38] | fadd[39] | fradd[23] | fadd[46] | fadd[47] | fradd[25] | fadd[50] | fadd[51] | (fradd[27] & y3_odd);
assign mas_in1_t6 = fradd[0] | fadd[0] | fadd[1] | fradd[2] | fadd[4] | fadd[5];
assign mas_in1_t7 = fradd[21] | fadd[42] | fadd[43] | fradd[22] | fadd[44] | fadd[45];

assign mas_in2_t1 = frdbl[19] | fdbl[38] | fdbl[39] | fradd[10] | fadd[20] | fadd[21] | fradd[21] | fadd[42] | fadd[43] | fradd[22] | fadd[44] | fadd[45];
assign mas_in2_t2 = frdbl[6] | fdbl[12] | fdbl[13] | frdbl[8] | fdbl[16] | fdbl[17] | frdbl[9] | fdbl[18] | fdbl[19] | frdbl[15] | fdbl[30] | fdbl[31] | frdbl[16] | fdbl[32] | fdbl[33] | frdbl[17] | fdbl[34] | fdbl[35] | frdbl[18] | fdbl[36] | fdbl[37] | frdbl[21] | fdbl[42] | fdbl[43] | fradd[12] | fadd[24] | fadd[25];
assign mas_in2_t3 = frdbl[0] | fdbl[0] | fdbl[1] | frdbl[7] | fdbl[14] | fdbl[15] | fradd[4] | fadd[8] | fadd[9];
assign mas_in2_t4 = frdbl[1] | fdbl[2] | fdbl[3] | frdbl[2] | fdbl[4] | fdbl[5] | frdbl[12] | fdbl[24] | fdbl[25] | frdbl[20] | fdbl[40] | fdbl[41] | fradd[8] | fadd[16] | fadd[17] | fradd[11] | fadd[22] | fadd[23] | fradd[15] | fadd[30] | fadd[31] | fradd[16] | fadd[32] | fadd[33] | fradd[24] | fadd[48] | fadd[49] | fradd[25] | fadd[50] | fadd[51];
assign mas_in2_t5 = frdbl[3] | fdbl[6] | fdbl[7] | frdbl[4] | fdbl[8] | fdbl[9] | frdbl[5] | fdbl[10] | fdbl[11] | frdbl[10] | fdbl[20] | fdbl[21] | frdbl[11] | fdbl[22] | fdbl[23] | frdbl[13] | fdbl[26] | fdbl[27] | frdbl[14] | fdbl[28] | fdbl[29] | fradd[9] | fadd[18] | fadd[19] | fradd[13] | fadd[26] | fadd[27] | fradd[19] | fadd[38] | fadd[39] | fradd[27];
assign mas_in2_t6 = fradd[0] | fadd[0] | fadd[1] | fradd[14] | fadd[28] | fadd[29];
assign mas_in2_t7 = fradd[1] | fadd[2] | fadd[3] | fradd[2] | fadd[4] | fadd[5] | fradd[3] | fadd[6] | fadd[7] | fradd[5] | fadd[10] | fadd[11] | fradd[6] | fadd[12] | fadd[13] | fradd[7] | fadd[14] | fadd[15] | fradd[17] | fadd[34] | fadd[35] | fradd[18] | fadd[36] | fadd[37] | fradd[20] | fadd[40] | fadd[41] | fradd[23] | fadd[46] | fadd[47];
assign mas_in2_t8 = fradd[26] | fadd[52] | fadd[53];

assign waddr_to_extn[4:0] = {2'b10,waddr_mem[2:0]};
assign waddr_mem[2:0] = (fread[2] | fdbl[25] | fdbl[27] | fdbl[29] | fadd[3] | fadd[21] | fadd[23] | fadd[39] | fadd[41]) ? 3'd0 : (fread[3] | fdbl[17] | fdbl[31] | fdbl[33] | fdbl[35] | fdbl[37] | fdbl[43] | fadd[7] | fadd[25] | fadd[27] | fadd[51] | fadd[54]) ? 3'd1 : (fread[4] | fdbl[13] | fdbl[15] | fadd[29] | fadd[31]) ? 3'd2 : (fread[5] | fdbl[1] | fdbl[5] | fdbl[9] | fdbl[11] | fadd[11] | fadd[17] | fadd[35] | fadd[49]) ? 3'd3 : (fread[6] | fdbl[3] | fdbl[7] | fdbl[19] | fdbl[21] | fdbl[23] | fdbl[39] | fdbl[41] | fadd[15] | fadd[19] | fadd[47] | fadd[53]) ? 3'd4 : (fread[7])  ? 3'd5 : (fadd[1] | fadd[5] | fadd[9] | fadd[13] | fadd[33] | fadd[37] | fadd[43] | fadd[45])? 3'd6 : 3'd7; 
assign wren_init = fread[2] | fread[3] | fread[4] | fread[5] | fread[6] | fread[7] | fread[8];
assign wren_fadd54 = fadd[54]; 

assign raddr_to_extn1[4:0] = (fread[0]) ? 5'd0 : (fread[1]) ? 5'd1 : (fread[2]) ? 5'd2 : (fread[3]) ? 5'd3 : (fread[4]) ? 5'd4 : (fread[5]) ? 5'd5 : (fread[6])?  5'd6 : {2'b10,raddr_mem1_int};
assign raddr_to_extn2[4:0] = {2'b10,raddr_mem2_int};
assign raddr_mem1_int[2:0] = (mas_in1_t1) ? 3'd0 : (mas_in1_t2) ? 3'd1 : (mas_in1_t3) ? 3'd2 :(mas_in1_t4) ? 3'd3 :(mas_in1_t5) ? 3'd4 :(mas_in1_t6) ? 3'd5 : 3'd6;
assign raddr_mem2_int[2:0] = (mas_in2_t1) ? 3'd0 : (mas_in2_t2) ? 3'd1 : (mas_in2_t3) ? 3'd2 :(mas_in2_t4) ? 3'd3 :(mas_in2_t5) ? 3'd4 :(mas_in2_t6) ? 3'd5 : (mas_in2_t7) ? 3'd6 : 3'd7;
assign y3_div2 = (y3_odd) ? mas_coutr : 1'b0;

always @(posedge clk or negedge resetn) begin
 if(~resetn) begin
     fdbl[45:0] <= 46'd0;
	 fadd[56:0] <= 57'd0;
	 fread[14:0] <= 15'd0;
	 fradd[28:0] <= 29'd0;
	 frdbl[21:0] <= 22'd0;
	 mas_mode <= 2'b00;
	 mas_start <= 1'b0;
	 done <= 1'b0;
	 y3_odd <= 1'b0;
	 mas_coutr <= 1'b0;
 end
 else if(start) begin
   fread[14:0] <= 15'd1;
 end
 else if (fread[0]) begin
  fread[0] <= 1'b0;
  fread[1] <= 1'b1;
 end
 else if (fread[1]) begin
   fread[1] <= 1'b0;
   fread[2] <= 1'b1;
 end
 else if (fread[2]) begin
   fread[2] <= 1'b0;
   fread[3] <= 1'b1;
 end
 else if (fread[3]) begin
   fread[3] <= 1'b0;
   fread[4] <= 1'b1;
 end
 else if (fread[4]) begin
   fread[4] <= 1'b0;
   fread[5] <= 1'b1;
 end
 else if (fread[5]) begin
   fread[5] <= 1'b0;
   fread[6] <= 1'b1;
 end
 else if (fread[6]) begin
   fread[6] <= 1'b0;
   fread[7] <= 1'b1;
 end
 else if (fread[7]) begin
   fread[7] <= 1'b0;
   fread[8] <= 1'b1;
 end
 else if (fread[8]) begin
   fread[8] <= 1'b0;
   fread[9] <= 1'b1;
 end
 else if (fread[9]) begin
  fread[9] <= 1'b0;
  if(ad) begin
    fadd[56:0] <= 57'd0;
    fdbl[45:0] <= 46'd0;
    fradd[27:0] <= 28'd16; //skipping as t6 = 1
    frdbl[21:0] <= 22'd0;
  end
  else begin 
   fadd[56:0] <= 57'd0;
   fdbl[45:0] <= 46'd0;
   fradd[27:0] <= 28'd0;
   frdbl[21:0] <= 22'd1;	
  end	
 end
 //start point addition flow
 else if(fradd[0]) begin
   fradd[0] <= 1'b0;
   fadd[0]  <= 1'b1;
 end
 else if(fadd[0]) begin
 	mas_start <= 1'b1;
	mas_mode <= 2'b11; // t_7 = t_6^2
   fadd[0] <= 1'b0;
   fadd[1] <= 1'b1;
 end
 else if(fadd[1]) begin
	 mas_start <= 1'b0;
	 if(mas_done) begin
      fadd[1] <= 1'b0;
      fradd[1] <= 1'b1; 
    end	 
 end 
 else if(fradd[1]) begin
   fradd[1] <= 1'b0;
   fadd[2]  <= 1'b1;
 end
 else if(fadd[2]) begin
    mas_start <= 1'b1;
    mas_mode <= 2'b11; // t_1 = t_1 . t_7
    fadd[2] <= 1'b0;
    fadd[3] <= 1'b1;
 end
 else if(fadd[3]) begin
	 mas_start <= 1'b0;
	 if(mas_done) begin
	  fadd[3] <= 1'b0;
      fradd[2] <= 1'b1; 
    end	 
 end 
 else if(fradd[2]) begin
   fradd[2] <= 1'b0;
   fadd[4]  <= 1'b1;
 end
 else if(fadd[4]) begin
    mas_start <= 1'b1;
	mas_mode <= 2'b11; // t_7 = t_6 . t_7
    fadd[4] <= 1'b0;
    fadd[5] <= 1'b1;
 end
 else if(fadd[5]) begin
	 mas_start <= 1'b0;
	 if(mas_done) begin
	  fadd[5] <= 1'b0;
      fradd[3] <= 1'b1; 
    end	 
 end 
 else if(fradd[3]) begin
   fradd[3] <= 1'b0;
   fadd[6]  <= 1'b1;
 end
 else if(fadd[6]) begin
    mas_start <= 1'b1;
    mas_mode <= 2'b11; // t_2 = t_2 . t_7
    fadd[6] <= 1'b0;
    fadd[7] <= 1'b1;
 end
 else if(fadd[7]) begin
	 mas_start <= 1'b0;
	 if(mas_done) begin
	  fadd[7] <= 1'b0;
      fradd[4] <= 1'b1; 
    end	 
 end 
 else if(fradd[4]) begin
   fradd[4] <= 1'b0;
   fadd[8]  <= 1'b1;
 end
 else if(fadd[8]) begin
    mas_start <= 1'b1;
    mas_mode <= 2'b11; // t_7 = t_3 . t_3
    fadd[8] <= 1'b0;
    fadd[9] <= 1'b1;
 end
 else if(fadd[9]) begin
	 mas_start <= 1'b0;
	 if(mas_done) begin
	  fadd[9] <= 1'b0;
      fradd[5] <= 1'b1; 
    end	 
 end 
 else if(fradd[5]) begin
   fradd[5] <= 1'b0;
   fadd[10]  <= 1'b1;
 end
 else if(fadd[10]) begin
    mas_start <= 1'b1;
    mas_mode <= 2'b11; // t_4 = t_4 . t_7
    fadd[10] <= 1'b0;
    fadd[11] <= 1'b1;
 end
 else if(fadd[11]) begin
	 mas_start <= 1'b0;
	 if(mas_done) begin
      fadd[11] <= 1'b0;
      fradd[6] <= 1'b1; 
    end	 
 end 
 else if(fradd[6]) begin
   fradd[6] <= 1'b0;
   fadd[12]  <= 1'b1;
 end 
 else if(fadd[12]) begin
    mas_start <= 1'b1;
    mas_mode <= 2'b11; // t_7 = t_3 . t_7
    fadd[12] <= 1'b0;
    fadd[13] <= 1'b1;
 end
 else if(fadd[13]) begin
	 mas_start <= 1'b0;
	 if(mas_done) begin
      fadd[13] <= 1'b0;
      fradd[7] <= 1'b1; 
    end	 
 end 
 else if(fradd[7]) begin
   fradd[7] <= 1'b0;
   fadd[14]  <= 1'b1;
 end 
 else if(fadd[14]) begin
    mas_start <= 1'b1;
    mas_mode <= 2'b11; // t_5 = t_5 . t_7
    fadd[14] <= 1'b0;
    fadd[15] <= 1'b1;
 end
 else if(fadd[15]) begin
	 mas_start <= 1'b0;
	 if(mas_done) begin
      fadd[15] <= 1'b0;
      fradd[8] <= 1'b1; 
    end	 
 end 
 else if(fradd[8]) begin
   fradd[8] <= 1'b0;
   fadd[16]  <= 1'b1;
 end 
 else if(fadd[16]) begin
    mas_start <= 1'b1;
    mas_mode <= 2'b01; // t_4 = t_1 - t_4
    fadd[16] <= 1'b0;
    fadd[17] <= 1'b1;
 end
 else if(fadd[17]) begin
	 mas_start <= 1'b0;
	 if(mas_done) begin
      fadd[17] <= 1'b0;
      fradd[9] <= 1'b1; 
    end	 
 end 
 else if(fradd[9]) begin
   fradd[9] <= 1'b0;
   fadd[18]  <= 1'b1;
 end  
 else if(fadd[18]) begin
    mas_start <= 1'b1;
    mas_mode <= 2'b01; // t_5 = t_2 - t_5
    fadd[18] <= 1'b0;
    fadd[19] <= 1'b1;
 end
 else if(fadd[19]) begin
	 mas_start <= 1'b0;
	 if(mas_done) begin
      fadd[19] <= 1'b0;
      fradd[10] <= 1'b1; 
    end	 
 end 
 else if(fradd[10]) begin
   fradd[10] <= 1'b0;
   fadd[20]  <= 1'b1;
 end 
 else if(fadd[20]) begin
   mas_start <= 1'b1;
   mas_mode <= 2'b10; // t_1 = t_1 + t_1
   fadd[20] <= 1'b0;
   fadd[21] <= 1'b1;
 end
 else if(fadd[21]) begin
	 mas_start <= 1'b0;
	 if(mas_done) begin
      fadd[21] <= 1'b0;
      fradd[11] <= 1'b1; 
    end	 
 end
 else if(fradd[11]) begin
   fradd[11] <= 1'b0;
   fadd[22]  <= 1'b1;
 end  
 else if(fadd[22]) begin
    mas_start <= 1'b1;
    mas_mode <= 2'b01; // t_1 = t_1 - t_4
    fadd[22] <= 1'b0;
    fadd[23] <= 1'b1;
 end
 else if(fadd[23]) begin
	 mas_start <= 1'b0;
	 if(mas_done) begin
      fadd[23] <= 1'b0;
      fradd[12] <= 1'b1; 
    end	 
 end
 else if(fradd[12]) begin
   fradd[12] <= 1'b0;
   fadd[24]  <= 1'b1;
 end 
 else if(fadd[24]) begin
    mas_start <= 1'b1;
    mas_mode <= 2'b10; // t_2 = t_2 + t_2
    fadd[24] <= 1'b0;
    fadd[25] <= 1'b1;
 end
 else if(fadd[25]) begin
	 mas_start <= 1'b0;
	 if(mas_done) begin
      fadd[25] <= 1'b0;
      fradd[13] <= 1'b1; 
    end	 
 end
 else if(fradd[13]) begin
   fradd[13] <= 1'b0;
   fadd[26]  <= 1'b1;
 end 
 else if(fadd[26]) begin
    mas_start <= 1'b1;
    mas_mode <= 2'b01; // t_2 = t_2 - t_5
    fadd[26] <= 1'b0;
    fadd[27] <= 1'b1;
 end
 else if(fadd[27]) begin
	 mas_start <= 1'b0;
	 if(mas_done) begin
      fadd[27] <= 1'b0;
      fradd[15] <= 1'b1; //was 14 skipping as t6 = 1 
    end	 
 end
 else if(fradd[14]) begin
   fradd[14] <= 1'b0;
   fadd[28]  <= 1'b1;
 end 
 else if(fadd[28]) begin
      mas_start <= 1'b1;
      mas_mode <= 2'b11; // t_3 = t_3 . t_6
      fadd[28] <= 1'b0;
      fadd[29] <= 1'b1;
 end
 else if(fadd[29]) begin
	 mas_start <= 1'b0;
	 if(mas_done) begin
	  fadd[29] <= 1'b0;
      fradd[15] <= 1'b1; 
    end	 
 end
 else if(fradd[15]) begin
   fradd[15] <= 1'b0;
   fadd[30]  <= 1'b1;
 end  
 else if(fadd[30]) begin
    mas_start <= 1'b1;
    mas_mode <= 2'b11; // t_3 = t_3 . t_4
    fadd[30] <= 1'b0;
    fadd[31] <= 1'b1;
 end
 else if(fadd[31]) begin
	 mas_start <= 1'b0;
	 if(mas_done) begin
      fadd[31] <= 1'b0;
      fradd[16] <= 1'b1; 
    end	 
 end
 else if(fradd[16]) begin
   fradd[16] <= 1'b0;
   fadd[32]  <= 1'b1;
 end 
 else if(fadd[32]) begin
    mas_start <= 1'b1;
    mas_mode <= 2'b11; // t_7 = t_4 . t_4
    fadd[32] <= 1'b0;
    fadd[33] <= 1'b1;
 end
 else if(fadd[33]) begin
	 mas_start <= 1'b0;
	 if(mas_done) begin
      fadd[33] <= 1'b0;
      fradd[17] <= 1'b1; 
    end	 
 end
 else if(fradd[17]) begin
   fradd[17] <= 1'b0;
   fadd[34]  <= 1'b1;
 end 
 else if(fadd[34]) begin
    mas_start <= 1'b1;
    mas_mode <= 2'b11; // t_4 = t_4 . t_7
    fadd[34] <= 1'b0;
    fadd[35] <= 1'b1;
 end
 else if(fadd[35]) begin
	 mas_start <= 1'b0;
	 if(mas_done) begin
      fadd[35] <= 1'b0;
      fradd[18] <= 1'b1; 
    end	 
 end
 else if(fradd[18]) begin
   fradd[18] <= 1'b0;
   fadd[36]  <= 1'b1;
 end 
 else if(fadd[36]) begin
    mas_start <= 1'b1;
    mas_mode <= 2'b11; // t_7 = t_1 . t_7
    fadd[36] <= 1'b0;
    fadd[37] <= 1'b1;
 end
 else if(fadd[37]) begin
	 mas_start <= 1'b0;
	 if(mas_done) begin
      fadd[37] <= 1'b0;
      fradd[19] <= 1'b1; 
    end	 
 end
 else if(fradd[19]) begin
   fradd[19] <= 1'b0;
   fadd[38]  <= 1'b1;
 end 
 else if(fadd[38]) begin
    mas_start <= 1'b1;
    mas_mode <= 2'b11; // t_1 = t_5 . t_5
    fadd[38] <= 1'b0;
    fadd[39] <= 1'b1;
 end
 else if(fadd[39]) begin
	 mas_start <= 1'b0;
	 if(mas_done) begin
      fadd[39] <= 1'b0;
      fradd[20] <= 1'b1; 
    end	 
 end
 else if(fradd[20]) begin
   fradd[20] <= 1'b0;
   fadd[40]  <= 1'b1;
 end 
 else if(fadd[40]) begin
    mas_start <= 1'b1;
    mas_mode <= 2'b01; // t_1 = t_1 - t_7
    fadd[40] <= 1'b0;
    fadd[41] <= 1'b1;
 end
 else if(fadd[41]) begin
	 mas_start <= 1'b0;
	 if(mas_done) begin
      fadd[41] <= 1'b0;
      fradd[21] <= 1'b1; 
    end	 
 end
 else if(fradd[21]) begin
   fradd[21] <= 1'b0;
   fadd[42]  <= 1'b1;
 end 
 else if(fadd[42]) begin
    mas_start <= 1'b1;
    mas_mode <= 2'b01; // t_7 = t_7 - t_1
    fadd[42] <= 1'b0;
    fadd[43] <= 1'b1;
 end
 else if(fadd[43]) begin
	 mas_start <= 1'b0;
	 if(mas_done) begin
      fadd[43] <= 1'b0;
      fradd[22] <= 1'b1; 
    end	 
 end
 else if(fradd[22]) begin
   fradd[22] <= 1'b0;
   fadd[44]  <= 1'b1;
 end
 else if(fadd[44]) begin
    mas_start <= 1'b1;
    mas_mode <= 2'b01; // t_7 = t_7 - t_1
    fadd[44] <= 1'b0;
    fadd[45] <= 1'b1;
 end
 else if(fadd[45]) begin
	 mas_start <= 1'b0;
	 if(mas_done) begin
      fadd[45] <= 1'b0;
      fradd[23] <= 1'b1; 
    end	 
 end
 else if(fradd[23]) begin
   fradd[23] <= 1'b0;
   fadd[46]  <= 1'b1;
 end 
 else if(fadd[46]) begin
    mas_start <= 1'b1;
    mas_mode <= 2'b11; // t_5 = t_7 . t_5
    fadd[46] <= 1'b0;
    fadd[47] <= 1'b1;
 end
 else if(fadd[47]) begin
	 mas_start <= 1'b0;
	 if(mas_done) begin
      fadd[47] <= 1'b0;
      fradd[24] <= 1'b1; 
    end	 
 end
 else if(fradd[24]) begin
   fradd[24] <= 1'b0;
   fadd[48]  <= 1'b1;
 end 
 else if(fadd[48]) begin
    mas_start <= 1'b1;
    mas_mode <= 2'b11; // t_4 = t_2 . t_4
    fadd[48] <= 1'b0;
    fadd[49] <= 1'b1;
 end
 else if(fadd[49]) begin
	 mas_start <= 1'b0;
	 if(mas_done) begin
      fadd[49] <= 1'b0;
      fradd[25] <= 1'b1; 
    end	 
 end
 else if(fradd[25]) begin
   fradd[25] <= 1'b0;
   fadd[50]  <= 1'b1;
 end 
 else if(fadd[50]) begin
    mas_start <= 1'b1;
    mas_mode <= 2'b01; // t_2 = t_5 - t_4
    fadd[50] <= 1'b0;
    fadd[51] <= 1'b1;
 end
 else if(fadd[51]) begin
	 mas_start <= 1'b0;
	 if(mas_done) begin
	  fadd[51] <= 1'b0;
      fradd[26] <= 1'b1; 
    end	 
 end
 else if(fradd[26]) begin
   fradd[26] <= 1'b0;
   fadd[52]  <= 1'b1;
 end 
 else if(fadd[52]) begin
    mas_start <= 1'b1;
    mas_mode <= 2'b00; // t_2 = t_2 + t4 (p) // p is in address 3'd7
    fadd[52] <= 1'b0;
    fadd[53] <= 1'b1;
 end
 else if(fadd[53]) begin
	 mas_start <= 1'b0;
	 if(mas_done) begin
	  y3_odd <= mem_out0; 
	  mas_coutr <= mas_cout;
      fadd[53] <= 1'b0;
      fradd[27] <= 1'b1; 
    end	 
 end
 else if(fradd[27]) begin
   fradd[27] <= 1'b0;
   fradd[28] <= 1'b1;
 end 
 else if(fradd[28]) begin
   fradd[28] <= 1'b0;
   fadd[54]  <= 1'b1;
 end 
 else if(fadd[54]) begin //no MAS done
     fadd[54] <= 1'b0;
     fadd[55] <= 1'b1; 
 end
 else if(fadd[55]) begin
    done <= 1'b1; 
    fadd[55] <= 1'b0;
    fadd[56] <= 1'b1;
 end
 else if(fadd[56]) begin
    done <= 1'b0; 
    fadd[56] <= 1'b0;
 end
 //END point addition
 
 //start point doubling flow
 else if(frdbl[0]) begin
   frdbl[0] <= 1'b0;
   fdbl[0]  <= 1'b1;
 end
 else if(fdbl[0]) begin
     mas_start <= 1'b1;
     mas_mode <= 2'b11; // t_4 = t_3^2
     fdbl[0] <= 1'b0;
     fdbl[1] <= 1'b1; 	 
 end
 else if(fdbl[1]) begin
	 mas_start <= 1'b0;
	 if(mas_done) begin
      fdbl[1] <= 1'b0;
      frdbl[1] <= 1'b1; 
    end	 
 end 
 else if(frdbl[1]) begin
   frdbl[1] <= 1'b0;
   fdbl[2]  <= 1'b1;
 end
 else if(fdbl[2]) begin
    mas_start <= 1'b1;
    mas_mode <= 2'b01; //t_5 = t_1 - t_4
    fdbl[2] <= 1'b0;
    fdbl[3] <= 1'b1; 	 
 end
 else if(fdbl[3]) begin
	 mas_start <= 1'b0;
	 if(mas_done) begin
      fdbl[3] <= 1'b0;
      frdbl[2] <= 1'b1; 
    end	 
 end 
 else if(frdbl[2]) begin
   frdbl[2] <= 1'b0;
   fdbl[4]  <= 1'b1;
 end 
 else if(fdbl[4]) begin
    mas_start <= 1'b1;
    mas_mode <= 2'b10; //t_4 = t_1 + t_4
    fdbl[4] <= 1'b0;
    fdbl[5] <= 1'b1; 	 
 end
 else if(fdbl[5]) begin
	 mas_start <= 1'b0;
	 if(mas_done) begin
      fdbl[5] <= 1'b0;
      frdbl[3] <= 1'b1; 
    end	 
 end 
 else if(frdbl[3]) begin
   frdbl[3] <= 1'b0;
   fdbl[6]  <= 1'b1;
 end 
 else if(fdbl[6]) begin
    mas_start <= 1'b1;
    mas_mode <= 2'b11; //t_4 = t_7(a) . t_4 //mas_mode <= 2'b11; //t_5 = t_5 . t_4
    fdbl[6] <= 1'b0;
    fdbl[7] <= 1'b1; 	 
 end
 else if(fdbl[7]) begin
	 mas_start <= 1'b0;
	 if(mas_done) begin
      fdbl[7] <= 1'b0;
      frdbl[4] <= 1'b1; 
    end	 
 end 
 else if(frdbl[4]) begin
   frdbl[4] <= 1'b0;
   fdbl[8]  <= 1'b1;
 end 
 else if(fdbl[8]) begin
    mas_start <= 1'b1;
    mas_mode <= 2'b10; //t_4 = t_4 + t_5 //mas_mode <= 2'b10; //t_4 = t_5 + t_5
    fdbl[8] <= 1'b0;
    fdbl[9] <= 1'b1; 	 
 end
 else if(fdbl[9]) begin
	 mas_start <= 1'b0;
	 if(mas_done) begin
      fdbl[9] <= 1'b0;
      frdbl[5] <= 1'b1; 
    end	 
 end 
 else if(frdbl[5]) begin
   frdbl[5] <= 1'b0;
   fdbl[10]  <= 1'b1;
 end 
 else if(fdbl[10]) begin
    mas_start <= 1'b1;
    mas_mode <= 2'b10; //t_5 = t_5 + t_5 //mas_mode <= 2'b10; //t_4 = t_4 + t_5
    fdbl[10] <= 1'b0;
    fdbl[11] <= 1'b1; 	 
 end
 else if(fdbl[11]) begin
	 mas_start <= 1'b0;
	 if(mas_done) begin
      fdbl[11] <= 1'b0;
      frdbl[6] <= 1'b1; 
    end	 
 end
 else if(frdbl[6]) begin
   frdbl[6] <= 1'b0;
   fdbl[12]  <= 1'b1;
 end 
 else if(fdbl[12]) begin
    mas_start <= 1'b1;
    mas_mode <= 2'b11; //t_3 = t_3 . t_2
    fdbl[12] <= 1'b0;
    fdbl[13] <= 1'b1; 	 
 end
 else if(fdbl[13]) begin
	 mas_start <= 1'b0;
	 if(mas_done) begin
      fdbl[13] <= 1'b0;
      frdbl[7] <= 1'b1; 
    end	 
 end 
 else if(frdbl[7]) begin
   frdbl[7] <= 1'b0;
   fdbl[14]  <= 1'b1;
 end 
 else if(fdbl[14]) begin
    mas_start <= 1'b1;
    mas_mode <= 2'b10; //t_3 = t_3 + t_3
    fdbl[14] <= 1'b0;
    fdbl[15] <= 1'b1; 	 
 end
 else if(fdbl[15]) begin
	 mas_start <= 1'b0;
	 if(mas_done) begin
      fdbl[15] <= 1'b0;
      frdbl[8] <= 1'b1; 
    end	 
 end 
 else if(frdbl[8]) begin
   frdbl[8] <= 1'b0;
   fdbl[16]  <= 1'b1;
 end 
 else if(fdbl[16]) begin
    mas_start <= 1'b1;
    mas_mode <= 2'b11; //t_2 = t_2 . t_2
    fdbl[16] <= 1'b0;
    fdbl[17] <= 1'b1; 	 
 end
 else if(fdbl[17]) begin
	 mas_start <= 1'b0;
	 if(mas_done) begin
      fdbl[17] <= 1'b0;
      frdbl[9] <= 1'b1; 
    end	 
 end 
 else if(frdbl[9]) begin
   frdbl[9] <= 1'b0;
   fdbl[18]  <= 1'b1;
 end 
 else if(fdbl[18]) begin
    mas_start <= 1'b1;
    mas_mode <= 2'b11; //t_5 = t_1 . t_2
    fdbl[18] <= 1'b0;
    fdbl[19] <= 1'b1; 	 
 end
 else if(fdbl[19]) begin
	 mas_start <= 1'b0;
	 if(mas_done) begin
      fdbl[19] <= 1'b0;
      frdbl[10] <= 1'b1; 
    end	 
 end 
 else if(frdbl[10]) begin
   frdbl[10] <= 1'b0;
   fdbl[20]  <= 1'b1;
 end 
 else if(fdbl[20]) begin
    mas_start <= 1'b1;
    mas_mode <= 2'b10; //t_5 = t_5 + t_5
    fdbl[20] <= 1'b0;
    fdbl[21] <= 1'b1; 	 
 end
 else if(fdbl[21]) begin
	 mas_start <= 1'b0;
	 if(mas_done) begin
      fdbl[21] <= 1'b0;
      frdbl[11] <= 1'b1; 
    end	 
 end 
 else if(frdbl[11]) begin
   frdbl[11] <= 1'b0;
   fdbl[22]  <= 1'b1;
 end  
 else if(fdbl[22]) begin
    mas_start <= 1'b1;
    mas_mode <= 2'b10; //t_5 = t_5 + t_5
    fdbl[22] <= 1'b0;
    fdbl[23] <= 1'b1; 	 
 end
 else if(fdbl[23]) begin
	 mas_start <= 1'b0;
	 if(mas_done) begin
      fdbl[23] <= 1'b0;
      frdbl[12] <= 1'b1; 
    end	 
 end 
 else if(frdbl[12]) begin
   frdbl[12] <= 1'b0;
   fdbl[24]  <= 1'b1;
 end  
 else if(fdbl[24]) begin
    mas_start <= 1'b1;
    mas_mode <= 2'b11; //t_1 = t_4 . t_4
    fdbl[24] <= 1'b0;
    fdbl[25] <= 1'b1; 	 
 end
 else if(fdbl[25]) begin
	 mas_start <= 1'b0;
	 if(mas_done) begin
      fdbl[25] <= 1'b0;
      frdbl[13] <= 1'b1; 
    end	 
 end 
 else if(frdbl[13]) begin
   frdbl[13] <= 1'b0;
   fdbl[26]  <= 1'b1;
 end  
 else if(fdbl[26]) begin
    mas_start <= 1'b1;
    mas_mode <= 2'b01; //t_1 = t_1 - t_5
    fdbl[26] <= 1'b0;
    fdbl[27] <= 1'b1; 	 
 end
 else if(fdbl[27]) begin
	 mas_start <= 1'b0;
	 if(mas_done) begin
      fdbl[27] <= 1'b0;
      frdbl[14] <= 1'b1; 
    end	 
 end 
 else if(frdbl[14]) begin
   frdbl[14] <= 1'b0;
   fdbl[28]  <= 1'b1;
 end  
 else if(fdbl[28]) begin
    mas_start <= 1'b1;
    mas_mode <= 2'b01; //t_1 = t_1 - t_5
    fdbl[28] <= 1'b0;
    fdbl[29] <= 1'b1; 	 
 end
 else if(fdbl[29]) begin
	 mas_start <= 1'b0;
	 if(mas_done) begin
	  fdbl[29] <= 1'b0;
      frdbl[15] <= 1'b1; 
    end	 
 end 
 else if(frdbl[15]) begin
   frdbl[15] <= 1'b0;
   fdbl[30]  <= 1'b1;
 end  
 else if(fdbl[30]) begin
    mas_start <= 1'b1;
    mas_mode <= 2'b11; //t_2 = t_2 . t_2
    fdbl[30] <= 1'b0;
    fdbl[31] <= 1'b1; 	 
 end
 else if(fdbl[31]) begin
	 mas_start <= 1'b0;
	 if(mas_done) begin
	  fdbl[31] <= 1'b0;
      frdbl[16] <= 1'b1; 
    end	 
 end 
 else if(frdbl[16]) begin
   frdbl[16] <= 1'b0;
   fdbl[32]  <= 1'b1;
 end  
 else if(fdbl[32]) begin
    mas_start <= 1'b1;
    mas_mode <= 2'b10; //t_2 = t_2 + t_2
    fdbl[32] <= 1'b0;
    fdbl[33] <= 1'b1; 	 
 end
 else if(fdbl[33]) begin
	 mas_start <= 1'b0;
	 if(mas_done) begin
      fdbl[33] <= 1'b0;
      frdbl[17] <= 1'b1; 
    end	 
 end 
 else if(frdbl[17]) begin
   frdbl[17] <= 1'b0;
   fdbl[34]  <= 1'b1;
 end  
 else if(fdbl[34]) begin
    mas_start <= 1'b1;
    mas_mode <= 2'b10; //t_2 = t_2 + t_2
    fdbl[34] <= 1'b0;
    fdbl[35] <= 1'b1; 	 
 end
 else if(fdbl[35]) begin
	 mas_start <= 1'b0;
	 if(mas_done) begin
      fdbl[35] <= 1'b0;
      frdbl[18] <= 1'b1; 
    end	 
 end 
 else if(frdbl[18]) begin
   frdbl[18] <= 1'b0;
   fdbl[36]  <= 1'b1;
 end  
 else if(fdbl[36]) begin
    mas_start <= 1'b1;
    mas_mode <= 2'b10; //t_2 = t_2 + t_2
    fdbl[36] <= 1'b0;
    fdbl[37] <= 1'b1; 	 
 end
 else if(fdbl[37]) begin
	 mas_start <= 1'b0;
	 if(mas_done) begin
      fdbl[37] <= 1'b0;
      frdbl[19] <= 1'b1; 
    end	 
 end 
 else if(frdbl[19]) begin
   frdbl[19] <= 1'b0;
   fdbl[38]  <= 1'b1;
 end  
 else if(fdbl[38]) begin
    mas_start <= 1'b1;
    mas_mode <= 2'b01; //t_5 = t_5 - t_1
    fdbl[38] <= 1'b0;
    fdbl[39] <= 1'b1; 	 
 end
 else if(fdbl[39]) begin
	 mas_start <= 1'b0;
	 if(mas_done) begin
      fdbl[39] <= 1'b0;
      frdbl[20] <= 1'b1; 
    end	 
 end 
 else if(frdbl[20]) begin
   frdbl[20] <= 1'b0;
   fdbl[40]  <= 1'b1;
 end  
 else if(fdbl[40]) begin
    mas_start <= 1'b1;
    mas_mode <= 2'b11; //t_5 = t_5 . t_4
    fdbl[40] <= 1'b0;
    fdbl[41] <= 1'b1; 	 
 end
 else if(fdbl[41]) begin
	 mas_start <= 1'b0;
	 if(mas_done) begin
      fdbl[41] <= 1'b0;
      frdbl[21] <= 1'b1; 
    end	 
 end 
 else if(frdbl[21]) begin
   frdbl[21] <= 1'b0;
   fdbl[42]  <= 1'b1;
 end  
 else if(fdbl[42]) begin
    mas_start <= 1'b1;
    mas_mode <= 2'b01; //t_2 = t_5 - t_2
    fdbl[42] <= 1'b0;
    fdbl[43] <= 1'b1; 	 
 end
 else if(fdbl[43]) begin
	 mas_start <= 1'b0;
	 if(mas_done) begin
      fdbl[43] <= 1'b0;
      fdbl[44] <= 1'b1; 
    end	 
 end  
 else if(fdbl[44]) begin
    mas_mode <= 2'b00;
    done <= 1'b1;	 
    fdbl[44] <= 1'b0;
    fdbl[45] <= 1'b1; 	 
 end
 else if(fdbl[45]) begin
  done <= 1'b0;
  fdbl[45] <= 1'b0; 
 end
end

endmodule
