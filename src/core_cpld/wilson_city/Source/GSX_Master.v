//////////////////////////////////////////////////////////////////////////////////
/*!
    \brief      <b>Serial GPIO parametrizable module. Based on SF-8485</b> 
    \details    Inputs/outputs size configurable serial expander module.\n
    \file       GSX_Slave.v
    \author     amr/jorge.juarez.campos@intel.com

   	\version    
				20180516 	\b  jorge.juarez.campos@intel.com - File creation. Leveraged from GSX_Master.v\n

    \copyright  Intel Proprietary -- Copyright 2018 Intel -- All rights reserved
*/
//////////////////////////////////////////////////////////////////////////////////

module GSX_Master #(parameter TOTAL_OUTPUT_MODULES = 3)
(
	input 			iClk,			//% 2MHz Reference Clock
	input 			iCE,			//% Chip enable to generate oSClock. 10uS to generate 100KHz SClock
	input 		    iReset,			//%	Input reset.

	output 			oSClock,		//%	Output clock.
	output reg		oSLoad,			//%	Output Last clock of a bit stream; begin a new bit stream on the next clock.
	output reg		oSDataOut,		//%	Input serial data bit stream.
	input           iSDataIn, 		//%	Output serial data bit stream.

	output reg		[(TOTAL_OUTPUT_MODULES*8)-1:0]  ovDataIn, 		//% Data receive vector. Master receives using DataIn
	input   		[(TOTAL_OUTPUT_MODULES*8)-1:0]  ivDataOut 		//% Data transmit vector. Master transmits using DataOut
);
//////////////////////////////////////////////////////////////////////////////////
// Internal Signals
//////////////////////////////////////////////////////////////////////////////////
reg [7:0] rCounter_Out;
wire wSClock;

//////////////////////////////////////////////////////////////////////////////////
// Continuous assignments
//////////////////////////////////////////////////////////////////////////////////
//Idle SGPIO bus by driving signals HIGH when system is in Reset state
assign oSClock	= (!iReset) ? 1'b1 : wSClock;


//////////////////////////////////////////////////////////////////////////////////
// Sequential logic
//////////////////////////////////////////////////////////////////////////////////
always @ (posedge wSClock) 	
begin 
	if(!iReset) 
	begin
		oSLoad    		<= 1'b1;
		oSDataOut 		<= 1'b1;
		rCounter_Out  	<= 8'd0;
	end
	else
	begin
		oSDataOut <= ivDataOut[rCounter_Out];				

		if( rCounter_Out == ((TOTAL_OUTPUT_MODULES*8)-1) )
		begin
			oSLoad  		<= 1'b1; 
			rCounter_Out	<= 8'd0; 
		end
		else
		begin
			oSLoad 			<= 1'b0;
			rCounter_Out	<= rCounter_Out + 1'b1; 
		end 
	end 
end

always @ (negedge wSClock) 	
begin 
	if(!iReset) 
	begin
		ovDataIn 				<= {(TOTAL_OUTPUT_MODULES*8){1'b0}};
	end
	else			
	begin
		ovDataIn[rCounter_Out]  <= iSDataIn;
	end
end


//////////////////////////////////////////////////////////////////////
// Instances 
//////////////////////////////////////////////////////////////////////
// Generate 100KHz Clock
Toggle mToggle100KHz_SClock
(  
    .iRst(~iReset), 		//%Reset Input
	.iClk(iClk), 			//% Clock Input<br>
	.iCE(iCE),				//% Clock Enable
    .oTSignal(wSClock)		//% Output Signal Toggle
);

endmodule
