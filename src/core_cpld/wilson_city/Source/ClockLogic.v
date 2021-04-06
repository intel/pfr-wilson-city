//////////////////////////////////////////////////////////////////////////////////
/*!

	\brief    <b>%Clock Logic </b>
	\file     ClockLogic.v
	\details    <b>Image of the Block:</b>
				\image html  ClockLogic.png

				This module allow to turn on the clock generator:\n

				FM_PLD_CLKS_OE_N used enable the output. \n
				FM_CPU1/2_MCP_CLK_OE_N used to turn on MCP  clocks


    \brief  <b>Last modified</b>
            $Date:   Jun 19, 2018 $
            $Author:  David.bolanos@intel.com $
            Project            : Wilson City RP
            Group            : BD
    \version
            20180118 \b	David.bolanos@intel.com - File creation\n
			20180522 \b	jorge.juarez.campos@intel.com - Modified for different MCP packages support\n
			20190813 \b	ricardo.ramos.contreras@intel.com -Changed comments, enables Ascyncronous reset

	\copyright Intel Proprietary -- Copyright \htmlonly <script>document.write(new Date().getFullYear())</script> \endhtmlonly Intel -- All rights reserved
*/
//////////////////////////////////////////////////////////////////////////////////

module ClockLogic
(
	input	iClk,				//%Clock
	input	iRst_n,				//%Reset

	input 	iMainVRPwrgd,		//% PWRGD of Main VR's from MainVR_Seq.v module
	input 	iMCP_EN_CLK,		//%Multi Chip Package Clock Enable
	input 	PWRGD_PCH_PWROK,

	output 	FM_PLD_CLKS_OE_N,	//OE active low
	output 	FM_CPU_BCLK5_OE_N	//OE active low
);

parameter  LOW =1'b0;
parameter  HIGH=1'b1;

reg	rFM_PLD_CLKS_OE;
reg	rFM_CPU_BCLK5_OE;

always @(posedge iClk, negedge iRst_n)
begin
	if (!iRst_n)
	begin
		rFM_PLD_CLKS_OE		<= LOW;
		rFM_CPU_BCLK5_OE	<= LOW;
	end
	else
	begin//Take into account the inversion at the end
		rFM_CPU_BCLK5_OE	<= PWRGD_PCH_PWROK && iMainVRPwrgd && iMCP_EN_CLK;
		rFM_PLD_CLKS_OE		<= iMainVRPwrgd && PWRGD_PCH_PWROK;
	end
end
//Outputs inverted to enable asyncronous reset and comply with spec
assign FM_PLD_CLKS_OE_N		= !rFM_PLD_CLKS_OE;
assign FM_CPU_BCLK5_OE_N	= !rFM_CPU_BCLK5_OE;

endmodule
