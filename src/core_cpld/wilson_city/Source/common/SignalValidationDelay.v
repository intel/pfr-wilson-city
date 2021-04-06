//////////////////////////////////////////////////////////////////////////////////
/*!
    \brief      <b>Parameterizable Signal Validation Delay</b>\n
    \details    Waits for an internal counter to reach his max count to set high\n
                or low,depending on the polarity input, an Output signal.\n
    \file       SignalValidationDelay.v
    \author     amr/carlos.l.bernal\@intel.com
    \date       Aug 4, 2012
    \brief      $RCSfile: SignalValidationDelay.v.rca $
                $Date: Thu Dec 19 13:04:21 2013 $
                $Author: clbernal $
                $Revision: 1.7 $
                $Aliases: DurangoFabA0x011A,DurangoFabA0x011B,Arandas_20131223_1100_0104,Durango0x0120,Durango0x0123,Durango0x0124,Durango0x0125,Colima0x0011,Durango0x0127,Colima0x0013,Durango0x0128,Durango0x0129,Colima0x0017,Colima0x001A,Colima0x001B,Colima0x002D,Durango0x012D,Durango0x012E,Colima0x001E,Durango0x0133,Durango0x0134,Durango0x0136,Durango0x0137,Durango0x0138,Durango0x013A $
                <b>Project:</b> FPGA coding\n
                <b>Group:</b> PVE-VHG\n
                <b>Testbench:</b> SignalValidationDelayTB.v\n
                <b>Resources:</b>   <ol>
                                        <li>Device Name 
                                            - 6 Slices (Depending on Parameters)
                                    </ol>
                <b>Instances:</b>   <ol>
                                    </ol>            
                <b>References:</b>  <ol>
                                        <li>Arandas
                                        <li>Summer Valley
                                    </ol>
                <b>Block Diagram:</b>
    \verbatim
        +-----------------------------------------+
 -----> |> iClk     .       .       .       oDone |----->
 -----> |  iRst     .       .       .       .     |
 -----> |  iCE      .       .       .       .     |
 =====> |  ivMaxCnt[(TOTAL_BITS -1):0]      .     |
 -----> |  iStart   .       .       .       .     |
        +-----------------------------------------+
                           SignalValidationDelay
    \endverbatim     
    \version    
                20120604 \b clbernal - File creation\n
                20130425 \b edgarara - Added comments and documentation form\n    
    \copyright Intel Proprietary -- Copyright 2012 Intel -- All rights reserved
*/

`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
//      SignalValidationDelay
//////////////////////////////////////////////////////////////////////////////////
module SignalValidationDelay#
(
    
    parameter   VALUE         = 1'b1, 
                //% Counter Total Bits<br>
    parameter   TOTAL_BITS    = 3'd4, 
                //% Counter Maximun value<br>
    parameter   MAX_COUNT     = 4'd10, 
                //% Signal Polarity<br>
    parameter   POL           = 1'b1
)
(
                                    //% Clock Input<br>
    input                           iClk,
                                    //% Asynchronous Reset Input<br>
    input                           iRst,
                                    //% Clock Enable<br>
    input                           iCE,
                                    //% Counter Maximun Value<br>
    input   [(TOTAL_BITS - 1):0]    ivMaxCnt,
                                    //% Start<br>
    input                           iStart,
                                    //% Done<br>
    output                          oDone
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

reg                     rDone_d;
reg                     rDone_q;

reg [(TOTAL_BITS-1):0]  rvCounter_d;
reg [(TOTAL_BITS-1):0]  rvCounter_q;

wire    wRst = iRst || (iStart ^ VALUE);
//////////////////////////////////////////////////////////////////////////////////
// Continous assigments
//////////////////////////////////////////////////////////////////////////////////
assign    oDone = rDone_q;
//////////////////////////////////////////////////////////////////////////////////
// Sequential logic
//////////////////////////////////////////////////////////////////////////////////
always @(posedge iClk or posedge wRst) 
begin
    if (wRst)
    begin
        rDone_q 	<= ~POL;
        rvCounter_q <= {TOTAL_BITS{1'b0}};
    end
    else
    begin
        rDone_q				<= rDone_d;
        if(iCE)
        begin
            rvCounter_q		<= rvCounter_d;
        end
        else
        begin
            rvCounter_q		<= rvCounter_q;
        end
    end
end
//////////////////////////////////////////////////////////////////////////////////
// Combinational logic
//////////////////////////////////////////////////////////////////////////////////
always @* 
begin
    rDone_d = ~POL;
    rvCounter_d =   rvCounter_q;
    if(rvCounter_q < ivMaxCnt)
    begin
        rvCounter_d     =   rvCounter_q + 1'b1;
    end
    else
    begin
        rDone_d =   POL;    
    end
end
//////////////////////////////////////////////////////////////////////////////////
// Instances
//////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////
endmodule
