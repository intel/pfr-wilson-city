//////////////////////////////////////////////////////////////////////////////////
/*!

    \brief    <b>%ADR </b>
    \file     ADR.v
    \details    <b>Image of the Block:</b>
                \image html ADR.png

                 <b>Description:</b> \n
                ADR NVDIMM features is to allow the system to backup important information to its
                non-volatile memory section when a power supply failure is detected with minimal baseboard level HW or FW support.\n\n

                The assertion of FM_ADR_TRIGGER from Main PLD would trigger the ADR (Asynchronous DIMM Self-Refresh)
                function of LBG which would instruct CPU to save required RAID information to system memory and put
                memory into Self-Refresh mode. \n\n

                After ADR timer expired (maximum 100us), LBG would assert ADR_COMPLETE signal which is connected
                to SAVE# pin of each DDR4 slot after inversion. There is a reserved implemented in PLD to have additional
                delay on ADR_COMPLETE in case 100us was determined as not sufficient for CPU to complete all the ADR operation.\n

                At the falling edge of SAVE# pin, NVDIMM would back up the content on DDR Chips onto the NVM (Typically NAND Flash) on NVDIMM.
                During this back-up operation, system power is not required to be valid, as the power will be sourced from its own battery.\n\n



                <b>Signal function:</b>\n
                The signal FM_ADR_TRIGGER_N /FM_ADR_SMI_GPIO_N is a copy of inverted of PSU Fault  also will be de-asserted if PWRGD_CPUPWRGD go to LOW\n

                FM_PS_PWROK_DLY_SEL control the power down delay added for PWRGD_PS_PWROK:
                    0 -> 15mS delay for eADR mode
                    1 -> 600uS delay for Legacy ADR mode\n

                PWRGD_PS_PWROK_DLY_ADR is a copy of PWRGD_PS_PWROK but will the ADR delay added. This will be used to delay the deassertion of PCH_PWROK\n
                FM_AUX_SW_EN_DLY_ADR is a copy of FM_AUX_SW_EN but will the ADR delay added.\n

                FM_DIS_PS_PWROK_DLY and FM_PLD_PCH_DATA will Disable delay from PWRGD_PS (0 DLY power-down condition)  \n





    \brief  <b>Last modified</b>
            $Date:   Jan 4, 2018 $
            $Author:  David.bolanos@intel.com $
            Project         : Wilson City RP
            Group           : BD
    \version
             20180104 \b  David.bolanos@intel.com - File creation\n
             20181405 \b  jorge.juarez.campos@intel.com - ADR Module Fix\n
			 20191031 rramosco redid all module to a state machine which starts power faulire detection after pltrst and keeps trigger low until ADR complete comes up

    \copyright Intel Proprietary -- Copyright \htmlonly <script>document.write(new Date().getFullYear())</script> \endhtmlonly Intel -- All rights reserved
*/
//////////////////////////////////////////////////////////////////////////////////

module ADR(
	input  iClk,//%Clock input
	input  iRst_n,//%Reset enable on high
	input  i10uSCE, //% 10 uS Clock Enable

	input   PWRGD_PS_PWROK,			//falling flank triggers ADR

	input FM_SLPS4_N,
	input   FM_PS_EN, //MAsking signal for PS_PWROK
	input   PWRGD_CPUPWRGD, //% Pwrgd CPU from PCH, without delay
	input 	RST_PLTRST_N,

	input   FM_PS_PWROK_DLY_SEL,  //%from PCH, 0 delay 15ms, 1 delay 600us(default)
	input   FM_DIS_PS_PWROK_DLY, //% Disable delay from PWRGD_PS (0 DLY power-down condition)

	input   FM_ADR_COMPLETE,  //%ADR complete from PCH
	input   FM_PLD_PCH_DATA,  //%DATAfrom PCH, 0 to disable NVDIMM function, 1 to enable NVDIMM(default)

	output  FM_ADR_SMI_GPIO_N,  //% GPIO to PCH
	output  FM_ADR_TRIGGER_N,  //% ADR Trigger# to PCH
	output  FM_ADR_COMPLETE_DLY,//% copy of FM_ADR_COMPLETE (no delay added)
	output  PWRGD_PS_PWROK_DLY_ADR //% PWRGD_PS_PWROK delay add acording ADR logic SEL 0 delay 15ms, 1 delay 600us(default)
);

