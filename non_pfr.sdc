#**************************************************************
# Time Information
#**************************************************************
set_time_format -unit ns -decimal_places 3

# Propigation delay (in/ns)
# In general on the PCB, the signal travels at the speed of ~160 ps/inch (1000 mils = 1 inch)
global PCB_PROP_IN_PER_NS
set PCB_PROP_IN_PER_NS 6.25

#**************************************************************
# Helper functions
#**************************************************************
proc set_min_relay_timing {port_name trace_length} {
	global PCB_PROP_IN_PER_NS
	set delay [format "%.2f" [expr { ($trace_length/1000.0) / $PCB_PROP_IN_PER_NS }]]
	post_message -type info "Setting min SMB Relay I/O Timing on $port_name of $delay ns"

	set_input_delay -min -clock { u_pfr_sys_clocks_reset|u_sys_pll_ip|altpll_component|auto_generated|pll1|clk[1] } $delay [get_ports $port_name]
	set_output_delay -min -clock { u_pfr_sys_clocks_reset|u_sys_pll_ip|altpll_component|auto_generated|pll1|clk[1] } $delay [get_ports $port_name]
}

proc set_max_relay_timing {port_name trace_length} {
	global PCB_PROP_IN_PER_NS
	set delay [format "%.2f" [expr { ($trace_length/1000.0) / $PCB_PROP_IN_PER_NS }]]
	post_message -type info "Setting max SMB Relay I/O Timing on $port_name of $delay ns"

	set_input_delay -max -clock { u_pfr_sys_clocks_reset|u_sys_pll_ip|altpll_component|auto_generated|pll1|clk[1] } $delay [get_ports $port_name]
	set_output_delay -max -clock { u_pfr_sys_clocks_reset|u_sys_pll_ip|altpll_component|auto_generated|pll1|clk[1] } $delay [get_ports $port_name]
}
#**************************************************************
# Create Clock
#**************************************************************
create_clock -name {CLK_25M_CKMNG_MAIN_PLD} -period 40.000 [get_ports {CLK_25M_CKMNG_MAIN_PLD}]
create_clock -name {SGPIO_BMC_CLK} -period 100 [get_ports {SGPIO_BMC_CLK}]

# Derive PLL clocks
derive_pll_clocks

# Create aliases for the PLL clocks
set CLK2M u_pfr_sys_clocks_reset|u_sys_pll_ip|altpll_component|auto_generated|pll1|clk[2]
set SYSCLK u_pfr_sys_clocks_reset|u_sys_pll_ip|altpll_component|auto_generated|pll1|clk[1]
set CLK50M u_pfr_sys_clocks_reset|u_sys_pll_ip|altpll_component|auto_generated|pll1|clk[0]

#**************************************************************
# Create Generated Clock
#**************************************************************

# Create the clock for mToggle100KHz_SClock
create_generated_clock -name Wilson_City_Main_wrapper:u_common_core|Wilson_City_Main:mWilson_City_Main|GSX_Master:mGSX_Master|Toggle:mToggle100KHz_SClock|rT_q -source $CLK2M -divide_by 1 -multiply_by 1 Wilson_City_Main_wrapper:u_common_core|Wilson_City_Main:mWilson_City_Main|GSX_Master:mGSX_Master|Toggle:mToggle100KHz_SClock|rT_q

#**************************************************************
# Set Clock Latency
#**************************************************************



#**************************************************************
# Set Clock Uncertainty
#**************************************************************
derive_clock_uncertainty


#**************************************************************
# Set Input Delay
#**************************************************************


#**************************************************************
# Set Output Delay
#**************************************************************



#**************************************************************
# Set Clock Groups
#**************************************************************
# Set all clocks as exclusive
set_clock_groups -logically_exclusive -group [get_clocks [list \
	$CLK2M \
	$CLK50M \
	$SYSCLK \
]] -group [get_clocks { \
	SGPIO_BMC_CLK \
}]



