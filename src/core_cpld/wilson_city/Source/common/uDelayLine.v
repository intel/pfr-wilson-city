`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
/*!
    \brief      <b>Short delay generator using a shift register</b>\n
    \details    Delays the input signal a specific amount of clocks\n
    \file       uDelayLine.v
    \author     amr/carlos.l.bernal@intel.com
    \date       Feb 22, 2012
    \brief      $RCSfile: uDelayLine.v.rca $
                $Date: Tue Dec 16 18:15:35 2014 $
                $Author: clbernal $
                $Revision: 1.5 $
                $Aliases:  $
                <b>Project:</b> FPGA coding\n
                <b>Group:</b> PVE-VHG\n
                <b>Testbench:</b> uDelayLineTB.v\n
                <b>Resources:</b>   <ol>
                                        <li>Spartan3AN
                                        <li>MachXO2
                                        <li>Cyclone IV
                                    </ol>
                <b>Instances:</b>   <ol>
                                    </ol>            
                <b>References:</b>  <ol>
                                        <li>Arandas
                                        <li>Summer Valley
                                        <li>NeonCity
                                        <li>Durango
                                        <li>Colima
                                        <li>SanBlas
                                    </ol>
                <b>Block Diagram:</b>
    \verbatim
        +-----------------------------------------------+
 -----> |> iClk     .        .       .       oDelayedIn | =====>
 -----> |  iRst     .        .       .       .          |
 -----> |  iCE      .        .       .       .          |
 -----> |  iSignal  .        .       .       .          |
        +-----------------------------------------------+
               uDelay
    \endverbatim
    \version    
                20120124 \b clbernal - File creation\n
                20130308 \b edgarara - Added comments and documentation format\n    
    \copyright Intel Proprietary -- Copyright 2015 Intel -- All rights reserved
*/
//////////////////////////////////////////////////////////////////////////////////
`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
module uDelay #(parameter TOTAL_BITS = 2)
(
    //% Clock
	input	iClk,
    //% Reset    
	input	iRst,
    //% Clock Enable    
	input	iCE,
    //% Signal to be delayed    
	input	iSignal,
    //% Delayed signal    
	output	oDelayedIn
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
//!
reg [(TOTAL_BITS-1):0]  rvDlyLine_d;
reg [(TOTAL_BITS-1):0]  rvDlyLine_q;
//////////////////////////////////////////////////////////////////////////////////
// Continous assigments
//////////////////////////////////////////////////////////////////////////////////
assign oDelayedIn = rvDlyLine_q[(TOTAL_BITS-1)];
//////////////////////////////////////////////////////////////////////////////////
// Sequential logic
//////////////////////////////////////////////////////////////////////////////////
always @(posedge iClk)
begin
	if(iRst)
	begin
		rvDlyLine_q	<=	{TOTAL_BITS{iSignal}};
	end
	else
	begin
		if(iCE)
		begin
			rvDlyLine_q	<=	rvDlyLine_d;
		end
		else
		begin
			rvDlyLine_q	<=	rvDlyLine_q;
		end

	end
end
//////////////////////////////////////////////////////////////////////////////////
// Combinational logic
//////////////////////////////////////////////////////////////////////////////////
always @*
begin
	rvDlyLine_d	=	{rvDlyLine_q[(TOTAL_BITS-2):0],iSignal};
end
//////////////////////////////////////////////////////////////////////////////////
// Instances
//////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////
endmodule
