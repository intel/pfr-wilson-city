//////////////////////////////////////////////////////////////////////////////////
/*!
    \brief      <b>Parameterizable Edge Detector</b>
    \details    sets a Flag high whenever a establish edge transition is detected.
                \n
    \file       EdgeDetector.v
    \author     amr/carlos.l.bernal\@intel.com
    \date       Oct 2, 2011
    \brief      $RCSfile: EdgeDetector.v.rca $
                $Date: Fri Feb  8 16:18:41 2013 $
                $Author: edgarara $
                $Revision: 1.6 $
                $Aliases: Arandas_20130312_1720_006C,Arandas_20130314_1025_006C,Arandas_20130321_1930_0100,Arandas_20130623_1550_0102,BellavistaVRTBCodeBase_20130705_0950,Arandas_20130802_2240_0103,BellavistaVRTB_20130912_1700,PowerupSequenceWorkingMissigPowerDown,Durango-FabA-0x0021,Durango-FabA-0x0022,Durango-FabA-0x0023,Durango-FabA-0x0024,Durango-FabA-0x0025,Durango-FabA-0x0028,Durango-FabA-0x0029,Durango-FabA-0x002A,Durango-FabA-0x002B,Durango-FabA-0x0101,Rev_0x0D,ColimaFabA0x0001,ColimaFabA0x0002,ColimaFabA0x0003,DurangoFabA0x0104,DurangoFabA0x0105,ColimaFabA0x0004,DurangoFabA0x0106,ColimaFabA0x0006,DurangoFabA0x0110,ColimaFabA0x000C,ColimaFabA0x000D,DurangoFabA0x0116,DurangoFabA0x0117,DurangoFabA0x011A,DurangoFabA0x011B,Arandas_20131223_1100_0104,Durango0x0120,Durango0x0123,Durango0x0124,Durango0x0125,Colima0x0011,Durango0x0127,Colima0x0013,Durango0x0128,Durango0x0129,Colima0x0017,Colima0x001A,Colima0x001B,Colima0x002D,Durango0x012D,Durango0x012E,Colima0x001E,Durango0x0133,Durango0x0134,Durango0x0136,Durango0x0137,Durango0x0138,Durango0x013A $
                <b>Project:</b> FPGA coding\n
                <b>Group:</b> PVE-VHG\n
                <b>Testbench:</b> \n
                <b>Description:</b><br>
                <b>Resources:</b>   <ol>
                                        <li>Spartan3A 
                                        <li>MachXO2
                                        <li>Cyclone IV
                <b>References:</b>  <ol>
                                        <li>Arandas
                                        <li>Summer Valley
                                        <li>Durango
                                        <li>Colima
                                        <li>SanBlas
                                        <li>NeonCity
                                    </ol>
                <b>Block Diagram:</b>
    \verbatim
       +--------------------------+
-----> |> iClk      oEdgeDetected |----->
-----> |  iRst      .       .     |
-----> |  iSignal   .       .     |
       +--------------------------+
                EdgeDetector
    \endverbatim
    \version    
                20111002 \b clbernal - File creation\n
                20130205 \b edgarara - Added version history\n
    \copyright  Intel Proprietary -- Copyright 2011 Intel -- All rights reserved
*/
//////////////////////////////////////////////////////////////////////////////////

`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
//         module EdgeDetector     
//////////////////////////////////////////////////////////////////////////////////
module EdgeDetector # //%Parameterizable Edge Detector<br>
(
                //% Defines Positive Edge (1) or Negative Edge (0)<br>
    parameter   EDGE = 1
)
(
            //% Clock Input<br>
    input   iClk,
            //% Asynchronous Reset Input<br>
    input   iRst,
            //% Monitored Input Signal<br>
    input   iSignal,
            //% Edge Detected Flag<br>
    output  oEdgeDetected
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
                //% Signal Before/After registers Input equation<br>
reg    			rSignal_d;
                //% Signal Befor/After registers Output<br>
reg    			rSignal_q;
                //% Edge Register Input equation<br>
reg             rEdge_d;
                //% Edge Register Output<br>
reg             rEdge_q;
//////////////////////////////////////////////////////////////////////////////////
// Continous assigments
//////////////////////////////////////////////////////////////////////////////////
assign oEdgeDetected = rEdge_q;
//////////////////////////////////////////////////////////////////////////////////
// Sequential logic
//////////////////////////////////////////////////////////////////////////////////
//% Sequential Section<br>
always @(posedge iClk or posedge iRst)
begin
    if(iRst)                                    //Reset?
    begin
        rSignal_q	<=    1'b0;       		//Yes, then before/after registers  
        rEdge_q     <=    1'b0;               //are set with the iSignal and        
    end                                         //the Edge register is cleared with
    else                                        //0.
    begin
        rSignal_q   <=    rSignal_d;         //No, then all registers are updated.
        rEdge_q     <=    rEdge_d;
    end
end
//////////////////////////////////////////////////////////////////////////////////
// Combinational logic
//////////////////////////////////////////////////////////////////////////////////
//% Combinational Section<br>
always @*
begin
    // The Before/After register is loaded with present iSignal and the last iSignal
    //value.
    rSignal_d    	=    iSignal;
    if(EDGE)                                               //posEdge or negEdge?
    begin
        rEdge_d     =   ~rSignal_q & iSignal;    //posEdge, then the flag
    end                                                    //is set when the positive
    else                                                   //transition is registered.
    begin
        rEdge_d     =   rSignal_q & ~iSignal;    //negEdge, then the flag 
    end                                                    //is set when the negative
end                                                        //transition is registered.
//////////////////////////////////////////////////////////////////////////////////
// Instances
//////////////////////////////////////////////////////////////////////////////////

endmodule
