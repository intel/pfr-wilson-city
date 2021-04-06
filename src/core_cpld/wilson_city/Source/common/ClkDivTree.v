//////////////////////////////////////////////////////////////////////////////////
/*!
    \brief      <b>Clock domain synchronization module</b>\n
    \details    2 FFs clock domain input synchronizer\n
    \file       ClkDivTree.v
    \date       Oct 2, 2011
    \brief      $RCSfile: ClkDivTree.v.rca $
                $Date: Tue Dec 16 18:15:35 2014 $
                $Author: $
                $Revision: 1.5 $
                $Aliases:  $
                <b>Project:</b> FPGA coding\n
                <b>Group:</b> PVE-VHG\n
                <b>Resources:</b>   <ol>
                                        <li>MachXO2
                                    </ol>
                <b>Instances:</b>   <ol>
                                        <li> ClkDiv
                                    </ol>            
                <b>References:</b>  <ol>
                                        <li>NeonCity
                                    </ol>
                <b>Block Diagram:</b>
    \verbatim
        +---------------------------+
        |           .        o1uSCE |----->  
 -----> |> iClk     .       o10uSCE |----->
 -----> |  iRst     .       o50uSCE |----->
 -----> |  iSignal  .      o500uSCE |----->
        |           .        o1mSCE |----->
        |           .      o250mSCE |----->
        |           .       o20mSCE |----->        
        +---------------------------+
                  ClkDivTree
    \endverbatim 
    \version    
                20120124 \b clbernal - File creation\n
                20130308 \b edgarara - Added comments and documentation format\n    
                20180323 \b jljuarez - Fixed MAX_DIV_CNT time for base 1uS and 10uS\n  
    \copyright Intel Proprietary -- Copyright 2015 Intel -- All rights reserved
*/
//////////////////////////////////////////////////////////////////////////////////
`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
module ClkDivTree
(
    //% Clock
    input   iClk,
    //% Reset
    input   iRst,
    //% 1uS Clock enable
    output  o1uSCE,
    //% 5uS Clock enable
    output  o5uSCE,
    //% 10uS Clock enable
    output  o10uSCE,
    //% 50uS Clock enable
    output  o50uSCE,
    //% 500uS Clock enable
    output  o500uSCE,
    //% 1mS Clock enable
    output  o1mSCE,
    //% 250mS Clock enable
    output  o250mSCE,
    //% 20mS Clock enable
    output  o20mSCE,
    //% 1SCE Clock Enable
    output  o1SCE

);

`ifdef FAST_SIM_MODE
localparam SIMULATION_MODE =  1'b1;
`else
localparam SIMULATION_MODE =  1'b0;
`endif

wire    w1uSCE;
wire    w5uSCE;
wire    w10uSCE;
wire    w50uSCE;
wire    w500uSCE;
wire    w1mSCE;
wire    w250mSCE;
wire    w20mSCE;
wire    w1SCE;

assign  o1uSCE      =   w1uSCE;
assign  o5uSCE      =   w5uSCE;
assign  o10uSCE     =   w10uSCE;
assign  o50uSCE     =   w50uSCE;
assign  o500uSCE    =   w500uSCE;
assign  o1mSCE      =   w1mSCE;
assign  o250mSCE    =   w250mSCE;
assign  o20mSCE     =   w20mSCE;
assign  o1SCE       =   w1SCE;

//
//% 1uS Clock divide
//
ClkDiv #
(
    .MAX_DIV_BITS ( 1 ),
    .MAX_DIV_CNT  ( 1 )
)m1uSCE
(
    .iClk               ( iClk ),
    .iRst               ( iRst ),
    .iCE                ( 1'b1 ),
    .oDivClk            ( w1uSCE )
);

//
//% 5uS Clock divide
//
ClkDiv #
(
    .MAX_DIV_BITS ( SIMULATION_MODE ? 1 : 3 ),
    .MAX_DIV_CNT  ( SIMULATION_MODE ? 1 : 4 )
)m5uSCE
(
    .iClk               ( iClk ),
    .iRst               ( iRst ),
    .iCE                ( w1uSCE ),
    .oDivClk            ( w5uSCE )
);

//
//% 10uS Clock divide
//
ClkDiv #
(
    .MAX_DIV_BITS ( SIMULATION_MODE ? 1 : 4 ),
    .MAX_DIV_CNT  ( SIMULATION_MODE ? 1 : 9 )
)m10uSCE
(
    .iClk               ( iClk ),
    .iRst               ( iRst ),
    .iCE                ( w1uSCE ),
    .oDivClk            ( w10uSCE )
);
//
//% 50uS Clock divide
//
ClkDiv #
(
    .MAX_DIV_BITS ( SIMULATION_MODE ? 1 : 3 ),
    .MAX_DIV_CNT  ( SIMULATION_MODE ? 1 : 4 )
)m50uSCE
(
    .iClk               ( iClk ),
    .iRst               ( iRst ),
    .iCE                ( w10uSCE ),
    .oDivClk            ( w50uSCE )
);
//
//% 500uS Clock divide
//
ClkDiv #
(
    .MAX_DIV_BITS       ( SIMULATION_MODE ? 1 : 4 ),
    .MAX_DIV_CNT        ( SIMULATION_MODE ? 1 : 9 )
)m500uSCE
(
    .iClk               ( iClk ),
    .iRst               ( iRst ),
    .iCE                ( w50uSCE ),
    .oDivClk            ( w500uSCE )
);  
//
//% 1mS Clock divide
//
ClkDiv #
(
    .MAX_DIV_BITS       ( 1 ),
    .MAX_DIV_CNT        ( 1 )
)m1mSCE
(
    .iClk               ( iClk ),
    .iRst               ( iRst ),
    .iCE                ( w500uSCE ),
    .oDivClk            ( w1mSCE )
);  
//
//% 250mS Clock divide
//
ClkDiv #
(
    .MAX_DIV_BITS       ( SIMULATION_MODE ? 1 : 4 ),
    .MAX_DIV_CNT        ( SIMULATION_MODE ? 1 : 11 )
)m250mSCE
(
    .iClk               ( iClk ),
    .iRst               ( iRst ),
    .iCE                ( w20mSCE ),
    .oDivClk            ( w250mSCE )
);

//
//% 1S Clock divide
//

ClkDiv #
(
    .MAX_DIV_BITS       ( SIMULATION_MODE ? 1 : 3 ),
    .MAX_DIV_CNT        ( SIMULATION_MODE ? 1 : 4 )
)m1SCE
(
    .iClk               ( iClk ),
    .iRst               ( iRst ),
    .iCE                ( w250mSCE ),
    .oDivClk            ( w1SCE )
);
//
//% 20mS Clock divide
//
ClkDiv #
(
    .MAX_DIV_BITS       ( SIMULATION_MODE ? 1 : 5 ),
    .MAX_DIV_CNT        ( SIMULATION_MODE ? 1 : 19 )
)m20mSCE
(
    .iClk               ( iClk ),
    .iRst               ( iRst ),
    .iCE                ( w1mSCE ),
    .oDivClk            ( w20mSCE )
);

endmodule