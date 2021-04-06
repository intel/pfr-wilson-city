# Map of pin names to SPI function
global spi_pin_names_map
array set spi_pin_names_map [list]
set spi_pin_names_map(BMC_SPI_DQ0) SPI_PFR_BMC_FLASH1_BT_MOSI
set spi_pin_names_map(BMC_SPI_DQ1) SPI_PFR_BMC_FLASH1_BT_MISO
set spi_pin_names_map(BMC_SPI_DQ2) SPI_PFR_BMC_BOOT_R_IO2
set spi_pin_names_map(BMC_SPI_DQ3) SPI_PFR_BMC_BOOT_R_IO3
set spi_pin_names_map(BMC_SPI_CLK) SPI_PFR_BMC_FLASH1_BT_CLK
set spi_pin_names_map(BMC_SPI_CS_IN) SPI_BMC_BOOT_CS_N
set spi_pin_names_map(BMC_SPI_CS_OUT) SPI_PFR_BOOT_CS1_N
set spi_pin_names_map(BMC_SPI_DQ0_MON) SPI_BMC_BT_MUXED_MON_MOSI
set spi_pin_names_map(BMC_SPI_CLK_MON) SPI_BMC_BT_MUXED_MON_CLK

set spi_pin_names_map(PCH_SPI_DQ0) SPI_PFR_PCH_R_IO0
set spi_pin_names_map(PCH_SPI_DQ1) SPI_PFR_PCH_R_IO1
set spi_pin_names_map(PCH_SPI_DQ2) SPI_PFR_PCH_R_IO2
set spi_pin_names_map(PCH_SPI_DQ3) SPI_PFR_PCH_R_IO3
set spi_pin_names_map(PCH_SPI_CLK) SPI_PFR_PCH_R_CLK
set spi_pin_names_map(PCH_SPI_CS_IN) SPI_PCH_BMC_PFR_CS0_N
set spi_pin_names_map(PCH_SPI_CS_OUT) SPI_PFR_PCH_BMC_SECURE_CS0_N
set spi_pin_names_map(PCH_SPI_DQ0_MON) SPI_PCH_BMC_SAFS_MUXED_MON_IO0
set spi_pin_names_map(PCH_SPI_DQ1_MON) SPI_PCH_BMC_SAFS_MUXED_MON_IO1
set spi_pin_names_map(PCH_SPI_DQ2_MON) SPI_PCH_BMC_SAFS_MUXED_MON_IO2
set spi_pin_names_map(PCH_SPI_DQ3_MON) SPI_PCH_BMC_SAFS_MUXED_MON_IO3
set spi_pin_names_map(PCH_SPI_CLK_MON) SPI_PCH_BMC_SAFS_MUXED_MON_CLK

# Map of GPOs to bit numbers
source gen_gpo_controls_pkg.tcl

#**************************************************************
# Time Information
#**************************************************************
set_time_format -unit ns -decimal_places 3

# Propigation delay (in/ns)
# In general on the PCB, the signal travels at the speed of ~160 ps/inch (1000 mils = 1 inch)
global PCB_PROP_IN_PER_NS
set PCB_PROP_IN_PER_NS 6.25

#**************************************************************
# Helper procs
#**************************************************************
proc calc_prop_delay {trace_length} {
	global PCB_PROP_IN_PER_NS

	set delay [format "%.2f" [expr { ($trace_length/1000.0) / $PCB_PROP_IN_PER_NS }]] 
	return $delay
}

proc report_spi_names {} {
	global spi_pin_names_map

	post_message -type info "SPI Pin Map:"
	foreach key [lsort [array names spi_pin_names_map]] {
		post_message -type info "\t$key => $spi_pin_names_map($key)"
	}
}

proc report_pch_spi_timing {} {
	global spi_pin_names_map

	# Report paths
	set T_SPI_PCH_CS_N_to_SPI_PCH_SECURE_CS_N [lindex [report_path -from [get_ports $spi_pin_names_map(PCH_SPI_CS_IN)] -to [get_ports $spi_pin_names_map(PCH_SPI_CS_OUT)]] 1]
	
	report_timing -from [get_ports $spi_pin_names_map(PCH_SPI_CS_IN)] -to [get_ports $spi_pin_names_map(PCH_SPI_CS_OUT)] -npath 100 -nworst 10 -show_routing -detail full_path -panel {PFR||PCH SPI||CS to Secure CS : Setup} -setup
	report_timing -from [get_ports $spi_pin_names_map(PCH_SPI_CS_IN)] -to [get_ports $spi_pin_names_map(PCH_SPI_CS_OUT)] -npath 100 -nworst 10 -show_routing -detail full_path -panel {PFR||PCH SPI||CS to Secure CS : Hold} -hold

	report_timing -to_clock PCH_SPI_CLK -to [get_ports] -npath 100 -nworst 10 -show_routing -detail full_path -panel {PFR||PCH SPI||SPI CLK output paths : Setup} -setup
	report_timing -to_clock PCH_SPI_CLK -to [get_ports] -npath 100 -nworst 10 -show_routing -detail full_path -panel {PFR||PCH SPI||SPI CLK output paths : Hold} -hold
	report_timing -from_clock PCH_SPI_CLK -from [get_ports] -npath 100 -nworst 10 -show_routing -detail full_path -panel {PFR||PCH SPI||SPI CLK input paths : Setup} -setup
	report_timing -from_clock PCH_SPI_CLK -from [get_ports] -npath 100 -nworst 10 -show_routing -detail full_path -panel {PFR||PCH SPI||SPI CLK input paths : Hold} -hold

	report_timing -to_clock PCH_SPI_CLK_virt -to [get_ports] -npath 100 -nworst 10 -show_routing -detail full_path -panel {PFR||PCH SPI||PCH_SPI_CLK_virt output paths : Setup} -setup
	report_timing -to_clock PCH_SPI_CLK_virt -to [get_ports] -npath 100 -nworst 10 -show_routing -detail full_path -panel {PFR||PCH SPI||PCH_SPI_CLK_virt output paths : Hold} -hold
	report_timing -from_clock PCH_SPI_CLK_virt -from [get_ports] -npath 100 -nworst 10 -show_routing -detail full_path -panel {PFR||PCH SPI||PCH_SPI_CLK_virt input paths : Setup} -setup
	report_timing -from_clock PCH_SPI_CLK_virt -from [get_ports] -npath 100 -nworst 10 -show_routing -detail full_path -panel {PFR||PCH SPI||PCH_SPI_CLK_virt input paths : Hold} -hold

	report_timing -to_clock PCH_SPI_MON_CLK -from [get_ports] -npath 100 -nworst 10 -show_routing -detail full_path -panel {PFR||PCH SPI||Monitored CLK input paths : Setup} -setup
	report_timing -to_clock PCH_SPI_MON_CLK -from [get_ports] -npath 100 -nworst 10 -show_routing -detail full_path -panel {PFR||PCH SPI||Monitored CLK input paths : Hold} -hold

	report_timing -to_clock { PCH_SPI_CLK_virt } -setup -npaths 100 -detail full_path -show_routing -panel_name {PFR||PCH SPI||To PCH_SPI_CLK_virt : Setup} -multi_corner
	report_timing -from_clock { PCH_SPI_CLK_virt } -setup -npaths 100 -detail full_path -show_routing -panel_name {PFR||PCH SPI||From PCH_SPI_CLK_virt : Setup} -multi_corner
	report_timing -to_clock { PCH_SPI_CLK } -setup -npaths 100 -detail full_path -show_routing -panel_name {PFR||PCH SPI||To PCH_SPI_CLK : Setup} -multi_corner
	report_timing -from_clock { PCH_SPI_CLK } -setup -npaths 100 -detail full_path -show_routing -panel_name {PFR||PCH SPI||From PCH_SPI_CLK : Setup} -multi_corner
	report_timing -to_clock { PCH_SPI_MON_CLK } -setup -npaths 100 -detail full_path -show_routing -panel_name {PFR||PCH SPI||To PCH_SPI_MON_CLK : Setup} -multi_corner
	report_timing -from_clock { PCH_SPI_MON_CLK } -setup -npaths 100 -detail full_path -show_routing -panel_name {PFR||PCH SPI||From PCH_SPI_MON_CLK : Setup} -multi_corner

	post_message -type info "PCH CS ($spi_pin_names_map(PCH_SPI_CS_IN)) to Secure CS ($spi_pin_names_map(PCH_SPI_CS_OUT)) Tpd : $T_SPI_PCH_CS_N_to_SPI_PCH_SECURE_CS_N ns"

}

proc report_bmc_spi_timing {} {
	global spi_pin_names_map

	# Report paths
	set T_SPI_BMC_CS_N_to_SPI_BMC_SECURE_CS_N [lindex [report_path -from [get_ports $spi_pin_names_map(BMC_SPI_CS_IN)] -to [get_ports $spi_pin_names_map(BMC_SPI_CS_OUT)]] 1]

	report_timing -from [get_ports $spi_pin_names_map(BMC_SPI_CS_IN)] -to [get_ports $spi_pin_names_map(BMC_SPI_CS_OUT)] -npath 100 -nworst 10 -show_routing -detail full_path -panel {PFR||BMC SPI||CS to Secure CS : Setup} -setup
	report_timing -from [get_ports $spi_pin_names_map(BMC_SPI_CS_IN)] -to [get_ports $spi_pin_names_map(BMC_SPI_CS_OUT)] -npath 100 -nworst 10 -show_routing -detail full_path -panel {PFR||BMC SPI||CS to Secure CS : Setup} -setup
	report_timing -from [get_ports $spi_pin_names_map(BMC_SPI_CS_IN)] -to [get_ports $spi_pin_names_map(BMC_SPI_CS_OUT)] -npath 100 -nworst 10 -show_routing -detail full_path -panel {PFR||BMC SPI||CS to Secure CS : Hold} -hold

	report_timing -to_clock BMC_SPI_CLK -to [get_ports] -npath 100 -nworst 10 -show_routing -detail full_path -panel {PFR||BMC SPI||SPI CLK output paths : Setup} -setup
	report_timing -to_clock BMC_SPI_CLK -to [get_ports] -npath 100 -nworst 10 -show_routing -detail full_path -panel {PFR||BMC SPI||SPI CLK output paths : Hold} -hold
	report_timing -from_clock BMC_SPI_CLK -from [get_ports] -npath 100 -nworst 10 -show_routing -detail full_path -panel {PFR||BMC SPI||SPI CLK input paths : Setup} -setup
	report_timing -from_clock BMC_SPI_CLK -from [get_ports] -npath 100 -nworst 10 -show_routing -detail full_path -panel {PFR||BMC SPI||SPI CLK input paths : Hold} -hold

	report_timing -to_clock BMC_SPI_CLK_virt -to [get_ports] -npath 100 -nworst 10 -show_routing -detail full_path -panel {PFR||BMC SPI||BMC_SPI_CLK_virt output paths : Setup} -setup
	report_timing -to_clock BMC_SPI_CLK_virt -to [get_ports] -npath 100 -nworst 10 -show_routing -detail full_path -panel {PFR||BMC SPI||BMC_SPI_CLK_virt output paths : Hold} -hold
	report_timing -from_clock BMC_SPI_CLK_virt -from [get_ports] -npath 100 -nworst 10 -show_routing -detail full_path -panel {PFR||BMC SPI||BMC_SPI_CLK_virt input paths : Setup} -setup
	report_timing -from_clock BMC_SPI_CLK_virt -from [get_ports] -npath 100 -nworst 10 -show_routing -detail full_path -panel {PFR||BMC SPI||BMC_SPI_CLK_virt input paths : Hold} -hold

	report_timing -to_clock BMC_SPI_MON_CLK -from [get_ports] -npath 100 -nworst 10 -show_routing -detail full_path -panel {PFR||BMC SPI||Monitored CLK input paths : Setup} -setup
	report_timing -to_clock BMC_SPI_MON_CLK -from [get_ports] -npath 100 -nworst 10 -show_routing -detail full_path -panel {PFR||BMC SPI||Monitored CLK input paths : Hold} -hold

	report_timing -to_clock { BMC_SPI_CLK_virt } -setup -npaths 100 -detail full_path -show_routing -panel_name {PFR||BMC SPI||To BMC_SPI_CLK_virt : Setup} -multi_corner
	report_timing -from_clock { BMC_SPI_CLK_virt } -setup -npaths 100 -detail full_path -show_routing -panel_name {PFR||BMC SPI||From BMC_SPI_CLK_virt : Setup} -multi_corner
	report_timing -to_clock { BMC_SPI_CLK } -setup -npaths 100 -detail full_path -show_routing -panel_name {PFR||BMC SPI||To BMC_SPI_CLK : Setup} -multi_corner
	report_timing -from_clock { BMC_SPI_CLK } -setup -npaths 100 -detail full_path -show_routing -panel_name {PFR||BMC SPI||From BMC_SPI_CLK : Setup} -multi_corner
	report_timing -to_clock { BMC_SPI_MON_CLK } -setup -npaths 100 -detail full_path -show_routing -panel_name {PFR||BMC SPI||To BMC_SPI_MON_CLK : Setup} -multi_corner
	report_timing -from_clock { BMC_SPI_MON_CLK } -setup -npaths 100 -detail full_path -show_routing -panel_name {PFR||BMC SPI||From BMC_SPI_MON_CLK : Setup} -multi_corner

	post_message -type info "BMC CS to Secure CS (Tpd) delay: $T_SPI_BMC_CS_N_to_SPI_BMC_SECURE_CS_N ns"
}

