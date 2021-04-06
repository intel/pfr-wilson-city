//////////////////////////////////////////////////////////////////////////////////
/*!
    \brief      <b>Flip Flop SR</b>\n
    \details    
                \n
    \file       FF_SR.v
    \author     amr/carlos.l.bernal\@intel.com
    \date       Dic 28, 2012
    \brief      $RCSfile: FF_SR.v.rca $
                $Date: Fri Feb 15 17:52:31 2013 $
                $Author: edgarara $
                $Revision: 1.3 $
                $Aliases: Arandas_20130312_1720_006C,Arandas_20130314_1025_006C,Arandas_20130321_1930_0100,Arandas_20130623_1550_0102,BellavistaVRTBCodeBase_20130705_0950,Arandas_20130802_2240_0103,BellavistaVRTB_20130912_1700,Durango-FabA-0x0021,Durango-FabA-0x0022,Durango-FabA-0x0023,Durango-FabA-0x0024,Durango-FabA-0x0025,Durango-FabA-0x0028,Durango-FabA-0x0029,Durango-FabA-0x002A,Durango-FabA-0x002B,Durango-FabA-0x0101,Rev_0x0D,ColimaFabA0x0001,ColimaFabA0x0002,ColimaFabA0x0003,DurangoFabA0x0104,DurangoFabA0x0105,ColimaFabA0x0004,DurangoFabA0x0106,ColimaFabA0x0006,DurangoFabA0x0110,ColimaFabA0x000C,ColimaFabA0x000D,DurangoFabA0x0116,DurangoFabA0x0117,DurangoFabA0x011A,DurangoFabA0x011B,Arandas_20131223_1100_0104,Durango0x0120,Durango0x0123,Durango0x0124,Durango0x0125,Colima0x0011,Durango0x0127,Colima0x0013,Durango0x0128,Durango0x0129,Colima0x0017,Colima0x001A,Colima0x001B,Colima0x002D,Durango0x012D,Durango0x012E,Colima0x001E,Durango0x0133,Durango0x0134,Durango0x0136,Durango0x0137,Durango0x0138,Durango0x013A $
                <b>Project:</b> FPGA coding\n
                <b>Group:</b> PVE-VHG\n
                <b>Testbench:</b> FR_SRTB\n
                <b>Resources:</b>   <ol>
                                        <li>Spartan3AN 
                                            - 1 Slice
                                    </ol>
                <b>Instances:</b>   <ol>

                                    </ol>            
                <b>References:</b>  <ol>
                                        <li>Arandas
                                        <li>Summer Valley
                                    </ol>
                <b>Block Diagram:</b>
    \verbatim
        +------------------+
 -----> |> iClk     .   oQ |----->
 -----> |  iRst     .      |
 -----> |  iSet     .      |
        +------------------+
               FF_D
    \endverbatim
     
    \version    
                20121228 \b clbernal - File creation\n
                20130214 \b edgarara - Added Comments and Documentation Format\n    
    \copyright Intel Proprietary -- Copyright 2015 Intel -- All rights reserved
*/
//////////////////////////////////////////////////////////////////////////////////

`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
//      FF SR
//////////////////////////////////////////////////////////////////////////////////
module FF_SR //% Flip Flop SR
(
                    //% Clock Input<br>
    input           iClk,
                    //% Asynchronous Reset Input<br>
    input           iRst,
                    //% Register Set<br>
    input           iSet,
                    //% Register Output<br>
    output          oQ
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
//%
reg    rQ_d;
reg    rQ_q;
//////////////////////////////////////////////////////////////////////////////////
// Continous assigments
//////////////////////////////////////////////////////////////////////////////////
assign  oQ    =   rQ_q;
//////////////////////////////////////////////////////////////////////////////////
// Sequential logic
//////////////////////////////////////////////////////////////////////////////////
always @(posedge iClk)
begin
    if(iRst)                            //Reset?
    begin
        rQ_q    <=    1'b0;             //Yes, then reset the register output to 0.
    end
    else
    begin
        if(iSet)
        begin
            rQ_q    <=    1'b1;         //No, then if iSet is up, set the register
        end                             //output to 1.
        else
        begin
            rQ_q    <=    rQ_d;        //else, the register output doesn't change.
        end
    end
end
//////////////////////////////////////////////////////////////////////////////////
// Combinational logic
//////////////////////////////////////////////////////////////////////////////////
always @*
begin
    rQ_d    =   rQ_q;
end
//////////////////////////////////////////////////////////////////////////////////
// Instances
//////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////
endmodule