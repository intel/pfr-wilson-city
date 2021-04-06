
//////////////////////////////////////////////////////////////////////////////////
/*!

	\brief    <b>%PCH Sequence Control </b>
	\file     Pch_Seq.v
	\details    <b>Image of the Block:</b>
				\image html Pch_Seq.png

				 <b>Description:</b> \n				
				
				The module controls the power sequence for PCH VR's, fault condition of each VR and RSMRST#. 
                This module does not depend on Mstr_Seq.v  to enable the VR's
                the PCH VR's are enabled on S5 state.\n

				\image html Pch_Seq_diagram.png

				\n

				\image html Pch_Seq_diagramrst.png
				
	\brief  <b>Last modified</b> 
			$Date:   Sept 5, 2016 $
			$Author:  David.bolanos@intel.com $			
			Project			: Wilson City RP
			Group			: BD
	\version    
			 20160509 \b  David.bolanos@intel.com - File creation\n
			   
	\copyright Intel Proprietary -- Copyright \htmlonly <script>document.write(new Date().getFullYear())</script> \endhtmlonly Intel -- All rights reserved
*/
//////////////////////////////////////////////////////////////////////////////////


module Pch_Seq 
(

	input			iClk,//%Clock input 
	input			iRst_n,//%Reset enable on low
	input 			i1mSCE, //% 1 mS Clock Enable
	input           i1uSCE,  //% 1 uS Clock Enable
	input           iGoOutFltSt,//% Go out fault state.

	input           FM_PCH_PRSNT_N,//% Detect if the PCH is present 
	input 			PWRGD_P3V3_AUX,//% P3V3_AUX VR PWRGD
	input           FM_SLP_SUS_N,//% SLP_SUS
	input           RST_SRST_BMC_N, //% RST BMC

	input 			PWRGD_PCH_P1V8_AUX,//% PCH VR PWRGD P1V8
	input 			PWRGD_PCH_P1V05_AUX,//% PCH VR PWRGD P1V05

	input 			RST_DEDI_BUSY_PLD_N, //% //Dediprog Detection Support 

	output 	reg		FM_PCH_P1V8_AUX_EN,//% PCH VR PWRGD P1V8

	output  reg     RST_RSMRST_N,//% RSMRST# 
	output  reg     oPchPwrgd, //% PWRGD of all PCH VR's 
	output  reg     oPchPwrFltP1V8,//% Fault PCH P1V8
	output  reg     oPchPwrFltP1V05,//% Fault PCH P1V05
	output  reg     oPchPwrFlt//% Fault PCH VR's
);
//////////////////////////////////////////////////////////////////////////////////
// Parameters
//////////////////////////////////////////////////////////////////////////////////
parameter  LOW =1'b0;
parameter  HIGH=1'b1;


//////////////////////////////////////////////////////////////////////////////////
// Internal Signals
//////////////////////////////////////////////////////////////////////////////////
wire wPwrgd_All_VRs_dly;
reg  rInternalEnableP1v8;
reg  rPwrgdPchP1V8_ff1,rPwrgdPchP1V05_ff1;
wire wFM_PCH_P1V8_AUX_EN;
reg wRST_DEDI_BUSY_PLD_N;


//////////////////////////////////////////////////////////////////////////////////
// Secuencial Logic
//////////////////////////////////////////////////////////////////////////////////
always @( posedge iClk) 
begin 
if  (!iRst_n)  
	begin
		oPchPwrgd              <= LOW;	
		FM_PCH_P1V8_AUX_EN     <= LOW;			
		oPchPwrFlt             <= LOW;	
		RST_RSMRST_N           <= LOW;

		rInternalEnableP1v8    <= LOW;

		rPwrgdPchP1V8_ff1      <= LOW;
		rPwrgdPchP1V05_ff1     <= LOW;

		oPchPwrFltP1V8         <= LOW;
		oPchPwrFltP1V05        <= LOW;

	end
else if ( iGoOutFltSt )
	begin				
		oPchPwrFlt             <= LOW;
		oPchPwrFltP1V8         <= LOW;
		oPchPwrFltP1V05        <= LOW;
	end
else begin	
		oPchPwrgd              <= PWRGD_PCH_P1V8_AUX && PWRGD_PCH_P1V05_AUX && PWRGD_P3V3_AUX;
		FM_PCH_P1V8_AUX_EN     <= wFM_PCH_P1V8_AUX_EN && rInternalEnableP1v8;

		RST_RSMRST_N           <= wPwrgd_All_VRs_dly && RST_DEDI_BUSY_PLD_N;

		rInternalEnableP1v8    <= PWRGD_P3V3_AUX && FM_SLP_SUS_N && !FM_PCH_PRSNT_N;
		
		rPwrgdPchP1V8_ff1      <= PWRGD_PCH_P1V8_AUX;
		rPwrgdPchP1V05_ff1     <= PWRGD_PCH_P1V05_AUX;


		oPchPwrFltP1V8         <= (FM_PCH_P1V8_AUX_EN  && !PWRGD_PCH_P1V8_AUX  && rPwrgdPchP1V8_ff1)  ? HIGH: oPchPwrFltP1V8;
		oPchPwrFltP1V05        <= (FM_PCH_P1V8_AUX_EN  && !PWRGD_PCH_P1V05_AUX && rPwrgdPchP1V05_ff1) ? HIGH: oPchPwrFltP1V05;

	    oPchPwrFlt             <= (PWRGD_P3V3_AUX && (oPchPwrFltP1V8 || oPchPwrFltP1V05)) ? HIGH: oPchPwrFlt;  //detect PCH VR fault 	
	    wRST_DEDI_BUSY_PLD_N   <= RST_DEDI_BUSY_PLD_N;
	end 
end 



//////////////////////////////////////////////////////////////////////
// Instances
//////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////
//% 11ms Delay for power UP  RSMRST
//
/*
We need to make sure the RSMRST signal to take into account the SRST_BMC; the 11ms timer should be trigger after all signals are valid then 11ms after RSMRST_N is high.

This required by AST2500, at least 10ms Between SRST and eSPI_RST. Since eSPI_RST is dependent on RSMRST_N, then the only way for control eSPI_RST is to hold RSMRST_N
*/
SignalValidationDelay#
(
    .VALUE                  ( 1'b1 ),
    .TOTAL_BITS             ( 4'd5 ),
    .POL                    ( 1'b1 )
)mRSMRST        
(           
    .iClk                   ( iClk ),
    .iRst                   ( ~iRst_n || (wRST_DEDI_BUSY_PLD_N && ~RST_DEDI_BUSY_PLD_N)),	//Restart timer
    .iCE                    ( i1mSCE ),
    .ivMaxCnt               ( 5'd20 ),        //20ms delay due to 11ms requirement
    .iStart                 ( oPchPwrgd && FM_SLP_SUS_N && RST_SRST_BMC_N),
    .oDone                  ( wPwrgd_All_VRs_dly )
 );


//% Enable delay down (1uS) 
//
SignalValidationDelay#
(
    .VALUE                  ( 1'b0 ),
    .TOTAL_BITS             ( 2'd2 ),
    .POL                    ( 1'b0 )
)mP1V8_AUX_EN       
(           
    .iClk                   ( iClk ),
    .iRst                   ( ~iRst_n),
    .iCE                    ( i1uSCE ),
    .ivMaxCnt               ( 2'd1 ),        //1us delay
    .iStart                 ( rInternalEnableP1v8),
    .oDone                  ( wFM_PCH_P1V8_AUX_EN )
 );


endmodule