//*****************************************************************************
//* INTEL CONFIDENTIAL
//*
//* Copyright(c) Intel Corporation (2015)
//****************************************************************************
//
// DATE:      12/5/2015
// ENGINEER:  Liu, Bruce Z 
// EMAIL:     bruce.z.liu@intel.com           
// FILE:      reset.v
// BLOCK:     
// REVISION:  
//	0.1   initial version
//
// DESCRIPTION:	time delay module. 
// This module generate an internal reset signal for CPLD, and CPLD use it for
// global reset.
//   
// ASSUMPTIONS:
//   
//**************************************************************************** 



 module reset (
 input pll_locked,
 input iClk_2M,
 output oRst_n
 );

   reg [7:0] rRstCntr=8'b00000000;  //this module is to generate a reliable reset
    reg rIntRst_n;
    always @( posedge iClk_2M or negedge pll_locked) 
    begin
        if (!pll_locked) 
        begin
            rRstCntr <= 8'b0;
            rIntRst_n <= 1'b0;
        end
        else begin
            if (rRstCntr == 8'b11110111) //123 us of reset
            begin
                rIntRst_n <= 1'b1;
    	        rRstCntr <= rRstCntr;
            end

            else begin
                rRstCntr <= rRstCntr + 1'b1;
                rIntRst_n <= 1'b0;
            end
        end
    end

    assign oRst_n = rIntRst_n;

 endmodule
 