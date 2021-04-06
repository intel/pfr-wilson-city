// (C) 2019 Intel Corporation. All rights reserved.
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

// spi_master_spi_master.v
//
// This file was originally auto-generated from intel_generic_serial_flash_interface_top_hw.tcl using ACDS version 18.1
// build 625 Standard edition.  It was then modified for use in the Root of Trust CPLD project.
//
// This modules implements a SPI master interface.  It provides two Avalong MM interfaces to interace with a SPI device.
// The CSR interface provides low-level access to an assortment of SPI commands and settings.  The MEM interface provides
// a memory-mapped view of the entire SPI FLASH memory and supports read and write commands only.

`timescale 1 ps / 1 ps
`default_nettype none

module spi_master_spi_master (
		input  wire [5:0]  avl_csr_address,       //   avl_csr.address
		input  wire        avl_csr_read,          //          .read
		output wire [31:0] avl_csr_readdata,      //          .readdata
		input  wire        avl_csr_write,         //          .write
		input  wire [31:0] avl_csr_writedata,     //          .writedata
		output wire        avl_csr_waitrequest,   //          .waitrequest
		output wire        avl_csr_readdatavalid, //          .readdatavalid
		input  wire        avl_mem_write,         //   avl_mem.write
		input  wire [6:0]  avl_mem_burstcount,    //          .burstcount
		output wire        avl_mem_waitrequest,   //          .waitrequest
		input  wire        avl_mem_read,          //          .read
		input  wire [24:0] avl_mem_address,       //          .address
		input  wire [31:0] avl_mem_writedata,     //          .writedata
		output wire [31:0] avl_mem_readdata,      //          .readdata
		output wire        avl_mem_readdatavalid, //          .readdatavalid
		input  wire [3:0]  avl_mem_byteenable,    //          .byteenable
		input  wire        clk_clk,               //       clk.clk
		input  wire        reset_reset,           //     reset.reset
		output wire        qspi_pins_dclk,        // qspi_pins.dclk
		output wire        qspi_pins_ncs,         //          .ncs
		output wire [3:0]  qspi_pins_data_out,    //          .data output
		output wire [3:0]  qspi_pins_data_oe,     //          .data output enable
		input  wire [3:0]  qspi_pins_data_in      //          .data input
	);

	wire         csr_controller_cmd_pck_valid;                                // csr_controller:cmd_valid -> multiplexer:sink1_valid
	wire  [31:0] csr_controller_cmd_pck_data;                                 // csr_controller:cmd_data -> multiplexer:sink1_data
	wire         csr_controller_cmd_pck_ready;                                // multiplexer:sink1_ready -> csr_controller:cmd_ready
	wire   [1:0] csr_controller_cmd_pck_channel;                              // csr_controller:cmd_channel -> multiplexer:sink1_channel
	wire         csr_controller_cmd_pck_startofpacket;                        // csr_controller:cmd_sop -> multiplexer:sink1_startofpacket
	wire         csr_controller_cmd_pck_endofpacket;                          // csr_controller:cmd_eop -> multiplexer:sink1_endofpacket
	wire         xip_controller_cmd_pck_valid;                                // xip_controller:cmd_valid -> multiplexer:sink0_valid
	wire  [31:0] xip_controller_cmd_pck_data;                                 // xip_controller:cmd_data -> multiplexer:sink0_data
	wire         xip_controller_cmd_pck_ready;                                // multiplexer:sink0_ready -> xip_controller:cmd_ready
	wire   [1:0] xip_controller_cmd_pck_channel;                              // xip_controller:cmd_channel -> multiplexer:sink0_channel
	wire         xip_controller_cmd_pck_startofpacket;                        // xip_controller:cmd_sop -> multiplexer:sink0_startofpacket
	wire         xip_controller_cmd_pck_endofpacket;                          // xip_controller:cmd_eop -> multiplexer:sink0_endofpacket
	wire         merlin_demultiplexer_0_src1_valid;                           // merlin_demultiplexer_0:src1_valid -> csr_controller:rsp_valid
	wire  [31:0] merlin_demultiplexer_0_src1_data;                            // merlin_demultiplexer_0:src1_data -> csr_controller:rsp_data
	wire         merlin_demultiplexer_0_src1_ready;                           // csr_controller:rsp_ready -> merlin_demultiplexer_0:src1_ready
	wire   [1:0] merlin_demultiplexer_0_src1_channel;                         // merlin_demultiplexer_0:src1_channel -> csr_controller:rsp_channel
	wire         merlin_demultiplexer_0_src1_startofpacket;                   // merlin_demultiplexer_0:src1_startofpacket -> csr_controller:rsp_sop
	wire         merlin_demultiplexer_0_src1_endofpacket;                     // merlin_demultiplexer_0:src1_endofpacket -> csr_controller:rsp_eop
	wire         merlin_demultiplexer_0_src0_valid;                           // merlin_demultiplexer_0:src0_valid -> xip_controller:rsp_valid
	wire  [31:0] merlin_demultiplexer_0_src0_data;                            // merlin_demultiplexer_0:src0_data -> xip_controller:rsp_data
	wire         merlin_demultiplexer_0_src0_ready;                           // xip_controller:rsp_ready -> merlin_demultiplexer_0:src0_ready
	wire   [1:0] merlin_demultiplexer_0_src0_channel;                         // merlin_demultiplexer_0:src0_channel -> xip_controller:rsp_channel
	wire         merlin_demultiplexer_0_src0_startofpacket;                   // merlin_demultiplexer_0:src0_startofpacket -> xip_controller:rsp_sop
	wire         merlin_demultiplexer_0_src0_endofpacket;                     // merlin_demultiplexer_0:src0_endofpacket -> xip_controller:rsp_eop
	wire  [31:0] xip_addr_adaption_gen_qspi_mem_readdata;                     // xip_controller:mem_rddata -> xip_addr_adaption:gen_qspi_mem_rddata
	wire         xip_addr_adaption_gen_qspi_mem_waitrequest;                  // xip_controller:mem_waitrequest -> xip_addr_adaption:gen_qspi_mem_waitrequest
	wire  [31:0] xip_addr_adaption_gen_qspi_mem_address;                      // xip_addr_adaption:gen_qspi_mem_addr -> xip_controller:mem_addr
	wire         xip_addr_adaption_gen_qspi_mem_read;                         // xip_addr_adaption:gen_qspi_mem_read -> xip_controller:mem_rd
	wire   [3:0] xip_addr_adaption_gen_qspi_mem_byteenable;                   // xip_addr_adaption:gen_qspi_mem_byteenable -> xip_controller:mem_byteenable
	wire         xip_addr_adaption_gen_qspi_mem_readdatavalid;                // xip_controller:mem_rddatavalid -> xip_addr_adaption:gen_qspi_mem_rddata_valid
	wire         xip_addr_adaption_gen_qspi_mem_write;                        // xip_addr_adaption:gen_qspi_mem_write -> xip_controller:mem_wr
	wire  [31:0] xip_addr_adaption_gen_qspi_mem_writedata;                    // xip_addr_adaption:gen_qspi_mem_wrdata -> xip_controller:mem_wrdata
	wire   [6:0] xip_addr_adaption_gen_qspi_mem_burstcount;                   // xip_addr_adaption:gen_qspi_mem_burstcount -> xip_controller:mem_burstcount
	wire   [3:0] csr_controller_chip_select_chip_select;                      // csr_controller:chip_select -> xip_controller:chip_select
	wire   [3:0] serial_flash_inf_cmd_gen_inst_addr_num_lines_addr_num_lines; // serial_flash_inf_cmd_gen_inst:addr_num_lines -> qspi_inf_inst:addr_num_lines
	wire   [3:0] serial_flash_inf_cmd_gen_inst_data_num_lines_data_num_lines; // serial_flash_inf_cmd_gen_inst:data_num_lines -> qspi_inf_inst:data_num_lines
	wire   [3:0] serial_flash_inf_cmd_gen_inst_op_num_lines_op_num_lines;     // serial_flash_inf_cmd_gen_inst:op_num_lines -> qspi_inf_inst:op_num_lines
	wire  [31:0] csr_controller_addr_bytes_csr_addr_bytes_csr;                // csr_controller:addr_bytes_csr -> serial_flash_inf_cmd_gen_inst:addr_bytes_csr
	wire  [31:0] xip_controller_addr_bytes_xip_addr_bytes_xip;                // xip_controller:addr_bytes_xip -> serial_flash_inf_cmd_gen_inst:addr_bytes_xip
	wire   [1:0] csr_controller_op_type_op_type;                              // csr_controller:op_type -> serial_flash_inf_cmd_gen_inst:op_type
	wire   [1:0] csr_controller_wr_addr_type_wr_addr_type;                    // csr_controller:wr_addr_type -> serial_flash_inf_cmd_gen_inst:wr_addr_type
	wire   [1:0] csr_controller_wr_data_type_wr_data_type;                    // csr_controller:wr_data_type -> serial_flash_inf_cmd_gen_inst:wr_data_type
	wire   [1:0] csr_controller_rd_addr_type_rd_addr_type;                    // csr_controller:rd_addr_type -> serial_flash_inf_cmd_gen_inst:rd_addr_type
	wire   [1:0] csr_controller_rd_data_type_rd_data_type;                    // csr_controller:rd_data_type -> serial_flash_inf_cmd_gen_inst:rd_data_type
	wire   [7:0] csr_controller_wr_en_opcode_wr_en_opcode;                    // csr_controller:wr_en_opcode -> xip_controller:wr_en_opcode
	wire   [7:0] csr_controller_polling_opcode_polling_opcode;                // csr_controller:polling_opcode -> xip_controller:polling_opcode
	wire   [2:0] csr_controller_polling_bit_polling_bit;                      // csr_controller:polling_bit -> xip_controller:polling_bit
	wire   [7:0] csr_controller_wr_opcode_wr_opcode;                          // csr_controller:wr_opcode -> xip_controller:wr_opcode
	wire   [7:0] csr_controller_rd_opcode_rd_opcode;                          // csr_controller:rd_opcode -> xip_controller:rd_opcode
	wire   [4:0] csr_controller_rd_dummy_cycles_rd_dummy_cycles;              // csr_controller:rd_dummy_cycles -> xip_controller:rd_dummy_cycles
	wire         csr_controller_is_4bytes_addr_xip_is_4bytes_addr_xip;        // csr_controller:is_4bytes_addr_xip -> xip_controller:is_4bytes_addr_xip
	wire   [4:0] csr_controller_baud_rate_divisor_baud_rate_divisor;          // csr_controller:baud_rate_divisor -> qspi_inf_inst:baud_rate_divisor
	wire   [7:0] csr_controller_cs_delay_setting_cs_delay_setting;            // csr_controller:cs_delay_setting -> qspi_inf_inst:cs_delay_setting
	wire   [3:0] csr_controller_read_capture_delay_read_capture_delay;        // csr_controller:read_capture_delay -> qspi_inf_inst:read_capture_delay
	wire   [1:0] xip_controller_xip_trans_type_xip_trans_type;                // xip_controller:xip_trans_type -> serial_flash_inf_cmd_gen_inst:xip_trans_type
	wire         serial_flash_inf_cmd_gen_inst_out_rsp_pck_valid;             // serial_flash_inf_cmd_gen_inst:out_rsp_valid -> merlin_demultiplexer_0:sink_valid
	wire  [31:0] serial_flash_inf_cmd_gen_inst_out_rsp_pck_data;              // serial_flash_inf_cmd_gen_inst:out_rsp_data -> merlin_demultiplexer_0:sink_data
	wire         serial_flash_inf_cmd_gen_inst_out_rsp_pck_ready;             // merlin_demultiplexer_0:sink_ready -> serial_flash_inf_cmd_gen_inst:out_rsp_ready
	wire   [1:0] serial_flash_inf_cmd_gen_inst_out_rsp_pck_channel;           // serial_flash_inf_cmd_gen_inst:out_rsp_channel -> merlin_demultiplexer_0:sink_channel
	wire         serial_flash_inf_cmd_gen_inst_out_rsp_pck_startofpacket;     // serial_flash_inf_cmd_gen_inst:out_rsp_sop -> merlin_demultiplexer_0:sink_startofpacket
	wire         serial_flash_inf_cmd_gen_inst_out_rsp_pck_endofpacket;       // serial_flash_inf_cmd_gen_inst:out_rsp_eop -> merlin_demultiplexer_0:sink_endofpacket
	wire         qspi_inf_inst_out_rsp_pck_valid;                             // qspi_inf_inst:out_rsp_valid -> serial_flash_inf_cmd_gen_inst:in_rsp_valid
	wire   [7:0] qspi_inf_inst_out_rsp_pck_data;                              // qspi_inf_inst:out_rsp_data -> serial_flash_inf_cmd_gen_inst:in_rsp_data
	wire         qspi_inf_inst_out_rsp_pck_ready;                             // serial_flash_inf_cmd_gen_inst:in_rsp_ready -> qspi_inf_inst:out_rsp_ready
	wire   [4:0] serial_flash_inf_cmd_gen_inst_dummy_cycles_dummy_cycles;     // serial_flash_inf_cmd_gen_inst:dummy_cycles -> qspi_inf_inst:dummy_cycles
	wire   [3:0] serial_flash_inf_cmd_gen_inst_chip_select_chip_select;       // serial_flash_inf_cmd_gen_inst:chip_select -> qspi_inf_inst:chip_select
	wire         serial_flash_inf_cmd_gen_inst_require_rdata_require_rdata;   // serial_flash_inf_cmd_gen_inst:require_rdata -> qspi_inf_inst:require_rdata
	wire         csr_controller_qspi_interface_en_qspi_interface_en;          // csr_controller:qspi_interface_en -> qspi_inf_inst:qspi_interface_en
	wire         serial_flash_inf_cmd_gen_inst_out_cmd_pck_valid;             // serial_flash_inf_cmd_gen_inst:out_cmd_valid -> qspi_inf_inst:in_cmd_valid
	wire   [7:0] serial_flash_inf_cmd_gen_inst_out_cmd_pck_data;              // serial_flash_inf_cmd_gen_inst:out_cmd_data -> qspi_inf_inst:in_cmd_data
	wire         serial_flash_inf_cmd_gen_inst_out_cmd_pck_ready;             // qspi_inf_inst:in_cmd_ready -> serial_flash_inf_cmd_gen_inst:out_cmd_ready
	wire   [8:0] serial_flash_inf_cmd_gen_inst_out_cmd_pck_channel;           // serial_flash_inf_cmd_gen_inst:out_cmd_channel -> qspi_inf_inst:in_cmd_channel
	wire         serial_flash_inf_cmd_gen_inst_out_cmd_pck_startofpacket;     // serial_flash_inf_cmd_gen_inst:out_cmd_sop -> qspi_inf_inst:in_cmd_sop
	wire         serial_flash_inf_cmd_gen_inst_out_cmd_pck_endofpacket;       // serial_flash_inf_cmd_gen_inst:out_cmd_eop -> qspi_inf_inst:in_cmd_eop
	wire         multiplexer_src_valid;                                       // multiplexer:src_valid -> serial_flash_inf_cmd_gen_inst:in_cmd_valid
	wire  [31:0] multiplexer_src_data;                                        // multiplexer:src_data -> serial_flash_inf_cmd_gen_inst:in_cmd_data
	wire         multiplexer_src_ready;                                       // serial_flash_inf_cmd_gen_inst:in_cmd_ready -> multiplexer:src_ready
	wire   [1:0] multiplexer_src_channel;                                     // multiplexer:src_channel -> serial_flash_inf_cmd_gen_inst:in_cmd_channel
	wire         multiplexer_src_startofpacket;                               // multiplexer:src_startofpacket -> serial_flash_inf_cmd_gen_inst:in_cmd_sop
	wire         multiplexer_src_endofpacket;                                 // multiplexer:src_endofpacket -> serial_flash_inf_cmd_gen_inst:in_cmd_eop


	intel_generic_serial_flash_interface_csr #(
		.ADD_W               (6),
		//.CHIP_SELECT_BYPASS  (0),
		.DEFAULT_VALUE_REG_0 (32'b00000000000000000000000100000001),
		.DEFAULT_VALUE_REG_1 (32'b00000000000000000000000000010000),
		.DEFAULT_VALUE_REG_2 (32'b00000000000000000000000000000000),
		.DEFAULT_VALUE_REG_3 (32'b00000000000000000000000000000000),
		.DEFAULT_VALUE_REG_4 (32'b00000000000000000000000000000000),
		.DEFAULT_VALUE_REG_5 (32'b00000000000000000000000000000011),
		.DEFAULT_VALUE_REG_6 (32'b00000000000000000111000000000010),
		.DEFAULT_VALUE_REG_7 (32'b00000000000000000001100000000101)
	) csr_controller (
		.csr_addr           (avl_csr_address),                                      //                csr.address
		.csr_rd             (avl_csr_read),                                         //                   .read
		.csr_rddata         (avl_csr_readdata),                                     //                   .readdata
		.csr_wr             (avl_csr_write),                                        //                   .write
		.csr_wrdata         (avl_csr_writedata),                                    //                   .writedata
		.csr_waitrequest    (avl_csr_waitrequest),                                  //                   .waitrequest
		.csr_rddatavalid    (avl_csr_readdatavalid),                                //                   .readdatavalid
		.clk                (clk_clk),                                              //                clk.clk
		.reset              (reset_reset),                       //              reset.reset
		.cmd_channel        (csr_controller_cmd_pck_channel),                       //            cmd_pck.channel
		.cmd_eop            (csr_controller_cmd_pck_endofpacket),                   //                   .endofpacket
		.cmd_ready          (csr_controller_cmd_pck_ready),                         //                   .ready
		.cmd_sop            (csr_controller_cmd_pck_startofpacket),                 //                   .startofpacket
		.cmd_data           (csr_controller_cmd_pck_data),                          //                   .data
		.cmd_valid          (csr_controller_cmd_pck_valid),                         //                   .valid
		.rsp_channel        (merlin_demultiplexer_0_src1_channel),                  //            rsp_pck.channel
		.rsp_data           (merlin_demultiplexer_0_src1_data),                     //                   .data
		.rsp_eop            (merlin_demultiplexer_0_src1_endofpacket),              //                   .endofpacket
		.rsp_ready          (merlin_demultiplexer_0_src1_ready),                    //                   .ready
		.rsp_sop            (merlin_demultiplexer_0_src1_startofpacket),            //                   .startofpacket
		.rsp_valid          (merlin_demultiplexer_0_src1_valid),                    //                   .valid
		.addr_bytes_csr     (csr_controller_addr_bytes_csr_addr_bytes_csr),         //     addr_bytes_csr.addr_bytes_csr
		.qspi_interface_en  (csr_controller_qspi_interface_en_qspi_interface_en),   //  qspi_interface_en.qspi_interface_en
		.op_type            (csr_controller_op_type_op_type),                       //            op_type.op_type
		.wr_addr_type       (csr_controller_wr_addr_type_wr_addr_type),             //       wr_addr_type.wr_addr_type
		.wr_data_type       (csr_controller_wr_data_type_wr_data_type),             //       wr_data_type.wr_data_type
		.rd_addr_type       (csr_controller_rd_addr_type_rd_addr_type),             //       rd_addr_type.rd_addr_type
		.rd_data_type       (csr_controller_rd_data_type_rd_data_type),             //       rd_data_type.rd_data_type
		.wr_en_opcode       (csr_controller_wr_en_opcode_wr_en_opcode),             //       wr_en_opcode.wr_en_opcode
		.polling_opcode     (csr_controller_polling_opcode_polling_opcode),         //     polling_opcode.polling_opcode
		.polling_bit        (csr_controller_polling_bit_polling_bit),               //        polling_bit.polling_bit
		.wr_opcode          (csr_controller_wr_opcode_wr_opcode),                   //          wr_opcode.wr_opcode
		.rd_opcode          (csr_controller_rd_opcode_rd_opcode),                   //          rd_opcode.rd_opcode
		.rd_dummy_cycles    (csr_controller_rd_dummy_cycles_rd_dummy_cycles),       //    rd_dummy_cycles.rd_dummy_cycles
		.is_4bytes_addr_xip (csr_controller_is_4bytes_addr_xip_is_4bytes_addr_xip), // is_4bytes_addr_xip.is_4bytes_addr_xip
		.baud_rate_divisor  (csr_controller_baud_rate_divisor_baud_rate_divisor),   //  baud_rate_divisor.baud_rate_divisor
		.cs_delay_setting   (csr_controller_cs_delay_setting_cs_delay_setting),     //   cs_delay_setting.cs_delay_setting
		.read_capture_delay (csr_controller_read_capture_delay_read_capture_delay), // read_capture_delay.read_capture_delay
		.chip_select        (csr_controller_chip_select_chip_select)                //        chip_select.chip_select
		//.in_chip_select     (4'b0000)                                               //        (terminated)
	);

	spi_master_spi_master_xip_controller #(
		.ADDR_WIDTH (32)
	) xip_controller (
		.clk                (clk_clk),                                              //                clk.clk
		.reset              (reset_reset),                       //              reset.reset
		.mem_addr           (xip_addr_adaption_gen_qspi_mem_address),               //                mem.address
		.mem_rd             (xip_addr_adaption_gen_qspi_mem_read),                  //                   .read
		.mem_rddata         (xip_addr_adaption_gen_qspi_mem_readdata),              //                   .readdata
		.mem_wr             (xip_addr_adaption_gen_qspi_mem_write),                 //                   .write
		.mem_wrdata         (xip_addr_adaption_gen_qspi_mem_writedata),             //                   .writedata
		.mem_byteenable     (xip_addr_adaption_gen_qspi_mem_byteenable),            //                   .byteenable
		.mem_burstcount     (xip_addr_adaption_gen_qspi_mem_burstcount),            //                   .burstcount
		.mem_waitrequest    (xip_addr_adaption_gen_qspi_mem_waitrequest),           //                   .waitrequest
		.mem_rddatavalid    (xip_addr_adaption_gen_qspi_mem_readdatavalid),         //                   .readdatavalid
		.addr_bytes_xip     (xip_controller_addr_bytes_xip_addr_bytes_xip),         //     addr_bytes_xip.addr_bytes_xip
		.cmd_channel        (xip_controller_cmd_pck_channel),                       //            cmd_pck.channel
		.cmd_eop            (xip_controller_cmd_pck_endofpacket),                   //                   .endofpacket
		.cmd_ready          (xip_controller_cmd_pck_ready),                         //                   .ready
		.cmd_sop            (xip_controller_cmd_pck_startofpacket),                 //                   .startofpacket
		.cmd_data           (xip_controller_cmd_pck_data),                          //                   .data
		.cmd_valid          (xip_controller_cmd_pck_valid),                         //                   .valid
		.rsp_channel        (merlin_demultiplexer_0_src0_channel),                  //            rsp_pck.channel
		.rsp_data           (merlin_demultiplexer_0_src0_data),                     //                   .data
		.rsp_eop            (merlin_demultiplexer_0_src0_endofpacket),              //                   .endofpacket
		.rsp_ready          (merlin_demultiplexer_0_src0_ready),                    //                   .ready
		.rsp_sop            (merlin_demultiplexer_0_src0_startofpacket),            //                   .startofpacket
		.rsp_valid          (merlin_demultiplexer_0_src0_valid),                    //                   .valid
		.wr_en_opcode       (csr_controller_wr_en_opcode_wr_en_opcode),             //       wr_en_opcode.wr_en_opcode
		.polling_opcode     (csr_controller_polling_opcode_polling_opcode),         //     polling_opcode.polling_opcode
		.polling_bit        (csr_controller_polling_bit_polling_bit),               //        polling_bit.polling_bit
		.wr_opcode          (csr_controller_wr_opcode_wr_opcode),                   //          wr_opcode.wr_opcode
		.rd_opcode          (csr_controller_rd_opcode_rd_opcode),                   //          rd_opcode.rd_opcode
		.rd_dummy_cycles    (csr_controller_rd_dummy_cycles_rd_dummy_cycles),       //    rd_dummy_cycles.rd_dummy_cycles
		.xip_trans_type     (xip_controller_xip_trans_type_xip_trans_type),         //     xip_trans_type.xip_trans_type
		.is_4bytes_addr_xip (csr_controller_is_4bytes_addr_xip_is_4bytes_addr_xip), // is_4bytes_addr_xip.is_4bytes_addr_xip
		.chip_select        (csr_controller_chip_select_chip_select)                //        chip_select.chip_select
	);

	intel_generic_serial_flash_interface_addr #(
		.ADDR_WIDTH (25)
	) xip_addr_adaption (
		.clk                       (clk_clk),                                      //   clock_sink.clk
		.reset                     (reset_reset),               //        reset.reset
		.avl_mem_write             (avl_mem_write),                                //      avl_mem.write
		.avl_mem_burstcount        (avl_mem_burstcount),                           //             .burstcount
		.avl_mem_waitrequest       (avl_mem_waitrequest),                          //             .waitrequest
		.avl_mem_read              (avl_mem_read),                                 //             .read
		.avl_mem_addr              (avl_mem_address),                              //             .address
		.avl_mem_wrdata            (avl_mem_writedata),                            //             .writedata
		.avl_mem_rddata            (avl_mem_readdata),                             //             .readdata
		.avl_mem_rddata_valid      (avl_mem_readdatavalid),                        //             .readdatavalid
		.avl_mem_byteenable        (avl_mem_byteenable),                           //             .byteenable
		.gen_qspi_mem_addr         (xip_addr_adaption_gen_qspi_mem_address),       // gen_qspi_mem.address
		.gen_qspi_mem_read         (xip_addr_adaption_gen_qspi_mem_read),          //             .read
		.gen_qspi_mem_rddata       (xip_addr_adaption_gen_qspi_mem_readdata),      //             .readdata
		.gen_qspi_mem_write        (xip_addr_adaption_gen_qspi_mem_write),         //             .write
		.gen_qspi_mem_wrdata       (xip_addr_adaption_gen_qspi_mem_writedata),     //             .writedata
		.gen_qspi_mem_byteenable   (xip_addr_adaption_gen_qspi_mem_byteenable),    //             .byteenable
		.gen_qspi_mem_burstcount   (xip_addr_adaption_gen_qspi_mem_burstcount),    //             .burstcount
		.gen_qspi_mem_waitrequest  (xip_addr_adaption_gen_qspi_mem_waitrequest),   //             .waitrequest
		.gen_qspi_mem_rddata_valid (xip_addr_adaption_gen_qspi_mem_readdatavalid)  //             .readdatavalid
	);

	spi_master_spi_master_merlin_demultiplexer_0 merlin_demultiplexer_0 (
		.clk                (clk_clk),                                                 //       clk.clk
		.reset              (reset_reset),                          // clk_reset.reset
		.sink_ready         (serial_flash_inf_cmd_gen_inst_out_rsp_pck_ready),         //      sink.ready
		.sink_channel       (serial_flash_inf_cmd_gen_inst_out_rsp_pck_channel),       //          .channel
		.sink_data          (serial_flash_inf_cmd_gen_inst_out_rsp_pck_data),          //          .data
		.sink_startofpacket (serial_flash_inf_cmd_gen_inst_out_rsp_pck_startofpacket), //          .startofpacket
		.sink_endofpacket   (serial_flash_inf_cmd_gen_inst_out_rsp_pck_endofpacket),   //          .endofpacket
		.sink_valid         (serial_flash_inf_cmd_gen_inst_out_rsp_pck_valid),         //          .valid
		.src0_ready         (merlin_demultiplexer_0_src0_ready),                       //      src0.ready
		.src0_valid         (merlin_demultiplexer_0_src0_valid),                       //          .valid
		.src0_data          (merlin_demultiplexer_0_src0_data),                        //          .data
		.src0_channel       (merlin_demultiplexer_0_src0_channel),                     //          .channel
		.src0_startofpacket (merlin_demultiplexer_0_src0_startofpacket),               //          .startofpacket
		.src0_endofpacket   (merlin_demultiplexer_0_src0_endofpacket),                 //          .endofpacket
		.src1_ready         (merlin_demultiplexer_0_src1_ready),                       //      src1.ready
		.src1_valid         (merlin_demultiplexer_0_src1_valid),                       //          .valid
		.src1_data          (merlin_demultiplexer_0_src1_data),                        //          .data
		.src1_channel       (merlin_demultiplexer_0_src1_channel),                     //          .channel
		.src1_startofpacket (merlin_demultiplexer_0_src1_startofpacket),               //          .startofpacket
		.src1_endofpacket   (merlin_demultiplexer_0_src1_endofpacket)                  //          .endofpacket
	);

	spi_master_spi_master_multiplexer multiplexer (
		.clk                 (clk_clk),                              //       clk.clk
		.reset               (reset_reset),       // clk_reset.reset
		.src_ready           (multiplexer_src_ready),                //       src.ready
		.src_valid           (multiplexer_src_valid),                //          .valid
		.src_data            (multiplexer_src_data),                 //          .data
		.src_channel         (multiplexer_src_channel),              //          .channel
		.src_startofpacket   (multiplexer_src_startofpacket),        //          .startofpacket
		.src_endofpacket     (multiplexer_src_endofpacket),          //          .endofpacket
		.sink0_ready         (xip_controller_cmd_pck_ready),         //     sink0.ready
		.sink0_valid         (xip_controller_cmd_pck_valid),         //          .valid
		.sink0_channel       (xip_controller_cmd_pck_channel),       //          .channel
		.sink0_data          (xip_controller_cmd_pck_data),          //          .data
		.sink0_startofpacket (xip_controller_cmd_pck_startofpacket), //          .startofpacket
		.sink0_endofpacket   (xip_controller_cmd_pck_endofpacket),   //          .endofpacket
		.sink1_ready         (csr_controller_cmd_pck_ready),         //     sink1.ready
		.sink1_valid         (csr_controller_cmd_pck_valid),         //          .valid
		.sink1_channel       (csr_controller_cmd_pck_channel),       //          .channel
		.sink1_data          (csr_controller_cmd_pck_data),          //          .data
		.sink1_startofpacket (csr_controller_cmd_pck_startofpacket), //          .startofpacket
		.sink1_endofpacket   (csr_controller_cmd_pck_endofpacket)    //          .endofpacket
	);

	intel_generic_serial_flash_interface_cmd serial_flash_inf_cmd_gen_inst (
		.clk             (clk_clk),                                                     //            clk.clk
		.reset           (reset_reset),                              //          reset.reset
		.in_cmd_channel  (multiplexer_src_channel),                                     //     in_cmd_pck.channel
		.in_cmd_eop      (multiplexer_src_endofpacket),                                 //               .endofpacket
		.in_cmd_ready    (multiplexer_src_ready),                                       //               .ready
		.in_cmd_sop      (multiplexer_src_startofpacket),                               //               .startofpacket
		.in_cmd_data     (multiplexer_src_data),                                        //               .data
		.in_cmd_valid    (multiplexer_src_valid),                                       //               .valid
		.out_cmd_channel (serial_flash_inf_cmd_gen_inst_out_cmd_pck_channel),           //    out_cmd_pck.channel
		.out_cmd_eop     (serial_flash_inf_cmd_gen_inst_out_cmd_pck_endofpacket),       //               .endofpacket
		.out_cmd_ready   (serial_flash_inf_cmd_gen_inst_out_cmd_pck_ready),             //               .ready
		.out_cmd_sop     (serial_flash_inf_cmd_gen_inst_out_cmd_pck_startofpacket),     //               .startofpacket
		.out_cmd_data    (serial_flash_inf_cmd_gen_inst_out_cmd_pck_data),              //               .data
		.out_cmd_valid   (serial_flash_inf_cmd_gen_inst_out_cmd_pck_valid),             //               .valid
		.in_rsp_data     (qspi_inf_inst_out_rsp_pck_data),                              //     in_rsp_pck.data
		.in_rsp_ready    (qspi_inf_inst_out_rsp_pck_ready),                             //               .ready
		.in_rsp_valid    (qspi_inf_inst_out_rsp_pck_valid),                             //               .valid
		.out_rsp_channel (serial_flash_inf_cmd_gen_inst_out_rsp_pck_channel),           //    out_rsp_pck.channel
		.out_rsp_data    (serial_flash_inf_cmd_gen_inst_out_rsp_pck_data),              //               .data
		.out_rsp_eop     (serial_flash_inf_cmd_gen_inst_out_rsp_pck_endofpacket),       //               .endofpacket
		.out_rsp_ready   (serial_flash_inf_cmd_gen_inst_out_rsp_pck_ready),             //               .ready
		.out_rsp_sop     (serial_flash_inf_cmd_gen_inst_out_rsp_pck_startofpacket),     //               .startofpacket
		.out_rsp_valid   (serial_flash_inf_cmd_gen_inst_out_rsp_pck_valid),             //               .valid
		.addr_bytes_csr  (csr_controller_addr_bytes_csr_addr_bytes_csr),                // addr_bytes_csr.addr_bytes_csr
		.addr_bytes_xip  (xip_controller_addr_bytes_xip_addr_bytes_xip),                // addr_bytes_xip.addr_bytes_xip
		.dummy_cycles    (serial_flash_inf_cmd_gen_inst_dummy_cycles_dummy_cycles),     //   dummy_cycles.dummy_cycles
		.chip_select     (serial_flash_inf_cmd_gen_inst_chip_select_chip_select),       //    chip_select.chip_select
		.require_rdata   (serial_flash_inf_cmd_gen_inst_require_rdata_require_rdata),   //  require_rdata.require_rdata
		.op_type         (csr_controller_op_type_op_type),                              //        op_type.op_type
		.wr_addr_type    (csr_controller_wr_addr_type_wr_addr_type),                    //   wr_addr_type.wr_addr_type
		.wr_data_type    (csr_controller_wr_data_type_wr_data_type),                    //   wr_data_type.wr_data_type
		.rd_addr_type    (csr_controller_rd_addr_type_rd_addr_type),                    //   rd_addr_type.rd_addr_type
		.rd_data_type    (csr_controller_rd_data_type_rd_data_type),                    //   rd_data_type.rd_data_type
		.op_num_lines    (serial_flash_inf_cmd_gen_inst_op_num_lines_op_num_lines),     //   op_num_lines.op_num_lines
		.addr_num_lines  (serial_flash_inf_cmd_gen_inst_addr_num_lines_addr_num_lines), // addr_num_lines.addr_num_lines
		.data_num_lines  (serial_flash_inf_cmd_gen_inst_data_num_lines_data_num_lines), // data_num_lines.data_num_lines
		.xip_trans_type  (xip_controller_xip_trans_type_xip_trans_type)                 // xip_trans_type.xip_trans_type
	);

	spi_master_spi_master_qspi_inf_inst #(
		.NCS_LENGTH       (1),
		.DATA_LENGTH      (4),
		.MODE_LENGTH      (4)
	) qspi_inf_inst (
		.clk                (clk_clk),                                                     //                clk.clk
		.reset              (reset_reset),                              //              reset.reset
		.in_cmd_channel     (serial_flash_inf_cmd_gen_inst_out_cmd_pck_channel),           //         in_cmd_pck.channel
		.in_cmd_eop         (serial_flash_inf_cmd_gen_inst_out_cmd_pck_endofpacket),       //                   .endofpacket
		.in_cmd_ready       (serial_flash_inf_cmd_gen_inst_out_cmd_pck_ready),             //                   .ready
		.in_cmd_sop         (serial_flash_inf_cmd_gen_inst_out_cmd_pck_startofpacket),     //                   .startofpacket
		.in_cmd_data        (serial_flash_inf_cmd_gen_inst_out_cmd_pck_data),              //                   .data
		.in_cmd_valid       (serial_flash_inf_cmd_gen_inst_out_cmd_pck_valid),             //                   .valid
		.out_rsp_data       (qspi_inf_inst_out_rsp_pck_data),                              //        out_rsp_pck.data
		.out_rsp_valid      (qspi_inf_inst_out_rsp_pck_valid),                             //                   .valid
		.out_rsp_ready      (qspi_inf_inst_out_rsp_pck_ready),                             //                   .ready
		.dummy_cycles       (serial_flash_inf_cmd_gen_inst_dummy_cycles_dummy_cycles),     //       dummy_cycles.dummy_cycles
		.chip_select        (serial_flash_inf_cmd_gen_inst_chip_select_chip_select),       //        chip_select.chip_select
		.qspi_interface_en  (csr_controller_qspi_interface_en_qspi_interface_en),          //  qspi_interface_en.qspi_interface_en
		.require_rdata      (serial_flash_inf_cmd_gen_inst_require_rdata_require_rdata),   //      require_rdata.require_rdata
		.op_num_lines       (serial_flash_inf_cmd_gen_inst_op_num_lines_op_num_lines),     //       op_num_lines.op_num_lines
		.addr_num_lines     (serial_flash_inf_cmd_gen_inst_addr_num_lines_addr_num_lines), //     addr_num_lines.addr_num_lines
		.data_num_lines     (serial_flash_inf_cmd_gen_inst_data_num_lines_data_num_lines), //     data_num_lines.data_num_lines
		.baud_rate_divisor  (csr_controller_baud_rate_divisor_baud_rate_divisor),          //  baud_rate_divisor.baud_rate_divisor
		.cs_delay_setting   (csr_controller_cs_delay_setting_cs_delay_setting),            //   cs_delay_setting.cs_delay_setting
		.read_capture_delay (csr_controller_read_capture_delay_read_capture_delay),        // read_capture_delay.read_capture_delay
		.qspi_pins_dclk     (qspi_pins_dclk),                                              //          qspi_pins.dclk
		.qspi_pins_ncs      (qspi_pins_ncs),                                               //                   .ncs
		.qspi_pins_data_out (qspi_pins_data_out),
		.qspi_pins_data_oe  (qspi_pins_data_oe),
		.qspi_pins_data_in  (qspi_pins_data_in)
	);


endmodule
