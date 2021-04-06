/////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Intel Corporation
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
/////////////////////////////////////////////////////////////////////////////////


`timescale 1 ps / 1 ps
`default_nettype none

/*

The following signals have been renamed to match the schematic:


FM_PLD_PCH_DATA ---> FM_PLD_PCH_DATA_R
FM_PCH_ESPI_MUX_SEL ---> FM_PCH_ESPI_MUX_SEL_R
FM_THROTTLE_N ---> FM_THROTTLE_R_N
FM_SYS_THROTTLE_LVC3_PLD ---> FM_SYS_THROTTLE_LVC3_PLD_R
FM_THERMTRIP_DLY ---> FM_THERMTRIP_DLY_R
FM_PVPP_CPU1_EN ---> FM_PVPP_CPU1_EN_R
FM_PVPP_CPU2_EN ---> FM_PVPP_CPU2_EN_R
FM_PVCCIN_CPU1_EN ---> FM_PVCCIN_CPU1_EN_R
FM_PVCCIN_CPU2_EN ---> FM_PVCCIN_CPU2_EN_R
FM_PVCCIO_CPU1_EN ---> FM_PVCCIO_CPU1_EN_R
FM_PVCCIO_CPU2_EN ---> FM_PVCCIO_CPU2_EN_R

*/

