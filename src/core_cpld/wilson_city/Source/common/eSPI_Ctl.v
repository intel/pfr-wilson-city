
//////////////////////////////////////////////////////////////////////////////////
/*!

	\brief    <b>%eSPI Control </b>
	\file     eSPI_Ctl.v
	\details    <b>Image of the Block:</b>
				\image html eSPI_Ctl.png

				 <b>Description:</b> \n
	            The module is used to control the mux used for determine if the system read the HW strap or enable a functionality.\n\n 

				\image html eSPI_Ctl_diagram.png			

				 
				
	\brief  <b>Last modified</b> 
			$Date:   Sept 19, 2016 $
			$Author:  David.bolanos@intel.com $			
			Project			: Mehlow  
			Group			: BD
	\version    
			 20161909 \b  David.bolanos@intel.com - File creation\n
			   
	\copyright Intel Proprietary -- Copyright \htmlonly <script>document.write(new Date().getFullYear())</script> \endhtmlonly Intel -- All rights reserved
*/
//////////////////////////////////////////////////////////////////////////////////

module eSPI_Ctl
(
	input			 iClk,//%Clock input
	input			 iRst_n,//%Reset enable on high
	input            i1uSCE,//% 1 uS Clock Enable 
	input            RST_SRST_BMC_N,//% SRST RST# from BMC
	input            iRsmRst_N,//% RSM RST# from PCH

    output  reg      oEspiMuxPCHSel,//% Mux selector control for PCH
    output  reg      oEspiMuxBMCSel//% Mux selector control for BMC
);


//////////////////////////////////////////////////////////////////////////////////
// Parameters
//////////////////////////////////////////////////////////////////////////////////

	parameter  LOW =1'b0;
	parameter  HIGH=1'b1;


//////////////////////////////////////////////////////////////////////////////////
// Internal Signals
//////////////////////////////////////////////////////////////////////////////////



wire            wPCHESPIMuxSel;
wire            wBMCStrapSample;

//////////////////////////////////////////////////////////////////////////////////
// Continuous assignments
//////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////
// Secuencial Logic
//////////////////////////////////////////////////////////////////////////////////

always @ ( posedge iClk) 
	begin 
		if (  !iRst_n  )   
			begin				
				oEspiMuxPCHSel      <= LOW;
				oEspiMuxBMCSel      <= LOW;
			end					
		
		else 
			begin				
				oEspiMuxPCHSel	 <=  wPCHESPIMuxSel;
				oEspiMuxBMCSel	 <=  wBMCStrapSample;
			end
	end  



//////////////////////////////////////////////////////////////////////
// Instances 
///////////////////////////////////////////////////////////////////////

//% RSMRst Sample delay (2uS) 

uDelay #
(
	.TOTAL_BITS ( 2'd2 )
)mDelayedRsmRst
(
	.iClk					( iClk ),
	.iRst					( !iRst_n ),
	.iCE					( i1uSCE ),
	.iSignal				( iRsmRst_N ),
	.oDelayedIn				( wPCHESPIMuxSel )
);



//% BMCRst Sample delay (2uS) 
//
uDelay #
(
	.TOTAL_BITS ( 2'd2 )
)mDelayedBMCRst
(
	.iClk					( iClk ),
	.iRst					( !iRst_n ),
	.iCE					( i1uSCE ),
	.iSignal				( RST_SRST_BMC_N ),
	.oDelayedIn				( wBMCStrapSample )
);
endmodule 
