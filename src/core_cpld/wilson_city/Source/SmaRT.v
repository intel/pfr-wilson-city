//////////////////////////////////////////////////////////////////////////////////
/*!

	\brief    <b>%SmaRT </b>
	\file     SmaRT.v
	\details    <b>Image of the Block:</b>
				\image html smart.png

				 <b>Description:</b> \n
				The module contain all logic needed for SmaRT logic\n\n	

                The logic for generate FM_THERMTRIP_DLY signal is the next:

				\image html thermlogic.PNG
				
	\brief  <b>Last modified</b> 
			$Date:   Jan 19, 2018 $
			$Author:  David.bolanos@intel.com $			
			Project			: Wilson City RP  
			Group			: BD
	\version    
			 20160609 \b  David.bolanos@intel.com - File creation\n
			 20181901 \b  David.bolanos@intel.com - Mofify to adapt to Wilson RP, leverage from Mehlow\n 
			   
	\copyright Intel Proprietary -- Copyright \htmlonly <script>document.write(new Date().getFullYear())</script> \endhtmlonly Intel -- All rights reserved
*/
//////////////////////////////////////////////////////////////////////////////////

module SmaRT
(
	input      iClk, //%Clock input 2 Mhz   
    input      iRst_n,//%Reset enable on low
    input      IRQ_SML1_PMBUS_PLD_ALERT_N,   
    input      FM_PMBUS_ALERT_B_EN,
    input      FM_THROTTLE_N,  
    input      PWRGD_SYS_PWROK,	

    output reg FM_SYS_THROTTLE_LVC3//% The logic is inverted using a FET.
);


//////////////////////////////////////////////////////////////////////////////////
// Parameters
//////////////////////////////////////////////////////////////////////////////////
parameter  LOW =1'b0;
parameter  HIGH=1'b1;

//////////////////////////////////////////////////////////////////////////////////
// Internal Signals
//////////////////////////////////////////////////////////////////////////////////
//Generate IRQ_SML1_PMBUS_ALERT_BUF_N to replace the On-Board Logic
reg  rIrqSml1_PMbus_Alert_Buf_N;


//////////////////////////////////////////////////////////////////////////////////
// Continuous assignments
//////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////
// Secuencial Logic
//////////////////////////////////////////////////////////////////////////////////

always @( posedge iClk) 
begin 
	if  (!iRst_n)  
	begin				
		rIrqSml1_PMbus_Alert_Buf_N <= HIGH;
		FM_SYS_THROTTLE_LVC3       <= LOW;
	end
	else 
	begin
		rIrqSml1_PMbus_Alert_Buf_N <= FM_PMBUS_ALERT_B_EN ? IRQ_SML1_PMBUS_PLD_ALERT_N : HIGH;
		FM_SYS_THROTTLE_LVC3       <= PWRGD_SYS_PWROK ? (~ (rIrqSml1_PMbus_Alert_Buf_N && FM_THROTTLE_N)) : LOW;
	end 
end 

endmodule
