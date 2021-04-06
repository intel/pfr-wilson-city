`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
/*!
    \brief      <b>IIC Continuous write, sequential read Registers Module</b>\n
    \details    Read only registers mux control\n
    \file       ContWriteSeqReadRegs.v
    \date       Aug 25, 2012
    \brief      $RCSfile: ContWriteSeqReadRegs.v.rca $
                $Date: $
                $Author:  $
                $Revision:  $
                $Aliases: $
                $Log: ContWriteSeqReadRegs.v.rca $
                
    \copyright Intel Proprietary -- Copyright 2015 Intel -- All rights reserved
    
*/
//////////////////////////////////////////////////////////////////////////////////
module ContWriteSeqReadRegs
(   
    //% Module's clock input
    input           iClk,
    //% Reset input
    input           iRst,
    //% Enable input
    input           iEnable,
    //% Register address input
    input   [5:0]   ivAddress,
    //% Read Only Registers values    
    input   [7:0]   ivReg00,
    input   [7:0]   ivReg01,
    input   [7:0]   ivReg02,
    input   [7:0]   ivReg03,
    input   [7:0]   ivReg04,
    input   [7:0]   ivReg05,
    input   [7:0]   ivReg06,
    input   [7:0]   ivReg07,
    input   [7:0]   ivReg08,
    input   [7:0]   ivReg09,
    input   [7:0]   ivReg0A,
    input   [7:0]   ivReg0B,
    input   [7:0]   ivReg0C,
    input   [7:0]   ivReg0D,
    input   [7:0]   ivReg0E,
    input   [7:0]   ivReg0F,
    input   [7:0]   ivReg10,
    input   [7:0]   ivReg11,
    input   [7:0]   ivReg12,
    input   [7:0]   ivReg13,
    input   [7:0]   ivReg14,
    input   [7:0]   ivReg15,
    input   [7:0]   ivReg16,
    input   [7:0]   ivReg17,
    input   [7:0]   ivReg18,
    input   [7:0]   ivReg19,
    input   [7:0]   ivReg1A,
    input   [7:0]   ivReg1B,
    input   [7:0]   ivReg1C,
    input   [7:0]   ivReg1D,
    input   [7:0]   ivReg1E,
    input   [7:0]   ivReg1F,
    input   [7:0]   ivReg20,
    input   [7:0]   ivReg21,
    input   [7:0]   ivReg22,
    input   [7:0]   ivReg23,
    input   [7:0]   ivReg24,
    input   [7:0]   ivReg25,
    input   [7:0]   ivReg26,
    input   [7:0]   ivReg27,
    input   [7:0]   ivReg28,
    input   [7:0]   ivReg29,
    input   [7:0]   ivReg2A,
    input   [7:0]   ivReg2B,
    input   [7:0]   ivReg2C,
    input   [7:0]   ivReg2D,
    input   [7:0]   ivReg2E,
    input   [7:0]   ivReg2F,
    //% Block data output
    output  [7:0]   ovQ,
    //% Access completed output
    output          oAccessDone
);
//////////////////////////////////////////////////////////////////////////////////
// Includes
//////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////
// Defines
//////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////
// Internal Signals
//////////////////////////////////////////////////////////////////////////////////
//!
reg [7:0]   rvData_d;
reg [7:0]   rvData_q;
//!
reg [1:0]   rvDone_d;
reg [1:0]   rvDone_q;
//////////////////////////////////////////////////////////////////////////////////
// Continous assigments
//////////////////////////////////////////////////////////////////////////////////
assign  ovQ = rvData_q;
assign  oAccessDone = rvDone_q[1];
//////////////////////////////////////////////////////////////////////////////////
// Sequential logic
//////////////////////////////////////////////////////////////////////////////////
always @(posedge iClk or posedge iRst)
begin
    if(iRst)
    begin
        rvData_q        <=  8'b0;
        rvDone_q        <=  2'b0;
    end
    else
    begin
        rvData_q    <=  rvData_d;
        rvDone_q    <=  rvDone_d;
    end
end
//////////////////////////////////////////////////////////////////////////////////
// Combinational logic
//////////////////////////////////////////////////////////////////////////////////
always @*
begin
    rvData_d        =   rvData_q;
    rvDone_d        =   {rvDone_q[0],iEnable};

    case (ivAddress)
        6'h01:      rvData_d    =   ivReg01;
        6'h02:      rvData_d    =   ivReg02;
        6'h03:      rvData_d    =   ivReg03;
        6'h04:      rvData_d    =   ivReg04;
        6'h05:      rvData_d    =   ivReg05;
        6'h06:      rvData_d    =   ivReg06;
        6'h07:      rvData_d    =   ivReg07;
        6'h08:      rvData_d    =   ivReg08;
        6'h09:      rvData_d    =   ivReg09;
        6'h0A:      rvData_d    =   ivReg0A;
        6'h0B:      rvData_d    =   ivReg0B;
        6'h0C:      rvData_d    =   ivReg0C;
        6'h0D:      rvData_d    =   ivReg0D;
        6'h0E:      rvData_d    =   ivReg0E;
        6'h0F:      rvData_d    =   ivReg0F;
        6'h10:      rvData_d    =   ivReg10;
        6'h11:      rvData_d    =   ivReg11;
        6'h12:      rvData_d    =   ivReg12;
        6'h13:      rvData_d    =   ivReg13;
        6'h14:      rvData_d    =   ivReg14;
        6'h15:      rvData_d    =   ivReg15;
        6'h16:      rvData_d    =   ivReg16;
        6'h17:      rvData_d    =   ivReg17;
        6'h18:      rvData_d    =   ivReg18;
        6'h19:      rvData_d    =   ivReg19;
        6'h1A:      rvData_d    =   ivReg1A;
        6'h1B:      rvData_d    =   ivReg1B;
        6'h1C:      rvData_d    =   ivReg1C;
        6'h1D:      rvData_d    =   ivReg1D;
        6'h1E:      rvData_d    =   ivReg1E;
        6'h1F:      rvData_d    =   ivReg1F;
        6'h20:      rvData_d    =   ivReg20;
        6'h21:      rvData_d    =   ivReg21;
        6'h22:      rvData_d    =   ivReg22;
        6'h23:      rvData_d    =   ivReg23;
        6'h24:      rvData_d    =   ivReg24;
        6'h25:      rvData_d    =   ivReg25;
        6'h26:      rvData_d    =   ivReg26;
        6'h27:      rvData_d    =   ivReg27;
        6'h28:      rvData_d    =   ivReg28;
        6'h29:      rvData_d    =   ivReg29;
        6'h2A:      rvData_d    =   ivReg2A;
        6'h2B:      rvData_d    =   ivReg2B;
        6'h2C:      rvData_d    =   ivReg2C;
        6'h2D:      rvData_d    =   ivReg2D;
        6'h2E:      rvData_d    =   ivReg2E;
        6'h2F:      rvData_d    =   ivReg2F;
        default:    rvData_d    =   ivReg00;
    endcase
end
//////////////////////////////////////////////////////////////////////////////////
// Instances
//////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////



endmodule
