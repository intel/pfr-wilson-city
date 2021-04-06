//////////////////////////////////////////////////////////////////////////////////
/*!

	\brief    <b>%Led Control  </b>
	\file     led_control.v
	\details    <b>Image of the Block:</b>
				\image html  led_control.png
				

				 <b>Description:</b> \n
				Controls the postcode LED, DIMM_FAULT LED, Fan Fault, 7-Seg1, 7-Seg2 (Common anode)\n\n	

								 
				
	\brief  <b>Last modified</b> 
			$Date:   May 19, 2017 $
			$Author:  David.bolanos@intel.com $			
			Project			: Wilson City RP  
			Group			: BD
	\version    
			 20170519 \b  David.bolanos@intel.com - File creation\n
			 20181901 \b  David.bolanos@intel.com - Modify to adapt to Wilson City, based on Mehlow\n
			   
	\copyright Intel Proprietary -- Copyright \htmlonly <script>document.write(new Date().getFullYear())</script> \endhtmlonly Intel -- All rights reserved
*/
//////////////////////////////////////////////////////////////////////////////////
 


module led_control 
(
	
	input                     	iClk,              //%Clock input 
	input                     	iRst_n,            //%Reset enable on Low

	input[7:0]                	iPostCodeLed,      //%from GSX module, port 80 post code LED

	input[7:0]           		iDimmFltLed_CPU1_1,  //%from GSX module, CPU1 dimm fault LED
	input[7:0]           		iDimmFltLed_CPU1_2,  //%from GSX module, CPU1 dimm fault LED
	input[7:0]           		iDimmFltLed_CPU2_1,  //%from GSX module, CPU1 dimm fault LED
	input[7:0]           		iDimmFltLed_CPU2_2,  //%from GSX module, CPU1 dimm fault LED

	input[7:0]            		iFanFltLed,        //%from GSX module, FAN fault LED

	input 		            	iShowDebug7seg,		//%The FSM st show on Postcode on 7 seg Byte 1 /2	
	input 		            	iShowDebugPostCode,		//%The FSM st show on Postcode 

	input[7:0]            		iDebugPostcode,    //%This input will be show on POSTCODE if the iShowDebugPostCode is on 1.  The propuse of this is show debug information. 

	input                  		iShowMainVer_N,		//%Used to define if main or debug PLD firmware version will be shown. Main when LOW, Debug when HIGH
	input                  		iShowPLDVersion,	//%When Low it will show PLD revision

	input[6:0]					iByteSeg1_RevMajor,	//Major PLD Revision
	input[6:0]					iByteSeg2_RevMinor,	//Minor PLD Revision

	input[6:0]             		iByteSeg1,		    //%This input will be show on Byte 1 if the iShowDebug7seg is on 1.  The propuse of this is show debug information. 
	input[6:0]             		iByteSeg2,			//%This input will be show on Byte 2 if the iShowDebug7seg is on 1.  The propuse of this is show debug information. 
	
	output [7:0]             	oLED_CONTROL,	    //%Data nets
	
	output reg              	oFanFlt_Sel_N,     //%Selector for Fan Fault LED's 
    output reg              	oPostCode_Led_Sel, //%Selector for POSTCODE LED's

    output reg              	oDimmFlt_CPU1_1_Led_Sel, //%Selector for Dimm CPU1 Group1 LED's
    output reg              	oDimmFlt_CPU1_2_Led_Sel, //%Selector for Dimm CPU1 Group2 LED's

    output reg               	oDimmFlt_CPU2_1_Led_Sel, //%Selector for Dimm CPU2 Group1 LED's
    output reg               	oDimmFlt_CPU2_2_Led_Sel, //%Selector for Dimm CPU2 Group2 LED's

    output reg                	oPost7Seg1_Sel_N,  //% Logic active in low 7-Segments Byte 1
    output reg                	oPost7Seg2_Sel_N   //% Logic active in low 7-Segments Byte 2

	);