#**************************************************************
# False paths on asynchronous IOs
#**************************************************************
set false_path_input_port_names [list \
	DBP_POWER_BTN_N \
	DBP_SYSPWROK_PLD \
	FM_ADR_COMPLETE \
	FM_BMC_ONCTL_N_PLD \
	FM_BMC_PWRBTN_OUT_N \
	FM_CPU1_FIVR_FAULT_LVT3_PLD \
	FM_CPU1_INTR_PRSNT \
	FM_CPU1_MEMTRIP_N \
	FM_CPU1_PKGID0 \
	FM_CPU1_PKGID1 \
	FM_CPU1_PKGID2 \
	FM_CPU1_PROC_ID0 \
	FM_CPU1_PROC_ID1 \
	FM_CPU1_SKTOCC_LVT3_PLD_N \
	FM_CPU1_THERMTRIP_LVT3_PLD_N \
	FM_CPU2_FIVR_FAULT_LVT3_PLD \
	FM_CPU2_INTR_PRSNT \
	FM_CPU2_MEMTRIP_N \
	FM_CPU2_PKGID0 \
	FM_CPU2_PKGID1 \
	FM_CPU2_PKGID2 \
	FM_CPU2_PROC_ID0 \
	FM_CPU2_PROC_ID1 \
	FM_CPU2_SKTOCC_LVT3_PLD_N \
	FM_CPU2_THERMTRIP_LVT3_PLD_N \
	FM_CPU_CATERR_PLD_LVT3_N \
	FM_DIS_PS_PWROK_DLY \
	FM_FORCE_PWRON_LVC3 \
	FM_MEM_THERM_EVENT_CPU1_LVT3_N \
	FM_MEM_THERM_EVENT_CPU2_LVT3_N \
	FM_PCH_PRSNT_N \
	FM_PFR_DEBUG_MODE_N \
	FM_PLD_PCH_DATA_R \
	FM_PLD_REV_N \
	FM_PMBUS_ALERT_B_EN \
	FM_PS_PWROK_DLY_SEL \
	FM_PVCCIN_CPU1_PWR_IN_ALERT_N \
	FM_PVCCIN_CPU2_PWR_IN_ALERT_N \
	FM_RST_PERST_BIT0 \
	FM_RST_PERST_BIT1 \
	FM_RST_PERST_BIT2 \
	FM_SLPS3_PLD_N \
	FM_SLPS4_PLD_N \
	FM_SLP_SUS_RSM_RST_N \
	FM_THROTTLE_R_N \
	FP_ID_BTN_N \
	FP_ID_LED_N \
	FP_LED_STATUS_AMBER_N \
	FP_LED_STATUS_GREEN_N \
	IRQ_PVCCIN_CPU1_VRHOT_LVC3_N \
	IRQ_PVCCIN_CPU2_VRHOT_LVC3_N \
	IRQ_PVDDQ_ABCD_CPU1_VRHOT_LVC3_N \
	IRQ_PVDDQ_ABCD_CPU2_VRHOT_LVC3_N \
	IRQ_PVDDQ_EFGH_CPU1_VRHOT_LVC3_N \
	IRQ_PVDDQ_EFGH_CPU2_VRHOT_LVC3_N \
	IRQ_SML1_PMBUS_PLD_ALERT_N \
	PWRGD_BIAS_P1V1 \
	PWRGD_CPU1_PVDDQ_ABCD \
	PWRGD_CPU1_PVDDQ_EFGH \
	PWRGD_CPU2_PVDDQ_ABCD \
	PWRGD_CPU2_PVDDQ_EFGH \
	PWRGD_CPUPWRGD_PLD_R \
	PWRGD_P1V1_BMC_AUX \
	PWRGD_P1V05_PCH_AUX \
	PWRGD_P1V8_PCH_AUX_PLD \
	PWRGD_P1V8_PCIE_CPU1 \
	PWRGD_P1V8_PCIE_CPU2 \
	PWRGD_P3V3 \
	PWRGD_P3V3_AUX_PLD_R \
	PWRGD_PS_PWROK_PLD_R \
	PWRGD_PVCCIN_CPU1 \
	PWRGD_PVCCIN_CPU2 \
	PWRGD_PVCCIO_CPU1 \
	PWRGD_PVCCIO_CPU2 \
	PWRGD_PVCCSA_CPU1 \
	PWRGD_PVCCSA_CPU2 \
	PWRGD_VCCANA_PCIE_CPU1 \
	PWRGD_VCCANA_PCIE_CPU2 \
	RST_DEDI_BUSY_PLD_N \
	RST_PLTRST_PLD_N \
	SGPIO_BMC_DOUT \
	SGPIO_BMC_LD_N \
	SGPIO_DEBUG_PLD_DIN \
	SMB_DEBUG_PLD_SCL \
	SMB_DEBUG_PLD_SDA \
]
set_false_path -from [get_ports $false_path_input_port_names]

