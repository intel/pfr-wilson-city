//////////////////////////////////////////////////////////////////////////////////
/*!
    \brief      <b>IIC Registers Module</b>\n
    \details    \n
    \file       IICRegs.v
    \date       Aug 25, 2012
    \brief      $RCSfile: IICRegs.v.rca $
                $Date: $
                $Author:  $
                $Revision:  $
                $Aliases: $
                $Log: IICRegs.v.rca $
                
    \copyright Intel Proprietary -- Copyright 2015 Intel -- All rights reserved
    
*/
//////////////////////////////////////////////////////////////////////////////////
`timescale 1ns / 1ps

module IICRegs#(parameter MODULE_ADDRESS = 7'h50)
(
    //% Clock input
    input           iClk,
    //% Asynchronous reset
    input           iRst,
    //% Serial Data Input
    input           iSDA,
    //% Serial clock
    input           iSCL,
    //% Tri-State output control
    output          onSDAOE,
    //% Read only registers
    input   [7:0]   ivSMBusReg00,
    input   [7:0]   ivSMBusReg01,
    input   [7:0]   ivSMBusReg02,
    input   [7:0]   ivSMBusReg03,
    input   [7:0]   ivSMBusReg04,
    input   [7:0]   ivSMBusReg05,
    input   [7:0]   ivSMBusReg06,
    input   [7:0]   ivSMBusReg07,
    input   [7:0]   ivSMBusReg08,
    input   [7:0]   ivSMBusReg09,
    input   [7:0]   ivSMBusReg0A,
    input   [7:0]   ivSMBusReg0B,
    input   [7:0]   ivSMBusReg0C,
    input   [7:0]   ivSMBusReg0D,
    input   [7:0]   ivSMBusReg0E,
    input   [7:0]   ivSMBusReg0F,

    input   [7:0]   ivSMBusReg10,
    input   [7:0]   ivSMBusReg11,
    input   [7:0]   ivSMBusReg12,
    input   [7:0]   ivSMBusReg13,
    input   [7:0]   ivSMBusReg14,
    input   [7:0]   ivSMBusReg15,
    input   [7:0]   ivSMBusReg16,
    input   [7:0]   ivSMBusReg17,
    input   [7:0]   ivSMBusReg18,
    input   [7:0]   ivSMBusReg19,
    input   [7:0]   ivSMBusReg1A,
    input   [7:0]   ivSMBusReg1B,
    input   [7:0]   ivSMBusReg1C,
    input   [7:0]   ivSMBusReg1D,
    input   [7:0]   ivSMBusReg1E,
    input   [7:0]   ivSMBusReg1F,

    input   [7:0]   ivSMBusReg20,
    input   [7:0]   ivSMBusReg21,
    input   [7:0]   ivSMBusReg22,
    input   [7:0]   ivSMBusReg23,
    input   [7:0]   ivSMBusReg24,
    input   [7:0]   ivSMBusReg25,
    input   [7:0]   ivSMBusReg26,
    input   [7:0]   ivSMBusReg27,
    input   [7:0]   ivSMBusReg28,
    input   [7:0]   ivSMBusReg29,
    input   [7:0]   ivSMBusReg2A,
    input   [7:0]   ivSMBusReg2B,
    input   [7:0]   ivSMBusReg2C,
    input   [7:0]   ivSMBusReg2D,
    input   [7:0]   ivSMBusReg2E,
    input   [7:0]   ivSMBusReg2F,
    
    //% Continous read/Sequential write regs            
    output  [7:0]   ovSMBusReg30,
	
    //% BRAM regs Address
    input   [9:0]   ivEventsAddress,
    //% BRAM Regs Write enable
    input           iEventsWE,
    //% BRAM Regs data
    input   [31:0]  ivEventsInData,
    //% BRAM Reset
    input           iEvRst
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
//! SMBus Interface - 
//! SMBus Interface: SDA tri-state control
wire    wnSDAOE;
//!
wire    wIICRnW;
wire    [7:0]   wvIICRxData;
wire    wIICRxOk; 
wire    wIICTxOk;

//!
wire    [2:0]   wvIICRegsEnable;
wire    [2:0]   wvIICRegsAccessDone;
wire    [12:0]  wvIICRegAddress;

//!
wire    [7:0]   wvIICRORegs;
wire    [7:0]   wvIICRWLutsRegs;
wire    [7:0]   wvIICRWBRAMEventsRegs;

//!
wire    [7:0]   wvReg30;
          
                
//////////////////////////////////////////////////////////////////////////////////
// Continous assigments
//////////////////////////////////////////////////////////////////////////////////
assign  onSDAOE         =   wnSDAOE;
assign  ovSMBusReg30    =   wvReg30;


//////////////////////////////////////////////////////////////////////////////////
// Sequential logic
//////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////
// Combinational logic
//////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////
// Instances
//////////////////////////////////////////////////////////////////////////////////
//
//
//
IICRegsController #
(
    .MODULE_ADDRESS(MODULE_ADDRESS)
) mIICRegsController
(
    .iClk                   ( iClk ),
    .iRst                   ( iRst ),
    .iSCL                   ( iSCL ), 
    .iSDA                   ( iSDA ),
    .ivData0                ( wvIICRORegs), 
    .ivData1                ( wvIICRWLutsRegs), 
    .ivData2                ( wvIICRWBRAMEventsRegs),
    .ivRegAccessDone        ( wvIICRegsAccessDone), 
    .onSDAOE                ( wnSDAOE),
    .ovRegID                ( wvIICRegAddress), 
    .ovData                 ( wvIICRxData), 
    .oRxOk                  ( wIICRxOk), 
    .oTxOk                  ( wIICTxOk),
    .oRnW                   ( wIICRnW),
    .ovRegBlockEnable       ( wvIICRegsEnable)
    
);
//
//
//
ContWriteSeqReadRegs mReadOnlyRegs
(
    .iClk                   (iClk),
    .iRst                   (iRst),
    .iEnable                (wvIICRegsEnable[0]), 
    .ivAddress              (wvIICRegAddress[5:0]), 
    .ivReg00                (ivSMBusReg00),
    .ivReg01                (ivSMBusReg01),
    .ivReg02                (ivSMBusReg02),
    .ivReg03                (ivSMBusReg03),
    .ivReg04                (ivSMBusReg04),
    .ivReg05                (ivSMBusReg05),
    .ivReg06                (ivSMBusReg06),
    .ivReg07                (ivSMBusReg07),
    .ivReg08                (ivSMBusReg08),
    .ivReg09                (ivSMBusReg09),
    .ivReg0A                (ivSMBusReg0A),
    .ivReg0B                (ivSMBusReg0B),
    .ivReg0C                (ivSMBusReg0C),
    .ivReg0D                (ivSMBusReg0D),
    .ivReg0E                (ivSMBusReg0E),
    .ivReg0F                (ivSMBusReg0F),
    .ivReg10                (ivSMBusReg10),
    .ivReg11                (ivSMBusReg11),
    .ivReg12                (ivSMBusReg12),
    .ivReg13                (ivSMBusReg13),
    .ivReg14                (ivSMBusReg14),
    .ivReg15                (ivSMBusReg15),
    .ivReg16                (ivSMBusReg16),
    .ivReg17                (ivSMBusReg17),
    .ivReg18                (ivSMBusReg18),
    .ivReg19                (ivSMBusReg19),
    .ivReg1A                (ivSMBusReg1A),
    .ivReg1B                (ivSMBusReg1B),
    .ivReg1C                (ivSMBusReg1C),
    .ivReg1D                (ivSMBusReg1D),
    .ivReg1E                (ivSMBusReg1E),
    .ivReg1F                (ivSMBusReg1F),
    .ivReg20                (ivSMBusReg20),
    .ivReg21                (ivSMBusReg21),
    .ivReg22                (ivSMBusReg22),
    .ivReg23                (ivSMBusReg23),
    .ivReg24                (ivSMBusReg24),
    .ivReg25                (ivSMBusReg25),
    .ivReg26                (ivSMBusReg26),
    .ivReg27                (ivSMBusReg27),
    .ivReg28                (ivSMBusReg28),
    .ivReg29                (ivSMBusReg29),
    .ivReg2A                (ivSMBusReg2A),
    .ivReg2B                (ivSMBusReg2B),
    .ivReg2C                (ivSMBusReg2C),
    .ivReg2D                (ivSMBusReg2D),
    .ivReg2E                (ivSMBusReg2E),
    .ivReg2F                (ivSMBusReg2F),
    .ovQ                    (wvIICRORegs),
    .oAccessDone            (wvIICRegsAccessDone[0])
);
//
//
//
ContReadSeqWriteRegs mRWLUTRegs
(   
    .iClk                   ( iClk ),
    .iRst                   ( iRst ),
    .iRnW                   ( wIICRnW ),
    .iEnable                ( wvIICRegsEnable[1] ),
    .ivAddress              ( wvIICRegAddress[5:0] ),
    .ovReg30                ( wvReg30 ),

    .ovQ                    ( wvIICRWLutsRegs ),
    .ivD                    ( wvIICRxData ),
    .oAccessDone            ( wvIICRegsAccessDone[1] )
);

//
//
//
EventsBRAMRegs mEventsRegs
(   
    //! Module's clock input
    .iClk                   (iClk),
    //! Reset input
    .iRst                   (iRst),
    //! Enable input
    .iEnable                (wvIICRegsEnable[2]),
    //!
    .oAccessDone            (wvIICRegsAccessDone[2]),
    //! App Access
    .ivAddress              (ivEventsAddress),
    .iAppWE                 (iEventsWE),
    .ivAppData              (ivEventsInData),
    .iAppDataRst            (iEvRst),
    //! SMBus regs access
    .ivRegID                (wvIICRegAddress[11:0]),
    .ovRegData              (wvIICRWBRAMEventsRegs)
);

//////////////////////////////////////////////////////////////////////////////////


endmodule