//////////////////////////////////////////////////////////////////////////////////
// Parameters
//////////////////////////////////////////////////////////////////////////////////

	parameter       LOW                   = 1'b0;
	parameter       HIGH                  = 1'b1;
	
	parameter		ST_POST_LED           = 4'd0;
	parameter		ST_DIMMFAULT0_LED     = 4'd1;
	parameter		ST_DIMMFAULT1_LED     = 4'd2;
	parameter		ST_DIMMFAULT2_LED     = 4'd3;
	parameter		ST_DIMMFAULT3_LED     = 4'd4;
	parameter		ST_POSTSEG1_LED       = 4'd5;
	parameter		ST_POSTSEG1_LED_DECAY = 4'd6;
	parameter		ST_POSTSEG2_LED       = 4'd7;
	parameter		ST_POSTSEG2_LED_DECAY = 4'd8;
	parameter       ST_FANFLT_LED         = 4'h9;
	parameter       T_300HZ_2M            = 13'd3666;  //3.333ms 300Hz
	

//////////////////////////////////////////////////////////////////////////////////
// Internal Signals
//////////////////////////////////////////////////////////////////////////////////
reg [7:0] rLedControl;
reg [6:0] rdata7bits,rv7SegConver;
 
reg [3:0] rCurrLedSt,rNextLedSt;
//300Hz pulse generation
reg[12:0] rCntr_300Hz;
reg rPulse_300Hz;

//////////////////////////////////////////////////////////////////////////////////
// Continuous assignments
//////////////////////////////////////////////////////////////////////////////////
assign oLED_CONTROL= rLedControl;

	
//////////////////////////////////////////////////////////////////////////////////
// Secuencial Logic
//////////////////////////////////////////////////////////////////////////////////
always @(negedge iRst_n or posedge iClk) 
begin
	if(!iRst_n) 
		   rCurrLedSt <= ST_POST_LED;
	else if (rPulse_300Hz) rCurrLedSt <= rNextLedSt; //switch the LED with 300Hz frequency
	else   rCurrLedSt <= rCurrLedSt;
	
end

//////////////////////////////////////////////////////////////////////////////////
// Combinational Logic
//////////////////////////////////////////////////////////////////////////////////	
	
