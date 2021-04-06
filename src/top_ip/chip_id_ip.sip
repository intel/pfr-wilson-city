set_global_assignment -entity "chip_id_ip" -library "lib_chip_id_ip" -name IP_TOOL_NAME "altchip_id"
set_global_assignment -entity "chip_id_ip" -library "lib_chip_id_ip" -name IP_TOOL_VERSION "18.1"
set_global_assignment -entity "chip_id_ip" -library "lib_chip_id_ip" -name IP_TOOL_ENV "mwpim"
set_global_assignment -library "lib_chip_id_ip" -name SPD_FILE [file join $::quartus(sip_path) "chip_id_ip.spd"]

set_global_assignment -library "lib_chip_id_ip" -name MISC_FILE [file join $::quartus(sip_path) "chip_id_ip_sim/chip_id_ip.v"]
set_global_assignment -library "lib_chip_id_ip" -name MISC_FILE [file join $::quartus(sip_path) "chip_id_ip_sim/altchip_id/altchip_id.v"]
