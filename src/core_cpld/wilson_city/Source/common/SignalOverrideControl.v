//////////////////////////////////////////////////////////////////////////////////
/*!
    \brief      <b>Module's brief...</b>\n
    \details    Detailed Description...
                \n
    \file       SignalOverrideControl.v
    \author     amr/carlos.l.bernal@intel.com
    \date       Jan 14, 2012
    \brief      $RCSfile: SignalOverrideControl.v.rca $
                $Date: Thu Sep  5 17:21:45 2013 $
                $Author: dggarci2 $
                $Revision: 1.4 $
                $Aliases: BellavistaVRTB_20130912_1700,Durango-FabA-0x0021,Durango-FabA-0x0022,Durango-FabA-0x0023,Durango-FabA-0x0024,Durango-FabA-0x0025,Durango-FabA-0x0028,Durango-FabA-0x0029,Durango-FabA-0x002A,Durango-FabA-0x002B,Durango-FabA-0x0101,Rev_0x0D,ColimaFabA0x0001,ColimaFabA0x0002,ColimaFabA0x0003,DurangoFabA0x0104,DurangoFabA0x0105,ColimaFabA0x0004,DurangoFabA0x0106,ColimaFabA0x0006,DurangoFabA0x0110,ColimaFabA0x000C,ColimaFabA0x000D,DurangoFabA0x0116,DurangoFabA0x0117,DurangoFabA0x011A,DurangoFabA0x011B,Arandas_20131223_1100_0104,Durango0x0120,Durango0x0123,Durango0x0124,Durango0x0125,Colima0x0011,Durango0x0127,Colima0x0013,Durango0x0128,Durango0x0129,Colima0x0017,Colima0x001A,Colima0x001B,Colima0x002D,Durango0x012D,Durango0x012E,Colima0x001E,Durango0x0133,Durango0x0134,Durango0x0136,Durango0x0137,Durango0x0138,Durango0x013A $
                <b>Project:</b> FPGA coding\n
                <b>Group:</b> PVE-VHG\n
                <b>Testbench:</b> ModuleTB.v\n
                <b>Resources:</b>   <ol>
                                        <li>Spartan3AN
                                            - xx Slices
                                    </ol>
                <b>References:</b>  <ol>
                                        <li>Durango
                                        <li>Colima
                                        <li>Bellavista
                                        <li>San Blas
                                    </ol>
                <b>Block Diagram:</b>
    \verbatim
        +------------------------------------------------+
 -----> |> iClk     .      ovSignal[(TOTAL_SIGNALS-1):0] |=====>
 -----> |  iRst     .       .       .       .       .    |
 =====> |  ivOvrEnable[(TOTAL_SIGNALS-1):0] .       .    |
 =====> |  ivOvrValue[(TOTAL_SIGNALS-1):0]  .       .    |
 =====> |  ivSignal[(TOTAL_SIGNALS-1):0]    .       .    | 
        +------------------------------------------------+
                           SignalOverrideControl
    \endverbatim
    \version    
                20120114 \b clbernal - File creation\n
                20130903 \b dggarci2 - Added doxygen tags
    \copyright Intel Proprietary -- Copyright 2013 Intel -- All rights reserved
*/
//////////////////////////////////////////////////////////////////////////////////

`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
//          SignalOverrideControl
//////////////////////////////////////////////////////////////////////////////////

module SignalOverrideControl #(parameter TOTAL_SIGNALS = 16)
(
                                    //% Clock Input<br>
	input                           iClk,
                                    //% Asynchronous Reset Input<br>
	input                           iRst,
                                    //% Override Enable Input vector<br>
    input   [(TOTAL_SIGNALS-1):0]   ivOvrEnable,
                                    //% Override Value Input vector<br>
    input   [(TOTAL_SIGNALS-1):0]   ivOvrValue,
                                    //% Input vector<br>
    input   [(TOTAL_SIGNALS-1):0]   ivSignal,
                                    //% Siganl override or signal output vector<br>
    output  [(TOTAL_SIGNALS-1):0]   ovSignal
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
                            //%Mux output, override value or input signal<br>
reg [(TOTAL_SIGNALS-1):0]   rvSignal_d;
reg [(TOTAL_SIGNALS-1):0]   rvSignal_q;
//!
integer iIndex;
//////////////////////////////////////////////////////////////////////////////////
// Continous assigments
//////////////////////////////////////////////////////////////////////////////////
assign ovSignal = rvSignal_q;
//////////////////////////////////////////////////////////////////////////////////
// Sequential logic
//////////////////////////////////////////////////////////////////////////////////
always @(posedge iClk)
begin
    if(iRst)
    begin
        rvSignal_q  <=  ivSignal;
    end
    else
    begin
        rvSignal_q  <=  rvSignal_d;
    end
end
//////////////////////////////////////////////////////////////////////////////////
// Combinational logic
//////////////////////////////////////////////////////////////////////////////////
always @*
begin
    for(iIndex = 0; iIndex < TOTAL_SIGNALS; iIndex = iIndex + 1)
    begin
        rvSignal_d[iIndex]  =   ivOvrEnable[iIndex] ? ivOvrValue[iIndex] : ivSignal[iIndex];    
    end                                      //Mux for each input signal to be able                                                    
end                                          //to select override value or input signal
//////////////////////////////////////////////////////////////////////////////////
//  History
//////////////////////////////////////////////////////////////////////////////////
/*
    $Log: SignalOverrideControl.v.rca $
    
     Revision: 1.4 Thu Sep  5 17:21:45 2013 dggarci2
     updated code comments
    
     Revision: 1.3 Thu Sep  5 10:20:44 2013 dggarci2
     Added Doxygen tags and comments
    
*/
//////////////////////////////////////////////////////////////////////////////////
endmodule
