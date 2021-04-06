`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
/*!
    \brief      <b>Serial GPIO parametrizable module</b> 
    \details    Inputs/outputs size configurable serial expander module.\n
    \file       GSX.v
    \author     amr/carlos.l.bernal@intel.com
    \date       Feb 18, 2013
    \brief      $RCSfile: GSX.v.rca $
                $Date:  $
                $Author:  $
                $Revision:  $
                $Aliases: $
                $Log: NeonCity.v.rca $
                
                <b>Project:</b> Neon City\n
                <b>Group:</b> BD\n
                <b>Testbench:</b>
                <b>Resources:</b>   <ol>
                                        <li>MachXO2
                                        <li>Arandas
                                        <li>Durango
                                        <li>Bellavista
                                    </ol>
                <b>Block Diagram:</b>
    \verbatim
        +------------------------------------------------+
 -----> |> iGSXClk     .       . ovRxData[PARAMETER_2:0] |=====>
 -----> |  inGSXReset  .       .       .     oGSXDataOut |----->
 -----> |  iGSXDataIn  .       .       .     .       .   |
 =====> |  ivTxData[PARAMETER_1:0]     .     .       .   |
        +------------------------------------------------+
                           GSX
    \endverbatim
    \copyright  Intel Proprietary -- Copyright 2015 Intel -- All rights reserved
*/
//////////////////////////////////////////////////////////////////////////////////
module GSX #(parameter TOTAL_INPUT_MODULES = 8, TOTAL_OUTPUT_MODULES = 8)
(
    //% Serial Data input
    input                                   iGSXDataIn,
    //% Serial Clock
    input                                   iGSXClk,
    //% Load control
    input                                   inGSXLoad,
    //% Reset
    input                                   inGSXReset,
    //% Data to transmit vector
    input   [(TOTAL_OUTPUT_MODULES*8)-1:0]  ivTxData,    
    //% Serial Data output
    output                                  oGSXDataOut,
    //% Received data vector    
    output  [(TOTAL_INPUT_MODULES*8)-1:0]   ovRxData
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
reg [(TOTAL_OUTPUT_MODULES*8)-1:0]  rvTxShifter_d;
reg [(TOTAL_OUTPUT_MODULES*8)-1:0]  rvTxShifter_q;
//!
reg [(TOTAL_INPUT_MODULES*8)-1:0]   rvRxShifter_d;
reg [(TOTAL_INPUT_MODULES*8)-1:0]   rvRxShifter_q;
//!
reg [(TOTAL_INPUT_MODULES*8)-1:0]   rvOutput_d;
reg [(TOTAL_INPUT_MODULES*8)-1:0]   rvOutput_q;
//////////////////////////////////////////////////////////////////////////////////
// Continous assigments
//////////////////////////////////////////////////////////////////////////////////
assign  ovRxData    =   rvOutput_q;
assign  oGSXDataOut =   rvTxShifter_q[(TOTAL_OUTPUT_MODULES*8)-1];
//////////////////////////////////////////////////////////////////////////////////
// Sequential logic
//////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////
always @ (posedge iGSXClk)
begin
	if ( !inGSXLoad )
	begin
		rvTxShifter_q   <=  ivTxData;
	end
	else
	begin
		rvTxShifter_q   <=  rvTxShifter_d;		
	end
end
//////////////////////////////////////////////////////////////////////////////////
always @ (posedge iGSXClk)
begin
	if ( !inGSXReset ) 
	begin
		rvRxShifter_q   <=  {(TOTAL_INPUT_MODULES*8){1'b0}};	
	end
	else
	begin
        rvRxShifter_q   <=  rvRxShifter_d;	
	end
end

reg prev_inGSXLoad;
always @ (posedge iGSXClk)
begin
	if (inGSXLoad && !prev_inGSXLoad)
	begin
		rvOutput_q  <=  rvOutput_d;
	end
	prev_inGSXLoad <= inGSXLoad;
end


//////////////////////////////////////////////////////////////////////////////////
// Combinational logic
//////////////////////////////////////////////////////////////////////////////////
always @*
begin
    rvOutput_d      =   rvRxShifter_q;   
    rvRxShifter_d   =   { rvRxShifter_q[(TOTAL_INPUT_MODULES*8)-2:0], iGSXDataIn};
    rvTxShifter_d   =   { rvTxShifter_q[((TOTAL_OUTPUT_MODULES*8)-2):0], 1'b0 };    
end
//////////////////////////////////////////////////////////////////////////////////
// Instances
//////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////
endmodule