module Wilson_City_Main_wrapper (
// input wire CLK
	input wire iClk_2M,
    input wire iClk_50M,
    input wire ipll_locked,

// Timers
	output wire o20mSCE,
	output wire o1SCE,

//GSX Inte  with BMC
	input wire SGPIO_BMC_CLK,
	input wire SGPIO_BMC_DOUT,
	output wire SGPIO_BMC_DIN,
	input wire SGPIO_BMC_LD_N,

//LED and 7-Seg Control Logic
	output wire LED_CONTROL_0,
	output wire LED_CONTROL_1,
	output wire LED_CONTROL_2,
	output wire LED_CONTROL_3,
	output wire LED_CONTROL_4,
	output wire LED_CONTROL_5,
	output wire LED_CONTROL_6,
	output wire LED_CONTROL_7,
	output wire FM_CPU1_DIMM_CH1_4_FAULT_LED_SEL,
	output wire FM_CPU1_DIMM_CH5_8_FAULT_LED_SEL,
	output wire FM_CPU2_DIMM_CH1_4_FAULT_LED_SEL,
	output wire FM_CPU2_DIMM_CH5_8_FAULT_LED_SEL,
	output wire FM_FAN_FAULT_LED_SEL_N,
	output wire FM_POST_7SEG1_SEL_N,
	output wire FM_POST_7SEG2_SEL_N,
	output wire FM_POSTLED_SEL,

//CATERR DLY
	output wire FM_CPU_CATERR_DLY_LVT3_N,
	input wire FM_CPU_CATERR_PLD_LVT3_N,

//ADR 
	input wire FM_ADR_COMPLETE,

	output wire FM_ADR_COMPLETE_DLY,
	output wire FM_ADR_SMI_GPIO_N,
	output wire FM_ADR_TRIGGER_N,

	input wire FM_PLD_PCH_DATA_R,
	input wire FM_PS_PWROK_DLY_SEL,
	input wire FM_DIS_PS_PWROK_DLY,

//ESPI Sup  
	output wire FM_PCH_ESPI_MUX_SEL_R,

//System T  E
	input wire FM_PMBUS_ALERT_B_EN,
	input wire FM_THROTTLE_R_N,
	input wire IRQ_SML1_PMBUS_PLD_ALERT_N,

	output wire FM_SYS_THROTTLE_LVC3_PLD_R,

// Termtrip dly
	input wire FM_CPU1_THERMTRIP_LVT3_PLD_N,
	input wire FM_CPU2_THERMTRIP_LVT3_PLD_N,
	input wire FM_MEM_THERM_EVENT_CPU1_LVT3_N,
	input wire FM_MEM_THERM_EVENT_CPU2_LVT3_N,

	output wire FM_THERMTRIP_DLY_R,
//MEMHOT
	input wire IRQ_PVDDQ_ABCD_CPU1_VRHOT_LVC3_N,
	input wire IRQ_PVDDQ_EFGH_CPU1_VRHOT_LVC3_N,
	input wire IRQ_PVDDQ_ABCD_CPU2_VRHOT_LVC3_N,
	input wire IRQ_PVDDQ_EFGH_CPU2_VRHOT_LVC3_N,

	output wire FM_CPU1_MEMHOT_IN,
	output wire FM_CPU2_MEMHOT_IN,

//MEMTRIP
	input wire FM_CPU1_MEMTRIP_N,
	input wire FM_CPU2_MEMTRIP_N,

//PROCHOT
	input wire FM_PVCCIN_CPU1_PWR_IN_ALERT_N,
	input wire FM_PVCCIN_CPU2_PWR_IN_ALERT_N,
	input wire IRQ_PVCCIN_CPU1_VRHOT_LVC3_N, 
	input wire IRQ_PVCCIN_CPU2_VRHOT_LVC3_N, 

	output wire FM_CPU1_PROCHOT_LVC3_N,
	output wire FM_CPU2_PROCHOT_LVC3_N,

//PERST &   
	input wire FM_RST_PERST_BIT0,
	input wire FM_RST_PERST_BIT1,
	input wire FM_RST_PERST_BIT2,

	output wire RST_PCIE_PERST0_N,
	output wire RST_PCIE_PERST1_N,
	output wire RST_PCIE_PERST2_N,

	output wire RST_CPU1_LVC3_N,
	output wire RST_CPU2_LVC3_N,

	output wire RST_PLTRST_PLD_B_N,
	input wire RST_PLTRST_PLD_N,

//FIVR
	input wire FM_CPU1_FIVR_FAULT_LVT3_PLD,
	input wire FM_CPU2_FIVR_FAULT_LVT3_PLD,

//CPU Misc
	input wire FM_CPU1_PKGID0,
	input wire FM_CPU1_PKGID1,
	input wire FM_CPU1_PKGID2,

	input wire FM_CPU1_PROC_ID0,
	input wire FM_CPU1_PROC_ID1,

	input wire FM_CPU1_INTR_PRSNT, 
	input wire FM_CPU1_SKTOCC_LVT3_PLD_N,

	input wire FM_CPU2_PKGID0,
	input wire FM_CPU2_PKGID1,
	input wire FM_CPU2_PKGID2,

	input wire FM_CPU2_PROC_ID0,
	input wire FM_CPU2_PROC_ID1,

	input wire FM_CPU2_INTR_PRSNT,
	input wire FM_CPU2_SKTOCC_LVT3_PLD_N,

//BMC
	input wire FM_BMC_PWRBTN_OUT_N,
	output wire FM_BMC_PLD_PWRBTN_OUT_N,

	input wire FM_BMC_ONCTL_N_PLD,
	input  RST_SRST_BMC_PLD_R_N,
	output  RST_SRST_BMC_PLD_R_N_REQ,

	output wire FM_P2V5_BMC_EN_R,
	input wire PWRGD_P1V1_BMC_AUX,

//PCH
	input  RST_RSMRST_PLD_R_N,
	output  RST_RSMRST_PLD_R_N_REQ,

	output wire PWRGD_PCH_PWROK_R,
	output wire PWRGD_SYS_PWROK_R,

	input wire FM_SLP_SUS_RSM_RST_N,
	input wire FM_SLPS3_PLD_N,
	input wire FM_SLPS4_PLD_N,
	input wire FM_PCH_PRSNT_N,

	output wire FM_PCH_P1V8_AUX_EN_R,

	input wire PWRGD_P1V05_PCH_AUX,
	input wire PWRGD_P1V8_PCH_AUX_PLD,

//PSU Ctl
	output wire FM_PS_EN_PLD_R,
	input wire PWRGD_PS_PWROK_PLD_R,

//Clock Lo   
	output wire FM_CPU_BCLK5_OE_R_N,
	inout wire FM_PLD_CLKS_OE_R_N,

//Base Log  
	input wire PWRGD_P3V3_AUX_PLD_R,

//Main VR & Logic
	input wire PWRGD_P3V3,

	output wire FM_P5V_EN,
	output wire FM_AUX_SW_EN,

//Mem
	input wire PWRGD_CPU1_PVDDQ_ABCD,
	input wire PWRGD_CPU1_PVDDQ_EFGH,
	input wire PWRGD_CPU2_PVDDQ_ABCD,
	input wire PWRGD_CPU2_PVDDQ_EFGH,

	output wire FM_PVPP_CPU1_EN_R,
	output wire FM_PVPP_CPU2_EN_R,

//CPU
	output wire PWRGD_CPU1_LVC3,
	output wire PWRGD_CPU2_LVC3,

	input wire PWRGD_CPUPWRGD_PLD_R,
	output wire PWRGD_DRAMPWRGD_CPU,

	output wire FM_P1V1_EN,

	output wire FM_P1V8_PCIE_CPU1_EN,
	output wire FM_P1V8_PCIE_CPU2_EN,

	output wire FM_PVCCANA_CPU1_EN,
	output wire FM_PVCCANA_CPU2_EN,

	output wire FM_PVCCIN_CPU1_EN_R,
	output wire FM_PVCCIN_CPU2_EN_R,

	output wire FM_PVCCIO_CPU1_EN_R,
	output wire FM_PVCCIO_CPU2_EN_R,

	output wire FM_VCCSA_CPU1_EN,
	output wire FM_VCCSA_CPU2_EN,

	input wire PWRGD_BIAS_P1V1,

	input wire PWRGD_P1V8_PCIE_CPU1,
	input wire PWRGD_P1V8_PCIE_CPU2,

	input wire PWRGD_PVCCIN_CPU1,
	input wire PWRGD_PVCCIN_CPU2,
 
	input wire PWRGD_PVCCIO_CPU1,
	input wire PWRGD_PVCCIO_CPU2,

	input wire PWRGD_PVCCSA_CPU1,
	input wire PWRGD_PVCCSA_CPU2,

	input wire PWRGD_VCCANA_PCIE_CPU1,
	input wire PWRGD_VCCANA_PCIE_CPU2,

//Dediprog Detection Support 
	input wire RST_DEDI_BUSY_PLD_N,

//DBP 
	input wire DBP_POWER_BTN_N,
	input wire DBP_SYSPWROK_PLD,

//Debug
	input wire FM_FORCE_PWRON_LVC3,
	output wire FM_PLD_HEARTBEAT_LVC3,

//Debug pins I/O
	output wire SGPIO_DEBUG_PLD_CLK,
	input wire SGPIO_DEBUG_PLD_DIN,
	output wire SGPIO_DEBUG_PLD_DOUT,
	output wire SGPIO_DEBUG_PLD_LD_N,

	input wire SMB_DEBUG_PLD_SCL,
	inout wire SMB_DEBUG_PLD_SDA,

	input wire SMB_PCH_PMBUS2_STBY_LVC3_SCL,
	input wire SMB_PCH_PMBUS2_STBY_LVC3_SDA,
	output wire SMB_PCH_PMBUS2_STBY_LVC3_SDA_OEn,

	input wire FM_PLD_REV_N,

// Front Panel 
	output wire FP_LED_FAN_FAULT_PWRSTBY_PLD_N,
	output wire FP_BMC_PWR_BTN_CO_N,
	
	output wire FM_PFR_MUX_OE_CTL_PLD

);