localparam	IDLE		= 0;
localparam	DETECT		= 1;
localparam	eADR		= 2;
localparam	HOLD		= 3;
localparam	ADR			= 4;
localparam	COMPLETE	= 5;
localparam HIGH = 1'b1;
localparam LOW = 1'b0;

reg [2:0]	rState;
reg  rFM_ADR_SMI_GPIO_N, rFM_ADR_TRIGGER_N, rFM_ADR_COMPLETE_DLY, rPWRGD_PS_PWROK_DLY_ADR;

always@(negedge iClk, negedge iRst_n) begin
	if (!iRst_n) begin
		rState <= IDLE;
	end
	else begin
		case(rState)
			DETECT: begin //Waits for PSU faulure or eADR
				rState <=
					(!PWRGD_PS_PWROK && FM_PLD_PCH_DATA)						?	ADR		:
					(!PWRGD_PS_PWROK)											?	eADR	:
					(FM_SLPS4_N && FM_PS_EN && PWRGD_CPUPWRGD && RST_PLTRST_N)	?	DETECT	:
																					IDLE	;
			end
			eADR: begin
				rState <=
					(FM_PLD_PCH_DATA)	?	HOLD	:
											eADR	;
			end
			HOLD: begin
				rState <= ADR	;
			end
			ADR: begin
				rState <=
					(FM_ADR_COMPLETE)	?	COMPLETE	:
											ADR			;
			end
			COMPLETE: begin
				rState <=
					(FM_ADR_COMPLETE)	?	COMPLETE	:
												IDLE	;
			end
			default: begin //IDLE
				rState <=
					(FM_SLPS4_N && FM_PS_EN && PWRGD_CPUPWRGD && RST_PLTRST_N)	?	DETECT	:
																					IDLE	;
			end
		endcase
	end
end

always@(*) begin
	case(rState)
		DETECT: begin
			rFM_ADR_TRIGGER_N		= HIGH;
			rFM_ADR_SMI_GPIO_N		= HIGH;
			rFM_ADR_COMPLETE_DLY	= LOW;//active high
			rPWRGD_PS_PWROK_DLY_ADR = HIGH;
		end
		eADR: begin
			rFM_ADR_TRIGGER_N		= HIGH;
			rFM_ADR_SMI_GPIO_N		= LOW;
			rFM_ADR_COMPLETE_DLY	= LOW;
			rPWRGD_PS_PWROK_DLY_ADR = HIGH;
		end
		HOLD: begin
			rFM_ADR_TRIGGER_N		= HIGH;
			rFM_ADR_SMI_GPIO_N		= HIGH;
			rFM_ADR_COMPLETE_DLY	= LOW;
			rPWRGD_PS_PWROK_DLY_ADR = HIGH;
		end
		ADR: begin
			rFM_ADR_TRIGGER_N		= LOW;
			rFM_ADR_SMI_GPIO_N		= HIGH;
			rFM_ADR_COMPLETE_DLY	= LOW;
			rPWRGD_PS_PWROK_DLY_ADR = HIGH;
		end
		COMPLETE: begin
			rFM_ADR_TRIGGER_N		= HIGH;
			rFM_ADR_SMI_GPIO_N		= HIGH;
			rFM_ADR_COMPLETE_DLY	= FM_ADR_COMPLETE;
			rPWRGD_PS_PWROK_DLY_ADR = LOW;
		end
		default: begin
			rFM_ADR_TRIGGER_N		= HIGH;
			rFM_ADR_SMI_GPIO_N		= HIGH;
			rFM_ADR_COMPLETE_DLY	= LOW;//active high
			rPWRGD_PS_PWROK_DLY_ADR = PWRGD_PS_PWROK;
		end
	endcase
end

assign FM_ADR_TRIGGER_N			= rFM_ADR_TRIGGER_N;
assign FM_ADR_SMI_GPIO_N		= rFM_ADR_SMI_GPIO_N;
assign FM_ADR_COMPLETE_DLY		= rFM_ADR_COMPLETE_DLY;
assign PWRGD_PS_PWROK_DLY_ADR	= rPWRGD_PS_PWROK_DLY_ADR;

endmodule
