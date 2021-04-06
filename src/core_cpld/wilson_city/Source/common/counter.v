//////////////////////////////////////////////////////////////////////////////////
/*!
    \brief      <b>Parameterizable Counter</b> 
    \details    Increments the output count with every Clock pulse while the\n 
                Clock Enable input is set high.\n
    \file       Counter.v
    \author     amr/carlos.l.bernal\@intel.com
    \date       Oct 6, 2011
    \brief      $RCSfile: Counter.v.rca $
                $Date: Tue Jul  2 15:46:36 2013 $
                $Author: edgarara $
                $Revision: 1.8 $
                $Aliases: BellavistaVRTBCodeBase_20130705_0950,Arandas_20130802_2240_0103,BellavistaVRTB_20130912_1700,PowerupSequenceWorkingMissigPowerDown,Durango-FabA-0x0021,Durango-FabA-0x0022,Durango-FabA-0x0023,Durango-FabA-0x0024,Durango-FabA-0x0025,Durango-FabA-0x0028,Durango-FabA-0x0029,Durango-FabA-0x002A,Durango-FabA-0x002B,Durango-FabA-0x0101,Rev_0x0D,ColimaFabA0x0001,ColimaFabA0x0002,ColimaFabA0x0003,DurangoFabA0x0104,DurangoFabA0x0105,ColimaFabA0x0004,DurangoFabA0x0106,ColimaFabA0x0006,DurangoFabA0x0110,ColimaFabA0x000C,ColimaFabA0x000D,DurangoFabA0x0116,DurangoFabA0x0117,DurangoFabA0x011A,DurangoFabA0x011B,Arandas_20131223_1100_0104,Durango0x0120,Durango0x0123,Durango0x0124,Durango0x0125,Colima0x0011,Durango0x0127,Colima0x0013,Durango0x0128,Durango0x0129,Colima0x0017,Colima0x001A,Colima0x001B,Colima0x002D,Durango0x012D,Durango0x012E,Colima0x001E,Durango0x0133,Durango0x0134,Durango0x0136,Durango0x0137,Durango0x0138,Durango0x013A $
                $Log: Counter.v.rca $
                
                 Revision: 1.8 Tue Jul  2 15:46:36 2013 edgarara
                 DS log history added
                <b>Project:</b> FPGA coding\n
                <b>Group:</b> PVE-VHG\n
                <b>Testbench:</b>
                <b>Resources:</b>   <ol>
                                        <li>Spartan3AN 
                                            - 2 Slices (Depending on Parameters)
                                    </ol>
                <b>References:</b>  <ol>
                                        <li>Arandas
                                        <li>Summer Valley
                                    </ol>
                <b>Block Diagram:</b>
    \verbatim
       +-----------------------------+
-----> |> iClk   ovCnt[TOTAL_BITS:0] |=====>
-----> |  iRst      .       .        |
-----> |  iCE       .       .        |
       +-----------------------------+
                  Counter
    \endverbatim
    
    \version    
                20111006 \b clbernal - File creation\n
                20130205 \b edgarara - Added version history\n
    \copyright  Intel Proprietary -- Copyright 2011 Intel -- All rights reserved
*/
//////////////////////////////////////////////////////////////////////////////////

`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
//           module ModCounter  
//////////////////////////////////////////////////////////////////////////////////
module Counter # //%Parameterizable Counter<br>
(
                //% Counter bits<br>
    parameter   TOTAL_BITS = 4
)
(
                                //% Clock Input<br>
    input                       iClk,
                                //% Asynchronous Reset Input<br>
    input                       iRst,
                                //% Clock Enable from a previous stage<br>
    input                       iCE,
                                //% Output Count value<br>
    output [(TOTAL_BITS-1):0]   ovCnt
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
                        //% Internal counter register input equation<br>
reg [(TOTAL_BITS-1):0]  rvCnt_d;
                        //% Internal counter register Output<br>
reg [(TOTAL_BITS-1):0]  rvCnt_q;
//////////////////////////////////////////////////////////////////////////////////
// Continous assigments
//////////////////////////////////////////////////////////////////////////////////
assign  ovCnt = rvCnt_q;
//////////////////////////////////////////////////////////////////////////////////
// Sequential logic
//////////////////////////////////////////////////////////////////////////////////
//% Sequential Logic<br>
always @(posedge iClk or posedge iRst)
begin
    if(iRst)                                    //Reset?
    begin
        rvCnt_q <= {TOTAL_BITS{1'b0}};          //Yes, then Count register is 
    end                                         //cleared to 0's.
    else
    begin
        if(iCE)                                 //Clock Enable?
        begin
            rvCnt_q <= rvCnt_d;                 //Yes, then the count register is
        end                                     //updated.
        else
        begin
            rvCnt_q <=  rvCnt_q;                //No, then the count register 
        end                                     //doesn't change
    end
end
//////////////////////////////////////////////////////////////////////////////////
// Combinational logic
//////////////////////////////////////////////////////////////////////////////////
//% Combinational Logic<br>
always @*
begin
    rvCnt_d = rvCnt_q + 1'b1;                   //The count increases by 1.
end
//////////////////////////////////////////////////////////////////////////////////
// Instances
//////////////////////////////////////////////////////////////////////////////////
endmodule

