//////////////////////////////////////////////////////////////////////////////////
/*!

	\brief    <b>%Power Good Logic </b>
	\file     PwrgdLogic.v
	\details    <b>Image of the Block:</b>
				\image html  PwrgdLogic.png
                
				 <b>Description:</b> \n
				PWRGD_DRAMPWRGD_CPU, PWRGD_CPU_LVC3 PWRGD_PCH_PWROK and PWRGD_SYS_PWROK are determined by this module.\n

				According to Powergood / Reset Block and Timing Diagram, 
                PWRGD_PCH_PWROK is only active after all PCH, BMC, Mem and CPU rails are ready also 100mS delay for PS_PWROK-/n

                PWRGD_SYS_PWROK  is a copy of     PWRGD_PCH_PWROK without delay.  

                PWRGD_DRAMPWRGD_CPU  is  AND condition of FM_SLPS4_N and PWRGD_VDDQ Vr's

                PWRGD_CPU_LVC3 is a copy of PWRGD_CPUPWRGD.
				

				
				\image html pwrgd_diagram_PCH.png		
				\n		

				\image html pwrgd_diagram_Sys.png	
				\n 
				
	\brief  <b>Last modified</b> 
			$Date:   Jan 19, 2018 $
			$Author:  David.bolanos@intel.com $			
			Project			: Wilson City RP  
			Group			: BD
	\version    
			 20181901 \b  David.bolanos@intel.com - File creation\n
			 20181405 \b  jorge.juarez.campos@intel.com - ADR Module Fix\n
			   
	\copyright Intel Proprietary -- Copyright \htmlonly <script>document.write(new Date().getFullYear())</script> \endhtmlonly Intel -- All rights reserved
*/
//////////////////////////////////////////////////////////////////////////////////


module PwrgdLogic
(
	input			iClk,//%Clock input
	input			iRst_n,//%Reset enable on high
	input 		    i1mSCE, //% 1 mS Clock Enable
	
	input           iMemPwrgd,//% Memory Pwrgd generate Mem_Seq.v
	input           iCpuPwrgd,//% Cpu Pwrgd generate Cpu_Seq.v
	input           iBmcPwrgd,//% Bmc Pwrgd generate Bmc_Seq.v
	input           iPchPwrgd,//% Pch Pwrgd generate Pch_Seq.v	
	input           FM_SLPS3_N,//% SLP3# from PCH
	input           FM_SLPS4_N,//% SLP4# from PCH
	input           PWRGD_PS_PWROK_DLY,//% PWRGD with generate by Power Supply with Delay.
	input           DBP_SYSPWROK,  //%DBP_SYSPWROK signal from DBP Connector
	input           PWRGD_CPUPWRGD, //& CPUPWRGD from PCH

	output reg      PWRGD_DRAMPWRGD_CPU, 
	output 			PWRGD_CPU_LVC3,
    output          PWRGD_PCH_PWROK, //% Pwrgd PCH 
    output reg      PWRGD_SYS_PWROK //% Pwrgd SYS 
);

//////////////////////////////////////////////////////////////////////////////////
// Parameters
//////////////////////////////////////////////////////////////////////////////////
parameter  LOW =1'b0;
parameter  HIGH=1'b1;

//////////////////////////////////////////////////////////////////////////////////
// Internal Signals
//////////////////////////////////////////////////////////////////////////////////
reg rPWRGD_PCH_PWROK, rPWRGD_PCH_PWROK_ff;


//////////////////////////////////////////////////////////////////////////////////
// Continuous assignments
//////////////////////////////////////////////////////////////////////////////////
assign PWRGD_CPU_LVC3 = PWRGD_CPUPWRGD;	//This to help Modular design timing requirements for CPUs PWRGD


//////////////////////////////////////////////////////////////////////////////////
// Secuencial Logic
//////////////////////////////////////////////////////////////////////////////////
reg rPWRGD_PS_PWROK_q;
reg rPCHPwrokNegEdge;

always @ ( posedge iClk) 
begin 
	if (  !iRst_n  )   
	begin
		rPWRGD_PCH_PWROK   	<= LOW;
		rPWRGD_PCH_PWROK_ff	<= LOW;
		PWRGD_SYS_PWROK    	<= LOW;
		PWRGD_DRAMPWRGD_CPU	<= LOW;
		rPWRGD_PS_PWROK_q	<= LOW;
		rPCHPwrokNegEdge 	<= LOW;
	end
	else 
	begin	
		rPWRGD_PCH_PWROK   <=  ( rPCHPwrokNegEdge ) ? PWRGD_PS_PWROK_DLY : (iMemPwrgd && iCpuPwrgd && iBmcPwrgd && iPchPwrgd && FM_SLPS3_N && PWRGD_PS_PWROK_DLY);
		rPWRGD_PCH_PWROK_ff<=	rPWRGD_PCH_PWROK;
		PWRGD_SYS_PWROK    <=  PWRGD_PCH_PWROK && DBP_SYSPWROK;	
		PWRGD_DRAMPWRGD_CPU<=  FM_SLPS4_N && iMemPwrgd;
		rPWRGD_PS_PWROK_q	<= PWRGD_PS_PWROK_DLY;
		rPCHPwrokNegEdge    <= (!PWRGD_PS_PWROK_DLY && rPWRGD_PS_PWROK_q && FM_SLPS3_N) ?  1'b1 : rPCHPwrokNegEdge;
	end
end  


//////////////////////////////////////////////////////////////////////
// Instances 
//////////////////////////////////////////////////////////////////////
SignalValidationDelay#
(
    .VALUE                  ( 1'b1 ),
    .TOTAL_BITS             ( 2'd3 ),
    .POL                    ( 1'b1 )
)mPWRGD_PCH_Pwrok_DLY         
(           
    .iClk                   ( iClk ),
    .iRst                   ( ~iRst_n ),
    .iCE                    ( i1mSCE ),
    .ivMaxCnt               ( 3'd6 ),        //6ms delay
    .iStart                 ( rPWRGD_PCH_PWROK || rPWRGD_PCH_PWROK_ff),
    .oDone                  ( PWRGD_PCH_PWROK )
);

endmodule