set false_path_output_port_names [list \
	FM_ADR_COMPLETE_DLY \
	FM_ADR_SMI_GPIO_N \
	FM_ADR_TRIGGER_N \
	FM_AUX_SW_EN \
	FM_BMC_PLD_PWRBTN_OUT_N \
	FM_CPU1_DIMM_CH1_4_FAULT_LED_SEL \
	FM_CPU1_DIMM_CH5_8_FAULT_LED_SEL \
	FM_CPU1_MEMHOT_IN \
	FM_CPU1_PROCHOT_LVC3_N \
	FM_CPU2_DIMM_CH1_4_FAULT_LED_SEL \
	FM_CPU2_DIMM_CH5_8_FAULT_LED_SEL \
	FM_CPU2_MEMHOT_IN \
	FM_CPU2_PROCHOT_LVC3_N \
	FM_CPU_BCLK5_OE_R_N \
	FM_CPU_CATERR_DLY_LVT3_N \
	FM_FAN_FAULT_LED_SEL_N \
	FM_P1V1_EN \
	FM_P1V8_PCIE_CPU1_EN \
	FM_P1V8_PCIE_CPU2_EN \
	FM_P2V5_BMC_EN_R \
	FM_P5V_EN \
	FM_PCH_ESPI_MUX_SEL_R \
	FM_PCH_P1V8_AUX_EN_R \
	FM_PFR_MUX_OE_CTL_PLD \
	FM_PLD_CLKS_OE_R_N \
	FM_PLD_HEARTBEAT_LVC3 \
	FM_POSTLED_SEL \
	FM_POST_7SEG1_SEL_N \
	FM_POST_7SEG2_SEL_N \
	FM_PS_EN_PLD_R \
	FM_PVCCANA_CPU1_EN \
	FM_PVCCANA_CPU2_EN \
	FM_PVCCIN_CPU1_EN_R \
	FM_PVCCIN_CPU2_EN_R \
	FM_PVCCIO_CPU1_EN_R \
	FM_PVCCIO_CPU2_EN_R \
	FM_PVPP_CPU1_EN_R \
	FM_PVPP_CPU2_EN_R \
	FM_SYS_THROTTLE_LVC3_PLD_R \
	FM_THERMTRIP_DLY_R \
	FM_VCCSA_CPU1_EN \
	FM_VCCSA_CPU2_EN \
	FP_BMC_PWR_BTN_CO_N \
	FP_ID_BTN_PFR_N \
	FP_ID_LED_PFR_N \
	FP_LED_FAN_FAULT_PWRSTBY_PLD_N \
	FP_LED_STATUS_AMBER_PFR_N \
	FP_LED_STATUS_GREEN_PFR_N \
	LED_CONTROL_0 \
	LED_CONTROL_1 \
	LED_CONTROL_2 \
	LED_CONTROL_3 \
	LED_CONTROL_4 \
	LED_CONTROL_5 \
	LED_CONTROL_6 \
	LED_CONTROL_7 \
	PWRGD_CPU1_LVC3 \
	PWRGD_CPU2_LVC3 \
	PWRGD_DRAMPWRGD_CPU \
	PWRGD_PCH_PWROK_R \
	PWRGD_SYS_PWROK_R \
	RST_CPU1_LVC3_N \
	RST_CPU2_LVC3_N \
	RST_PCIE_PERST0_N \
	RST_PCIE_PERST1_N \
	RST_PCIE_PERST2_N \
	RST_PLTRST_PLD_B_N \
	RST_RSMRST_PLD_R_N \
	RST_SRST_BMC_PLD_R_N \
	SGPIO_BMC_DIN \
	SGPIO_DEBUG_PLD_CLK \
	SGPIO_DEBUG_PLD_DOUT \
	SGPIO_DEBUG_PLD_LD_N \
	SMB_DEBUG_PLD_SDA \
]
set_false_path -to [get_ports $false_path_output_port_names]

# Relay timing. Set these as false paths and require matched lengths on the PCB
set_false_path -to [get_ports {SMB_BMC_HSBP_STBY_LVC3_SCL SMB_BMC_HSBP_STBY_LVC3_SDA}]
set_false_path -from [get_ports {SMB_BMC_HSBP_STBY_LVC3_SCL SMB_BMC_HSBP_STBY_LVC3_SDA}]

set_false_path -to [get_ports {SMB_PFR_HSBP_STBY_LVC3_SCL SMB_PFR_HSBP_STBY_LVC3_SDA}]
set_false_path -from [get_ports {SMB_PFR_HSBP_STBY_LVC3_SCL SMB_PFR_HSBP_STBY_LVC3_SDA}]

set_false_path -to [get_ports {SMB_PFR_PMB1_STBY_LVC3_SCL SMB_PFR_PMB1_STBY_LVC3_SDA}]
set_false_path -from [get_ports {SMB_PFR_PMB1_STBY_LVC3_SCL SMB_PFR_PMB1_STBY_LVC3_SDA}]

set_false_path -to [get_ports {SMB_PMBUS_SML1_STBY_LVC3_SCL SMB_PMBUS_SML1_STBY_LVC3_SDA}]
set_false_path -from [get_ports {SMB_PMBUS_SML1_STBY_LVC3_SCL SMB_PMBUS_SML1_STBY_LVC3_SDA}]

set_false_path -to [get_ports {SMB_PCH_PMBUS2_STBY_LVC3_SCL SMB_PCH_PMBUS2_STBY_LVC3_SDA}]
set_false_path -from [get_ports {SMB_PCH_PMBUS2_STBY_LVC3_SCL SMB_PCH_PMBUS2_STBY_LVC3_SDA}]

set_false_path -to [get_ports {SMB_PFR_PMBUS2_STBY_LVC3_SCL SMB_PFR_PMBUS2_STBY_LVC3_SDA}]
set_false_path -from [get_ports {SMB_PFR_PMBUS2_STBY_LVC3_SCL SMB_PFR_PMBUS2_STBY_LVC3_SDA}]

# BMC/PCH SPI Filter
# These are false paths for non-pfr
set_false_path -from [get_ports {SPI_BMC_BOOT_CS_N SPI_PCH_BMC_PFR_CS0_N SPI_PCH_CS1_N}]
set_false_path -from [get_ports {SPI_PFR_BOOT_CS1_N SPI_PFR_PCH_BMC_SECURE_CS0_N SPI_PFR_PCH_SECURE_CS1_N}]
