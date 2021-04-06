
//////////////////////////////////////////////////////////////////////////////////
/*!

	\brief    <b>%Fault Converter for Led Display </b>
	\file     Fault2Code8bits.v 
	\details    <b>Image of the Block:</b>
				

				 <b>Description:</b> \n
				The idea of this module is to know what VR's is in Fault,\n\n    
                If in the inputvector the bit 0 is enable the Number should be \n
                0, bit1 -> 1.  
                 
				
	\brief  <b>Last modified</b> 
			$Date:   Dic 13, 2017 $
			$Author:  David.bolanos@intel.com $			
			Project			: Wilson City RP  
			Group			: BD 
	\version    
			 20171213 \b  David.bolanos@intel.com - File creation\n
			   
	\copyright Intel Proprietary -- Copyright \htmlonly <script>document.write(new Date().getFullYear())</script> \endhtmlonly Intel -- All rights reserved
*/
//////////////////////////////////////////////////////////////////////////////////


module Fault2Code8bits (

	input         	iClk,    //% Clock
	input         	iRst_n,  //%Reset enable on low
	input  [254:0] 	iFaultstate,//% Input vector of Fault state 
	input         	i250mSCE, //% Used for CE
	input  [7:0]  	iMaxNumberValue, //% max number of signals to monitor
	output [3:0]	oFaultStage,
	output [7:0]  	oFaultState //% Output Vector of 5 bits
);

//////////////////////////////////////////////////////////////////////////////////
// Internal Registers
//////////////////////////////////////////////////////////////////////////////////
wire [7:0] wvCnt;
wire wDone;
reg  [7:0] rCurrValue,rNextValue;
reg  [3:0] rFaultStage;


//////////////////////////////////////////////////////////////////////////////////
// Continuos assigment 
//////////////////////////////////////////////////////////////////////////////////
assign oFaultState = rCurrValue;
assign oFaultStage = rFaultStage;

//////////////////////////////////////////////////////////////////////////////////
// Secuencial Logic
//////////////////////////////////////////////////////////////////////////////////


always @(posedge iClk) begin  
	if(~iRst_n) 
		begin
			rCurrValue <= 8'd00;			
		end 
	else if (i250mSCE) 
		begin
			rCurrValue <= rNextValue ;
		end
	else
		begin
			rCurrValue <= rCurrValue; 
		end
end


always @(posedge iClk) 
begin 
	if(iFaultstate[wvCnt]==1'b1)
	begin 
		if(rCurrValue!=wvCnt)
			begin
			    rNextValue <= wvCnt;
		    end
	end

	case (iFaultstate[18:0])
		19'b10000_00000_00_00_0_0_0_0_0: 	rFaultStage <= 4'd14;	//CPU2 VCCIO
		19'b01000_00000_00_00_0_0_0_0_0: 	rFaultStage <= 4'd12;	//CPU2 P1V8
		19'b00100_00000_00_00_0_0_0_0_0: 	rFaultStage <= 4'd12;	//CPU2 VCCANA
		19'b00010_00000_00_00_0_0_0_0_0: 	rFaultStage <= 4'd10;	//CPU2 VCCIN
		19'b00001_00000_00_00_0_0_0_0_0: 	rFaultStage <= 4'd9;	//CPU2 VCCSA
		19'b00000_10000_00_00_0_0_0_0_0: 	rFaultStage <= 4'd8;	//CPU1 VCCIO
		19'b00000_01000_00_00_0_0_0_0_0: 	rFaultStage <= 4'd7;	//CPU1 P1V8
		19'b00000_00100_00_00_0_0_0_0_0: 	rFaultStage <= 4'd7;	//CPU1 VCCANA
		19'b00000_00010_00_00_0_0_0_0_0: 	rFaultStage <= 4'd6;	//CPU1 VCCIN
		19'b00000_00001_00_00_0_0_0_0_0: 	rFaultStage <= 4'd5;	//CPU1 VCCSA
		19'b00000_00000_10_00_0_0_0_0_0: 	rFaultStage <= 4'd4;	//VDDQ CPU2 EFGH
		19'b00000_00000_01_00_0_0_0_0_0: 	rFaultStage <= 4'd4;	//VDDQ CPU2 ABCD
		19'b00000_00000_00_10_0_0_0_0_0: 	rFaultStage <= 4'd3;	//VDDQ CPU1 EFGH
		19'b00000_00000_00_01_0_0_0_0_0: 	rFaultStage <= 4'd3;	//VDDQ CPU1 ABCD
		19'b00000_00000_00_00_1_0_0_0_0: 	rFaultStage <= 4'd2;	//Main VRs
		19'b00000_00000_00_00_0_1_0_0_0: 	rFaultStage <= 4'd1;	//PSU
		19'b00000_00000_00_00_0_0_1_0_0: 	rFaultStage <= 4'd0;	//PCH P1V05
		19'b00000_00000_00_00_0_0_0_1_0: 	rFaultStage <= 4'd0;	//PCH P1v8
		19'b00000_00000_00_00_0_0_0_0_1: 	rFaultStage <= 4'd0;	//BMC
		default: 							rFaultStage <= 4'd15;	//Multiple Faults
	endcase
end

//////////////////////////////////////////////////////////////////////////////////
// Instances 
//////////////////////////////////////////////////////////////////////////////////

// Counters 

counter2 #( .MAX_COUNT(9'd255) ) 
		mMainCounter  
		
(       .iClk         	(iClk), 
		.iRst_n       	(iRst_n),		
		.iCntRst_n  	(!wDone),
		.iCntEn      	(i250mSCE),
		.iLoad        	(1'b1),
		.iSetCnt    	(iMaxNumberValue),		
		.oDone        	(wDone),
		.oCntr         	(wvCnt)
);

endmodule