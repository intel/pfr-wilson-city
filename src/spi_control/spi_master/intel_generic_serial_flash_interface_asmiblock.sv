// (C) 2001-2018 Intel Corporation. All rights reserved.
// Your use of Intel Corporation's design tools, logic functions and other 
// software and tools, and its AMPP partner logic functions, and any output 
// files from any of the foregoing (including device programming or simulation 
// files), and any associated documentation or information are expressly subject 
// to the terms and conditions of the Intel Program License Subscription 
// Agreement, Intel FPGA IP License Agreement, or other applicable 
// license agreement, including, without limitation, that your use is for the 
// sole purpose of programming logic devices manufactured by Intel and sold by 
// Intel or its authorized distributors.  Please refer to the applicable 
// agreement for further details.



`timescale 1 ns / 1 ns
module intel_generic_serial_flash_interface_asmiblock #(
    parameter DEVICE_FAMILY = "Arria 10",
    parameter NCS_LENGTH    = 3,
    parameter DATA_LENGTH   = 4,
    parameter ENABLE_SIM_MODEL = "false"
) (
    input                           atom_ports_dclk,
    input [NCS_LENGTH-1:0]          atom_ports_ncs,
    input                           atom_ports_oe,
    input [DATA_LENGTH-1:0]         atom_ports_dataout,
    input [DATA_LENGTH-1:0]         atom_ports_dataoe,
        
    output [DATA_LENGTH-1:0]        atom_ports_datain
);
    
    generate
        if (DEVICE_FAMILY == "Arria 10") begin
            twentynm_asmiblock  dut_asmiblock (
                .dclk(atom_ports_dclk),
                .sce(atom_ports_ncs),
                .oe(atom_ports_oe),
                .data0out(atom_ports_dataout[0]),
                .data1out(atom_ports_dataout[1]),
                .data2out(atom_ports_dataout[2]),
                .data3out(atom_ports_dataout[3]),
                .data0oe(atom_ports_dataoe[0]),
                .data1oe(atom_ports_dataoe[1]),
                .data2oe(atom_ports_dataoe[2]),
                .data3oe(atom_ports_dataoe[3]),
                .data0in(atom_ports_datain[0]),
                .data1in(atom_ports_datain[1]),
                .data2in(atom_ports_datain[2]),
                .data3in(atom_ports_datain[3]),
                
                .spidclk(),
                .spisce(),
                .spidataout(),
                .spidatain());
                
               	defparam
                    dut_asmiblock.enable_sim = ENABLE_SIM_MODEL;

        end else if (DEVICE_FAMILY == "Cyclone 10 GX") begin
            cyclone10gx_asmiblock  dut_asmiblock (
                .dclk(atom_ports_dclk),
                .sce(atom_ports_ncs),
                .oe(atom_ports_oe),
                .data0out(atom_ports_dataout[0]),
                .data1out(atom_ports_dataout[1]),
                .data2out(atom_ports_dataout[2]),
                .data3out(atom_ports_dataout[3]),
                .data0oe(atom_ports_dataoe[0]),
                .data1oe(atom_ports_dataoe[1]),
                .data2oe(atom_ports_dataoe[2]),
                .data3oe(atom_ports_dataoe[3]),
                .data0in(atom_ports_datain[0]),
                .data1in(atom_ports_datain[1]),
                .data2in(atom_ports_datain[2]),
                .data3in(atom_ports_datain[3]),
                
                .spidclk(),
                .spisce(),
                .spidataout(),
                .spidatain());
                
                defparam
                    dut_asmiblock.enable_sim = ENABLE_SIM_MODEL;
                
        end else if (DEVICE_FAMILY == "Stratix V") begin
            stratixv_asmiblock  dut_asmiblock (
                .dclk(atom_ports_dclk),
                .sce(atom_ports_ncs),
                .oe(atom_ports_oe),
                .data0out(atom_ports_dataout[0]),
                .data1out(atom_ports_dataout[1]),
                .data2out(atom_ports_dataout[2]),
                .data3out(atom_ports_dataout[3]),
                .data0oe(atom_ports_dataoe[0]),
                .data1oe(atom_ports_dataoe[1]),
                .data2oe(atom_ports_dataoe[2]),
                .data3oe(atom_ports_dataoe[3]),
                .data0in(atom_ports_datain[0]),
                .data1in(atom_ports_datain[1]),
                .data2in(atom_ports_datain[2]),
                .data3in(atom_ports_datain[3]));
           defparam
                    dut_asmiblock.enable_sim = ENABLE_SIM_MODEL;
                
        end else if (DEVICE_FAMILY == "Arria V GZ") begin
            arriavgz_asmiblock  dut_asmiblock (
                .dclk(atom_ports_dclk),
                .sce(atom_ports_ncs),
                .oe(atom_ports_oe),
                .data0out(atom_ports_dataout[0]),
                .data1out(atom_ports_dataout[1]),
                .data2out(atom_ports_dataout[2]),
                .data3out(atom_ports_dataout[3]),
                .data0oe(atom_ports_dataoe[0]),
                .data1oe(atom_ports_dataoe[1]),
                .data2oe(atom_ports_dataoe[2]),
                .data3oe(atom_ports_dataoe[3]),
                .data0in(atom_ports_datain[0]),
                .data1in(atom_ports_datain[1]),
                .data2in(atom_ports_datain[2]),
                .data3in(atom_ports_datain[3]));
            defparam
                    dut_asmiblock.enable_sim = ENABLE_SIM_MODEL;
                
        end else if (DEVICE_FAMILY == "Arria V") begin
            arriav_asmiblock  dut_asmiblock (
                .dclk(atom_ports_dclk),
                .sce(atom_ports_ncs),
                .oe(atom_ports_oe),
                .data0out(atom_ports_dataout[0]),
                .data1out(atom_ports_dataout[1]),
                .data2out(atom_ports_dataout[2]),
                .data3out(atom_ports_dataout[3]),
                .data0oe(atom_ports_dataoe[0]),
                .data1oe(atom_ports_dataoe[1]),
                .data2oe(atom_ports_dataoe[2]),
                .data3oe(atom_ports_dataoe[3]),
                .data0in(atom_ports_datain[0]),
                .data1in(atom_ports_datain[1]),
                .data2in(atom_ports_datain[2]),
                .data3in(atom_ports_datain[3]));
            defparam
                    dut_asmiblock.enable_sim = ENABLE_SIM_MODEL;
                
        end else if (DEVICE_FAMILY == "Cyclone V") begin
            cyclonev_asmiblock  dut_asmiblock (
                .dclk(atom_ports_dclk),
                .sce(atom_ports_ncs),
                .oe(atom_ports_oe),
                .data0out(atom_ports_dataout[0]),
                .data1out(atom_ports_dataout[1]),
                .data2out(atom_ports_dataout[2]),
                .data3out(atom_ports_dataout[3]),
                .data0oe(atom_ports_dataoe[0]),
                .data1oe(atom_ports_dataoe[1]),
                .data2oe(atom_ports_dataoe[2]),
                .data3oe(atom_ports_dataoe[3]),
                .data0in(atom_ports_datain[0]),
                .data1in(atom_ports_datain[1]),
                .data2in(atom_ports_datain[2]),
                .data3in(atom_ports_datain[3]));
           defparam
                    dut_asmiblock.enable_sim = ENABLE_SIM_MODEL;
        
        end else if (DEVICE_FAMILY == "Max 10") begin
            fiftyfivenm_asmiblock  dut_asmiblock (
                .dclkin(atom_ports_dclk),
                .scein(atom_ports_ncs),
                .oe(atom_ports_oe),
                .sdoin(atom_ports_dataout[0]),
                .data0out(atom_ports_datain[1]));
        defparam
                    dut_asmiblock.enable_sim = ENABLE_SIM_MODEL;     
        
        end else if (DEVICE_FAMILY == "Cyclone IV E") begin
            cycloneive_asmiblock  dut_asmiblock (
                .dclkin(atom_ports_dclk),
                .scein(atom_ports_ncs),
                .oe(atom_ports_oe),
                .sdoin(atom_ports_dataout[0]),
                .data0out(atom_ports_datain[1]));
        defparam
                    dut_asmiblock.enable_sim = ENABLE_SIM_MODEL;
        
        end else if (DEVICE_FAMILY == "Cyclone 10 LP") begin
            cyclone_asmiblock  dut_asmiblock (
                .dclkin(atom_ports_dclk),
                .scein(atom_ports_ncs),
                .oe(atom_ports_oe),
                .sdoin(atom_ports_dataout[0]),
                .data0out(atom_ports_datain[1]));
        defparam
                    dut_asmiblock.enable_sim = ENABLE_SIM_MODEL;

        end else if (DEVICE_FAMILY == "Cyclone IV GX") begin
            cycloneiv_asmiblock  dut_asmiblock (
                .dclkin(atom_ports_dclk),
                .scein(atom_ports_ncs),
                .oe(atom_ports_oe),
                .sdoin(atom_ports_dataout[0]),
                .data0out(atom_ports_datain[1]));
        defparam
                    dut_asmiblock.enable_sim = ENABLE_SIM_MODEL;
                    
        end else if (DEVICE_FAMILY == "Stratix IV") begin
            stratixiv_asmiblock  dut_asmiblock (
                .dclkin(atom_ports_dclk),
                .scein(atom_ports_ncs),
                .oe(atom_ports_oe),
                .sdoin(atom_ports_dataout[0]),
                .data0out(atom_ports_datain[1]),
                .data0in(1'b0));
         defparam
                    dut_asmiblock.enable_sim = ENABLE_SIM_MODEL;
                    
        end else if (DEVICE_FAMILY == "Arria II") begin
            arriaii_asmiblock  dut_asmiblock (
                .dclkin(atom_ports_dclk),
                .scein(atom_ports_ncs),
                .oe(atom_ports_oe),
                .sdoin(atom_ports_dataout[0]),
                .data0out(atom_ports_datain[1]),
                .data0in(1'b0));
         defparam
                    dut_asmiblock.enable_sim = ENABLE_SIM_MODEL;
                    
        end else if (DEVICE_FAMILY == "Arria II GZ") begin
            arriaiigz_asmiblock  dut_asmiblock (
                .dclkin(atom_ports_dclk),
                .scein(atom_ports_ncs),
                .oe(atom_ports_oe),
                .sdoin(atom_ports_dataout[0]),
                .data0out(atom_ports_datain[1]),
                .data0in(1'b0));
        defparam
                    dut_asmiblock.enable_sim = ENABLE_SIM_MODEL;
                    
        end
        
    endgenerate

endmodule