//////////////////////////////////////////////////////////////////////////////////
// Parameters
//////////////////////////////////////////////////////////////////////////////////
localparam  LOW =1'b0;
localparam  HIGH=1'b1;  
localparam  Z=1'bz;

//////////////////////////////////////////////////////////////////////////////////
// Internal Signals
//////////////////////////////////////////////////////////////////////////////////
wire wFP_BMC_PWR_BTN_CO_N;
wire wnSDAOE;
wire wFM_PLD_CLKS_OE_R_N;
wire wFM_PVCCIN_CPU1_EN;
wire wFM_PVCCIN_CPU2_EN;
wire wSMB_DEBUG_PLD_SDA_OE;
wire wFM_CPU_BCLK5_OE_R_N;


////////////////////////////////////////////////////////////////////////////////// //
// Continuous assignments                                                          //
////////////////////////////////////////////////////////////////////////////////// //
`ifdef SIMULATION
assign FP_BMC_PWR_BTN_CO_N = wFP_BMC_PWR_BTN_CO_N ? HIGH : LOW;
`else
assign FP_BMC_PWR_BTN_CO_N = wFP_BMC_PWR_BTN_CO_N ? Z : LOW;
`endif

assign 	FM_BMC_PLD_PWRBTN_OUT_N	= FM_BMC_PWRBTN_OUT_N 	? Z : LOW; // De-bouncer is not needed in this case
assign 	SMB_DEBUG_PLD_SDA 		= wSMB_DEBUG_PLD_SDA_OE ? Z	: LOW;


wire [7:0]LED_CONTROL;

assign LED_CONTROL_0 = LED_CONTROL[0];
assign LED_CONTROL_1 = LED_CONTROL[1];
assign LED_CONTROL_2 = LED_CONTROL[2];
assign LED_CONTROL_3 = LED_CONTROL[3];
assign LED_CONTROL_4 = LED_CONTROL[4];
assign LED_CONTROL_5 = LED_CONTROL[5];
assign LED_CONTROL_6 = LED_CONTROL[6];
assign LED_CONTROL_7 = LED_CONTROL[7];


//Clock control as OD to remove leakage
assign 		FM_PLD_CLKS_OE_R_N       	= (wFM_PLD_CLKS_OE_R_N) 	? 1'bz : 1'b0;
assign 		FM_CPU_BCLK5_OE_R_N       	= (wFM_CPU_BCLK5_OE_R_N)	? 1'bz : 1'b0;

//VCCIN VR Drop Support
`ifdef SIMULATION
assign 		FM_PVCCIN_CPU1_EN_R        	= (wFM_PVCCIN_CPU1_EN) 		? 1'b1 : 1'b0;
assign 		FM_PVCCIN_CPU2_EN_R        	= (wFM_PVCCIN_CPU2_EN) 		? 1'b1 : 1'b0;
`else
assign 		FM_PVCCIN_CPU1_EN_R        	= (wFM_PVCCIN_CPU1_EN) 		? 1'bz : 1'b0;
assign 		FM_PVCCIN_CPU2_EN_R        	= (wFM_PVCCIN_CPU2_EN) 		? 1'bz : 1'b0;
`endif