proc report_pfr_timing {} {
	report_spi_names
	report_pch_spi_timing
	report_bmc_spi_timing
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
global SYSCLK
set SYSCLK u_pfr_sys_clocks_reset|u_sys_pll_ip|altpll_component|auto_generated|pll1|clk[1]
set CLK50M u_pfr_sys_clocks_reset|u_sys_pll_ip|altpll_component|auto_generated|pll1|clk[0]
global SPICLK
set SPICLK u_pfr_sys_clocks_reset|u_sys_pll_ip|altpll_component|auto_generated|pll1|clk[3]

#**************************************************************
# Create Generated Clock
#**************************************************************

# Missing constraints from IPs                                                                           
#https://www.intel.com/content/www/us/en/programmable/support/support-resources/knowledge-base/tools/2017/why-do-we-see-unconstrained-clock--alteradualboot-dualboot0-altd.html
create_generated_clock -name {ru_clk} -source $SYSCLK -divide_by 2 -master_clock $SYSCLK [get_registers {u_core|u_pfr_sys|u_dual_config|alt_dual_boot_avmm_comp|alt_dual_boot|ru_clk}]

#https://www.intel.com/content/www/us/en/programmable/support/support-resources/knowledge-base/tools/2016/warning--332060---node---alteraonchipflash-onchipflash-alteraonc.html
create_generated_clock -name flash_se_neg_reg -divide_by 2 -source [get_pins { u_core|u_pfr_sys|u_ufm|avmm_data_controller|flash_se_neg_reg|clk }] [get_pins { u_core|u_pfr_sys|u_ufm|avmm_data_controller|flash_se_neg_reg|q } ]

# Create the clock for the SPI master IP
create_generated_clock -name spi_master_clk -source $SPICLK -divide_by 2 [get_pins {u_core|u_spi_control|spi_master_inst|qspi_inf_inst|flash_clk_reg|q}]

# Create the clock for mToggle100KHz_SClock in the common core
create_generated_clock -name u_common_core|mWilson_City_Main|mGSX_Master|Toggle:mToggle100KHz_SClock|rT_q -source $CLK2M -divide_by 1 -multiply_by 1 u_common_core|mWilson_City_Main|mGSX_Master|mToggle100KHz_SClock|rT_q


# Constrain the clock crossing
constrain_alt_handshake_clock_crosser_skew *:u_core|*:u_pfr_sys|*_mm_interconnect_0:mm_interconnect_0|altera_avalon_st_handshake_clock_crosser:crosser

#**************************************************************
# Set Clock Latency
#**************************************************************



#**************************************************************
# Set Clock Uncertainty
#**************************************************************
derive_clock_uncertainty

#**************************************************************
# Set Clock Groups
#**************************************************************
# Set all PLL clocks as exclusive from the SGPIO_BMC_CLK clock
set_clock_groups -logically_exclusive -group [get_clocks [list \
	$CLK2M \
	$CLK50M \
	$SYSCLK \
	$SPICLK
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
	FM_PFR_PROV_UPDATE_N \
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
	FM_ME_PFR_1 \
	FM_ME_PFR_2 \
	SPI_PCH_CS1_N \
]
set_false_path -from [get_ports $false_path_input_port_names]
foreach key [lsort [array names spi_pin_names_map]] {
	if {[lsearch -exact $false_path_input_port_names $spi_pin_names_map($key)] != -1} {
		post_message -type error "SPI Pin $spi_pin_names_map($key) ($key) found in input false patch list"
	}
}


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
	FM_PFR_SLP_SUS_N \
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
	RST_PFR_OVR_RTC_N \
	RST_PFR_OVR_SRTC_N \
	SGPIO_BMC_DIN \
	SGPIO_DEBUG_PLD_CLK \
	SGPIO_DEBUG_PLD_DOUT \
	SGPIO_DEBUG_PLD_LD_N \
	SMB_DEBUG_PLD_SDA \
	FM_SPI_PFR_BMC_BT_MASTER_SEL \
	FM_SPI_PFR_PCH_MASTER_SEL \
	PWRGD_DSW_PWROK_R \
	RST_PFR_EXTRST_N \
	SPI_PFR_PCH_SECURE_CS1_N \
    RST_SPI_PFR_BMC_BOOT_N \
    RST_SPI_PFR_PCH_N \
]
set_false_path -to [get_ports $false_path_output_port_names]
foreach key [lsort [array names spi_pin_names_map]] {
	if {[lsearch -exact $false_path_output_port_names $spi_pin_names_map($key)] != -1} {
		post_message -type error "SPI Pin $spi_pin_names_map($key) ($key) found in output false patch list"
	}
}

#****************************************************************
# SMBus Mailbox
#**************************************************************
# Use the set_max_skew constraint to perform maximum allowable skew (<1.5ns) analysis between SCL and SDA pins for input path.
# This is to set the max_skew from the SDA/SCL ports to the first register
set_max_skew -to_clock {u_pfr_sys_clocks_reset|u_sys_pll_ip|altpll_component|auto_generated|pll1|clk[1]} -from [get_ports {SMB_PFR_DDRABCD_CPU1_LVC2_SCL SMB_PFR_DDRABCD_CPU1_LVC2_SDA}] 1.5

# Use set_max_delay to constrain the output path of SMB_PFR_DDRABCD_CPU1_LVC2_SDA since it is a INOUT port
# Simply choose a 'reasonable' delay (5 ns) to prevent Quartus from adding excessive routing delay to this signal
set_max_delay -to [get_ports {SMB_PFR_DDRABCD_CPU1_LVC2_SDA}] 5

#****************************************************************
# RFNVRAM
#**************************************************************
# Use the set_max_skew constraint to perform maximum allowable skew (<2ns) analysis between SCL and SDA pins for output path
# This is to set the max_skew from the first register to the SDA/SCL ports
set_max_skew -from_clock {u_pfr_sys_clocks_reset|u_sys_pll_ip|altpll_component|auto_generated|pll1|clk[1]} -to [get_ports {SMB_PFR_RFID_STBY_LVC3_SDA SMB_PFR_RFID_STBY_LVC3_SCL}] 2

# Use set_input_delay to constrain the input path of SMB_PFR_RFID_STBY_LVC3_SDA since it is a INOUT port
# Taking a conventional ratio 30:70 of the system clock period (50ns), 30% for external delay (15ns) versus 70% for internal delay (35ns)
# The allowable input data delay = 0ns(min) + 35ns(50ns-max) = 35ns
set_input_delay -clock { u_pfr_sys_clocks_reset|u_sys_pll_ip|altpll_component|auto_generated|pll1|clk[1] }  -max 15 [get_ports {SMB_PFR_RFID_STBY_LVC3_SDA}]
set_input_delay -clock { u_pfr_sys_clocks_reset|u_sys_pll_ip|altpll_component|auto_generated|pll1|clk[1] }  -min 0 [get_ports {SMB_PFR_RFID_STBY_LVC3_SDA}]


#****************************************************************
# BMC/PCH SMBus RELAY 
#**************************************************************
# Use the set_max_skew constraint to perform maximum allowable skew (<1.5ns) analysis between SCL and SDA pins.
# In order to constrain skew across multiple paths, all such paths must be defined within a single set_max_skew constraint.

# This is to set the max_skew from the first register to the SDA/SCL ports
set_max_skew -from_clock { u_pfr_sys_clocks_reset|u_sys_pll_ip|altpll_component|auto_generated|pll1|clk[1] } -to [get_ports {SMB_BMC_HSBP_STBY_LVC3_SDA   SMB_BMC_HSBP_STBY_LVC3_SCL}] 1.5
set_max_skew -from_clock { u_pfr_sys_clocks_reset|u_sys_pll_ip|altpll_component|auto_generated|pll1|clk[1] } -to [get_ports {SMB_PMBUS_SML1_STBY_LVC3_SDA SMB_PMBUS_SML1_STBY_LVC3_SCL}] 1.5
set_max_skew -from_clock { u_pfr_sys_clocks_reset|u_sys_pll_ip|altpll_component|auto_generated|pll1|clk[1] } -to [get_ports {SMB_PFR_PMB1_STBY_LVC3_SDA   SMB_PFR_PMB1_STBY_LVC3_SCL}] 1.5
set_max_skew -from_clock { u_pfr_sys_clocks_reset|u_sys_pll_ip|altpll_component|auto_generated|pll1|clk[1] } -to [get_ports {SMB_PCH_PMBUS2_STBY_LVC3_SDA SMB_PCH_PMBUS2_STBY_LVC3_SCL}] 1.5
set_max_skew -from_clock { u_pfr_sys_clocks_reset|u_sys_pll_ip|altpll_component|auto_generated|pll1|clk[1] } -to [get_ports {SMB_PFR_PMBUS2_STBY_LVC3_SDA SMB_PFR_PMBUS2_STBY_LVC3_SCL}] 1.5
set_max_skew -from_clock { u_pfr_sys_clocks_reset|u_sys_pll_ip|altpll_component|auto_generated|pll1|clk[1] } -to [get_ports {SMB_PFR_HSBP_STBY_LVC3_SDA   SMB_PFR_HSBP_STBY_LVC3_SCL}] 1.5

# This is to set the max_skew from the SDA/SCL ports to the first register
set_max_skew -to_clock { u_pfr_sys_clocks_reset|u_sys_pll_ip|altpll_component|auto_generated|pll1|clk[1] } -from [get_ports {SMB_BMC_HSBP_STBY_LVC3_SDA   SMB_BMC_HSBP_STBY_LVC3_SCL}] 1.5
set_max_skew -to_clock { u_pfr_sys_clocks_reset|u_sys_pll_ip|altpll_component|auto_generated|pll1|clk[1] } -from [get_ports {SMB_PMBUS_SML1_STBY_LVC3_SDA SMB_PMBUS_SML1_STBY_LVC3_SCL}] 1.5
set_max_skew -to_clock { u_pfr_sys_clocks_reset|u_sys_pll_ip|altpll_component|auto_generated|pll1|clk[1] } -from [get_ports {SMB_PFR_PMB1_STBY_LVC3_SDA   SMB_PFR_PMB1_STBY_LVC3_SCL}] 1.5
set_max_skew -to_clock { u_pfr_sys_clocks_reset|u_sys_pll_ip|altpll_component|auto_generated|pll1|clk[1] } -from [get_ports {SMB_PCH_PMBUS2_STBY_LVC3_SDA SMB_PCH_PMBUS2_STBY_LVC3_SCL}] 1.5
set_max_skew -to_clock { u_pfr_sys_clocks_reset|u_sys_pll_ip|altpll_component|auto_generated|pll1|clk[1] } -from [get_ports {SMB_PFR_PMBUS2_STBY_LVC3_SDA SMB_PFR_PMBUS2_STBY_LVC3_SCL}] 1.5
set_max_skew -to_clock { u_pfr_sys_clocks_reset|u_sys_pll_ip|altpll_component|auto_generated|pll1|clk[1] } -from [get_ports {SMB_PFR_HSBP_STBY_LVC3_SDA   SMB_PFR_HSBP_STBY_LVC3_SCL}] 1.5

# Report the SPI pin map names
report_spi_names

#**************************************************************
# BMC/PCH SPI Filter and SPI Master
#**************************************************************
proc set_pch_spi_interface_constraints {} {
	global SPICLK
	global spi_pin_names_map

	post_message -type info "Creating PCH SPI interface constraints"
	post_message -type info [string repeat # 80]

	# SPI Mux properties from datasheet
	set Tmux_prop_min 0.2
	set Tmux_prop_max 0.2

	# PCH SPI Interface numbers
	# Use 40 MHz SPI CLK
	# TCO numbers from from PCH Datasheet 
	set SPI_PCH_SPI_FLASH_CLK_PERIOD 25.000
	set T_PCH_SPI_DQ_TCO_MIN -3.5000
	set T_PCH_SPI_DQ_TCO_MAX 5.000
	set T_PCH_SPI_CS_TCO_MIN 5.000
	set T_PCH_SPI_CS_TCO_MAX 5.000

	# PCH Flash device properties from datasheet
	set T_SPI_PCH_SPI_FLASH_DQ_S 1.75
	set T_SPI_PCH_SPI_FLASH_DQ_H 2.3
	set T_SPI_PCH_SPI_FLASH_CS_S 3.375
	set T_SPI_PCH_SPI_FLASH_CS_H 3.375
	set T_SPI_PCH_SPI_FLASH_DQ_TCO 6.0

	#######################################################################################################
	# PCH Flash board delays
	#######################################################################################################
	# CPLD -> SPI_PFR_PCH_R_CLK mux
	# Path: U6D2.B11 (SPI_PFR_PCH_R_CLK) to U5P1.13 (SPI_PFR_PCH_CLK)
	# U6D2.B11 -> (945.65) -> R6D12.1 -> (70.36) -> R6D11.1 -> (70.36) -> R6D10.1 -> (0) -> R6D10.2 -> (1451.51) -> U5P1.13
	# Total path length: 2537.88
	set T_CPLD_SPI_PCH_CLK_to_MUX [calc_prop_delay [expr {945.65 + 70.36 + 70.36 + 1451.51}]]

	# CPLD -> SPI_PFR_PCH_SECURE_CS0_N mux
	# Path: U6D2.A17 (SPI_PFR_PCH_BMC_SECURE_CS0_N) to U5P2.7 (SPI_PCH_MUXED_CS0_N)
	# U6D2.A17 -> (2470.98) -> R5P20.1 -> (0) -> R5P20.2 -> (683.23) -> R5P27.2 -> (0) -> R5P27.1 -> (153.98) -> U5P2.7
	# Total path length: 3308.19
	set T_CPLD_SPI_PCH_CS_N_OUT_to_MUX [calc_prop_delay [expr {2470.98 + 683.23 + 153.9}]]

	# CPLD -> SPI_PFR_PCH_R_IO0 mux
	# Path: U6D2.B12 (SPI_PFR_PCH_R_IO0) to U5P1.3 (SPI_PFR_PCH_IO0)
	# U6D2.B12 -> (849.7) -> R6D15.1 -> (70.36) -> R6D14.1 -> (0) -> R6D14.2 -> (1519.83) -> U5P1.3
	# Total path length: 2439.89
	set T_CPLD_SPI_PCH_IO0_to_MUX [calc_prop_delay [expr {849.7 + 70.36 + 1519.83}]]

	# CPLD -> SPI_PFR_PCH_R_IO1
	# Path: U6D2.B13 (SPI_PFR_PCH_R_IO1) to U5P1.6 (SPI_PFR_PCH_IO1)
	# U6D2.B13 -> (825.76) -> R6D16.1 -> (70.36) -> R6D17.1 -> (0) -> R6D17.2 -> (1549.4) -> U5P1.6
	# Total path length: 2445.52
	set T_CPLD_SPI_PCH_IO1_to_MUX [calc_prop_delay [expr {825.76 + 70.36 + 1549.4}]]

	# CPLD -> SPI_PFR_PCH_R_IO2 mux
	# Path: U6D2.B14 (SPI_PFR_PCH_R_IO2) to U5P1.10 (SPI_PFR_PCH_IO2)
	# U6D2.B14 -> (858.65) -> R6D19.1 -> (0) -> R6D19.2 -> (1528.24) -> U5P1.10
	# Total path length: 2386.89
	set T_CPLD_SPI_PCH_IO2_to_MUX [calc_prop_delay [expr {858.65 + 1528.24}]]

	# CPLD -> SPI_PFR_PCH_R_IO3 mux
	# Path: U6D2.B16 (SPI_PFR_PCH_R_IO3) to U5P2.3 (SPI_PFR_PCH_IO3)
	# U6D2.B16 -> (871.13) -> R6D22.1 -> (0) -> R6D22.2 -> (2576.06) -> U5P2.3
	# Total path length: 3447.19
	set T_CPLD_SPI_PCH_IO3_to_MUX [calc_prop_delay [expr {871.13 + 2576.06}]]

	# mux -> SPI_PCH_CLK_FLASH_R1 flash
	# Path: U5P1.12 (SPI_PCH_MUXED_CLK) to XU5D1.16 (SPI_PCH_CLK_FLASH_R1)
	# U5P1.12 -> (125.01) -> R4P3.1 -> (3685.54) -> R6D35.1 -> (0) -> R6D35.2 -> (367.77) -> XU5D1.16
	# Total path length: 4178.32
	set Tmux_to_PCH_SPI_FLASH_CLK [calc_prop_delay [expr {125.01 + 3685.54 + 367.77}]]

	# mux -> SPI_PCH_CS_FLASH_R1 flash
	# Path: U5P2.7 (SPI_PCH_MUXED_CS0_N) to XU5D1.7 (SPI_PCH_CS0_FLASH_R1_N)
	# U5P2.7 -> (153.98) -> R5P27.1 -> (334.15) -> R5P12.1 -> (0) -> R5P12.2 -> (120.98) -> XU5D1.7
	# Total path length: 609.11
	set Tmux_to_PCH_SPI_FLASH_CS [calc_prop_delay [expr {153.98 + 334.15 + 120.98}]]

	# mux -> SPI_PCH_IO0_FLASH_R1 flash
	# Path: U5P1.4 (SPI_PCH_MUXED_IO0) to XU5D1.15 (SPI_PCH_IO0_FLASH_R1)
	# U5P1.4 -> (3614.54) -> R6D34.1 -> (0) -> R6D34.2 -> (373.65) -> XU5D1.15
	# Total path length: 3988.19
	set Tmux_to_PCH_SPI_FLASH_DQ0 [calc_prop_delay [expr {3614.54 + 373.65}]]

	# mux -> SPI_PCH_IO1_FLASH_R1 flash
	# Path: U5P1.7 (SPI_PCH_MUXED_IO1) to XU5D1.8 (SPI_PCH_IO1_FLASH_R1)
	# U5P1.7 -> (145.91) -> R5P5.1 -> (3465.91) -> R5P11.1 -> (0) -> R5P11.2 -> (229.16) -> XU5D1.8
	# Total path length: 3840.98
	set Tmux_to_PCH_SPI_FLASH_DQ1 [calc_prop_delay [expr {145.91 + 3465.91 + 229.16}]]

	# mux -> SPI_PCH_IO2_FLASH_R1 flash
	# Path: U5P1.9 (SPI_PCH_MUXED_IO2) to XU5D1.9 (SPI_PCH_IO2_FLASH_R1)
	# U5P1.9 -> (3556.34) -> R4P7.1 -> (0) -> R4P7.2 -> (233.06) -> XU5D1.9
	# Total path length: 3789.4
	set Tmux_to_PCH_SPI_FLASH_DQ2 [calc_prop_delay [expr {3556.34 + 233.06}]]

	# mux -> SPI_PCH_IO3_FLASH_R1 flash
	# Path: U5P2.4 (SPI_PCH_MUXED_IO3) to XU5D1.1 (SPI_PCH_IO3_FLASH_R1)
	# U5P2.4 -> (3638.73) -> R5D6.1 -> (0) -> R5D6.2 -> (287.55) -> XU5D1.1
	# Total path length: 3926.28
	set Tmux_to_PCH_SPI_FLASH_DQ3 [calc_prop_delay [expr {3638.73 + 287.55}]]

	# mux -> SPI_PCH_BMC_SAFS_MUXED_MON_CLK cpld pin
	# Path: U6D2.B17 (SPI_PCH_BMC_SAFS_MUXED_MON_CLK) to U5P1.12 (SPI_PCH_MUXED_CLK)
	# U6D2.B17 -> (962.94) -> R6D78.2 -> (0.0) -> R6D77.2 -> (0) -> R6D77.1 -> (580.69) -> R6D71.1 -> (3748.61) -> R6D83.1 -> (2846.71) -> R4P3.1 -> (125.01) -> U5P1.12
	# Total path length: 8263.96
	set Tmux_to_CPLD_SPI_PCH_CLK_MON [calc_prop_delay [expr {962.94 + 0.0 + 580.69 + 3748.61 + 2846.71 + 125.01}]]

	# mux -> SPI_PCH_BMC_SAFS_MUXED_MON_IO0 cpld pin
	# Path: U6D2.C11 (SPI_PCH_BMC_SAFS_MUXED_MON_IO0) to U5P1.4 (SPI_PCH_MUXED_IO0)
	# U6D2.C11 -> (1087.21) -> R6D66.2 -> (0.0) -> R6D65.2 -> (0) -> R6D65.1 -> (503.56) -> R6D64.2 -> (3793.82) -> R6D87.1 -> (2595.12) -> U5P1.4
	# Total path length: 7979.71
	set Tmux_to_CPLD_SPI_PCH_DQ0_MON [calc_prop_delay [expr {1087.21 + 0.0 + 503.56 + 3793.82 + 2595.12}]]

	# mux -> SPI_PCH_BMC_SAFS_MUXED_MON_IO1 cpld pin
	# Path: U6D2.C12 (SPI_PCH_BMC_SAFS_MUXED_MON_IO1) to U5P1.7 (SPI_PCH_MUXED_IO1)
	# U6D2.C12 -> (1202.15) -> R6D63.2 -> (0.0) -> R6D62.2 -> (0) -> R6D62.1 -> (312.66) -> R6D67.2 -> (2552.99) -> R5P11.1 -> (3465.91) -> R5P5.1 -> (145.91) -> U5P1.7
	# Total path length: 7679.62
	set Tmux_to_CPLD_SPI_PCH_DQ1_MON [calc_prop_delay [expr {1202.15 + 0.0 + 312.66 + 2552.99 + 3465.91 + 145.91}]]

	# mux -> SPI_PCH_BMC_SAFS_MUXED_MON_IO2 cpld pin
	# Path: U6D2.C13 (SPI_PCH_BMC_SAFS_MUXED_MON_IO2) to U5P1.9 (SPI_PCH_MUXED_IO2)
	# U6D2.C13 -> (1163.22) -> R6D57.2 -> (0) -> R6D57.1 -> (699.14) -> R6D37.2 -> (6384.08) -> R4P1.1 -> (179.07) -> U5P1.9
	# Total path length: 8425.51
	set Tmux_to_CPLD_SPI_PCH_DQ2_MON [calc_prop_delay [expr {1163.22 + 699.14 + 6384.08 + 179.07}]]

	# mux -> SPI_PCH_BMC_SAFS_MUXED_MON_IO3 cpld pin
	# Path: U6D2.C14 (SPI_PCH_BMC_SAFS_MUXED_MON_IO3) to U5P2.4 (SPI_PCH_MUXED_IO3)
	# U6D2.C14 -> (1188.26) -> R6D49.2 -> (0) -> R6D49.1 -> (620.9) -> R6D25.1 -> (2770.86) -> R5D6.1 -> (3638.73) -> U5P2.4
	# Total path length: 8218.75
	set Tmux_to_CPLD_SPI_PCH_DQ3_MON [calc_prop_delay [expr {1188.26 + 620.9 + 2770.86 + 3638.73}]]

	# PCH -> SPI_PCH_R_CLK mux
	# Path: U4D1.K2 (SPI_PCH_CLK) to U5P1.14 (SPI_PCH_CLK_R)
	# U4D1.K2 -> (3267.49) -> R4P4.2 -> (0) -> R4P4.1 -> (168.57) -> U5P1.14
	# Total path length: 3436.06
	set T_PCH_SPI_CLK_to_mux [calc_prop_delay [expr {3267.49 + 168.57}]]

	# PCH -> DQ0 Mux
	# Path: U4D1.H4 (SPI_PCH_IO0) to U5P1.2 (SPI_PCH_IO0)
	# U4D1.H4 -> (522.55) -> R4D72.2 -> (137.53) -> R6P18.1 -> (1229.81) -> J5D5.1 -> (1755.63) -> U5P1.2
	# Total path length: 3645.52
	set T_PCH_SPI_DQ0_to_mux [calc_prop_delay [expr {522.55 + 137.53 + 1229.81 + 1755.63}]]

	# PCH -> DQ1 Mux
	# Path: U4D1.L3 (SPI_PCH_IO1) to U5P1.5 (SPI_PCH_IO1)
	# U4D1.L3 -> (3377.08) -> U5P1.5
	# Total path length: 3377.08
	set T_PCH_SPI_DQ1_to_mux [calc_prop_delay [expr {3377.08}]]

	# PCH -> DQ2 Mux
	# Path: U4D1.F5 (SPI_PCH_IO2) to U5P1.11 (SPI_PCH_IO2_R)
	# U4D1.F5 -> (469.17) -> R4D75.1 -> (3046.54) -> R4P2.2 -> (0) -> R4P2.1 -> (210.75) -> U5P1.11
	# Total path length: 3726.46
	set T_PCH_SPI_DQ2_to_mux [calc_prop_delay [expr {469.17 + 3046.54 + 210.75}]]

	# PCH -> DQ3 Mux
	# Path: U4D1.L1 (SPI_PCH_IO3) to U5P2.2 (SPI_PCH_IO3_R)
	# U4D1.L1 -> (416.6) -> R4D64.1 -> (35.0) -> R4D68.2 -> (2985.01) -> R5P31.2 -> (45.0) -> R5P35.2 -> (0) -> R5P35.1 -> (185.68) -> U5P2.2
	# Total path length: 3667.29
	set T_PCH_SPI_DQ3_to_mux [calc_prop_delay [expr {416.6 + 35.0 + 2985.01 + 45.0 + 185.68}]]

	# PCH -> SPI_PCH_BMC_PFR_CS0_N CPLD
	# Path: U4D1.J3 (SPI_PCH_CS0_N) to U6D2.A15 (SPI_PCH_BMC_PFR_CS0_N)
	# U4D1.J3 -> (2986.89) -> R5P36.1 -> (0) -> R5P36.2 -> (35.0) -> R5P32.2 -> (639.52) -> R5P28.2 -> (2097.59) -> U6D2.A15
	# Total path length: 5759.0
	set T_PCH_SPI_CS_to_CPLD_PCH_SPI_CS_IN [calc_prop_delay [expr {2986.89 + 35.0 + 639.52 + 2097.59}]]

	#######################################################################################################
	# Compute shorthand variables
	#######################################################################################################
	set T_CPLD_to_PCH_SPI_FLASH_CLK_min [expr {$T_CPLD_SPI_PCH_CLK_to_MUX + $Tmux_prop_min + $Tmux_to_PCH_SPI_FLASH_CLK}]
	set T_CPLD_to_PCH_SPI_FLASH_CLK_max [expr {$T_CPLD_SPI_PCH_CLK_to_MUX + $Tmux_prop_max + $Tmux_to_PCH_SPI_FLASH_CLK}]

	set T_CPLD_to_PCH_SPI_FLASH_CS_min [expr {$T_CPLD_SPI_PCH_CS_N_OUT_to_MUX + $Tmux_prop_min + $Tmux_to_PCH_SPI_FLASH_CS}]
	set T_CPLD_to_PCH_SPI_FLASH_CS_max [expr {$T_CPLD_SPI_PCH_CS_N_OUT_to_MUX + $Tmux_prop_max + $Tmux_to_PCH_SPI_FLASH_CS}]

	set T_PCH_to_PCH_SPI_FLASH_CLK_min [expr {$T_PCH_SPI_CLK_to_mux + $Tmux_prop_min + $Tmux_to_PCH_SPI_FLASH_CLK}]
	set T_PCH_to_PCH_SPI_FLASH_CLK_max [expr {$T_PCH_SPI_CLK_to_mux + $Tmux_prop_max + $Tmux_to_PCH_SPI_FLASH_CLK}]

	set T_PCH_to_CPLD_PCH_SPI_CLK_min [expr {$T_PCH_SPI_CLK_to_mux + $Tmux_prop_min + $Tmux_to_CPLD_SPI_PCH_CLK_MON}]
	set T_PCH_to_CPLD_PCH_SPI_CLK_max [expr {$T_PCH_SPI_CLK_to_mux + $Tmux_prop_max + $Tmux_to_CPLD_SPI_PCH_CLK_MON}]

	set T_PCH_SPI_DQ0_to_CPLD_min [expr {$T_PCH_SPI_DQ0_to_mux + $Tmux_prop_min + $Tmux_to_CPLD_SPI_PCH_DQ0_MON}]
	set T_PCH_SPI_DQ0_to_CPLD_max [expr {$T_PCH_SPI_DQ0_to_mux + $Tmux_prop_max + $Tmux_to_CPLD_SPI_PCH_DQ0_MON}]
	set T_PCH_SPI_DQ1_to_CPLD_min [expr {$T_PCH_SPI_DQ1_to_mux + $Tmux_prop_min + $Tmux_to_CPLD_SPI_PCH_DQ1_MON}]
	set T_PCH_SPI_DQ1_to_CPLD_max [expr {$T_PCH_SPI_DQ1_to_mux + $Tmux_prop_max + $Tmux_to_CPLD_SPI_PCH_DQ1_MON}]
	set T_PCH_SPI_DQ2_to_CPLD_min [expr {$T_PCH_SPI_DQ2_to_mux + $Tmux_prop_min + $Tmux_to_CPLD_SPI_PCH_DQ2_MON}]
	set T_PCH_SPI_DQ2_to_CPLD_max [expr {$T_PCH_SPI_DQ2_to_mux + $Tmux_prop_max + $Tmux_to_CPLD_SPI_PCH_DQ2_MON}]
	set T_PCH_SPI_DQ3_to_CPLD_min [expr {$T_PCH_SPI_DQ3_to_mux + $Tmux_prop_min + $Tmux_to_CPLD_SPI_PCH_DQ3_MON}]
	set T_PCH_SPI_DQ3_to_CPLD_max [expr {$T_PCH_SPI_DQ3_to_mux + $Tmux_prop_max + $Tmux_to_CPLD_SPI_PCH_DQ3_MON}]

	# Create the clock on the pin for the PCH SPI clock. This clock is created in the CPLD
	create_generated_clock -name PCH_SPI_CLK -source [get_clock_info -targets [get_clocks spi_master_clk]] [get_ports $spi_pin_names_map(PCH_SPI_CLK)]

	# SPI input timing. Data is output on falling edge of SPI clock and latched on rising edge of FPGA clock.
	# The latch edge is aligns with the next falling edge of the launch clock. Since the dest clock
	# is 2x the source clock, a multicycle is used. 
	#  
	#   Data Clock : PCH_SPI_CLK
	#       (Launch)
	#         v
	#   +-----+     +-----+     +-----+
	#   |     |     |     |     |     |	
	#   +     +-----+     +-----+     +-----
	#
	#   FPGA Clock : $SPICLK 
	#            (Latch)
	#                     v
	#   +--+  +--+  +--+  +--+  +--+  +--+
	#   |  |  |  |  |  |  |  |  |  |  |  |	
	#   +  +--+  +--+  +--+  +--+  +--+  +--
	#        (H)         (S)          
	# Setup relationship : PCH_SPI_CLK period == 2 * $SPICLK period
	# Hold relationship : 0
	set_multicycle_path -from [get_clocks PCH_SPI_CLK] -to [get_clocks $SPICLK] -setup -end 2
	set_multicycle_path -from [get_clocks PCH_SPI_CLK] -to [get_clocks $SPICLK] -hold -end 1

	# SPI Output multicycle to ensure previous edge used for hold. This is because the SPI output clock is 180deg
	# from the SPI clock
	#  

	#   Data Clock : $SPICLK 
	#            (Launch)
	#               v
	#   +--+  +--+  +--+  +--+  +--+  +--+
	#   |  |  |  |  |  |  |  |  |  |  |  |	
	#   +  +--+  +--+  +--+  +--+  +--+  +--

	#   Data Clock 2X : spi_master_clk
	#            (Launch)
	#               v
	#   +-----+     +-----+     +-----+
	#   |     |     |     |     |     |	
	#   +     +-----+     +-----+     +-----
	#
	#   Output Clock : PCH_SPI_CLK
	#                  (Latch)
	#                     v
	#   +     +-----+     +-----+     +-----+
	#   |     |     |     |     |     |     |	
	#   +-----+     +-----+     +-----+     +-----
	#        (H)         (S)          
	#
	# Setup relationship : PCH_SPI_CLK period / 2
	# Hold relationship : -PCH_SPI_CLK period / 2
	set_multicycle_path -from [get_clocks $SPICLK] -to [get_clocks PCH_SPI_CLK] -hold -start 1


	# SPI CS timing when in filtering mode. Data is output from BMC/PCH on rising edge and latched on rising edge.
	#  
	#   Data Clock : PCH_SPI_CLK_virt
	#          (Launch)
	#             v
	#   +----+    +----+    +----+
	#   |    |    |    |    |    |	
	#   +    +----+    +----+    +----
	#
	#   SPI Clock : PCH_SPI_CLK_virt
	#                    (Latch)
	#                       v
	#   +----+    +----+    +----+
	#   |    |    |    |    |    |	
	#   +    +----+    +----+    +----
	#            (H)       (S)        
	#
	# Setup relationship : SPI_CLK period
	# Hold relationship : 0

	# Create the virtual clock for the PCH SPI clock located in the PCH. It is exclusive from the PLL clock
	create_clock -name PCH_SPI_CLK_virt -period $SPI_PCH_SPI_FLASH_CLK_PERIOD
	set_clock_groups -logically_exclusive -group {PCH_SPI_CLK_virt} -group {PCH_SPI_CLK}
	# Create the monitored clock
	create_clock -name PCH_SPI_MON_CLK -period $SPI_PCH_SPI_FLASH_CLK_PERIOD [get_ports $spi_pin_names_map(PCH_SPI_CLK_MON)]

	#######################################################################################################
	# Create the output constraints for the SPI Master IP to PCH flash
	#######################################################################################################
	# This are classic input/output delays with respect to the SPI clock for all data out and CS
	# Output max delay = max trace delay for data + tsu - min trace delay of clock
	# Output min delay = min trace delay for data - th - max trace delay of clock

	post_message -type info "PCH CLK Period: $SPI_PCH_SPI_FLASH_CLK_PERIOD ns"
	post_message -type info "CPLD PCH CLK to flash delay: [expr {$T_CPLD_to_PCH_SPI_FLASH_CLK_min}] ns"
	post_message -type info "CPLD PCH CS to flash delay: [expr {$T_CPLD_to_PCH_SPI_FLASH_CS_max}] ns"
	post_message -type info "CPLD PCH DQ0 to flash delay: [expr {$T_CPLD_SPI_PCH_IO0_to_MUX + $Tmux_prop_max + $Tmux_to_PCH_SPI_FLASH_DQ0}] ns"
	post_message -type info "CPLD PCH DQ1 to flash delay: [expr {$T_CPLD_SPI_PCH_IO1_to_MUX + $Tmux_prop_max + $Tmux_to_PCH_SPI_FLASH_DQ1}] ns"
	post_message -type info "CPLD PCH DQ2 to flash delay: [expr {$T_CPLD_SPI_PCH_IO2_to_MUX + $Tmux_prop_max + $Tmux_to_PCH_SPI_FLASH_DQ2}] ns"
	post_message -type info "CPLD PCH DQ3 to flash delay: [expr {$T_CPLD_SPI_PCH_IO3_to_MUX + $Tmux_prop_max + $Tmux_to_PCH_SPI_FLASH_DQ3}] ns"
	post_message -type info "PCH Flash DQ Setup : $T_SPI_PCH_SPI_FLASH_DQ_S ns"
	post_message -type info "PCH Flash DQ Hold : $T_SPI_PCH_SPI_FLASH_DQ_H ns"
	post_message -type info "PCH Flash CS Setup : $T_SPI_PCH_SPI_FLASH_CS_S ns"
	post_message -type info "PCH Flash CS Hold : $T_SPI_PCH_SPI_FLASH_CS_H ns"

	set_output_delay -max -clock [get_clocks PCH_SPI_CLK] [expr {$T_CPLD_SPI_PCH_IO0_to_MUX + $Tmux_prop_max + $Tmux_to_PCH_SPI_FLASH_DQ0 + $T_SPI_PCH_SPI_FLASH_DQ_S - $T_CPLD_to_PCH_SPI_FLASH_CLK_min}] [get_ports $spi_pin_names_map(PCH_SPI_DQ0)]
	set_output_delay -min -clock [get_clocks PCH_SPI_CLK] [expr {$T_CPLD_SPI_PCH_IO0_to_MUX + $Tmux_prop_min + $Tmux_to_PCH_SPI_FLASH_DQ0 - $T_SPI_PCH_SPI_FLASH_DQ_H - $T_CPLD_to_PCH_SPI_FLASH_CLK_max}] [get_ports $spi_pin_names_map(PCH_SPI_DQ0)]

	set_output_delay -max -clock [get_clocks PCH_SPI_CLK] [expr {$T_CPLD_SPI_PCH_IO1_to_MUX + $Tmux_prop_max + $Tmux_to_PCH_SPI_FLASH_DQ1 + $T_SPI_PCH_SPI_FLASH_DQ_S - $T_CPLD_to_PCH_SPI_FLASH_CLK_min}] [get_ports $spi_pin_names_map(PCH_SPI_DQ1)]
	set_output_delay -min -clock [get_clocks PCH_SPI_CLK] [expr {$T_CPLD_SPI_PCH_IO1_to_MUX + $Tmux_prop_min + $Tmux_to_PCH_SPI_FLASH_DQ1 - $T_SPI_PCH_SPI_FLASH_DQ_H - $T_CPLD_to_PCH_SPI_FLASH_CLK_max}] [get_ports $spi_pin_names_map(PCH_SPI_DQ1)]

	set_output_delay -max -clock [get_clocks PCH_SPI_CLK] [expr {$T_CPLD_SPI_PCH_IO2_to_MUX + $Tmux_prop_max + $Tmux_to_PCH_SPI_FLASH_DQ2 + $T_SPI_PCH_SPI_FLASH_DQ_S - $T_CPLD_to_PCH_SPI_FLASH_CLK_min}] [get_ports $spi_pin_names_map(PCH_SPI_DQ2)]
	set_output_delay -min -clock [get_clocks PCH_SPI_CLK] [expr {$T_CPLD_SPI_PCH_IO2_to_MUX + $Tmux_prop_min + $Tmux_to_PCH_SPI_FLASH_DQ2 - $T_SPI_PCH_SPI_FLASH_DQ_H - $T_CPLD_to_PCH_SPI_FLASH_CLK_max}] [get_ports $spi_pin_names_map(PCH_SPI_DQ2)]

	set_output_delay -max -clock [get_clocks PCH_SPI_CLK] [expr {$T_CPLD_SPI_PCH_IO3_to_MUX + $Tmux_prop_max + $Tmux_to_PCH_SPI_FLASH_DQ3 + $T_SPI_PCH_SPI_FLASH_DQ_S - $T_CPLD_to_PCH_SPI_FLASH_CLK_min}] [get_ports $spi_pin_names_map(PCH_SPI_DQ3)]
	set_output_delay -min -clock [get_clocks PCH_SPI_CLK] [expr {$T_CPLD_SPI_PCH_IO3_to_MUX + $Tmux_prop_min + $Tmux_to_PCH_SPI_FLASH_DQ3 - $T_SPI_PCH_SPI_FLASH_DQ_H - $T_CPLD_to_PCH_SPI_FLASH_CLK_max}] [get_ports $spi_pin_names_map(PCH_SPI_DQ3)]

	set_output_delay -max -clock [get_clocks PCH_SPI_CLK] [expr {$T_CPLD_to_PCH_SPI_FLASH_CS_max + $T_SPI_PCH_SPI_FLASH_CS_S - $T_CPLD_to_PCH_SPI_FLASH_CLK_min}] [get_ports $spi_pin_names_map(PCH_SPI_CS_OUT)]
	set_output_delay -min -clock [get_clocks PCH_SPI_CLK] [expr {$T_CPLD_to_PCH_SPI_FLASH_CS_min - $T_SPI_PCH_SPI_FLASH_CS_H - $T_CPLD_to_PCH_SPI_FLASH_CLK_max}] [get_ports $spi_pin_names_map(PCH_SPI_CS_OUT)]
	#######################################################################################################

	#######################################################################################################
	# Create the input constraints for the SPI Master IP from the PCH Flash
	#######################################################################################################
	# This are classic input/output delays with respect to the SPI clock for all Data in.
	# The SPI devices drive the data on the falling edge of the clock
	# Input max delay = max trace delay for data + tco_max + min trace delay for clock
	# Input min delay = max trace delay for data + tco_min + max trace delay for clock
	post_message -type info "PCH Flash DQ Tco : $T_SPI_PCH_SPI_FLASH_DQ_TCO ns"

	post_message -type info "MON DQ0 Delay : [expr {$T_CPLD_SPI_PCH_IO0_to_MUX + $Tmux_prop_max + $Tmux_to_PCH_SPI_FLASH_DQ0 + $T_SPI_PCH_SPI_FLASH_DQ_TCO + $T_CPLD_to_PCH_SPI_FLASH_CLK_min}]"
	post_message -type info "   T_CPLD_SPI_PCH_IO0_to_MUX : $T_CPLD_SPI_PCH_IO0_to_MUX"
	post_message -type info "   Tmux_prop_max : $Tmux_prop_max"
	post_message -type info "   Tmux_to_PCH_SPI_FLASH_DQ0 : $Tmux_to_PCH_SPI_FLASH_DQ0"
	post_message -type info "   T_SPI_PCH_SPI_FLASH_DQ_TCO : $T_SPI_PCH_SPI_FLASH_DQ_TCO"
	post_message -type info "   T_CPLD_to_PCH_SPI_FLASH_CLK_min : $T_CPLD_to_PCH_SPI_FLASH_CLK_min"

	set_input_delay -max -clock_fall -clock [get_clocks PCH_SPI_CLK] [expr {$T_CPLD_SPI_PCH_IO0_to_MUX + $Tmux_prop_max + $Tmux_to_PCH_SPI_FLASH_DQ0 + $T_SPI_PCH_SPI_FLASH_DQ_TCO + $T_CPLD_to_PCH_SPI_FLASH_CLK_min}] [get_ports $spi_pin_names_map(PCH_SPI_DQ0)]
	set_input_delay -min -clock_fall -clock [get_clocks PCH_SPI_CLK] [expr {$T_CPLD_SPI_PCH_IO0_to_MUX + $Tmux_prop_min + $Tmux_to_PCH_SPI_FLASH_DQ0 + $T_SPI_PCH_SPI_FLASH_DQ_TCO + $T_CPLD_to_PCH_SPI_FLASH_CLK_min}] [get_ports $spi_pin_names_map(PCH_SPI_DQ0)]

	set_input_delay -max -clock_fall -clock [get_clocks PCH_SPI_CLK] [expr {$T_CPLD_SPI_PCH_IO1_to_MUX + $Tmux_prop_max + $Tmux_to_PCH_SPI_FLASH_DQ1 + $T_SPI_PCH_SPI_FLASH_DQ_TCO + $T_CPLD_to_PCH_SPI_FLASH_CLK_min}] [get_ports $spi_pin_names_map(PCH_SPI_DQ1)]
	set_input_delay -min -clock_fall -clock [get_clocks PCH_SPI_CLK] [expr {$T_CPLD_SPI_PCH_IO1_to_MUX + $Tmux_prop_min + $Tmux_to_PCH_SPI_FLASH_DQ1 + $T_SPI_PCH_SPI_FLASH_DQ_TCO + $T_CPLD_to_PCH_SPI_FLASH_CLK_min}] [get_ports $spi_pin_names_map(PCH_SPI_DQ1)]

	set_input_delay -max -clock_fall -clock [get_clocks PCH_SPI_CLK] [expr {$T_CPLD_SPI_PCH_IO2_to_MUX + $Tmux_prop_max + $Tmux_to_PCH_SPI_FLASH_DQ2 + $T_SPI_PCH_SPI_FLASH_DQ_TCO + $T_CPLD_to_PCH_SPI_FLASH_CLK_min}] [get_ports $spi_pin_names_map(PCH_SPI_DQ2)]
	set_input_delay -min -clock_fall -clock [get_clocks PCH_SPI_CLK] [expr {$T_CPLD_SPI_PCH_IO2_to_MUX + $Tmux_prop_min + $Tmux_to_PCH_SPI_FLASH_DQ2 + $T_SPI_PCH_SPI_FLASH_DQ_TCO + $T_CPLD_to_PCH_SPI_FLASH_CLK_min}] [get_ports $spi_pin_names_map(PCH_SPI_DQ2)]

	set_input_delay -max -clock_fall -clock [get_clocks PCH_SPI_CLK] [expr {$T_CPLD_SPI_PCH_IO3_to_MUX + $Tmux_prop_max + $Tmux_to_PCH_SPI_FLASH_DQ3 + $T_SPI_PCH_SPI_FLASH_DQ_TCO + $T_CPLD_to_PCH_SPI_FLASH_CLK_min}] [get_ports $spi_pin_names_map(PCH_SPI_DQ3)]
	set_input_delay -min -clock_fall -clock [get_clocks PCH_SPI_CLK] [expr {$T_CPLD_SPI_PCH_IO3_to_MUX + $Tmux_prop_min + $Tmux_to_PCH_SPI_FLASH_DQ3 + $T_SPI_PCH_SPI_FLASH_DQ_TCO + $T_CPLD_to_PCH_SPI_FLASH_CLK_min}] [get_ports $spi_pin_names_map(PCH_SPI_DQ3)]
	#######################################################################################################

	post_message -type info [string repeat - 80]

	#######################################################################################################
	# Create the CSn constraints when in filtering mode. In this mode the clock is the virtual clock in the
	# PCH
	#######################################################################################################
	post_message -type info "PCH CLK to flash delay: $T_PCH_to_PCH_SPI_FLASH_CLK_min ns"
	post_message -type info "   PCH CLK to mux delay: $T_PCH_SPI_CLK_to_mux ns"
	post_message -type info "   PCH CLK mux delay: $Tmux_prop_min ns"
	post_message -type info "   PCH CLK mux to flash delay: $Tmux_to_PCH_SPI_FLASH_CLK ns"
	post_message -type info "PCH CS to CPLD delay: $T_PCH_SPI_CS_to_CPLD_PCH_SPI_CS_IN ns"
	post_message -type info "   CPLD PCH CS to mux delay: [expr {$T_CPLD_SPI_PCH_CS_N_OUT_to_MUX}] ns"
	post_message -type info "   CPLD PCH CS mux delay: [expr {$Tmux_prop_max}] ns"
	post_message -type info "   CPLD PCH CS mux to flash delay: [expr {$Tmux_to_PCH_SPI_FLASH_CS}] ns"
	post_message -type info "CPLD PCH CS to flash delay: [expr {$T_CPLD_to_PCH_SPI_FLASH_CS_max}] ns"
	post_message -type info "PCH SPI CS Tco : $T_PCH_SPI_CS_TCO_MAX ns"
	set_output_delay -add_delay -max -clock PCH_SPI_CLK_virt [expr {$T_CPLD_to_PCH_SPI_FLASH_CS_max + $T_SPI_PCH_SPI_FLASH_CS_S - $T_PCH_to_PCH_SPI_FLASH_CLK_min}] [get_ports $spi_pin_names_map(PCH_SPI_CS_OUT)]
	set_output_delay -add_delay -min -clock PCH_SPI_CLK_virt [expr {$T_CPLD_to_PCH_SPI_FLASH_CS_max - $T_SPI_PCH_SPI_FLASH_CS_H - $T_PCH_to_PCH_SPI_FLASH_CLK_max}] [get_ports $spi_pin_names_map(PCH_SPI_CS_OUT)]

	# Don't include the clock delay on the input paths. Only include it above in the output delay
	set_input_delay -max -clock PCH_SPI_CLK_virt [expr {$T_PCH_SPI_CS_TCO_MAX + $T_PCH_SPI_CS_to_CPLD_PCH_SPI_CS_IN}] [get_ports $spi_pin_names_map(PCH_SPI_CS_IN)]
	set_input_delay -min -clock PCH_SPI_CLK_virt [expr {$T_PCH_SPI_CS_TCO_MIN + $T_PCH_SPI_CS_to_CPLD_PCH_SPI_CS_IN}] [get_ports $spi_pin_names_map(PCH_SPI_CS_IN)]
	#######################################################################################################

	post_message -type info [string repeat - 80]

	#######################################################################################################
	# Create the DQ/CLK constraints when in filtering mode. In this mode the clock is the virtual clock in the
	# PCH. The PCH output is on the falling edge of the clock
	#######################################################################################################
	# These paths are PCH Tco then PCH to Mux, Mux Tpd, mux to CPLD minus PCH clock to mux to CPLD
	post_message -type info "PCH CLK to CPLD delay: $T_PCH_to_CPLD_PCH_SPI_CLK_min ns"
	post_message -type info "PCH DQ0 to CPLD delay: $T_PCH_SPI_DQ0_to_CPLD_min ns"
	post_message -type info "PCH DQ1 to CPLD delay: $T_PCH_SPI_DQ1_to_CPLD_min ns"
	post_message -type info "PCH DQ2 to CPLD delay: $T_PCH_SPI_DQ2_to_CPLD_min ns"
	post_message -type info "PCH DQ3 to CPLD delay: $T_PCH_SPI_DQ3_to_CPLD_min ns"
	post_message -type info "PCH SPI DQ Tco : $T_PCH_SPI_DQ_TCO_MAX ns"
	post_message -type info "PCH DQ0 input delay : [expr {$T_PCH_SPI_DQ_TCO_MAX + $T_PCH_SPI_DQ0_to_CPLD_max - $T_PCH_to_CPLD_PCH_SPI_CLK_min}] ns"
	set_input_delay -max -clock_fall -clock PCH_SPI_CLK_virt [expr {$T_PCH_SPI_DQ_TCO_MAX + $T_PCH_SPI_DQ0_to_CPLD_max - $T_PCH_to_CPLD_PCH_SPI_CLK_min}] [get_ports $spi_pin_names_map(PCH_SPI_DQ0_MON)]
	set_input_delay -min -clock_fall -clock PCH_SPI_CLK_virt [expr {$T_PCH_SPI_DQ_TCO_MIN + $T_PCH_SPI_DQ0_to_CPLD_min - $T_PCH_to_CPLD_PCH_SPI_CLK_max}] [get_ports $spi_pin_names_map(PCH_SPI_DQ0_MON)]
	set_input_delay -max -clock_fall -clock PCH_SPI_CLK_virt [expr {$T_PCH_SPI_DQ_TCO_MAX + $T_PCH_SPI_DQ1_to_CPLD_max - $T_PCH_to_CPLD_PCH_SPI_CLK_min}] [get_ports $spi_pin_names_map(PCH_SPI_DQ1_MON)]
	set_input_delay -min -clock_fall -clock PCH_SPI_CLK_virt [expr {$T_PCH_SPI_DQ_TCO_MIN + $T_PCH_SPI_DQ1_to_CPLD_min - $T_PCH_to_CPLD_PCH_SPI_CLK_max}] [get_ports $spi_pin_names_map(PCH_SPI_DQ1_MON)]
	set_input_delay -max -clock_fall -clock PCH_SPI_CLK_virt [expr {$T_PCH_SPI_DQ_TCO_MAX + $T_PCH_SPI_DQ2_to_CPLD_max - $T_PCH_to_CPLD_PCH_SPI_CLK_min}] [get_ports $spi_pin_names_map(PCH_SPI_DQ2_MON)]
	set_input_delay -min -clock_fall -clock PCH_SPI_CLK_virt [expr {$T_PCH_SPI_DQ_TCO_MIN + $T_PCH_SPI_DQ2_to_CPLD_min - $T_PCH_to_CPLD_PCH_SPI_CLK_max}] [get_ports $spi_pin_names_map(PCH_SPI_DQ2_MON)]
	set_input_delay -max -clock_fall -clock PCH_SPI_CLK_virt [expr {$T_PCH_SPI_DQ_TCO_MAX + $T_PCH_SPI_DQ3_to_CPLD_max - $T_PCH_to_CPLD_PCH_SPI_CLK_min}] [get_ports $spi_pin_names_map(PCH_SPI_DQ3_MON)]
	set_input_delay -min -clock_fall -clock PCH_SPI_CLK_virt [expr {$T_PCH_SPI_DQ_TCO_MIN + $T_PCH_SPI_DQ3_to_CPLD_min - $T_PCH_to_CPLD_PCH_SPI_CLK_max}] [get_ports $spi_pin_names_map(PCH_SPI_DQ3_MON)]

	post_message -type info [string repeat # 80]
}
set_pch_spi_interface_constraints

proc set_bmc_spi_interface_constraints {} {
	global SPICLK
	global spi_pin_names_map

	post_message -type info "Creating BMC SPI interface constraints"
	post_message -type info [string repeat # 80]

	# BMC SPI Interface numbers
	# Use 50 MHz SPI CLK
	# TCO numbers from from BMC Datasheet
	set SPI_BMC_SPI_CLK_PERIOD 20.000
	set T_BMC_SPI_DQ_TCO_MIN 1.000
	set T_BMC_SPI_DQ_TCO_MAX 2.000
	# CS Tco == 2*tAHB + 0.5*tCK
	set T_BMC_SPI_CS_TCO_MIN [expr {$SPI_BMC_SPI_CLK_PERIOD - (2*5.0 + ($SPI_BMC_SPI_CLK_PERIOD/2.0))}]
	set T_BMC_SPI_CS_TCO_MAX [expr {$SPI_BMC_SPI_CLK_PERIOD - (2*5.0 + ($SPI_BMC_SPI_CLK_PERIOD/2.0))}]

	# PCH Flash device properties from datasheet
	set T_SPI_BMC_SPI_FLASH_DQ_S 1.75
	set T_SPI_BMC_SPI_FLASH_DQ_H 2.3
	set T_SPI_BMC_SPI_FLASH_CS_S 3.375
	set T_SPI_BMC_SPI_FLASH_CS_H 3.375
	set T_SPI_BMC_SPI_FLASH_DQ_TCO 6.0
	
	# SPI Mux properties from datasheet
	set Tmux_prop_min 0.2
	set Tmux_prop_max 0.2

	#######################################################################################################
	# BMC Flash board delays
	#######################################################################################################
	# CPLD -> SPI_PFR_BMC_FLASH1_BT_CLK mux
	# Path: U6D2.C15 (SPI_PFR_BMC_FLASH1_BT_CLK) to U3P1.10 (SPI_PFR_BMC_FLASH1_BT_CLK)
	# U6D2.C15 -> (2459.81) -> U3P1.10
	# Total path length: 2459.81
	set T_CPLD_SPI_BMC_CLK_to_MUX [calc_prop_delay [expr {2459.81}]]

	# CPLD -> SPI_PFR_BOOT_CS1_N mux
	# Path: U6D2.B18 (SPI_BMC_BOOT_CS_N) to U3P1.14 (SPI_PFR_BMC_BT_SECURE_CS_R2_N)
	# U6D2.B18 -> (2532.72) -> R3P7.1 -> (0) -> R3P7.2 -> (98.53) -> R3P6.2 -> (35.0) -> R3P8.1 -> (93.9) -> U3P1.13 -> (117.79) -> U3P1.14
	# Total path length: 2877.94
	set T_CPLD_SPI_BMC_CS_N_OUT_to_MUX [calc_prop_delay [expr {2532.72 + 98.53 + 35.0 + 93.9 + 117.79}]]

	# CPLD -> SPI_PFR_BMC_FLASH1_BT_MOSI
	# Path: U6D2.D12 (SPI_PFR_BMC_FLASH1_BT_MOSI) to U3P1.3 (SPI_PFR_BMC_FLASH1_BT_MOSI)
	# U6D2.D12 -> (3135.34) -> U3P1.3
	# Total path length: 3135.34
	set T_CPLD_SPI_BMC_IO0_to_MUX [calc_prop_delay [expr {3135.34}]]

	# CPLD -> SPI_PFR_BMC_FLASH1_BT_MISO mux
	# Path: U6D2.C16 (SPI_PFR_BMC_FLASH1_BT_MISO) to U3P1.6 (SPI_PFR_BMC_FLASH1_BT_MISO)
	# U6D2.C16 -> (3125.39) -> U3P1.6
	# Total path length: 3125.39
	set T_CPLD_SPI_BMC_IO1_to_MUX [calc_prop_delay [expr {3125.39}]]

	# CPLD -> BMC SPI FLASH DQ2
	# Path: U6D2.D13 (SPI_PFR_BMC_BOOT_R_IO2) to XU7D1.9 (SPI_PFR_BMC_BOOT_R1_IO2)
	# U6D2.D13 -> (579.37) -> R4P9.1 -> (0) -> R4P9.2 -> (8739.87) -> R7D1.1 -> (0) -> R7D1.2 -> (161.4) -> XU7D1.9
	# Total path length: 9480.64
	set T_CPLD_SPI_BMC_IO2_to_BMC_SPI_FLASH_DQ2 [calc_prop_delay [expr {579.37 + 8739.87 + 161.4}]]

	# CPLD -> BMC SPI FLASH DQ3
	# Path: U6D2.D14 (SPI_PFR_BMC_BOOT_R_IO3) to XU7D1.1 (SPI_PFR_BMC_BOOT_R1_IO3)
	# U6D2.D14 -> (324.13) -> R4P13.1 -> (0) -> R4P13.2 -> (9545.96) -> R7D10.1 -> (0) -> R7D10.2 -> (139.42) -> XU7D1.1
	# Total path length: 10009.51
	set T_CPLD_SPI_BMC_IO3_to_BMC_SPI_FLASH_DQ3 [calc_prop_delay [expr {324.13 + 9545.96 + 139.42}]]

	# mux -> BMC SPI FLASH CLK flash
	# Path: U3P1.9 (SPI_PFR_BMC_BOOT_MUXED_CLK) to XU7D1.16 (SPI_BMC_BOOT_MUXED_R_CLK)
	# U3P1.9 -> (127.88) -> R3P12.2 -> (7330.2) -> R7D9.1 -> (0) -> R7D9.2 -> (344.38) -> XU7D1.16
	# Total path length: 7802.46
	set Tmux_to_BMC_SPI_FLASH_CLK [calc_prop_delay [expr {127.88 + 7330.2 + 344.38}]]

	# mux -> BMC SPI FLASH CS flash
	# Path: U3P1.12 (SPI_BMC_PFR_BOOT_CS0_N) to XU7D1.7 (SPI_BMC_PFR_BOOT_R2_CS_N)
	# U3P1.12 -> (92.24) -> R3P8.2 -> (1554.94) -> R6C98.1 -> (0) -> R6C98.2 -> (2954.95) -> R7D36.2 -> (0) -> R7D36.1 -> (45.3) -> R7D2.1 -> (0) -> R7D2.2 -> (229.88) -> XU7D1.7
	# Total path length: 4877.31
	set Tmux_to_BMC_SPI_FLASH_CS [calc_prop_delay [expr {92.24 + 1554.94 + 2954.95 + 45.3 + 229.88}]]

	# mux -> BMC SPI FLASH DQ0 (MSI) flash
	# Path: U3P1.4 (SPI_BMC_FLASH1_MUXED_MOSI_R) to XU7D1.15 (SPI_BMC_BOOT_MUXED_R_MOSI)
	# U3P1.4 -> (157.94) -> R3P9.1 -> (0) -> R3P9.2 -> (7577.85) -> R7D7.1 -> (0) -> R7D7.2 -> (155.02) -> XU7D1.15
	# Total path length: 7890.81
	set Tmux_to_BMC_SPI_FLASH_DQ0 [calc_prop_delay [expr {157.94 + 7577.85 + 155.02}]]

	# mux -> BMC SPI FLASH DQ1 (MSO) flash
	# Path: U3P1.7 (SPI_PFR_BMC_BOOT_MUXED_MISO) to XU7D1.8 (SPI_BMC_BOOT_MUXED_R_MISO)
	# U3P1.7 -> (7279.45) -> R7D3.1 -> (0) -> R7D3.2 -> (630.91) -> XU7D1.8
	# Total path length: 7910.36
	set Tmux_to_BMC_SPI_FLASH_DQ1 [calc_prop_delay [expr {7279.45 + 630.91}]]

	# mux -> SPI_BMC_BT_MUXED_MON_CLK cpld pin
	# Path: U6D2.E14 (SPI_BMC_BT_MUXED_MON_CLK) to U3P1.9 (SPI_PFR_BMC_BOOT_MUXED_CLK)
	# U6D2.E14 -> (599.32) -> R6D79.2 -> (0) -> R6D79.1 -> (2302.74) -> R7D9.1 -> (7330.2) -> R3P12.2 -> (127.88) -> U3P1.9
	# Total path length: 10360.14
	set Tmux_to_CPLD_SPI_BMC_CLK_MON [calc_prop_delay [expr {599.32 + 2302.74 + 7330.2 + 127.88}]]

	# mux -> SPI_BMC_BT_MUXED_MON_MOSI cpld pin
	# Path: U6D2.F12 (SPI_BMC_BT_MUXED_MON_MOSI) to U3P1.4 (SPI_BMC_FLASH1_MUXED_MOSI_R)
	# U6D2.F12 -> (522.04) -> R4P19.2 -> (0) -> R4P19.1 -> (2301.86) -> R7D7.1 -> (7577.85) -> R3P9.2 -> (0) -> R3P9.1 -> (157.94) -> U3P1.4
	# Total path length: 10559.69
	set Tmux_to_CPLD_SPI_BMC_DQ0_MON [calc_prop_delay [expr {522.04 + 2301.86 + 7577.85 + 157.94}]]

	# mux -> SPI_BMC_BT_MUXED_MON_MISO cpld pin
	# Path: U6D2.F11 (SPI_BMC_BT_MUXED_MON_MISO) to U3P1.7 (SPI_PFR_BMC_BOOT_MUXED_MISO)
	# U6D2.F11 -> (601.32) -> R6D81.2 -> (0) -> R6D81.1 -> (6554.43) -> R6C95.2 -> (4653.04) -> R7D3.1 -> (7279.45) -> U3P1.7
	# Total path length: 19088.24
	set Tmux_to_CPLD_SPI_BMC_DQ1_MON [calc_prop_delay [expr {601.32 + 6554.43 + 4653.04 + 7279.45}]]

	# BMC -> BMC SPI CLK mux
	# Path: U3P1.11 (SPI_BMC_BOOT_CLK) to U7B6.AA18 (SPI_BMC_BOOT_R_CLK)
	# U3P1.11 -> (3722.79) -> R3M96.2 -> (0) -> R3M96.1 -> (538.09) -> U7B6.AA18
	# Total path length: 4260.88
	set T_BMC_SPI_CLK_to_mux [calc_prop_delay [expr {3722.79 + 538.09}]]

	# BMC -> DQ0 (MSI) Mux
	# Path: U3P1.2 (SPI_BMC_BOOT_MOSI) to U7B6.U17 (SPI_BMC_BOOT_R_MOSI)
	# U3P1.2 -> (110.18) -> R3P3.1 -> (3802.38) -> R3M91.2 -> (0) -> R3M91.1 -> (494.16) -> U7B6.U17
	# Total path length: 4406.72
	set T_BMC_SPI_DQ0_to_mux [calc_prop_delay [expr {110.18 + 3802.38 + 494.16}]]

	# BMC -> DQ1 Mux
	# Path: U3P1.5 (SPI_BMC_BOOT_MISO) to U7B6.T18 (SPI_BMC_BOOT_R_MISO)
	# U3P1.5 -> (95.6) -> R3P11.1 -> (3792.51) -> R3M78.1 -> (61.14) -> R3M85.2 -> (0) -> R3M85.1 -> (499.55) -> U7B6.T18
	# Total path length: 4448.8
	set T_BMC_SPI_DQ1_to_mux [calc_prop_delay [expr {95.6 + 3792.51 + 61.14 + 499.55}]]

	# BMC -> SPI_BMC_BOOT_CS_N CPLD
	# Path: U6D2.B18 (SPI_BMC_BOOT_CS_N) to U7B6.AB19 (SPI_BMC_BOOT_CS_R_N)
	# U6D2.B18 -> (2532.72) -> R3P7.1 -> (3995.58) -> R7B67.1 -> (37.0) -> R7B66.1 -> (68.7) -> R7B64.2 -> (0) -> R7B64.1 -> (500.04) -> U7B6.AB19
	# Total path length: 7134.04
	set T_BMC_SPI_CS_to_CPLD_SPI_BMC_CS_N_IN [calc_prop_delay [expr {2532.72 + 3995.58 + 37.0 + 68.7 + 500.04}]]

	#######################################################################################################
	# Compute shorthand variables
	#######################################################################################################
	set T_CPLD_to_BMC_SPI_FLASH_CLK_min [expr {$T_CPLD_SPI_BMC_CLK_to_MUX + $Tmux_prop_min + $Tmux_to_BMC_SPI_FLASH_CLK}]
	set T_CPLD_to_BMC_SPI_FLASH_CLK_max [expr {$T_CPLD_SPI_BMC_CLK_to_MUX + $Tmux_prop_max + $Tmux_to_BMC_SPI_FLASH_CLK}]

	set T_CPLD_to_BMC_SPI_FLASH_CS_min [expr {$T_CPLD_SPI_BMC_CS_N_OUT_to_MUX + $Tmux_prop_min + $Tmux_to_BMC_SPI_FLASH_CS}]
	set T_CPLD_to_BMC_SPI_FLASH_CS_max [expr {$T_CPLD_SPI_BMC_CS_N_OUT_to_MUX + $Tmux_prop_max + $Tmux_to_BMC_SPI_FLASH_CS}]

	set T_BMC_to_BMC_SPI_FLASH_CLK_min [expr {$T_BMC_SPI_CLK_to_mux + $Tmux_prop_min + $Tmux_to_BMC_SPI_FLASH_CLK}]
	set T_BMC_to_BMC_SPI_FLASH_CLK_max [expr {$T_BMC_SPI_CLK_to_mux + $Tmux_prop_max + $Tmux_to_BMC_SPI_FLASH_CLK}]

	set T_BMC_to_CPLD_BMC_SPI_CLK_min [expr {$T_BMC_SPI_CLK_to_mux + $Tmux_prop_min + $Tmux_to_CPLD_SPI_BMC_CLK_MON}]
	set T_BMC_to_CPLD_BMC_SPI_CLK_max [expr {$T_BMC_SPI_CLK_to_mux + $Tmux_prop_max + $Tmux_to_CPLD_SPI_BMC_CLK_MON}]

	set T_BMC_SPI_DQ0_to_CPLD_min [expr {$T_BMC_SPI_DQ0_to_mux + $Tmux_prop_min + $Tmux_to_CPLD_SPI_BMC_DQ0_MON}]
	set T_BMC_SPI_DQ0_to_CPLD_max [expr {$T_BMC_SPI_DQ0_to_mux + $Tmux_prop_max + $Tmux_to_CPLD_SPI_BMC_DQ0_MON}]
	set T_BMC_SPI_DQ1_to_CPLD_min [expr {$T_BMC_SPI_DQ1_to_mux + $Tmux_prop_min + $Tmux_to_CPLD_SPI_BMC_DQ1_MON}]
	set T_BMC_SPI_DQ1_to_CPLD_max [expr {$T_BMC_SPI_DQ1_to_mux + $Tmux_prop_max + $Tmux_to_CPLD_SPI_BMC_DQ1_MON}]

	# Create the clock on the pin for the BMC SPI clock
	create_generated_clock -name BMC_SPI_CLK -source [get_clock_info -targets [get_clocks spi_master_clk]] [get_ports $spi_pin_names_map(BMC_SPI_CLK)]

	# SPI input timing. Data is output on falling edge of SPI clock and latched on rising edge of FPGA clock.
	# The latch edge is aligns with the next falling edge of the launch clock. Since the dest clock
	# is 2x the source clock, a multicycle is used. 
	#  
	#   Data Clock : BMC_SPI_CLK
	#       (Launch)
	#         v
	#   +-----+     +-----+     +-----+
	#   |     |     |     |     |     |	
	#   +     +-----+     +-----+     +-----
	#
	#   FPGA Clock : $SPICLK 
	#            (Latch)
	#                     v
	#   +--+  +--+  +--+  +--+  +--+  +--+
	#   |  |  |  |  |  |  |  |  |  |  |  |	
	#   +  +--+  +--+  +--+  +--+  +--+  +--
	#        (H)         (S)          
	# Setup relationship : BMC_SPI_CLK period == 2 * $SPICLK period
	# Hold relationship : 0
	set_multicycle_path -from [get_clocks BMC_SPI_CLK] -to [get_clocks $SPICLK] -setup -end 2
	set_multicycle_path -from [get_clocks BMC_SPI_CLK] -to [get_clocks $SPICLK] -hold -end 1

	# SPI Output multicycle to ensure previous edge used for hold. This is because the SPI output clock is 180deg
	# from the SPI clock
	#  

	#   Data Clock : $SPICLK 
	#            (Launch)
	#               v
	#   +--+  +--+  +--+  +--+  +--+  +--+
	#   |  |  |  |  |  |  |  |  |  |  |  |	
	#   +  +--+  +--+  +--+  +--+  +--+  +--

	#   Data Clock 2X : spi_master_clk
	#            (Launch)
	#               v
	#   +-----+     +-----+     +-----+
	#   |     |     |     |     |     |	
	#   +     +-----+     +-----+     +-----
	#
	#   Output Clock : BMC_SPI_CLK
	#                  (Latch)
	#                     v
	#   +     +-----+     +-----+     +-----+
	#   |     |     |     |     |     |     |	
	#   +-----+     +-----+     +-----+     +-----
	#        (H)         (S)          
	#
	# Setup relationship : BMC_SPI_CLK period / 2
	# Hold relationship : -BMC_SPI_CLK period / 2
	set_multicycle_path -from [get_clocks $SPICLK] -to [get_clocks BMC_SPI_CLK] -hold -start 1


	# SPI CS timing when in filtering mode. Data is output from BMC/PCH on rising edge and latched on rising edge. 
	#  
	#   Data Clock : BMC_SPI_CLK_virt
	#          (Launch)
	#             v
	#   +----+    +----+    +----+
	#   |    |    |    |    |    |	
	#   +    +----+    +----+    +----
	#
	#   SPI Clock : BMC_SPI_CLK_virt
	#                    (Latch)
	#                       v
	#   +----+    +----+    +----+
	#   |    |    |    |    |    |	
	#   +    +----+    +----+    +----
	#            (H)       (S)        
	#
	# Setup relationship : SPI_CLK period
	# Hold relationship : 0

	# Create the virtual clock for the BMC SPI clock located in the BMC. It is exclusive from the PLL clock
	create_clock -name BMC_SPI_CLK_virt -period $SPI_BMC_SPI_CLK_PERIOD
	set_clock_groups -logically_exclusive -group {BMC_SPI_CLK_virt} -group {BMC_SPI_CLK}

	# Create the monitored SPI clock pin. This is the clock from the BMC
	create_clock -name BMC_SPI_MON_CLK -period $SPI_BMC_SPI_CLK_PERIOD [get_ports $spi_pin_names_map(BMC_SPI_CLK_MON)]
	
	#######################################################################################################
	# Create the output constraints for the SPI Master IP to BMC flash
	#######################################################################################################
	# This are classic output delays with respect to the SPI clock for all data out and CS. The SPI master
	# drives these 180deg earlier than then the clock
	# Output max delay = max trace delay for data + tsu - min trace delay of clock
	# Output min delay = min trace delay for data - th - max trace delay of clock

	post_message -type info "BMC CLK Period: $SPI_BMC_SPI_CLK_PERIOD ns"
	post_message -type info "CPLD BMC CLK to flash delay: [expr {$T_CPLD_to_BMC_SPI_FLASH_CLK_min}] ns"
	post_message -type info "CPLD BMC CS to flash delay: [expr {$T_CPLD_to_BMC_SPI_FLASH_CS_max}] ns"
	post_message -type info "CPLD BMC DQ0 to flash delay: [expr {$T_CPLD_SPI_BMC_IO0_to_MUX + $Tmux_prop_max + $Tmux_to_BMC_SPI_FLASH_DQ0}] ns"
	post_message -type info "CPLD BMC DQ1 to flash delay: [expr {$T_CPLD_SPI_BMC_IO1_to_MUX + $Tmux_prop_max + $Tmux_to_BMC_SPI_FLASH_DQ1}] ns"
	post_message -type info "CPLD BMC DQ2 to flash delay: [expr {$T_CPLD_SPI_BMC_IO2_to_BMC_SPI_FLASH_DQ2}] ns"
	post_message -type info "CPLD BMC DQ3 to flash delay: [expr {$T_CPLD_SPI_BMC_IO3_to_BMC_SPI_FLASH_DQ3}] ns"
	post_message -type info "BMC Flash DQ Setup : $T_SPI_BMC_SPI_FLASH_DQ_S ns"
	post_message -type info "BMC Flash DQ Hold : $T_SPI_BMC_SPI_FLASH_DQ_H ns"
	post_message -type info "BMC Flash CS Setup : $T_SPI_BMC_SPI_FLASH_CS_S ns"
	post_message -type info "BMC Flash CS Hold : $T_SPI_BMC_SPI_FLASH_CS_H ns"

	set_output_delay -max -clock [get_clocks BMC_SPI_CLK] [expr {$T_CPLD_SPI_BMC_IO0_to_MUX + $Tmux_prop_max + $Tmux_to_BMC_SPI_FLASH_DQ0 + $T_SPI_BMC_SPI_FLASH_DQ_S - $T_CPLD_to_BMC_SPI_FLASH_CLK_min}] [get_ports $spi_pin_names_map(BMC_SPI_DQ0)]
	set_output_delay -min -clock [get_clocks BMC_SPI_CLK] [expr {$T_CPLD_SPI_BMC_IO0_to_MUX + $Tmux_prop_min + $Tmux_to_BMC_SPI_FLASH_DQ0 - $T_SPI_BMC_SPI_FLASH_DQ_H - $T_CPLD_to_BMC_SPI_FLASH_CLK_max}] [get_ports $spi_pin_names_map(BMC_SPI_DQ0)]

	set_output_delay -max -clock [get_clocks BMC_SPI_CLK] [expr {$T_CPLD_SPI_BMC_IO1_to_MUX + $Tmux_prop_max + $Tmux_to_BMC_SPI_FLASH_DQ1 + $T_SPI_BMC_SPI_FLASH_DQ_S - $T_CPLD_to_BMC_SPI_FLASH_CLK_min}] [get_ports $spi_pin_names_map(BMC_SPI_DQ1)]
	set_output_delay -min -clock [get_clocks BMC_SPI_CLK] [expr {$T_CPLD_SPI_BMC_IO1_to_MUX + $Tmux_prop_min + $Tmux_to_BMC_SPI_FLASH_DQ1 - $T_SPI_BMC_SPI_FLASH_DQ_H - $T_CPLD_to_BMC_SPI_FLASH_CLK_max}] [get_ports $spi_pin_names_map(BMC_SPI_DQ1)]

	set_output_delay -max -clock [get_clocks BMC_SPI_CLK] [expr {$T_CPLD_SPI_BMC_IO2_to_BMC_SPI_FLASH_DQ2 + $T_SPI_BMC_SPI_FLASH_DQ_S - $T_CPLD_to_BMC_SPI_FLASH_CLK_min}] [get_ports $spi_pin_names_map(BMC_SPI_DQ2)]
	set_output_delay -min -clock [get_clocks BMC_SPI_CLK] [expr {$T_CPLD_SPI_BMC_IO2_to_BMC_SPI_FLASH_DQ2 - $T_SPI_BMC_SPI_FLASH_DQ_H - $T_CPLD_to_BMC_SPI_FLASH_CLK_max}] [get_ports $spi_pin_names_map(BMC_SPI_DQ2)]

	set_output_delay -max -clock [get_clocks BMC_SPI_CLK] [expr {$T_CPLD_SPI_BMC_IO3_to_BMC_SPI_FLASH_DQ3 + $T_SPI_BMC_SPI_FLASH_DQ_S - $T_CPLD_to_BMC_SPI_FLASH_CLK_min}] [get_ports $spi_pin_names_map(BMC_SPI_DQ3)]
	set_output_delay -min -clock [get_clocks BMC_SPI_CLK] [expr {$T_CPLD_SPI_BMC_IO3_to_BMC_SPI_FLASH_DQ3 - $T_SPI_BMC_SPI_FLASH_DQ_H - $T_CPLD_to_BMC_SPI_FLASH_CLK_max}] [get_ports $spi_pin_names_map(BMC_SPI_DQ3)]

	set_output_delay -max -clock [get_clocks BMC_SPI_CLK] [expr {$T_CPLD_to_BMC_SPI_FLASH_CS_max + $T_SPI_BMC_SPI_FLASH_CS_S - $T_CPLD_to_BMC_SPI_FLASH_CLK_min}] [get_ports $spi_pin_names_map(BMC_SPI_CS_OUT)]
	set_output_delay -min -clock [get_clocks BMC_SPI_CLK] [expr {$T_CPLD_to_BMC_SPI_FLASH_CS_min - $T_SPI_BMC_SPI_FLASH_CS_H - $T_CPLD_to_BMC_SPI_FLASH_CLK_max}] [get_ports $spi_pin_names_map(BMC_SPI_CS_OUT)]
	#######################################################################################################

	#######################################################################################################
	# Create the input constraints for the SPI Master IP from the BMC Flash
	#######################################################################################################
	# This are classic input delays with respect to the SPI clock for all Data in.
	# The flash drives the data on the falling edge of the clock
	# Input max delay = max trace delay for data + tco_max + min trace delay for clock
	# Input min delay = max trace delay for data + tco_min + max trace delay for clock

	post_message -type info "BMC Flash DQ Tco : $T_SPI_BMC_SPI_FLASH_DQ_TCO ns"
	post_message -type info "MON DQ0 Delay : [expr {$T_CPLD_SPI_BMC_IO0_to_MUX + $Tmux_prop_max + $Tmux_to_BMC_SPI_FLASH_DQ0 + $T_SPI_BMC_SPI_FLASH_DQ_TCO + $T_CPLD_to_BMC_SPI_FLASH_CLK_min}]"
	post_message -type info "   T_CPLD_SPI_BMC_IO0_to_MUX : $T_CPLD_SPI_BMC_IO0_to_MUX"
	post_message -type info "   Tmux_prop_max : $Tmux_prop_max"
	post_message -type info "   Tmux_to_BMC_SPI_FLASH_DQ0 : $Tmux_to_BMC_SPI_FLASH_DQ0"
	post_message -type info "   T_SPI_BMC_SPI_FLASH_DQ_TCO : $T_SPI_BMC_SPI_FLASH_DQ_TCO"
	post_message -type info "   T_CPLD_to_BMC_SPI_FLASH_CLK_min : $T_CPLD_to_BMC_SPI_FLASH_CLK_min"

	set_input_delay -max -clock_fall -clock [get_clocks BMC_SPI_CLK] [expr {$T_CPLD_SPI_BMC_IO0_to_MUX + $Tmux_prop_max + $Tmux_to_BMC_SPI_FLASH_DQ0 + $T_SPI_BMC_SPI_FLASH_DQ_TCO + $T_CPLD_to_BMC_SPI_FLASH_CLK_min}] [get_ports $spi_pin_names_map(BMC_SPI_DQ0)]
	set_input_delay -min -clock_fall -clock [get_clocks BMC_SPI_CLK] [expr {$T_CPLD_SPI_BMC_IO0_to_MUX + $Tmux_prop_min + $Tmux_to_BMC_SPI_FLASH_DQ0 + $T_SPI_BMC_SPI_FLASH_DQ_TCO + $T_CPLD_to_BMC_SPI_FLASH_CLK_min}] [get_ports $spi_pin_names_map(BMC_SPI_DQ0)]

	set_input_delay -max -clock_fall -clock [get_clocks BMC_SPI_CLK] [expr {$T_CPLD_SPI_BMC_IO1_to_MUX + $Tmux_prop_max + $Tmux_to_BMC_SPI_FLASH_DQ1 + $T_SPI_BMC_SPI_FLASH_DQ_TCO + $T_CPLD_to_BMC_SPI_FLASH_CLK_min}] [get_ports $spi_pin_names_map(BMC_SPI_DQ1)]
	set_input_delay -min -clock_fall -clock [get_clocks BMC_SPI_CLK] [expr {$T_CPLD_SPI_BMC_IO1_to_MUX + $Tmux_prop_min + $Tmux_to_BMC_SPI_FLASH_DQ1 + $T_SPI_BMC_SPI_FLASH_DQ_TCO + $T_CPLD_to_BMC_SPI_FLASH_CLK_min}] [get_ports $spi_pin_names_map(BMC_SPI_DQ1)]

	set_input_delay -max -clock_fall -clock [get_clocks BMC_SPI_CLK] [expr {$T_CPLD_SPI_BMC_IO2_to_BMC_SPI_FLASH_DQ2 + $T_SPI_BMC_SPI_FLASH_DQ_TCO + $T_CPLD_to_BMC_SPI_FLASH_CLK_min}] [get_ports $spi_pin_names_map(BMC_SPI_DQ2)]
	set_input_delay -min -clock_fall -clock [get_clocks BMC_SPI_CLK] [expr {$T_CPLD_SPI_BMC_IO2_to_BMC_SPI_FLASH_DQ2 + $T_SPI_BMC_SPI_FLASH_DQ_TCO + $T_CPLD_to_BMC_SPI_FLASH_CLK_min}] [get_ports $spi_pin_names_map(BMC_SPI_DQ2)]

	set_input_delay -max -clock_fall -clock [get_clocks BMC_SPI_CLK] [expr {$T_CPLD_SPI_BMC_IO3_to_BMC_SPI_FLASH_DQ3 + $T_SPI_BMC_SPI_FLASH_DQ_TCO + $T_CPLD_to_BMC_SPI_FLASH_CLK_min}] [get_ports $spi_pin_names_map(BMC_SPI_DQ3)]
	set_input_delay -min -clock_fall -clock [get_clocks BMC_SPI_CLK] [expr {$T_CPLD_SPI_BMC_IO3_to_BMC_SPI_FLASH_DQ3 + $T_SPI_BMC_SPI_FLASH_DQ_TCO + $T_CPLD_to_BMC_SPI_FLASH_CLK_min}] [get_ports $spi_pin_names_map(BMC_SPI_DQ3)]
	#######################################################################################################

	post_message -type info [string repeat - 80]

	#######################################################################################################
	# Create the CSn constraints when in filtering mode. In this mode the clock is the virtual clock in the
	# BMC
	#######################################################################################################
	post_message -type info "BMC CLK to flash delay: $T_BMC_to_BMC_SPI_FLASH_CLK_min ns"
	post_message -type info "   BMC CLK to mux delay: $T_BMC_SPI_CLK_to_mux ns"
	post_message -type info "   BMC CLK mux delay: $Tmux_prop_min ns"
	post_message -type info "   BMC CLK mux to flash delay: $Tmux_to_BMC_SPI_FLASH_CLK ns"
	post_message -type info "BMC CS to CPLD delay: $T_BMC_SPI_CS_to_CPLD_SPI_BMC_CS_N_IN ns"
	post_message -type info "CPLD BMC CS to flash delay: [expr {$T_CPLD_to_BMC_SPI_FLASH_CS_max}] ns"
	post_message -type info "   CPLD BMC CS to mux delay: [expr {$T_CPLD_SPI_BMC_CS_N_OUT_to_MUX}] ns"
	post_message -type info "   CPLD BMC CS mux delay: [expr {$Tmux_prop_max}] ns"
	post_message -type info "   CPLD BMC CS mux to flash delay: [expr {$Tmux_to_BMC_SPI_FLASH_CS}] ns"
	post_message -type info "BMC SPI CS Tco : $T_BMC_SPI_CS_TCO_MAX ns"

	set_output_delay -add_delay -max -clock BMC_SPI_CLK_virt [expr {$T_CPLD_to_BMC_SPI_FLASH_CS_max + $T_SPI_BMC_SPI_FLASH_CS_S - $T_BMC_to_BMC_SPI_FLASH_CLK_min}] [get_ports $spi_pin_names_map(BMC_SPI_CS_OUT)]
	set_output_delay -add_delay -min -clock BMC_SPI_CLK_virt [expr {$T_CPLD_to_BMC_SPI_FLASH_CS_max - $T_SPI_BMC_SPI_FLASH_CS_H - $T_BMC_to_BMC_SPI_FLASH_CLK_max}] [get_ports $spi_pin_names_map(BMC_SPI_CS_OUT)]

	# Don't include the clock delay here. It is included above.
	set_input_delay -max -clock BMC_SPI_CLK_virt [expr {$T_BMC_SPI_CS_TCO_MAX + $T_BMC_SPI_CS_to_CPLD_SPI_BMC_CS_N_IN}] [get_ports $spi_pin_names_map(BMC_SPI_CS_IN)]
	set_input_delay -min -clock BMC_SPI_CLK_virt [expr {$T_BMC_SPI_CS_TCO_MIN + $T_BMC_SPI_CS_to_CPLD_SPI_BMC_CS_N_IN}] [get_ports $spi_pin_names_map(BMC_SPI_CS_IN)]
	#######################################################################################################

	post_message -type info [string repeat - 80]

	#######################################################################################################
	# Create the DQ/CLK constraints on the monitor interface. In this mode the clock is the virtual clock in the
	# BMC. The BMC output is on the falling edge of the clock
	#######################################################################################################
	# These paths are BMC Tco then BMC to Mux, Mux Tpd, mux to CPLD minus BMC clock to mux to CPLD
	post_message -type info "BMC DQ0 input delay : [expr {$T_BMC_SPI_DQ_TCO_MAX + $T_BMC_SPI_DQ0_to_CPLD_max - $T_BMC_to_CPLD_BMC_SPI_CLK_min}] ns"
	post_message -type info "BMC CLK to CPLD delay: $T_BMC_to_CPLD_BMC_SPI_CLK_min ns"
	post_message -type info "BMC DQ0 to CPLD delay: $T_BMC_SPI_DQ0_to_CPLD_min ns"
	post_message -type info "BMC SPI DQ Tco : $T_BMC_SPI_DQ_TCO_MAX ns"
	set_input_delay -max -clock_fall -clock BMC_SPI_CLK_virt [expr {$T_BMC_SPI_DQ_TCO_MAX + $T_BMC_SPI_DQ0_to_CPLD_max - $T_BMC_to_CPLD_BMC_SPI_CLK_min}] [get_ports $spi_pin_names_map(BMC_SPI_DQ0_MON)]
	set_input_delay -min -clock_fall -clock BMC_SPI_CLK_virt [expr {$T_BMC_SPI_DQ_TCO_MIN + $T_BMC_SPI_DQ0_to_CPLD_min - $T_BMC_to_CPLD_BMC_SPI_CLK_max}] [get_ports $spi_pin_names_map(BMC_SPI_DQ0_MON)]

	post_message -type info [string repeat # 80]
}
set_bmc_spi_interface_constraints

### set false paths for SPI signals which cross clock domains

# false paths from the SPI Filter chip select disable output to the SPI master clock(s), we do not filter any commands from the CPLD internal master
set_false_path -from {u_core|u_spi_control|pch_spi_filter_inst|o_spi_disable_cs} -to [get_clocks {PCH_SPI_CLK PCH_SPI_CLK_virt}]
set_false_path -from {u_core|u_spi_control|bmc_spi_filter_inst|o_spi_disable_cs} -to [get_clocks {BMC_SPI_CLK BMC_SPI_CLK_virt}]

# false paths from NIOS controlled GPO signals to SPI CS pins, these GPO signals will only change state in T-1 when there is no activity on the SPI bus
set_false_path -from "u_core|u_pfr_sys|u_gpo_1|data_out\[$gen_gpo_controls_pkg(GPO_1_SPI_MASTER_BMC_PCHN_BIT_POS)\]" -to [get_ports [list $spi_pin_names_map(PCH_SPI_CS_OUT) $spi_pin_names_map(PCH_SPI_DQ0) $spi_pin_names_map(PCH_SPI_DQ1) $spi_pin_names_map(PCH_SPI_DQ2) $spi_pin_names_map(PCH_SPI_DQ3)]]
set_false_path -from "u_core|u_pfr_sys|u_gpo_1|data_out\[$gen_gpo_controls_pkg(GPO_1_SPI_MASTER_BMC_PCHN_BIT_POS)\]" -to [get_ports [list $spi_pin_names_map(BMC_SPI_CS_OUT) $spi_pin_names_map(BMC_SPI_DQ0) $spi_pin_names_map(BMC_SPI_DQ1) $spi_pin_names_map(BMC_SPI_DQ2) $spi_pin_names_map(BMC_SPI_DQ3)]]

set_false_path -from "u_core|u_pfr_sys|u_gpo_1|data_out\[$gen_gpo_controls_pkg(GPO_1_FM_SPI_PFR_PCH_MASTER_SEL_BIT_POS)\]" -to [get_ports [list $spi_pin_names_map(PCH_SPI_CS_OUT) $spi_pin_names_map(PCH_SPI_DQ0) $spi_pin_names_map(PCH_SPI_DQ1) $spi_pin_names_map(PCH_SPI_DQ2) $spi_pin_names_map(PCH_SPI_DQ3)]]
set_false_path -from "u_core|u_pfr_sys|u_gpo_1|data_out\[$gen_gpo_controls_pkg(GPO_1_FM_SPI_PFR_BMC_BT_MASTER_SEL_BIT_POS)\]" -to [get_ports [list $spi_pin_names_map(BMC_SPI_CS_OUT) $spi_pin_names_map(BMC_SPI_DQ0) $spi_pin_names_map(BMC_SPI_DQ1) $spi_pin_names_map(BMC_SPI_DQ2) $spi_pin_names_map(BMC_SPI_DQ3)]]

# Cut the async path from the filter disable to CS
set_false_path -from "u_core|u_pfr_sys|u_gpo_1|data_out\[$gen_gpo_controls_pkg(GPO_1_PCH_SPI_FILTER_DISABLE_BIT_POS)\]" -to {u_core|u_spi_control|pch_spi_filter_inst|o_spi_disable_cs}
set_false_path -from "u_core|u_pfr_sys|u_gpo_1|data_out\[$gen_gpo_controls_pkg(GPO_1_BMC_SPI_FILTER_DISABLE_BIT_POS)\]" -to {u_core|u_spi_control|bmc_spi_filter_inst|o_spi_disable_cs}


# false paths from the CPLD internal SPI master to the 'virtual' SPI clocks. When the BMC/PCH are driving CS then the internal SPI master is not
set_false_path -from {u_core|u_spi_control|spi_master_inst|qspi_inf_inst|ncs_reg[0]} -to [get_clocks PCH_SPI_CLK_virt]
set_false_path -from {u_core|u_spi_control|spi_master_inst|qspi_inf_inst|ncs_reg[0]} -to [get_clocks BMC_SPI_CLK_virt]

# false path from CPLD reset signal to external SPI clock
set_false_path -from {*u_pfr_sys|altera_reset_controller:rst_controller|r_sync_rst} -to [get_clocks [list PCH_SPI_MON_CLK BMC_SPI_MON_CLK]]

# false path from ibb_addr_detected to ibb_access_detectedn
set_false_path -from {u_core|u_spi_control|bmc_spi_filter_inst|ibb_addr_detected} -to {u_core|u_spi_control|bmc_spi_filter_inst|ibb_access_detectedn}
set_false_path -from "u_core|u_pfr_sys|u_gpo_1|data_out\[$gen_gpo_controls_pkg(GPO_1_BMC_SPI_ADDR_MODE_SET_3B_BIT_POS)\]" -to {u_core|u_spi_control|bmc_spi_filter_inst|address_mode_4b}


# false paths from the spi filter 'filtered_command_info' registers to the system clock
# these signals change infrequently and will be read by the NIOS multiple times to ensure data consistency before result of read is used
set regs [get_registers *_spi_filter_inst|o_filtered_command_info* -nowarn]
foreach_in_collection reg $regs {
    set_false_path -from {*_spi_filter_inst|o_filtered_command_info*} -to [get_clocks {u_pfr_sys_clocks_reset|u_sys_pll_ip|altpll_component|auto_generated|pll1|clk[1]}]
}

proc report_pfr_overconstraints {} {
	report_timing -to {pfr_core:u_core|pfr_sys:u_pfr_sys|altera_onchip_flash:u_ufm|altera_onchip_flash_block:altera_onchip_flash_block|*} -npath 100 -nworst 10 -show_routing -detail full_path  -multi_corner -panel {PFR||Flash Block||C2P : Setup} -setup
	report_timing -to {pfr_core:u_core|pfr_sys:u_pfr_sys|altera_onchip_flash:u_ufm|altera_onchip_flash_block:altera_onchip_flash_block|*} -npath 100 -nworst 10 -show_routing -detail full_path  -multi_corner -panel {PFR||Flash Block||C2P : Hold} -hold

	report_timing -from {pfr_core:u_core|pfr_sys:u_pfr_sys|altera_onchip_flash:u_ufm|altera_onchip_flash_block:altera_onchip_flash_block|*} -npath 100 -nworst 10 -show_routing -detail full_path  -multi_corner -panel {PFR||Flash Block||P2C : Setup} -setup
	report_timing -from {pfr_core:u_core|pfr_sys:u_pfr_sys|altera_onchip_flash:u_ufm|altera_onchip_flash_block:altera_onchip_flash_block|*} -npath 100 -nworst 10 -show_routing -detail full_path  -multi_corner -panel {PFR||Flash Block||P2C : Hold} -hold
}

if { $::TimeQuestInfo(nameofexecutable) == "quartus_fit" } {
	puts "Applying Quartus FIT specific constraints"
}
