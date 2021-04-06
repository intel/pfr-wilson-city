//////////////////////////////////////////////////////////////////////////////////
/*!
    \brief      <b>Parameterizable Glitch Filter</b>\n
    \details    Delivers a glitchless signal if the sampled data in all stages\n
                is consistent between each other.\n
    \file       GlitchFilter.v
    \author     amr/carlos.l.bernal@intel.com
    \date       Oct 2, 2011
    \brief      $RCSfile: GlitchFilter.v.rca $
                $Date: Fri Feb 15 17:52:11 2013 $
                $Author: edgarara $
                $Revision: 1.3 $
                $Aliases: Arandas_20130312_1720_006C,Arandas_20130314_1025_006C,Arandas_20130321_1930_0100,Arandas_20130623_1550_0102,BellavistaVRTBCodeBase_20130705_0950,Arandas_20130802_2240_0103,BellavistaVRTB_20130912_1700,PowerupSequenceWorkingMissigPowerDown,Durango-FabA-0x0021,Durango-FabA-0x0022,Durango-FabA-0x0023,Durango-FabA-0x0024,Durango-FabA-0x0025,Durango-FabA-0x0028,Durango-FabA-0x0029,Durango-FabA-0x002A,Durango-FabA-0x002B,Durango-FabA-0x0101,Rev_0x0D,ColimaFabA0x0001,ColimaFabA0x0002,ColimaFabA0x0003,DurangoFabA0x0104,DurangoFabA0x0105,ColimaFabA0x0004,DurangoFabA0x0106,ColimaFabA0x0006,DurangoFabA0x0110,ColimaFabA0x000C,ColimaFabA0x000D,DurangoFabA0x0116,DurangoFabA0x0117,DurangoFabA0x011A,DurangoFabA0x011B,Arandas_20131223_1100_0104,Durango0x0120,Durango0x0123,Durango0x0124,Durango0x0125,Colima0x0011,Durango0x0127,Colima0x0013,Durango0x0128,Durango0x0129,Colima0x0017,Colima0x001A,Colima0x001B,Colima0x002D,Durango0x012D,Durango0x012E,Colima0x001E,Durango0x0133,Durango0x0134,Durango0x0136,Durango0x0137,Durango0x0138,Durango0x013A $
                <b>Project:</b> FPGA coding\n
                <b>Group:</b> PVE-VHG\n
                <b>Testbench:</b> GlitchFilterTB.v\n
                <b>Resources:</b>   <ol>
                                        <li>Spartan3AN
                                            - 4 Slices (Depends on parameters)
                                    </ol>
                <b>Instances:</b>   <ol>

                                    </ol>            
                <b>References:</b>  <ol>
                                        <li>Arandas
                                        <li>Summer Valley
                                    </ol>
                <b>Block Diagram:</b>
    \verbatim
        +---------------------------------+
 -----> |> iClk     .   oGlitchlessSignal |----->
 -----> |  iRst     .       .       .     |
 -----> |  iCE      .       .       .     |
 -----> |  iSignal  .       .       .     |
        +---------------------------------+
                   GlitchFilter
    \endverbatim
       
    \version    
                20120916 \b clbernal - File creation\n
                20130214 \b edgarara - Added Comments and Documentation Format\n 
                20160325 \b mdeckar1 - Changed rvSampledData_q reset value to static '0'\n
    \copyright Intel Proprietary -- Copyright 2011 Intel -- All rights reserved
*/
//////////////////////////////////////////////////////////////////////////////////


`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
//  Glitch Filter
//////////////////////////////////////////////////////////////////////////////////
module GlitchFilter # //% <br>
(
    parameter   TOTAL_STAGES = 3
)
(
                //% Clock Input<br> 
    input       iClk,
                //% Asynchronous Reset Input<br>
    input       iRst,
                //% Clock Enable from a previous stage<br>
    input       iCE,
                //% Input Signals<br>
    input       iSignal,
                //%Glitchless Signal Flag<br>
    output      oGlitchlessSignal
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
                        //%Glitchless Signal Register Data Input<br>
reg                     rGlitchlessSignal_d;
                        //%Glitchless Signal Register Output<br>
reg                     rGlitchlessSignal_q;

                        //%Sampled Data Register Data Input<br>
reg [TOTAL_STAGES-1:0]  rvSampledData_d;
                        //%Sampled Data Register Output<br>
reg [TOTAL_STAGES-1:0]  rvSampledData_q;


//////////////////////////////////////////////////////////////////////////////////
// Continous assigments
//////////////////////////////////////////////////////////////////////////////////
assign  oGlitchlessSignal   =   rGlitchlessSignal_q;

//////////////////////////////////////////////////////////////////////////////////
// Sequential logic
//////////////////////////////////////////////////////////////////////////////////
always @(posedge iClk or posedge iRst)
//If Reset, then the GlitchlessSignal is set to 0 and all the Sampled Data  
//Registers outputs are set with the present input Signal value.

//If no, then the Glitchless Signal register data is updated and if the Clock 
//enable is high, also the Sampled Data registers.
//Otherwise, the Sampled Data registers maintains their values.
begin
    if(iRst)                                                    
    begin
        rGlitchlessSignal_q     <=      1'b0;                           
        rvSampledData_q         <=      {TOTAL_STAGES{1'b0}};    
    end
    else
    begin
        rGlitchlessSignal_q     <=      rGlitchlessSignal_d;
        if(iCE)
        begin
            rvSampledData_q     <=      rvSampledData_d;
        end
        else
        begin
            rvSampledData_q     <=      rvSampledData_q;
        end
    end
end
//////////////////////////////////////////////////////////////////////////////////
// Combinational logic
//////////////////////////////////////////////////////////////////////////////////
//Compares the sampled data among themselves and if the data values are consistent 
//in all stages, stablish a Glitchless Signal with it.
always @*
begin
    rvSampledData_d         =    {rvSampledData_q[(TOTAL_STAGES-2):0],iSignal};
    rGlitchlessSignal_d     =    rGlitchlessSignal_q;
    if(~|rvSampledData_q)
    begin
        rGlitchlessSignal_d =   1'b0;
    end
    if(&rvSampledData_q)
    begin
        rGlitchlessSignal_d =   1'b1;
    end
end
//////////////////////////////////////////////////////////////////////////////////
// Instances
//////////////////////////////////////////////////////////////////////////////////

endmodule
//////////////////////////////////////////////////////////////////////////////////