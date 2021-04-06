//////////////////////////////////////////////////////////////////////////////////
/*!

	\brief    <b>%Prochot </b>
	\file     Prochot.v
	\details    <b>Image of the Block:</b>
				\image html Prochot_dia.png

				 <b>Description:</b> \n
				The module contain all logic needed for Prochot# logic\n\n	

				\section Prochot	Prochot# Logic 
				PROCHOT# is a bidirectional pin used to signal that a processor internal temperature has exceeded a predetermined limit
				 and is self-throttling, or as a method of throttling the processor by an external source.  
				PROCHOT# is asserted by the processor when the TCC is active. 
				When any DTS temperature reaches the TCC activation temperature, the PROCHOT# signal will be asserted. 
				
				An external source can also toggle the PROCHOT# (INPUT mode) to force the Processor CPU to go into throttling mode.  
				This usually happens when the system reaches a certain thermal threshold.  In the motherboard implementation, 
				3 events can trigger forced PROCHOT# activation and they are VRHOT# by CPU VR, PMBUS_ALERT_N by PSU_Seq.v and SYS_THROTTLE_N by ME FW.  
				Firmware monitors VRHOT# and creates a SEL event if VRHOT# is asserted. There is no fan action as a result of the BMC seeing VRHOT.
				\n
				\image html Prochot.png
				
			

	\brief  <b>Last modified</b> 
			$Date:   Jan 19, 2018 $
			$Author:  David.bolanos@intel.com $			
			Project			: Wilson City RP 
			Group			: BD
	\version    
			 20160509 \b  David.bolanos@intel.com - File creation\n
			 20181901 \b  David.bolanos@intel.com - Mofify to adapt to Wilson RP, leverage from Mehlow\n 
			   
	\copyright Intel Proprietary -- Copyright \htmlonly <script>document.write(new Date().getFullYear())</script> \endhtmlonly Intel -- All rights reserved
*/
//////////////////////////////////////////////////////////////////////////////////


module Prochot
(
	input      iClk, //%Clock input 2 Mhz   
    input      iRst_n,//%Reset enable on low  
    input      PWRGD_SYS_PWROK,	
    input      FM_PVCCIN_PWR_IN_ALERT_N,
    input      IRQ_PVCCIN_VRHOT_LVC3_N,
    input      FM_SYS_THROTTLE_LVC3, 
    input      FM_SKTOCC_LVT3_N,

    output reg FM_PROCHOT_LVC3_N
);


//////////////////////////////////////////////////////////////////////////////////
// Parameters
//////////////////////////////////////////////////////////////////////////////////

	parameter  LOW =1'b0;
	parameter  HIGH=1'b1;

//////////////////////////////////////////////////////////////////////////////////
// Internal Signals
//////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////
// Continuous assignments
//////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////
// Secuencial Logic
//////////////////////////////////////////////////////////////////////////////////

always @( posedge iClk) 
	begin 
		if  (!iRst_n || FM_SKTOCC_LVT3_N)  
			begin
				FM_PROCHOT_LVC3_N         <= HIGH;
			end		
		else begin
				FM_PROCHOT_LVC3_N         <= PWRGD_SYS_PWROK ? ( (!FM_SYS_THROTTLE_LVC3 && FM_PVCCIN_PWR_IN_ALERT_N && IRQ_PVCCIN_VRHOT_LVC3_N )) : HIGH;
			end 
	end 


endmodule

