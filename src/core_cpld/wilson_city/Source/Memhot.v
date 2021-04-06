//////////////////////////////////////////////////////////////////////////////////
/*!

	\brief    <b>%Memhot </b>
	\file     Memhot.v
	\details    <b>Image of the Block:</b>
				\image html Memhot_module.png

<b>Description:</b> \n
The PLD toggles the MEMHOT_IN# to force the memory controller to go into throttling mode. 
This happens when the system reaches a certain power / thermal threshold. 
In the Whitley platform implementation, VRHOT# is an output from the memory subsystem VR13.0 controller,
 which is capable of throttling the ICX memory controller via MEMHOT_IN#.\n

PLD will assert MEMHOT_IN# if a PMBUS Alert is asserted from the system PSU. 
The ME can also force a MEMHOT_IN# event via SYS_THROTTLE.
 Management firmware monitors the memory subsystem VR13.0 VRHOT# and creates a SEL event if VRHOT# is asserted.
	
 \image html Memhot.PNG 			


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


module Memhot
(
	input      iClk, //%Clock input 2 Mhz   
    input      iRst_n,//%Reset enable on low  
    input      PWRGD_SYS_PWROK,	
    input      IRQ_PVDDQ_ABCD_VRHOT_LVC3_N,
    input      IRQ_PVDDQ_EFGH_VRHOT_LVC3_N,
    input      FM_SYS_THROTTLE_LVC3, 
    input      FM_SKTOCC_LVT3_N,

    output reg FM_MEMHOT_IN//% The logic is inverted using a FET
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
				FM_MEMHOT_IN         <= LOW;
			end		
		else begin
				FM_MEMHOT_IN         <= PWRGD_SYS_PWROK ? (~ (!FM_SYS_THROTTLE_LVC3 && IRQ_PVDDQ_ABCD_VRHOT_LVC3_N && IRQ_PVDDQ_EFGH_VRHOT_LVC3_N )) : LOW;
			end 
	end 


endmodule