/// Instantiate the Main

Wilson_City_Main mWilson_City_Main
(
	.iClk_2M                          (iClk_2M),
	.iClk_50M                         (iClk_50M),
	.ipll_locked                      (ipll_locked),
	
// Timers
	.o20mSCE                          (o20mSCE),
	.o1SCE                            (o1SCE),

//GSX Interface with BMC
	.SGPIO_BMC_CLK                    (SGPIO_BMC_CLK), 
	.SGPIO_BMC_DOUT                   (SGPIO_BMC_DOUT),
	.SGPIO_BMC_DIN                    (SGPIO_BMC_DIN),
	.SGPIO_BMC_LD_N                   (SGPIO_BMC_LD_N),

//I2C Support 
	.SMB_PLD_SDA                      (SMB_PCH_PMBUS2_STBY_LVC3_SDA),
	.SMB_PLD_SCL                      (SMB_PCH_PMBUS2_STBY_LVC3_SCL),
	.onSDAOE                          (SMB_PCH_PMBUS2_STBY_LVC3_SDA_OEn),

//LED and 7-Seg Control Logic
	.LED_CONTROL                      (LED_CONTROL),
	.FM_CPU1_DIMM_CH1_4_FAULT_LED_SEL (FM_CPU1_DIMM_CH1_4_FAULT_LED_SEL),
	.FM_CPU1_DIMM_CH5_8_FAULT_LED_SEL (FM_CPU1_DIMM_CH5_8_FAULT_LED_SEL),
	.FM_CPU2_DIMM_CH1_4_FAULT_LED_SEL (FM_CPU2_DIMM_CH1_4_FAULT_LED_SEL),
	.FM_CPU2_DIMM_CH5_8_FAULT_LED_SEL (FM_CPU2_DIMM_CH5_8_FAULT_LED_SEL),
	.FM_FAN_FAULT_LED_SEL_N           (FM_FAN_FAULT_LED_SEL_N),
	.FM_POST_7SEG1_SEL_N              (FM_POST_7SEG1_SEL_N),
	.FM_POST_7SEG2_SEL_N              (FM_POST_7SEG2_SEL_N),
	.FM_POSTLED_SEL                   (FM_POSTLED_SEL),

//CATERR DLY
	.FM_CPU_CATERR_DLY_LVT3_N         (FM_CPU_CATERR_DLY_LVT3_N),
	.FM_CPU_CATERR_PLD_LVT3_N         (FM_CPU_CATERR_PLD_LVT3_N),

//ADR 
	.FM_ADR_COMPLETE                  (FM_ADR_COMPLETE), 

	.FM_ADR_COMPLETE_DLY              (FM_ADR_COMPLETE_DLY),
	.FM_ADR_SMI_GPIO_N                (FM_ADR_SMI_GPIO_N),
	.FM_ADR_TRIGGER_N                 (FM_ADR_TRIGGER_N),

	.FM_PLD_PCH_DATA                  (FM_PLD_PCH_DATA_R), 
	.FM_PS_PWROK_DLY_SEL              (FM_PS_PWROK_DLY_SEL), 
	.FM_DIS_PS_PWROK_DLY              (FM_DIS_PS_PWROK_DLY), 

//ESPI Support
	.FM_PCH_ESPI_MUX_SEL              (FM_PCH_ESPI_MUX_SEL_R),

//System THROTTLE
	.FM_PMBUS_ALERT_B_EN              (FM_PMBUS_ALERT_B_EN), 
	.FM_THROTTLE_N                    (FM_THROTTLE_R_N), 
	.IRQ_SML1_PMBUS_PLD_ALERT_N       (IRQ_SML1_PMBUS_PLD_ALERT_N), 

	.FM_SYS_THROTTLE_LVC3_PLD         (FM_SYS_THROTTLE_LVC3_PLD_R),
// Termtrip dly
	.FM_CPU1_THERMTRIP_LVT3_PLD_N     (FM_CPU1_THERMTRIP_LVT3_PLD_N), 
	.FM_CPU2_THERMTRIP_LVT3_PLD_N     (FM_CPU2_THERMTRIP_LVT3_PLD_N), 
	.FM_MEM_THERM_EVENT_CPU1_LVT3_N   (FM_MEM_THERM_EVENT_CPU1_LVT3_N), 
	.FM_MEM_THERM_EVENT_CPU2_LVT3_N   (FM_MEM_THERM_EVENT_CPU2_LVT3_N), 

	.FM_THERMTRIP_DLY                 (FM_THERMTRIP_DLY_R),
//MEMHOT
	.IRQ_PVDDQ_ABCD_CPU1_VRHOT_LVC3_N (IRQ_PVDDQ_ABCD_CPU1_VRHOT_LVC3_N),
	.IRQ_PVDDQ_EFGH_CPU1_VRHOT_LVC3_N (IRQ_PVDDQ_EFGH_CPU1_VRHOT_LVC3_N),
	.IRQ_PVDDQ_ABCD_CPU2_VRHOT_LVC3_N (IRQ_PVDDQ_ABCD_CPU2_VRHOT_LVC3_N),
	.IRQ_PVDDQ_EFGH_CPU2_VRHOT_LVC3_N (IRQ_PVDDQ_EFGH_CPU2_VRHOT_LVC3_N),

	.FM_CPU1_MEMHOT_IN                (FM_CPU1_MEMHOT_IN),
	.FM_CPU2_MEMHOT_IN                (FM_CPU2_MEMHOT_IN),

//MEMTRIP
	.FM_CPU1_MEMTRIP_N                (FM_CPU1_MEMTRIP_N), 
	.FM_CPU2_MEMTRIP_N                (FM_CPU2_MEMTRIP_N), 

//PROCHOT
	.FM_PVCCIN_CPU1_PWR_IN_ALERT_N    (FM_PVCCIN_CPU1_PWR_IN_ALERT_N), 
	.FM_PVCCIN_CPU2_PWR_IN_ALERT_N    (FM_PVCCIN_CPU2_PWR_IN_ALERT_N), 
	.IRQ_PVCCIN_CPU1_VRHOT_LVC3_N     (IRQ_PVCCIN_CPU1_VRHOT_LVC3_N),  
	.IRQ_PVCCIN_CPU2_VRHOT_LVC3_N     (IRQ_PVCCIN_CPU2_VRHOT_LVC3_N),  

	.FM_CPU1_PROCHOT_LVC3_N           (FM_CPU1_PROCHOT_LVC3_N),
	.FM_CPU2_PROCHOT_LVC3_N           (FM_CPU2_PROCHOT_LVC3_N),

//PERST & RST
	.FM_RST_PERST_BIT0                (FM_RST_PERST_BIT0), 
	.FM_RST_PERST_BIT1                (FM_RST_PERST_BIT1), 
	.FM_RST_PERST_BIT2                (FM_RST_PERST_BIT2), 

	.RST_PCIE_PERST0_N                (RST_PCIE_PERST0_N),
	.RST_PCIE_PERST1_N                (RST_PCIE_PERST1_N),
	.RST_PCIE_PERST2_N                (RST_PCIE_PERST2_N),

	.RST_CPU1_LVC3_N                  (RST_CPU1_LVC3_N),
	.RST_CPU2_LVC3_N                  (RST_CPU2_LVC3_N),

	.RST_PLTRST_B_N                   (RST_PLTRST_PLD_B_N),
	.RST_PLTRST_N                     (RST_PLTRST_PLD_N),

//FIVR
	.FM_CPU1_FIVR_FAULT_LVT3          (FM_CPU1_FIVR_FAULT_LVT3_PLD), 
	.FM_CPU2_FIVR_FAULT_LVT3          (FM_CPU2_FIVR_FAULT_LVT3_PLD), 

//CPU Misc
	.FM_CPU1_PKGID0                   (FM_CPU1_PKGID0), 
	.FM_CPU1_PKGID1                   (FM_CPU1_PKGID1), 
	.FM_CPU1_PKGID2                   (FM_CPU1_PKGID2), 

	.FM_CPU1_PROC_ID0                 (FM_CPU1_PROC_ID0), 
	.FM_CPU1_PROC_ID1                 (FM_CPU1_PROC_ID1), 

	.FM_CPU1_INTR_PRSNT               (FM_CPU1_INTR_PRSNT),  
	.FM_CPU1_SKTOCC_LVT3_N            (FM_CPU1_SKTOCC_LVT3_PLD_N), 

	.FM_CPU2_PKGID0                   (FM_CPU2_PKGID0), 
	.FM_CPU2_PKGID1                   (FM_CPU2_PKGID1), 
	.FM_CPU2_PKGID2                   (FM_CPU2_PKGID2), 

	.FM_CPU2_PROC_ID0                 (FM_CPU2_PROC_ID0), 
	.FM_CPU2_PROC_ID1                 (FM_CPU2_PROC_ID1), 

	.FM_CPU2_INTR_PRSNT               (FM_CPU2_INTR_PRSNT), 
	.FM_CPU2_SKTOCC_LVT3_N            (FM_CPU2_SKTOCC_LVT3_PLD_N), 

//BMC
	.FM_BMC_PWRBTN_OUT_N              (FM_BMC_PWRBTN_OUT_N), 

	.FM_BMC_ONCTL_N                   (FM_BMC_ONCTL_N_PLD), 
	.RST_SRST_BMC_PLD_N               (RST_SRST_BMC_PLD_R_N), 
	.RST_SRST_BMC_PLD_N_REQ           (RST_SRST_BMC_PLD_R_N_REQ), 

	.FM_P2V5_BMC_EN                   (FM_P2V5_BMC_EN_R), 
	.PWRGD_P1V1_BMC_AUX               (PWRGD_P1V1_BMC_AUX), 

//PCH
	.RST_RSMRST_N                     (RST_RSMRST_PLD_R_N),
	.RST_RSMRST_N_REQ                 (RST_RSMRST_PLD_R_N_REQ),

	.PWRGD_PCH_PWROK                  (PWRGD_PCH_PWROK_R), 
	.PWRGD_SYS_PWROK                  (PWRGD_SYS_PWROK_R),

	.FM_SLP_SUS_RSM_RST_N             (FM_SLP_SUS_RSM_RST_N), 
	.FM_SLPS3_N                       (FM_SLPS3_PLD_N), 
	.FM_SLPS4_N                       (FM_SLPS4_PLD_N), 
	.FM_PCH_PRSNT_N                   (FM_PCH_PRSNT_N), 

	.FM_PCH_P1V8_AUX_EN               (FM_PCH_P1V8_AUX_EN_R), 
  
	.PWRGD_P1V05_PCH_AUX              (PWRGD_P1V05_PCH_AUX), 
	.PWRGD_P1V8_PCH_AUX               (PWRGD_P1V8_PCH_AUX_PLD),

	.FM_PFR_MUX_OE_CTL_PLD            (FM_PFR_MUX_OE_CTL_PLD),
	.RST_DEDI_BUSY_PLD_N              (RST_DEDI_BUSY_PLD_N),

//PSU Ctl
	.FM_PS_EN                         (FM_PS_EN_PLD_R), 
	.PWRGD_PS_PWROK                   (PWRGD_PS_PWROK_PLD_R), 

//Clock Logic    
	.FM_PLD_CLKS_OE_N                 (wFM_PLD_CLKS_OE_R_N),
	.FM_CPU_BCLK5_OE_R_N  			  (wFM_CPU_BCLK5_OE_R_N),

//Base Logic
	.PWRGD_P3V3_AUX                   (PWRGD_P3V3_AUX_PLD_R),

//Main VR & Logic
	.PWRGD_P3V3                       (PWRGD_P3V3), 

	.FM_P5V_EN                        (FM_P5V_EN),
	.FM_AUX_SW_EN                     (FM_AUX_SW_EN), 

//Mem
	.PWRGD_CPU1_PVDDQ_ABCD            (PWRGD_CPU1_PVDDQ_ABCD), 
	.PWRGD_CPU1_PVDDQ_EFGH            (PWRGD_CPU1_PVDDQ_EFGH), 
	.PWRGD_CPU2_PVDDQ_ABCD            (PWRGD_CPU2_PVDDQ_ABCD), 
	.PWRGD_CPU2_PVDDQ_EFGH            (PWRGD_CPU2_PVDDQ_EFGH), 

	.FM_PVPP_CPU1_EN                  (FM_PVPP_CPU1_EN_R),
	.FM_PVPP_CPU2_EN                  (FM_PVPP_CPU2_EN_R),

//CPU

	.PWRGD_CPU1_LVC3                  (PWRGD_CPU1_LVC3),
	.PWRGD_CPU2_LVC3                  (PWRGD_CPU2_LVC3),

	.PWRGD_CPUPWRGD                   (PWRGD_CPUPWRGD_PLD_R), 
	.PWRGD_DRAMPWRGD_CPU              (PWRGD_DRAMPWRGD_CPU),

	.FM_P1V1_EN                       (FM_P1V1_EN), 

	.FM_P1V8_PCIE_CPU1_EN             (FM_P1V8_PCIE_CPU1_EN),
	.FM_P1V8_PCIE_CPU2_EN             (FM_P1V8_PCIE_CPU2_EN),

	.FM_PVCCANA_CPU1_EN               (FM_PVCCANA_CPU1_EN),
	.FM_PVCCANA_CPU2_EN               (FM_PVCCANA_CPU2_EN),

	.FM_PVCCIN_CPU1_EN                (wFM_PVCCIN_CPU1_EN),
	.FM_PVCCIN_CPU2_EN                (wFM_PVCCIN_CPU2_EN),

	.FM_PVCCIO_CPU1_EN                (FM_PVCCIO_CPU1_EN_R),
	.FM_PVCCIO_CPU2_EN                (FM_PVCCIO_CPU2_EN_R),

	.FM_PVCCSA_CPU1_EN                (FM_VCCSA_CPU1_EN),
	.FM_PVCCSA_CPU2_EN                (FM_VCCSA_CPU2_EN),

	.PWRGD_BIAS_P1V1                  (PWRGD_BIAS_P1V1), 
 
	.PWRGD_P1V8_PCIE_CPU1             (PWRGD_P1V8_PCIE_CPU1), 
	.PWRGD_P1V8_PCIE_CPU2             (PWRGD_P1V8_PCIE_CPU2), 

	.PWRGD_PVCCIN_CPU1                (PWRGD_PVCCIN_CPU1), 
	.PWRGD_PVCCIN_CPU2                (PWRGD_PVCCIN_CPU2), 
 
	.PWRGD_PVCCIO_CPU1                (PWRGD_PVCCIO_CPU1), 
	.PWRGD_PVCCIO_CPU2                (PWRGD_PVCCIO_CPU2), 

	.PWRGD_PVCCSA_CPU1                (PWRGD_PVCCSA_CPU1), 
	.PWRGD_PVCCSA_CPU2                (PWRGD_PVCCSA_CPU2), 

	.PWRGD_VCCANA_PCIE_CPU1           (PWRGD_VCCANA_PCIE_CPU1), 
	.PWRGD_VCCANA_PCIE_CPU2           (PWRGD_VCCANA_PCIE_CPU2), 

//DBP 
	.DBP_POWER_BTN_N                  (DBP_POWER_BTN_N), 
	.DBP_SYSPWROK                     (DBP_SYSPWROK_PLD), 

//Debug
	.FM_FORCE_PWRON_LVC3              (FM_FORCE_PWRON_LVC3),
	.FM_PLD_HEARTBEAT_LVC3            (FM_PLD_HEARTBEAT_LVC3), 

// Front Panel 
	.FP_LED_FAN_FAULT_PWRSTBY_PLD_N	(FP_LED_FAN_FAULT_PWRSTBY_PLD_N), 
	.FP_BMC_PWR_BTN_CO_N           	(wFP_BMC_PWR_BTN_CO_N),

//Debug pins I/O
	.SGPIO_DEBUG_PLD_CLK 			(SGPIO_DEBUG_PLD_CLK),
	.SGPIO_DEBUG_PLD_DIN 			(SGPIO_DEBUG_PLD_DIN),
	.SGPIO_DEBUG_PLD_DOUT 			(SGPIO_DEBUG_PLD_DOUT),
	.SGPIO_DEBUG_PLD_LD_N 			(SGPIO_DEBUG_PLD_LD_N),

	.SMB_DEBUG_PLD_SCL 				(SMB_DEBUG_PLD_SCL),
	.SMB_DEBUG_PLD_SDA 				(SMB_DEBUG_PLD_SDA),
	.oSMB_DEBUG_PLD_SDA_OE			(wSMB_DEBUG_PLD_SDA_OE),

	.FM_PLD_REV_N 					(FM_PLD_REV_N)

);


endmodule
