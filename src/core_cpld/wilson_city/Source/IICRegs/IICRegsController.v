`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
/*!
    \brief      <b>IIC Regs controller module</b>\n
    \details    \n
    \file       IICRegsController.v
    \date       Aug 25, 2012
    \brief      $RCSfile: IICRegsController.v.rca $
                $Date: $
                $Author:  $
                $Revision:  $
                $Aliases: $
                $Log: IICRegsController.v.rca $
                <b>Instances:</b>   <ol>
                                        <li>IICSlave
                                    </ol>            
                
                
    \copyright Intel Proprietary -- Copyright 2015 Intel -- All rights reserved
    
*/
//////////////////////////////////////////////////////////////////////////////////
module IICRegsController #(parameter MODULE_ADDRESS = 7'h27)
(
    //% Clock input
    input           iClk,
    //% Asynchronous reset
    input           iRst,
    //% Serial Clock
    input           iSCL,
    //% Serial Data
    input           iSDA,
    //% Register block 0 data
    input   [7:0]   ivData0,
    //% Register block 1 data    
    input   [7:0]   ivData1,
    //% Register block 2 data    
    input   [7:0]   ivData2,
    //% Register's access done
    input   [2:0]   ivRegAccessDone,
    //% Serial Data tristate output control
    output          onSDAOE,
    //% Register ID (Address)
    output  [12:0]  ovRegID,
    //% Received data
    output  [7:0]   ovData,
    //% Reception ok
    output          oRxOk,
    //% Transmition Ok
    output          oTxOk,
    //% Read/Write
    output          oRnW,
    //% Register block select
    output  [2:0]   ovRegBlockEnable
);


//////////////////////////////////////////////////////////////////////////////////
// Includes
//////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////
// Defines
//////////////////////////////////////////////////////////////////////////////////
localparam    REG_CNTROLLER_IDLE_STATE                =   3'd0;
localparam    REG_CNTROLLER_WAIT_ADDRESS_MATCH_STATE  =   3'd1;   
localparam    REG_CNTROLLER_ADDRESS_STATE0            =   3'd2;
localparam    REG_CNTROLLER_ADDRESS_STATE1            =   3'd3;
localparam    REG_CNTROLLER_TX_DATA_STATE             =   3'd4;
localparam    REG_CNTROLLER_RX_DATA_STATE             =   3'd5;
localparam    REG_CNTROLLER_WAIT_ACCESS_STATE         =   3'd6;
//////////////////////////////////////////////////////////////////////////////////
// Internal Signals
//////////////////////////////////////////////////////////////////////////////////
//!
wire            wnSDAOE;
//!
wire            wAddressMatch;
//!
wire    [7:0]   wvRxData;
//!
wire            wRxOk;
//!
wire            wTxOk;
//!
//!
reg     [12:0]  rvRegID_d;
reg     [12:0]  rvRegID_q;
//!
reg             rRxOk_d;
reg             rRxOk_q;
//!
reg             rTxOk_d;
reg             rTxOk_q;
//!
reg             rValidRegID_d;
reg             rValidRegID_q;
//!
reg     [2:0]   rvState_d;
reg     [2:0]   rvState_q;
//!
//reg             rAccess_d;
//reg             rAccess_q;
`undef USE_DEBUG
`ifdef USE_DEBUG
    (* KEEP = "TRUE" *) reg rCapture_d;
    (* KEEP = "TRUE" *) reg rCapture_q;
`endif
//!
wire    wRnW;
//!
wire    wStopOk;
//!
wire    wStartOk;
//!
reg [2:0]   rvRegBlockEnable_d;
reg [2:0]   rvRegBlockEnable_q;
//!
/*
reg    [1:0]    rvRegCheck_d;
reg    [1:0]    rvRegCheck_q;
*/
//!
reg         rAccessDone_d;
reg         rAccessDone_q;
//!
reg [7:0]   rvTxData_d;
reg [7:0]   rvTxData_q;
//!
reg [7:0]   rvRxData_d;
reg [7:0]   rvRxData_q;
//////////////////////////////////////////////////////////////////////////////////
// Continous assigments
//////////////////////////////////////////////////////////////////////////////////
assign  onSDAOE =   wnSDAOE;
assign  ovData  =   rvRxData_q;
assign  ovRegID =   rvRegID_q[12:0];
assign  oTxOk   =   wTxOk;
assign  oRxOk   =   wRxOk;
assign  oRnW    =   wRnW;
assign  ovRegBlockEnable = rvRegBlockEnable_q;
//////////////////////////////////////////////////////////////////////////////////
// Sequential logic
//////////////////////////////////////////////////////////////////////////////////
always @(posedge iClk or posedge iRst)
begin
    if(iRst)
    begin
        rvRegID_q           <=  13'b0;
        rRxOk_q             <=  1'b0;
        rTxOk_q             <=  1'b0;
        rvState_q           <=  REG_CNTROLLER_IDLE_STATE;
        `ifdef USE_DEBUG
            rCapture_q      <=  1'b0;
        `endif
        rValidRegID_q       <=  1'b0;
        rvRegBlockEnable_q  <=  3'b0;
        //rAccess_q           <=  1'b0;
        rAccessDone_q       <=  1'b0;
        rvTxData_q          <=  8'b0;
        rvRxData_q          <=  8'b0;
    end
    else
    begin
        rvRegID_q           <=  rvRegID_d;
        rRxOk_q             <=  rRxOk_d;
        rTxOk_q             <=  rTxOk_d;
        rvState_q           <=  rvState_d;
        `ifdef USE_DEBUG
            rCapture_q      <=  rCapture_d;
        `endif
        rValidRegID_q       <=  rValidRegID_d;
        rvRegBlockEnable_q  <=  rvRegBlockEnable_d;
        //rAccess_q           <=  rAccess_d;
        rAccessDone_q       <=  rAccessDone_d;
        rvTxData_q          <=  rvTxData_d;
        rvRxData_q          <=  rvRxData_d;
    end    
