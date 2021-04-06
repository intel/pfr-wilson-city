`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
/*!
    \brief      <b>IIC Continuous read/ sequential write Registers Block</b>\n
    \details    LUTs/FF implemented registers block with Read/Write capability\n
    \file       ContReadSeqWriteRegs.v
    \date       Aug 25, 2012
    \brief      $RCSfile: ContReadSeqWriteRegs.v.rca $
                $Date: $
                $Author:  $
                $Revision:  $
                $Aliases: $
                $Log: ContReadSeqWriteRegs.v.rca $
                
    \copyright Intel Proprietary -- Copyright 2015 Intel -- All rights reserved
    
*/
//////////////////////////////////////////////////////////////////////////////////

module ContReadSeqWriteRegs
(   
    //% Module's clock input
    input           iClk,
    //% Reset input
    input           iRst,
    //% Read/Write control input
    input           iRnW,
    //% Enable input
    input           iEnable,
    //% Register address input
    //input   [3:0]   ivAddress,
    input   [5:0]   ivAddress,
    //% Read Write registers output values    
    output  [7:0]   ovReg30,

    output  [7:0]   ovQ,
    input   [7:0]   ivD,
    output          oAccessDone
);
//////////////////////////////////////////////////////////////////////////////////
// Includes
//////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////
// Defines
//////////////////////////////////////////////////////////////////////////////////
    `define DEFAULT_REG030  8'b0000_0000
//////////////////////////////////////////////////////////////////////////////////
// Internal Signals
//////////////////////////////////////////////////////////////////////////////////
//%
reg [7:0]   rvData_d;
reg [7:0]   rvData_q;
//%
//% \brief Internal Registers declaration
//%
reg [7:0]   rvReg30_d;
reg [7:0]   rvReg30_q;

reg [7:0]   rvRegDef_d;
reg [7:0]   rvRegDef_q;
//%
reg [1:0]   rvDone_d;
reg [1:0]   rvDone_q;
//%
reg         rEnable_d;
reg         rEnable_q;
//////////////////////////////////////////////////////////////////////////////////
// Continous assigments
//////////////////////////////////////////////////////////////////////////////////
assign  ovQ = rvData_q;

assign ovReg30  =   rvReg30_q;
assign  oAccessDone = rvDone_q[1];

//////////////////////////////////////////////////////////////////////////////////
// Sequential logic
//////////////////////////////////////////////////////////////////////////////////
always @(posedge iClk or posedge iRst)
begin
    if(iRst)
    begin
        rvData_q    <=  8'b0;
        rvDone_q    <=  2'b0;        
        rvReg30_q   <=  `DEFAULT_REG030; 
        rvRegDef_q  <=  8'b0;; 

        rEnable_q   <=  1'b0;
    end
    else
    begin
        rvDone_q    <=  rvDone_d;        
        rvReg30_q   <=  rvReg30_d;
        rvRegDef_q  <=  rvRegDef_d;

        rEnable_q   <=  rEnable_d;        
        rvData_q    <=  rvData_d;
    end
end
//////////////////////////////////////////////////////////////////////////////////
// Combinational logic
//////////////////////////////////////////////////////////////////////////////////
always @*
begin

    rvData_d    =   rvData_q;
    //Keep the same values...    
    rvReg30_d   =   rvReg30_q; 
    rvRegDef_d  =   rvRegDef_q;

    rEnable_d   =   iEnable;
    
    rvDone_d     = {rvDone_q[0],iEnable};
    if(iRnW)
    begin 
        case (ivAddress)
            6'h30:      rvData_d    =    rvReg30_q;
            default:    rvData_d    =    rvRegDef_q;
        endcase
    end    
    if({iRnW,rEnable_q,iEnable} == 3'b001)
    begin
        case (ivAddress)
            6'h30:      rvReg30_d   = ivD;
            default:    rvRegDef_d  = ivD;
        endcase        
    end
end
//////////////////////////////////////////////////////////////////////////////////
// Instances
//////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////


endmodule
