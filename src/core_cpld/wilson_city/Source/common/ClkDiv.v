//////////////////////////////////////////////////////////////////////////////////
/*!
    \brief      <b>Parameterizable Synchronus Clock Divider</b>
    \details    Generates a single clock pulse when the internal counter reaches\n 
                the MAX_DIV_CNT\n
    \file       ClkDiv.v
    \author     amr/carlos.l.bernal@intel.com
    \date       Aug 21, 2011
    \brief      $RCSfile: ClkDiv.v.rca $
                $Date: Tue Jul  2 15:46:17 2013 $
                $Author: edgarara $
                $Revision: 1.11 $
                $Aliases: $
                $Log: ClkDiv.v.rca $
                
                 Revision: 1.11 Tue Jul  2 15:46:17 2013 edgarara
                 DS log history added
                <b>Project:</b> FPGA coding\n
                <b>Group:</b> PVE-VHG\n
                <b>Testbench:</b> ClkDivTB.v\n
                <b>Resources:</b>   <ol>
                                        <li>Spartan3AN 
                                        <li>Cyclone IV
                                        <li>MachXO2
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
       +-----------------+
-----> |> iClk   oDivClk |----->
-----> |  iRst      .    |
-----> |  iCE       .    |
       +-----------------+
              ClkDiv
    \endverbatim
    
    \version    
                20110621 \b clbernal - File creation\n
                20130205 \b edgarara - Added version history\n   
    \copyright  Intel Proprietary -- Copyright 2011 Intel -- All rights reserved
*/
//////////////////////////////////////////////////////////////////////////////////

`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
//         ClkDiv
//////////////////////////////////////////////////////////////////////////////////

module ClkDiv # //%Parameterizable Synchronus Clock Divider<br>
(
                //% Counter bits<br>
    parameter   MAX_DIV_BITS = 4, 
                //% Maximun count value<br>
    parameter   MAX_DIV_CNT = 15  
)
(
            //% Clock Input<br>    
    input   iClk,
            //% Asynchronous Reset Input<br>
    input   iRst,
            //% Clock Enable from a previous stage<br>
    input   iCE,
            //% Output Divided Clock Signal<br>
    output  oDivClk
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
                                //% Internal Counter register input equation<br>
reg    [(MAX_DIV_BITS - 1):0]   rvDivCnt_d;
                                //% Internal Counter register Output<br>
reg    [(MAX_DIV_BITS - 1):0]   rvDivCnt_q;
                                //% Divided Clock register input equation<br>
reg                             rDivClk_d;
                                //% Divided Clock register output<br>
reg                             rDivClk_q;
//////////////////////////////////////////////////////////////////////////////////
// Continous assigment
//////////////////////////////////////////////////////////////////////////////////
assign    oDivClk    =    rDivClk_q;
//////////////////////////////////////////////////////////////////////////////////
// Sequential Section
//////////////////////////////////////////////////////////////////////////////////
//% Sequential Section<br>
always @(posedge iClk or posedge iRst)
begin
    if(iRst)                                    //Reset?
    begin
        rvDivCnt_q  <=  {MAX_DIV_BITS{1'b0}};   // Yes, then set Count Reset to all 
        rDivClk_q   <=  1'b0;                   //0's and Divided Clock 0
    end
    else
    begin
        if(iCE)                                 //No, Clock Enable?
        begin
            rvDivCnt_q  <=  rvDivCnt_d;         // Yes, then update Count register
        end
        else
        begin
            rvDivCnt_q  <=  rvDivCnt_q;         // No, then keep the same value
        end
        rDivClk_q   <=  rDivClk_d;              //Update Divided Clock Signal 
                                                //Register
    end
end
//////////////////////////////////////////////////////////////////////////////////
// Combinational Section
//////////////////////////////////////////////////////////////////////////////////
//% Combinational Section<br>
always @*
begin
    //Clock divider will be set to 1 when rvDivCnt_q reaches the MAX_DIV_CNT
    rDivClk_d   =   (iCE) ? (rvDivCnt_q == MAX_DIV_CNT) : 1'b0; 
    //rvDivCnt will be incremented while it is below the MAX_DIV_CNT, otherwise
    //it is set to 0
    rvDivCnt_d  =   {MAX_DIV_BITS{1'b0}};   
    if(rvDivCnt_q != MAX_DIV_CNT)   
    begin
        rvDivCnt_d  =   rvDivCnt_q + 1'b1;
    end
end
//////////////////////////////////////////////////////////////////////////////////
//Instances
//////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////
endmodule