end
//////////////////////////////////////////////////////////////////////////////////
// Combinational logic
//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
// Combinational logic
//////////////////////////////////////////////////////////////////////////////////
always @*
begin
    rvState_d           =   rvState_q;
    
    rValidRegID_d       =   (   (rvState_q     == REG_CNTROLLER_ADDRESS_STATE0)       
                             || (rvState_q     == REG_CNTROLLER_ADDRESS_STATE1)
                             || ( (   (rvState_q    == REG_CNTROLLER_RX_DATA_STATE)      
                                    || (rvState_q    ==  REG_CNTROLLER_WAIT_ACCESS_STATE) 
                                   )   
                                  &&                                     
                                  (   (rvRegID_q <= 13'h2F)  
                                    //|| (rvRegID_q >= 13'h10 && rvRegID_q <= 13'h1F)
									|| (rvRegID_q >= 13'h30 && rvRegID_q <= 13'h33)
                                    || (rvRegID_q >= 13'h1000 && rvRegID_q <= 13'h1FFF)
                                   )
                                 )
                             );
                             
    rRxOk_d             =   wRxOk;
    rTxOk_d             =   wTxOk;
    rvRegID_d           =   rvRegID_q;
    rvRegBlockEnable_d  =   rvRegBlockEnable_q;
    //rAccess_d           =   rAccess_q;
    rvTxData_d          =   rvTxData_q;
    rvRxData_d          =   rvRxData_q;
    case(rvState_q)
        REG_CNTROLLER_ADDRESS_STATE0:
        begin
            if({rRxOk_q,wRxOk} == 2'b01)
            begin
                rvState_d                =    REG_CNTROLLER_IDLE_STATE;
                //Capture Address
                if(wvRxData[7:6] == 2'b0)
                begin
                    rvRegID_d[12:8] =   wvRxData[4:0];   
                    rvState_d       =   REG_CNTROLLER_ADDRESS_STATE1;
                end
            end
        end
        REG_CNTROLLER_ADDRESS_STATE1:
        begin
            if({rRxOk_q,wRxOk} == 2'b01)
            begin
                //Capture Address
                rvRegID_d[7:0]      =   wvRxData[7:0];
                rvState_d           =   REG_CNTROLLER_RX_DATA_STATE;
            end
        end
        REG_CNTROLLER_TX_DATA_STATE:
        begin
            if({rTxOk_q,wTxOk} == 2'b01)
            begin
                //Set data
                //rAccess_d   =   1'b1;    
                rvRegID_d   =   rvRegID_q + 1'b1;                
            end
        end
        REG_CNTROLLER_RX_DATA_STATE:
        begin
            if({rRxOk_q,wRxOk} == 2'b01)
            begin
                //Set data
                rvState_d       =   REG_CNTROLLER_WAIT_ACCESS_STATE; 
                //rAccess_d       =   1'b1;
                rvRxData_d      =   wvRxData;
            end
        end
        REG_CNTROLLER_WAIT_ACCESS_STATE:
        begin
            if( rAccessDone_q )
            begin
                rvState_d   =   REG_CNTROLLER_RX_DATA_STATE;
                //rAccess_d   =   1'b0;
                rvRegID_d   =   rvRegID_q + 1'b1;
            end
            
        end
        REG_CNTROLLER_WAIT_ADDRESS_MATCH_STATE:
        begin
            if( wAddressMatch )
            begin
                rvState_d           =   REG_CNTROLLER_ADDRESS_STATE0;
                if(wRnW)
                begin
                    rvState_d        =    REG_CNTROLLER_TX_DATA_STATE;
                end
            end
        end
        default:
        begin
            rvState_d           =   REG_CNTROLLER_IDLE_STATE;
            rvRegBlockEnable_d  =   3'b0; 
        end
    endcase
    if(( wStopOk )
        ||( wStartOk ))
    begin
        rvState_d           =   REG_CNTROLLER_WAIT_ADDRESS_MATCH_STATE;
    end
    //Valid reg?
    rvRegBlockEnable_d  =   3'b0;
    rAccessDone_d = 1'b0;

    
    if(( wAddressMatch )
        && ((rvState_q == REG_CNTROLLER_RX_DATA_STATE)
            ||(rvState_q == REG_CNTROLLER_TX_DATA_STATE)
                ||(rvState_q == REG_CNTROLLER_WAIT_ACCESS_STATE)))
    begin
        if (rvRegID_q <=  13'h2F)                    //0x000 - 0x00F - RO registers
        begin
          rvRegBlockEnable_d    =   3'b1;
          rAccessDone_d         =   ivRegAccessDone[0];
          rvTxData_d            =   ivData0;
        end
		if( rvRegID_q >= 13'h30 && rvRegID_q <= 13'h33)    //0x0030 - 0x0033 - RW LUTs registers
        begin
            rvRegBlockEnable_d  =   3'b10;
            rAccessDone_d       =   ivRegAccessDone[1]; 
            rvTxData_d          =   ivData1;
        end        
        if( rvRegID_q >= 13'h1000 && rvRegID_q <= 13'h1FFF)    //0x1000 - 0x1FFF - Events
        begin
          rvRegBlockEnable_d    =   3'b100;
          rAccessDone_d         =   ivRegAccessDone[2];
          rvTxData_d            =   ivData2;
        end
        if(({rTxOk_q,wTxOk} != 2'b01)
            &&({rRxOk_q,wRxOk} != 2'b01)) 
        begin
            rvRegBlockEnable_d  =   3'b0;        
        end
    end
end


//////////////////////////////////////////////////////////////////////////////////
// Instances
//////////////////////////////////////////////////////////////////////////////////
IICSlave #(MODULE_ADDRESS) mIICSlave 
(
    .iClk           (iClk), 
    .iRst           (iRst), 
    .iSDA           (iSDA), 
    .iSCL           (iSCL), 
    .ivTxData       (rvTxData_q), 
    .iTxAck         (rValidRegID_q),
    .onSDAOE        (wnSDAOE), 
    .oAddressMatch  (wAddressMatch), 
    .oStartOk       (wStartOk),
    .oStopOk        (wStopOk),
    .oRnW           (wRnW), 
    .ovRxData       (wvRxData), 
    .oRxAck         (),
    .oTxOk          (wTxOk), 
    .oRxOk          (wRxOk)
);
//////////////////////////////////////////////////////////////////////////////////

endmodule