always @(*)
	begin
		case (rCurrLedSt)
				ST_POST_LED: 
				 	begin
						rNextLedSt              = ST_DIMMFAULT0_LED;
						rdata7bits              = 7'h00;
						rLedControl             = iShowDebugPostCode ? iDebugPostcode : iPostCodeLed;
						oPostCode_Led_Sel       = HIGH;
						oDimmFlt_CPU1_1_Led_Sel = LOW;
						oDimmFlt_CPU1_2_Led_Sel = LOW;
						oDimmFlt_CPU2_1_Led_Sel = LOW;
						oDimmFlt_CPU2_2_Led_Sel = LOW;
						oPost7Seg1_Sel_N        = HIGH;
						oPost7Seg2_Sel_N        = HIGH;
						oFanFlt_Sel_N           = HIGH;
					end

				ST_DIMMFAULT0_LED: 
				 	begin 
						rNextLedSt              = ST_DIMMFAULT1_LED;
						rdata7bits              = 7'h00;
						rLedControl             = iDimmFltLed_CPU1_1;
						oPostCode_Led_Sel       = LOW;
						oDimmFlt_CPU1_1_Led_Sel = HIGH;
						oDimmFlt_CPU1_2_Led_Sel = LOW;
						oDimmFlt_CPU2_1_Led_Sel = LOW;
						oDimmFlt_CPU2_2_Led_Sel = LOW;
						oPost7Seg1_Sel_N        = HIGH;
						oPost7Seg2_Sel_N        = HIGH;
						oFanFlt_Sel_N           = HIGH;
					end
				ST_DIMMFAULT1_LED: 
				 	begin 
						rNextLedSt              = ST_DIMMFAULT2_LED;
						rdata7bits              = 7'h00;
						rLedControl             = iDimmFltLed_CPU1_2;
						oPostCode_Led_Sel       = LOW;
						oDimmFlt_CPU1_1_Led_Sel = LOW;
						oDimmFlt_CPU1_2_Led_Sel = HIGH;
						oDimmFlt_CPU2_1_Led_Sel = LOW;
						oDimmFlt_CPU2_2_Led_Sel = LOW;
						oPost7Seg1_Sel_N        = HIGH;
						oPost7Seg2_Sel_N        = HIGH;
						oFanFlt_Sel_N           = HIGH;
					end
				ST_DIMMFAULT2_LED: 
				 	begin 
						rNextLedSt              = ST_DIMMFAULT3_LED;
						rdata7bits              = 7'h00;
						rLedControl             = iDimmFltLed_CPU2_1;
						oPostCode_Led_Sel       = LOW;
						oDimmFlt_CPU1_1_Led_Sel = LOW;
						oDimmFlt_CPU1_2_Led_Sel = LOW;
						oDimmFlt_CPU2_1_Led_Sel = HIGH;
						oDimmFlt_CPU2_2_Led_Sel = LOW;
						oPost7Seg1_Sel_N        = HIGH;
						oPost7Seg2_Sel_N        = HIGH;
						oFanFlt_Sel_N           = HIGH;
					end
				ST_DIMMFAULT3_LED: 
				 	begin 
						rNextLedSt              = ST_POSTSEG1_LED;
						rdata7bits              = 7'h00;
						rLedControl             = iDimmFltLed_CPU2_2;
						oPostCode_Led_Sel       = LOW;
						oDimmFlt_CPU1_1_Led_Sel = LOW;
						oDimmFlt_CPU1_2_Led_Sel = LOW;
						oDimmFlt_CPU2_1_Led_Sel = LOW;
						oDimmFlt_CPU2_2_Led_Sel = HIGH;
						oPost7Seg1_Sel_N        = HIGH;
						oPost7Seg2_Sel_N        = HIGH;
						oFanFlt_Sel_N           = HIGH;
					end
				ST_POSTSEG1_LED: 
				 	begin 
						rNextLedSt              =  ST_POSTSEG1_LED_DECAY;						
						rdata7bits              = !iShowPLDVersion ? iByteSeg1_RevMajor : (iShowDebug7seg ?  iByteSeg1 : {3'b000,iPostCodeLed[7:4]});
						rLedControl             = (!iShowPLDVersion&&!iShowMainVer_N) ? {1'b0,rv7SegConver} : ( (!iShowPLDVersion&&iShowMainVer_N) ? {1'b1,rv7SegConver} : (iShowDebug7seg ? {1'b0,rv7SegConver} : {1'b1,rv7SegConver}) );
						oPostCode_Led_Sel       = LOW;
						oDimmFlt_CPU1_1_Led_Sel = LOW;
						oDimmFlt_CPU1_2_Led_Sel = LOW;
						oDimmFlt_CPU2_1_Led_Sel = LOW;
						oDimmFlt_CPU2_2_Led_Sel = LOW;
						oPost7Seg1_Sel_N        = LOW;
						oPost7Seg2_Sel_N        = HIGH;
						oFanFlt_Sel_N           = HIGH;
					end
				ST_POSTSEG1_LED_DECAY: 
				 	begin //time to wait turn off the led
						rNextLedSt              = ST_POSTSEG2_LED;
						rdata7bits              = 7'h0;
						rLedControl             = iShowDebug7seg ? {1'b0,rv7SegConver} : {1'b1,rv7SegConver};
						oPostCode_Led_Sel       = LOW;
						oDimmFlt_CPU1_1_Led_Sel = LOW;
						oDimmFlt_CPU1_2_Led_Sel = LOW;
						oDimmFlt_CPU2_1_Led_Sel = LOW;
						oDimmFlt_CPU2_2_Led_Sel = LOW;
						oPost7Seg1_Sel_N        = HIGH;
						oPost7Seg2_Sel_N        = HIGH;
						oFanFlt_Sel_N           = HIGH;
					end
				ST_POSTSEG2_LED: 
				 	begin 
						rNextLedSt              = ST_POSTSEG2_LED_DECAY;
						rdata7bits              = !iShowPLDVersion ? iByteSeg2_RevMinor : (iShowDebug7seg ? iByteSeg2 : {3'b000, iPostCodeLed[3:0]});
						rLedControl             = (!iShowPLDVersion&&!iShowMainVer_N) ? {1'b1,rv7SegConver} : ( (!iShowPLDVersion&&iShowMainVer_N) ? {1'b0,rv7SegConver} : (iShowDebug7seg ? {1'b0,rv7SegConver} : {1'b1,rv7SegConver}) );
						oPostCode_Led_Sel       = LOW;
						oDimmFlt_CPU1_1_Led_Sel = LOW;
						oDimmFlt_CPU1_2_Led_Sel = LOW;
						oDimmFlt_CPU2_1_Led_Sel = LOW;
						oDimmFlt_CPU2_2_Led_Sel = LOW;
						oPost7Seg1_Sel_N        = HIGH;
						oPost7Seg2_Sel_N        = LOW;
						oFanFlt_Sel_N           = HIGH;
					end
				ST_POSTSEG2_LED_DECAY: 
				 	begin //time to wait turn off the led
						rNextLedSt              = ST_FANFLT_LED;
						rdata7bits              = 7'h0;
						rLedControl             = iShowDebug7seg ? {1'b0,rv7SegConver} : {1'b1,rv7SegConver};
						oPostCode_Led_Sel       = LOW;
						oDimmFlt_CPU1_1_Led_Sel = LOW;
						oDimmFlt_CPU1_2_Led_Sel = LOW;
						oDimmFlt_CPU2_1_Led_Sel = LOW;
						oDimmFlt_CPU2_2_Led_Sel = LOW;
						oPost7Seg1_Sel_N        = HIGH;
						oPost7Seg2_Sel_N        = HIGH;
						oFanFlt_Sel_N           = HIGH;
					end
			    ST_FANFLT_LED: 
					begin 
						rNextLedSt              = ST_POST_LED;
						rdata7bits              = 7'h0;
						rLedControl             = iFanFltLed;
						oPostCode_Led_Sel       = LOW;
						oDimmFlt_CPU1_1_Led_Sel = LOW;
						oDimmFlt_CPU1_2_Led_Sel = LOW;
						oDimmFlt_CPU2_1_Led_Sel = LOW;
						oDimmFlt_CPU2_2_Led_Sel = LOW;
						oPost7Seg1_Sel_N        = HIGH;
						oPost7Seg2_Sel_N        = HIGH;
						oFanFlt_Sel_N           = LOW;
						
					end
				default:  
					begin
						rNextLedSt              = ST_POST_LED;
						rdata7bits              = 7'h0;
						rLedControl             = iShowDebugPostCode ? iDebugPostcode : iPostCodeLed;
						oPostCode_Led_Sel       = HIGH;
						oDimmFlt_CPU1_1_Led_Sel = LOW;
						oDimmFlt_CPU1_2_Led_Sel = LOW;
						oDimmFlt_CPU2_1_Led_Sel = LOW;
						oDimmFlt_CPU2_2_Led_Sel = LOW;
						oPost7Seg1_Sel_N        = HIGH;
						oPost7Seg2_Sel_N        = HIGH;
						oFanFlt_Sel_N           = HIGH;
					end
		endcase
    end
	

	

	

//////////////////////////////////////////////////////////////////////////////////
// Local function 
//////////////////////////////////////////////////////////////////////////////////

	
	always @ (posedge iClk or negedge iRst_n)
		if (! iRst_n)
			begin
				rCntr_300Hz  <= 13'b0;
				rPulse_300Hz <= LOW;
			end
		else if ( rCntr_300Hz < T_300HZ_2M)
			begin
				rCntr_300Hz  <= rCntr_300Hz + 1'b1;
				rPulse_300Hz <= LOW;
			end
		else begin
				rCntr_300Hz  <= 1'b0;
				rPulse_300Hz <= HIGH;
			end

	always @ (*)
		begin
			if( iRst_n == LOW)
				           rv7SegConver = 7'b1000000;
			else
				case (rdata7bits)
					7'd0:  rv7SegConver = 7'b1000000; //0 
					7'd1:  rv7SegConver = 7'b1111001; //1 
					7'd2:  rv7SegConver = 7'b0100100; //2
					7'd3:  rv7SegConver = 7'b0110000; //3 
					7'd4:  rv7SegConver = 7'b0011001; //4
					7'd5:  rv7SegConver = 7'b0010010; //5 .
					7'd6:  rv7SegConver = 7'b0000010; //6 
					7'd7:  rv7SegConver = 7'b1111000; //7 
					7'd8:  rv7SegConver = 7'b0000000; //8 
					7'd9:  rv7SegConver = 7'b0011000; //9 
					7'd10: rv7SegConver = 7'b0001000; //A 
					7'd11: rv7SegConver = 7'b0000011; //B 
					7'd12: rv7SegConver = 7'b1000110; //C 
					7'd13: rv7SegConver = 7'b0100001; //D 
					7'd14: rv7SegConver = 7'b0000110; //E  
					7'd15: rv7SegConver = 7'b0001110; //F 
					7'd16: rv7SegConver = 7'b0111111; //- 
					default: 
					       rv7SegConver = 7'b1000000; //0 
				endcase
	end 
	
	


	
	
	
endmodule