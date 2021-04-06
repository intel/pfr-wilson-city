//////////////////////////////////////////////////////////////////////////////////
/*!
    \brief      <b>PFR byPass code</b> 
    \details    PFR byPass code.\n
    \file       PFR_ByPass.v
    \author     amr/david.bolanos@intel.com
    \date       Dec 1th, 2017
    \brief      $RCSfile: PFR_ByPass.v.rca $
                $Date:  $
                $Author:  $
                $Revision:  $
                $Aliases: $
                $Log: WolfPassTop.v.rca $
                
                <b>Project:</b> PFR\n
                <b>Group:</b> BD\n
                <b>Testbench:</b>
                <b>Resources:</b>   <ol>
                                        <li>Max 10
                                    </ol>  
    \copyright  Intel Proprietary -- Copyright \htmlonly <script>document.write(new Date().getFullYear())</script> \endhtmlonly Intel -- All rights reserved
*/
//////////////////////////////////////////////////////////////////////////////////
module PFR_ByPass
(
	input   PWRGD_DSW_PWROK_R, 
	input   FM_BMC_BMCINIT, 

	input   FM_ME_PFR_1,
	input   FM_ME_PFR_2,

	input   FM_PFR_PROV_UPDATE_N, 
	input   FM_PFR_DEBUG_MODE_N, 
	input   FM_PFR_FORCE_RECOVERY_N, 

	inout   RST_PFR_EXTRST_N,

	input	FP_ID_BTN_N,
	output	FP_ID_BTN_PFR_N,

	output	FP_ID_LED_PFR_N,
	input	FP_ID_LED_N,

	output	FP_LED_STATUS_AMBER_PFR_N,
	input	FP_LED_STATUS_AMBER_N,

	output	FP_LED_STATUS_GREEN_PFR_N,
	input	FP_LED_STATUS_GREEN_N,

	//  SPI
	output  FM_SPI_PFR_PCH_MASTER_SEL,
	output  FM_SPI_PFR_BMC_BT_MASTER_SEL,

	output  RST_SPI_PFR_BMC_BOOT_N,
	output  RST_SPI_PFR_PCH_N,

	input   SPI_BMC_BOOT_CS_N, 
	output  SPI_PFR_BMC_BT_SECURE_CS_N, 

	input   SPI_PCH_BMC_PFR_CS0_N, 
	output  SPI_PFR_PCH_BMC_SECURE_CS0_N, 

	input   SPI_PCH_CS1_N, 
	output  SPI_PFR_PCH_SECURE_CS1_N
);

parameter  	LOW =1'b0;
parameter  	HIGH=1'b1;  
parameter  	Z=1'bz;

assign 		RST_PFR_EXTRST_N 				= HIGH;
assign 		FP_ID_BTN_PFR_N 				= FP_ID_BTN_N;
assign 		FP_ID_LED_PFR_N 				= FP_ID_LED_N;
assign 		FP_LED_STATUS_AMBER_PFR_N		= FP_LED_STATUS_AMBER_N;
assign 		FP_LED_STATUS_GREEN_PFR_N		= FP_LED_STATUS_GREEN_N;
// SPI 
assign 		FM_SPI_PFR_PCH_MASTER_SEL 		= LOW;
assign 		FM_SPI_PFR_BMC_BT_MASTER_SEL	= LOW; 
assign 		RST_SPI_PFR_BMC_BOOT_N 			= HIGH;
assign 		RST_SPI_PFR_PCH_N 				= HIGH;
assign 		SPI_PFR_BMC_BT_SECURE_CS_N 		= SPI_BMC_BOOT_CS_N;
assign 		SPI_PFR_PCH_BMC_SECURE_CS0_N 	= SPI_PCH_BMC_PFR_CS0_N;
assign 		SPI_PFR_PCH_SECURE_CS1_N 		= SPI_PCH_CS1_N;

endmodule 
