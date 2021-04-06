
//////////////////////////////////////////////////////////////////////////////////
/*!

	\brief    <b>%PERST Logic  </b>
	\file     Rst_Perst.v
	\details    <b>Image of the Block:</b>
				\image html Rst_Perst.png

				 <b>Description:</b> \n
				The module contain all logic needed for Perst RST\n\n				

				 
				
	\brief  <b>Last modified</b> 
			$Date:   Sept 6, 2016 $
			$Author:  David.bolanos@intel.com $			
			Project			: Mehlow 
			Group			: BD
	\version    
			 20160609 \b  David.bolanos@intel.com - File creation\n
			   
	\copyright Intel Proprietary -- Copyright \htmlonly <script>document.write(new Date().getFullYear())</script> \endhtmlonly Intel -- All rights reserved
*/
//////////////////////////////////////////////////////////////////////////////////

module  Rst_Perst # 
(
    parameter   NUM_PCIE_SIGNALS = 7 //% Number of PCIE resets <br>
)
(
    input           iClk, //% System Clock - 2MHz    
    input           iRst_n,//% System asynchronous reset  
    	//% PERST Table control
    input   [NUM_PCIE_SIGNALS-1:0] ivOverride_Enable,//% Override Enable 
	input   [NUM_PCIE_SIGNALS-1:0] ivOvrValues,//% Override values
	input   [NUM_PCIE_SIGNALS-1:0] ivDefaultValues, //% Default values
	
    output  [NUM_PCIE_SIGNALS-1:0] ovRstPCIePERst_n //% Output PCIE Resets for PERST Table
);

//////////////////////////////////////////////////////////////////////
// Instances 
//////////////////////////////////////////////////////////////////////
//% CPU PCIe Reset output generation - PltRst or CPUPwrGd selection
SignalOverrideControl #
(
    .TOTAL_SIGNALS ( NUM_PCIE_SIGNALS )
)mPERst
(
    .iClk                   ( iClk ),
    .iRst                   ( ~iRst_n ),
    .ivOvrEnable            ( ivOverride_Enable ),
    .ivOvrValue             ( ivOvrValues ),
    .ivSignal               ( ivDefaultValues ),
    .ovSignal               ( ovRstPCIePERst_n )
);

endmodule

