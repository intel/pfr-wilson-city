#! /bin/bash

set -e

qsys-generate --synthesis=verilog --simulation=verilog src/top_ip/dual_config_ip.qsys --output-directory=src/top_ip/dual_config_ip
qsys-generate --synthesis=verilog --simulation=verilog src/top_ip/int_osc_ip.qsys --output-directory=src/top_ip/int_osc_ip
qsys-generate --synthesis=verilog --simulation=verilog src/pfr_sys/pfr_sys.qsys --output-directory=src/pfr_sys/pfr_sys
qsys-generate --synthesis=verilog --simulation=verilog src/recovery_sys/recovery_sys.qsys --output-directory=src/recovery_sys/recovery_sys

make -C fw/bsp/pfr_sys	
make -C fw/bsp/recovery_sys
make -C fw/code/hw/wilson_city_fab2
make -C fw/code/hw/wilson_city_fab2_recovery
cp fw/code/hw/wilson_city_fab2/mem_init/u_ufm.hex ./pfr_code.hex
cp fw/code/hw/wilson_city_fab2_recovery/mem_init/u_ufm.hex ./recovery_code.hex

gzip -dr fw/test/testdata

touch prep_done.txt
 