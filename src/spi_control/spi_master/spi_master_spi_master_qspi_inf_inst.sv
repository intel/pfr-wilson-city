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


// $File: //acds/main/ip/pgm/intel_generic_serial_flash_interface/intel_generic_serial_flash_interface_cmd/intel_generic_serial_flash_interface_cmd.sv $
// $Revision: #2 $
// $Date: 2018/01/10 $
// $Author: tgngo $

//altera message_off 10036 10858 10230 10030 10034
`timescale 1 ns / 1 ns
module spi_master_spi_master_qspi_inf_inst #(
    parameter NCS_LENGTH    = 3,            // number of FPGA SPI NCS interfaces
    parameter DATA_LENGTH   = 4,            // number of FPGA SPI Data interfaces
    parameter MODE_LENGTH   = 1             // SPI Data interfaces in used for selected mode
) (
    input               clk,
    input               reset,

    input [8:0]         in_cmd_channel,     // WY: not using, remove
    input               in_cmd_eop,
    output logic        in_cmd_ready,
    input               in_cmd_sop,
    input [7:0]         in_cmd_data,
    input               in_cmd_valid,

    output logic [7:0]  out_rsp_data,   
    input               out_rsp_ready,      // WY: flash could not be backpressure, remove
    output logic        out_rsp_valid,
    output logic                    qspi_pins_dclk,
    output logic [NCS_LENGTH-1:0]   qspi_pins_ncs,
    
    output [DATA_LENGTH-1:0]        qspi_pins_data_out,
    output [DATA_LENGTH-1:0]        qspi_pins_data_oe,
    input  [DATA_LENGTH-1:0]        qspi_pins_data_in,
    
    // Signals directly from the command gen component
    input [3:0]         chip_select,        // Chip select values
    input [3:0]         op_num_lines,
    input [3:0]         addr_num_lines,
    input [3:0]         data_num_lines,
    input [4:0]         dummy_cycles,
    input               qspi_interface_en,
    input               require_rdata,
    input [4:0]         baud_rate_divisor,
    input [7:0]         cs_delay_setting,
    input [3:0]         read_capture_delay
);
    
    logic                   oe_wire;

    logic ncs_active_state;     
    logic [15:0]            ncs_active_wire;
   
    logic [NCS_LENGTH-1:0]  ncs_wire;
    logic [NCS_LENGTH-1:0]  ncs_reg;
    
   

    //+-------------------------------------------------------------------------------
    //| Internal signals
    //+--------------------------------------------------------------------------------
    logic [2:0]             demux_channel;
    logic                   cmd_eop_adp_8_1;
    logic                   cmd_ready_adp_8_1;
    logic                   cmd_sop_adp_8_1;
    logic [7:0]            cmd_data_adp_8_1;
    logic                   cmd_valid_adp_8_1;
    logic [11:0]            cmd_channel_adp_8_1;

    logic                   cmd_eop_adp_8_2;
    logic                   cmd_ready_adp_8_2;
    logic                   cmd_sop_adp_8_2;
    logic [7:0]            cmd_data_adp_8_2;
    logic                   cmd_valid_adp_8_2;
    logic [11:0]            cmd_channel_adp_8_2;
    
    logic                   cmd_eop_adp_8_4;
    logic                   cmd_ready_adp_8_4;
    logic                   cmd_sop_adp_8_4;
    logic [7:0]            cmd_data_adp_8_4;
    logic                   cmd_valid_adp_8_4;
    logic [11:0]            cmd_channel_adp_8_4;
    
    logic                   cmd_out_ready_adp_8_1;
    logic                   cmd_out_valid_adp_8_1; 
    logic [11:0]            cmd_out_channel_adp_8_1;   
    logic [3:0]             cmd_out_data_adp_8_1;
    logic                   cmd_out_sop_adp_8_1;
    logic                   cmd_out_eop_adp_8_1;
    logic                   cmd_out_ready_adp_8_2;
    logic                   cmd_out_valid_adp_8_2;
    logic [11:0]            cmd_out_channel_adp_8_2;
    logic [3:0]             cmd_out_data_adp_8_2;
    logic                   cmd_out_sop_adp_8_2;
    logic                   cmd_out_eop_adp_8_2;
    logic                   cmd_out_ready_adp_8_4;
    logic                   cmd_out_valid_adp_8_4; 
    logic [11:0]            cmd_out_channel_adp_8_4;
    logic [3:0]             cmd_out_data_adp_8_4;
    logic                   cmd_out_sop_adp_8_4;
    logic                   cmd_out_eop_adp_8_4;
    logic                   cmd_out_ready;
    logic                   cmd_out_valid;
    logic [3:0]             cmd_out_data;
    logic [11:0]            cmd_out_channel;
    logic                   cmd_out_sop;
    logic                   cmd_out_eop;
    logic [3:0]             sc_fifo_0_out_data;
    logic                   sc_fifo_0_out_valid;
    logic                   sc_fifo_0_out_ready;
    logic                   sc_fifo_0_out_startofpacket;
    logic                   sc_fifo_0_out_endofpacket;
    logic [11:0]            sc_fifo_0_out_channel;
    logic [3:0]             type_of_transaction;

    // Signals to flash
    logic [3:0]             flash_data_out;
    logic [3:0]             flash_data_in;
    logic [3:0]             flash_data_oe;
    logic                   flash_clk_reg;
    logic                   oe_reg;
    logic [3:0]             flash_data_out_reg;
    logic [3:0]             flash_data_oe_reg;

    // Signals to clock generator
    logic                   clock_genenrator_en;
    logic                   flash_clk;
    logic                   flash_clk_neg;
    logic                   flash_clk_neg_reg;
    logic flash_clk_pos;
    logic flash_clk_latch_datain;

    logic cs_assert_cnt_done;
    logic [3:0] cs_assert_cnt;
    logic [3:0] cs_assert_cnt_next;
    logic cs_deassert_cnt_done;
    logic [3:0] cs_deassert_cnt;
    logic [3:0] cs_deassert_cnt_next;
    logic [4:0] dummy_cnt_val;
    
    // Decode the channel to check the value is belong to opcode, address, data, or dummy
    // base on this, check with the line each filed is set, route them to correct adapter
    // For incoming packet:
    //                  Channel decode: 0001: opcode
    //                                  0010: address
    //                                  0100: write data
    //                                  1000: dummy
    // For data lines (take from user);  op_num_lines; addr_num_lines; data_num_lines
    //                  Channel decode: 0001: 1 line
    //                                  0010: 2 lines
    //                                  0100: 4 lines
    //                                  1000: reserved
    // 
    // for the in_cmd_channel; the [MSB, MSB-1] os the decode to indicate where this packet come from
    //                          2'b01 is the channel for XIP controller
    //                          2'b10 is the channel for CST controller
    //                          need this information since now the instruction lines are runtime.
    always_comb begin
        // For the packet comes from CSR, the data lines are same for opcode, address, data in, out
        // For write/read comes from XIP, the opcode lines, address lines and data lines can be different
        if (in_cmd_channel[5:4] == 2'b10) begin // from csr
            demux_channel  = op_num_lines[2:0];
        end else begin // from xip -not correct, please check this
            if (in_cmd_channel[3:0] == 4'b0001) // this is opcode
                demux_channel  = op_num_lines[2:0];
            else if (in_cmd_channel[3:0] == 4'b0010) // address
                demux_channel  = addr_num_lines[2:0];
                 else
                demux_channel = data_num_lines[2:0]; // dummy and data use same setting for data lines
        end
    end // always_comb

    //+-------------------------------------------------------------------------------    
    //| The Demux, route the incomming packet to correct adapter depend on the setting
    //| of the data lines are used.
    //+-------------------------------------------------------------------------------
    demultiplexer_12_channels demultiplexer_inst (
        .clk                (clk),
        .reset              (reset),
        .sink_ready         (in_cmd_ready),
        .sink_channel       ({in_cmd_channel,demux_channel}),
        .sink_data          (in_cmd_data),
        .sink_startofpacket (in_cmd_channel[7]),
        .sink_endofpacket   (in_cmd_channel[6]),
        .sink_valid         (in_cmd_valid),
        .src0_ready         (cmd_ready_adp_8_1),
        .src0_valid         (cmd_valid_adp_8_1),
        .src0_data          (cmd_data_adp_8_1),
        .src0_channel       (cmd_channel_adp_8_1),
        .src0_startofpacket (cmd_sop_adp_8_1),
        .src0_endofpacket   (cmd_eop_adp_8_1),
        .src1_ready         (cmd_ready_adp_8_2),
        .src1_valid         (cmd_valid_adp_8_2),
        .src1_data          (cmd_data_adp_8_2),
        .src1_channel       (cmd_channel_adp_8_2),
        .src1_startofpacket (cmd_sop_adp_8_2),
        .src1_endofpacket   (cmd_eop_adp_8_2),
        .src2_ready         (cmd_ready_adp_8_4),
        .src2_valid         (cmd_valid_adp_8_4),
        .src2_data          (cmd_data_adp_8_4),
        .src2_channel       (cmd_channel_adp_8_4),
        .src2_startofpacket (cmd_sop_adp_8_4),
        .src2_endofpacket   (cmd_eop_adp_8_4)
    );

    //+-------------------------------------------------------------------------------    
    //| The adapters which converts the packet (8 bits) to either 1, 2, or 4 bits each
    //+-------------------------------------------------------------------------------
    adapter_8_1 adapter_8_1_inst (
        .clk               (clk),
        .reset_n           (!reset),
        .in_data           (cmd_data_adp_8_1),
        .in_valid          (cmd_valid_adp_8_1),
        .in_ready          (cmd_ready_adp_8_1),
        .in_startofpacket  (cmd_sop_adp_8_1),
        .in_endofpacket    (cmd_eop_adp_8_1),
        .in_channel        (cmd_channel_adp_8_1),
        .out_channel       (cmd_out_channel_adp_8_1),
        .out_data          (cmd_out_data_adp_8_1),         
        .out_valid         (cmd_out_valid_adp_8_1),
        .out_ready         (cmd_out_ready_adp_8_1),
        .out_startofpacket (cmd_out_sop_adp_8_1),
        .out_endofpacket   (cmd_out_eop_adp_8_1)
    );

    adapter_8_2 adapter_8_2_inst (
        .clk                (clk),
        .reset_n            (!reset),
        .in_data            (cmd_data_adp_8_2),
        .in_valid           (cmd_valid_adp_8_2),
        .in_startofpacket   (cmd_sop_adp_8_2),
        .in_endofpacket     (cmd_eop_adp_8_2),
        .in_ready           (cmd_ready_adp_8_2),
        .in_channel        (cmd_channel_adp_8_2),
        .out_channel       (cmd_out_channel_adp_8_2),
        .out_data           (cmd_out_data_adp_8_2),
        .out_valid          (cmd_out_valid_adp_8_2),
        .out_ready          (cmd_out_ready_adp_8_2),
        .out_startofpacket  (cmd_out_sop_adp_8_2),
        .out_endofpacket    (cmd_out_eop_adp_8_2)
    );

    adapter_8_4 adapter_8_4_inst (
        .clk                (clk),
        .reset_n            (!reset),
        .in_data            (cmd_data_adp_8_4),
        .in_valid           (cmd_valid_adp_8_4),
        .in_startofpacket   (cmd_sop_adp_8_4),
        .in_endofpacket     (cmd_eop_adp_8_4),
        .in_ready           (cmd_ready_adp_8_4),
        .in_channel        (cmd_channel_adp_8_4),
        .out_channel       (cmd_out_channel_adp_8_4),
        .out_data           (cmd_out_data_adp_8_4),
        .out_valid          (cmd_out_valid_adp_8_4),
        .out_ready          (cmd_out_ready_adp_8_4),
        .out_startofpacket  (cmd_out_sop_adp_8_4),
        .out_endofpacket    (cmd_out_eop_adp_8_4)
    );

    //+-------------------------------------------------------------------------------    
    //| The Mux, route converted data acornginly to the number of line to the fifo
    //+-------------------------------------------------------------------------------
    qspi_inf_mux qspi_inf_mux_inst (
        .clk                 (clk),
        .reset               (reset),
        .src_ready           (cmd_out_ready),
        //.src_ready           (1),
        .src_valid           (cmd_out_valid),
        .src_data            (cmd_out_data),
        .src_channel         (cmd_out_channel),
        .src_startofpacket   (cmd_out_sop),
        .src_endofpacket     (cmd_out_eop),
        .sink0_ready         (cmd_out_ready_adp_8_1),
        .sink0_valid         (cmd_out_valid_adp_8_1),
        .sink0_channel       (cmd_out_channel_adp_8_1), 
        .sink0_data          (cmd_out_data_adp_8_1),
        .sink0_startofpacket (cmd_out_sop_adp_8_1),
        .sink0_endofpacket   (cmd_out_eop_adp_8_1),
        .sink1_ready         (cmd_out_ready_adp_8_2),
        .sink1_valid         (cmd_out_valid_adp_8_2),
        .sink1_channel       (cmd_out_channel_adp_8_2),
        .sink1_data          (cmd_out_data_adp_8_2),
        .sink1_startofpacket (cmd_out_sop_adp_8_2),
        .sink1_endofpacket   (cmd_out_eop_adp_8_2),
        .sink2_ready         (cmd_out_ready_adp_8_4),
        .sink2_valid         (cmd_out_valid_adp_8_4),
        .sink2_channel       (cmd_out_channel_adp_8_4),
        .sink2_data          (cmd_out_data_adp_8_4),
        .sink2_startofpacket (cmd_out_sop_adp_8_4),
        .sink2_endofpacket   (cmd_out_eop_adp_8_4)
    );

    logic fifo_pop_out;
    logic fifo_read_trigger;
    //+-------------------------------------------------------------------------------    
    //| The fifo buffer data before sending to the flash
    //+-------------------------------------------------------------------------------
    inf_sc_fifo_ser_data inf_sc_fifo_ser_data_inst (
        .clk               (clk),
        .reset             (reset),
        .in_data           (cmd_out_data),
        .in_valid          (cmd_out_valid),
        .in_ready          (cmd_out_ready),
        .in_startofpacket  (cmd_out_sop),
        .in_endofpacket    (cmd_out_eop),
        .in_channel        (cmd_out_channel),
        .out_data          (sc_fifo_0_out_data),
        .out_valid         (sc_fifo_0_out_valid),
        .out_ready         (fifo_pop_out),
        .out_startofpacket (sc_fifo_0_out_startofpacket),
        .out_endofpacket   (sc_fifo_0_out_endofpacket),
        .out_channel       (sc_fifo_0_out_channel)
    );
    //sc_fifo_0_out_channel: the demux input channel has format: [2bits]: controller csr or xip][4bits]: type of transaction: opcode, addr, ..., [3bits]: demux channels
    // at output, the demux shift right 3 bits (num ouput) so the final channel here [3bits]: shifted - should not read [4bits]: type of transaction: opcode, addr, ...
    // use last 4 bits to indicate what kind of transaction is this.

   // State machine
    typedef enum bit [9:0]
    {
        ST_IDLE           = 10'b0000000001,
        ST_ASSERT_CS_DLY    = 10'b0000000010,
        ST_START_CLK        = 10'b0000000100,
        ST_ASSERT_CS        = 10'b0000001000,
        ST_SEND             = 10'b0000010000,
        ST_DUMMY_CYCLES     = 10'b0000100000,
        ST_RECEIVE          = 10'b0001000000,
        ST_STOP_CLK         = 10'b0010000000,
        ST_DEASSERT_CS      = 10'b0100000000,
        ST_DEASSERT_CS_DLY  = 10'b1000000000
     } t_state;
    t_state state, next_state;

    logic [3:0]         op_num_lines_reg;
    logic [3:0]         addr_num_lines_reg;
    logic [3:0]         data_num_lines_reg;
     always_ff @(posedge clk or posedge reset) begin
        if (reset) begin 
            op_num_lines_reg   <= '0;
            addr_num_lines_reg <= '0;
            data_num_lines_reg <= '0;
        end 
        else begin 
            if (state == ST_START_CLK) begin
                op_num_lines_reg   <= op_num_lines;
                addr_num_lines_reg <= addr_num_lines;
                data_num_lines_reg <= data_num_lines;
            end
        end // else: !if(reset)
     end // always_ff @
    

    logic require_rdata_reg;
    // latch this value at start of transaction, since delay, this component might still sending data but it can 
    // take another command, this signal might make thing goes nut
    // store this every time starty the clock generator which starts of eeach transaction
    always @(posedge clk or posedge reset) begin 
        if(reset)
            require_rdata_reg <= '0;
        else begin
            if (state == ST_START_CLK)
                require_rdata_reg <= require_rdata;
        end
    end
    

    always_comb begin 
        //flash_data_out = sc_fifo_0_out_data;
        //if (sc_fifo_0_out_channel == 4'h01) begin // opcode
        if (type_of_transaction == 4'h1) begin 
            if (op_num_lines_reg == 4'b0001) begin 
                flash_data_out[3]  = 1'b1;
                flash_data_out[2]  = 1'b1;
                flash_data_out[1]  = 1'b1;
                flash_data_out[0]  = sc_fifo_0_out_data[0];
            end
            else if (op_num_lines_reg == 4'b0010) begin // x2
                flash_data_out[3]  = 1'b1;
                flash_data_out[2]  = 1'b1;
                flash_data_out[1]  = sc_fifo_0_out_data[1];
                flash_data_out[0]  = sc_fifo_0_out_data[0];
            end
            else 
                flash_data_out  = sc_fifo_0_out_data;
        end
        else if (type_of_transaction == 4'h2) begin 
            if (addr_num_lines_reg == 4'b0001) begin 
                flash_data_out[3]  = 1'b1;
                flash_data_out[2]  = 1'b1;
                flash_data_out[1]  = 1'b1;
                flash_data_out[0]  = sc_fifo_0_out_data[0];
            end
            else if (addr_num_lines_reg == 4'b0010) begin // x2
                flash_data_out[3]  = 1'b1;
                flash_data_out[2]  = 1'b1;
                flash_data_out[1]  = sc_fifo_0_out_data[1];
                flash_data_out[0]  = sc_fifo_0_out_data[0];
            end
            else 
                flash_data_out  = sc_fifo_0_out_data;
        end
        else if (type_of_transaction == 4'h4) begin 
            if (data_num_lines_reg == 4'b0001) begin 
                flash_data_out[3]  = 1'b1;
                flash_data_out[2]  = 1'b1;
                flash_data_out[1]  = 1'b1;
                flash_data_out[0]  = sc_fifo_0_out_data[0];
            end
            else if (data_num_lines_reg == 4'b0010) begin // x2
                flash_data_out[3]  = 1'b1;
                flash_data_out[2]  = 1'b1;
                flash_data_out[1]  = sc_fifo_0_out_data[1];
                flash_data_out[0]  = sc_fifo_0_out_data[0];
            end
            else 
                flash_data_out  = sc_fifo_0_out_data;
        end
        else begin 
            flash_data_out[3]  = 1'b1;
            flash_data_out[2]  = 1'b1;
            flash_data_out[1]  = 1'b1;
            flash_data_out[0]  = 1'b1;
        end // else: !if(type_of_transaction == 4'h4)
    end // always_comb
    
    logic pol;
    assign pol = 1'b1;
    logic clock_genenrator_en_pol0;
    logic clock_gen_en;
    always_ff @(posedge clk) begin
        clock_genenrator_en_pol0 <= clock_genenrator_en;
    end
    assign clock_gen_en = pol ? clock_genenrator_en : clock_genenrator_en_pol0;

    clk_div #( 
        .WIDTH  (5)
    ) clk_div_new_inst_2
    (
        .clk                    (clk),
        .reset                  (reset),
        //.divisor                (5'h1),
        .divisor                (baud_rate_divisor),
        //.divisor                (5'h32),
        .pol                    (pol),
        //.enable                 (clock_genenrator_en),
        .enable                 (clock_gen_en),
        .falling_edge_trigger   (fifo_read_trigger),
        .clk_out                (flash_clk),
        .rising_edge            (flash_clk_pos),
        .falling_edge           (flash_clk_neg)
    );

    // +--------------------------------------------------
    // | Dummy cycles counter
    // +--------------------------------------------------
    logic dummy_cnt_done;
    logic [4:0] dummy_cnt;
    logic [4:0] dummy_cnt_next;
    
    always_ff @(posedge clk or posedge reset) begin
        if (reset)
            dummy_cnt <= '0;
        else 
            dummy_cnt <= dummy_cnt_next;
    end

    assign dummy_cnt_done = (dummy_cnt == (dummy_cycles - 5'h1)) && fifo_read_trigger;

    always_comb begin 
        dummy_cnt_next = dummy_cnt;
        if ((state == ST_DUMMY_CYCLES) && fifo_read_trigger) begin
            dummy_cnt_next = dummy_cnt_next + 4'h1;
            if (dummy_cnt_done)
                dummy_cnt_next = '0;
        end
    end


    // +--------------------------------------------------
    // | State Machine: update state
    // +--------------------------------------------------
    // |
    always_ff @(posedge clk or posedge reset) begin
        if (reset)
            state <= ST_IDLE;
        else
            state <= next_state;
    end
    // +--------------------------------------------------
    // | State Machine: next state condition
    // +--------------------------------------------------
    always_comb begin
        next_state  = ST_IDLE;
        case (state)
            ST_IDLE: begin
                next_state  = ST_IDLE;
                if (sc_fifo_0_out_valid) begin
                    if (cs_delay_setting[3:0] <= 4'h1) // if CS delay is 0, then to start_clk, if 1 then use at start_clk, assert CS
                        next_state = ST_START_CLK;
                    else // if not 1 then go to delay CS, if it is 1 then make use of start_clk state
                        next_state = ST_ASSERT_CS_DLY;
                end
            end
            ST_ASSERT_CS_DLY: begin 
                next_state = ST_ASSERT_CS_DLY;
                if (cs_assert_cnt_done)
                    next_state = ST_START_CLK;
            end
            ST_START_CLK: begin 
                next_state = ST_ASSERT_CS;
            end
            ST_ASSERT_CS: begin
               next_state = ST_SEND; 
            end
            ST_SEND: begin 
                next_state = ST_SEND;
                if (sc_fifo_0_out_endofpacket && sc_fifo_0_out_channel[8] && fifo_pop_out) begin
                    if (require_rdata_reg) begin
                        if (dummy_cycles > 0)
                            next_state = ST_DUMMY_CYCLES;
                        else    
                            next_state = ST_RECEIVE;
                    end
                    else
                        next_state = ST_STOP_CLK;
                end
            end
            ST_DUMMY_CYCLES: begin
                next_state = ST_DUMMY_CYCLES;
                if (dummy_cnt_done) 
                    next_state = ST_RECEIVE;
            end
            ST_RECEIVE: begin
                if (require_rdata) 
                    next_state = ST_RECEIVE;
                else 
                    next_state = ST_STOP_CLK;
            end
            ST_STOP_CLK: begin 
                if (cs_delay_setting[7:4] == 4'h0) // if CS deassert delay is 0, then to deaasert, 
                    next_state = ST_DEASSERT_CS;
                else
                    next_state = ST_DEASSERT_CS_DLY;
            end
            ST_DEASSERT_CS_DLY: begin 
                next_state = ST_DEASSERT_CS_DLY;
                if (cs_deassert_cnt_done)
                    next_state = ST_DEASSERT_CS;
            end
            ST_DEASSERT_CS: begin 
                next_state = ST_IDLE;
            end

        endcase // case (state)
    end // always_comb
    
    // +--------------------------------------------------
    // | State Machine: state outputs
    // +--------------------------------------------------
    always_comb begin
        clock_genenrator_en  = '0;
        fifo_pop_out         = '0;
        case (state)
            ST_IDLE: begin
                clock_genenrator_en  = '0;
                fifo_pop_out         = '0;
            end
            ST_ASSERT_CS_DLY: begin
                clock_genenrator_en  = '0;
                fifo_pop_out         = '0;
            end
            ST_START_CLK: begin 
                clock_genenrator_en  = 1;
                fifo_pop_out         = '0;
            end
            ST_ASSERT_CS: begin 
                clock_genenrator_en  = 1;
                fifo_pop_out         = '0;
            end
            ST_SEND: begin 
                clock_genenrator_en  = 1;
                fifo_pop_out         = fifo_read_trigger;
            end
            ST_RECEIVE: begin 
                clock_genenrator_en  = 1;
                fifo_pop_out         = '0;
            end
            ST_DUMMY_CYCLES: begin 
                clock_genenrator_en  = 1;
                fifo_pop_out         = '0;
            end
            ST_STOP_CLK: begin 
                clock_genenrator_en  = 0;
                fifo_pop_out         = '0;
            end
            ST_DEASSERT_CS: begin 
                clock_genenrator_en  = 0;
                fifo_pop_out         = '0;
            end
            default: begin
                clock_genenrator_en  = '0;
                fifo_pop_out         = '0;
            end
        endcase // case (state)
    end


    //+-------------------------------------------------------------------------------    
    //| oe, active low, qspi_interface_en must be registered in previous component
    //+-------------------------------------------------------------------------------
    assign oe_wire  = ~qspi_interface_en;

    //+-------------------------------------------------------------------------------
    //| ncs, active low
    //+-------------------------------------------------------------------------------
    // convert chip_select to one-hot signal
    always @(chip_select) begin
        ncs_active_wire               = 0;
        ncs_active_wire[chip_select]  = 1'b1;
    end
    assign ncs_active_state = ((state == ST_START_CLK && (cs_delay_setting[3:0] >= 4'h1)) ||  state == ST_ASSERT_CS || state == ST_ASSERT_CS_DLY
                              || state == ST_SEND
                              || state == ST_RECEIVE || state == ST_DUMMY_CYCLES
                              || state == ST_STOP_CLK || state == ST_DEASSERT_CS_DLY);

    genvar i;
    generate
        for (i=0; i<NCS_LENGTH; i++) begin : ncs_active_inst
            assign ncs_wire[i]  = ~ncs_active_wire[i] || ~ncs_active_state;
        end
    endgenerate

    always_ff @(posedge clk or posedge reset) begin
        if (reset)
            ncs_reg <= {NCS_LENGTH{1'b1}};
        else
            ncs_reg <= ncs_wire;
    end

    // +--------------------------------------------------
    // | CS Assert counter
    // +--------------------------------------------------

    
    always_ff @(posedge clk or posedge reset) begin
        if (reset)
            cs_assert_cnt <= '0;
        else 
            cs_assert_cnt <= cs_assert_cnt_next;
    end

    assign cs_assert_cnt_done = (cs_assert_cnt == (cs_delay_setting[3:0] - 4'h2)); // one is at start_clk state, one for this calculate, so - 2 as start from 0

    always_comb begin 
        cs_assert_cnt_next = cs_assert_cnt;
        if (state == ST_ASSERT_CS_DLY) begin
            cs_assert_cnt_next = cs_assert_cnt_next + 4'h1;
            if (cs_assert_cnt_done)
                cs_assert_cnt_next = '0;
        end
    end
    // +--------------------------------------------------
    // | CS Deassert counter
    // +--------------------------------------------------

    
    always_ff @(posedge clk or posedge reset) begin
        if (reset)
            cs_deassert_cnt <= '0;
        else 
            cs_deassert_cnt <= cs_deassert_cnt_next;
    end

    assign cs_deassert_cnt_done = (cs_deassert_cnt == (cs_delay_setting[7:4] - 4'h1)); 

    always_comb begin 
        cs_deassert_cnt_next = cs_deassert_cnt;
        if (state == ST_DEASSERT_CS_DLY) begin
            cs_deassert_cnt_next = cs_deassert_cnt_next + 4'h1;
            if (cs_deassert_cnt_done)
                cs_deassert_cnt_next = '0;
        end
    end


    //+-------------------------------------------------------------------------------    
    //|      datain(read at posedge) 
    //+-------------------------------------------------------------------------------    
    logic [15:0] read_capture_delay_reg;
    always_ff @(posedge clk or posedge reset) begin
        if (reset)
            read_capture_delay_reg <= '0;
        else begin
            if (state == ST_RECEIVE)
                read_capture_delay_reg      <= {read_capture_delay_reg[13:0], flash_clk_pos};
            else 
                read_capture_delay_reg <= '0;     
        end
    end

    assign flash_clk_latch_datain = read_capture_delay_reg[read_capture_delay];

    logic [7:0] flash_datain_reg;
    logic [7:0] flash_datain_reg_t;
    

    //always_ff @(negedge clk or posedge reset) begin
    always_ff @(posedge clk or posedge reset) begin
        if (reset)
            flash_datain_reg <= '0;
        else begin
            if (flash_clk_latch_datain) begin
                if (data_num_lines == 4'b0001) //x1
                    flash_datain_reg <= {flash_datain_reg[6:0], flash_data_in[1]};
                else if (data_num_lines == 4'b0010)  //x2
                    flash_datain_reg <= {flash_datain_reg[5:0], flash_data_in[1:0]};
                else //x4
                    flash_datain_reg <= {flash_datain_reg[3:0], flash_data_in};
            end
        end
    end

    //+-------------------------------------------------------------------------------    
    logic [3:0] read_data_cnt;
    logic [3:0] read_data_cnt_next;
    logic       read_data_done;

    logic       read_data_valid;
    // +--------------------------------oo------------------
    // | Address bytes counter
    // +--------------------------------------------------
    always_ff @(posedge clk or posedge reset) begin
        if (reset)
            read_data_cnt <= '0;
        else begin
            if (state == ST_SEND) begin
                if (data_num_lines == 4'b0001) //x1
                    read_data_cnt <= 7;
                else if (data_num_lines == 4'b0010) //x2
                    read_data_cnt <= 3;
                else  //x4
                    read_data_cnt <= 1;
            end
            else
                read_data_cnt <= read_data_cnt_next;
        end
    end


    //assign read_data_done = ((read_data_cnt == 0) & flash_clk_neg & (state == ST_RECEIVE));
    assign read_data_done = ((read_data_cnt == 0) & flash_clk_latch_datain & (state == ST_RECEIVE));

    always_comb begin 
        read_data_cnt_next = read_data_cnt;
        //if ((state == ST_RECEIVE) & flash_clk_neg) begin
            if ((state == ST_RECEIVE) & flash_clk_latch_datain) begin
            read_data_cnt_next = read_data_cnt_next - 4'h1;
            if (read_data_done) begin
                if (data_num_lines == 4'b0001) //x1
                    read_data_cnt_next = 7;
                else if (data_num_lines == 4'b0010) //x2
                    read_data_cnt_next = 3;
                else  //x4
                    read_data_cnt_next = 1;
            end
        end
    end

    always_ff @(posedge clk) begin    
        read_data_valid <= read_data_done;
    end

    always_ff @(posedge clk or posedge reset) begin
        if (reset) begin
            out_rsp_data  <= '0;
            out_rsp_valid <= '0;
        end
        else if (read_data_valid) begin
            //out_rsp_data  <= flash_datain_reg;
            out_rsp_data  <= flash_datain_reg;
            out_rsp_valid <= 1'b1;
        end
        else begin
            out_rsp_data  <= out_rsp_data;
            out_rsp_valid <= '0;
        end
    end

// +--------------------------------------------------------------------------------
// |     dataoe, 1=output, 0=input
// +--------------------------------------------------------------------------------

    assign type_of_transaction = sc_fifo_0_out_channel[3:0];

    always_comb begin 
        //flash_data_oe = 4'b1101; // default is x1
        flash_data_oe = 4'b1111;
        if (state == ST_SEND) begin 
            case (type_of_transaction)
                4'h1: begin // opcode
                    if (op_num_lines_reg == 4'b0001) //x1
                        flash_data_oe = 4'b1101;
                    else // x2, x4: set all to 1 - output
                        flash_data_oe = 4'b1111;
                end
                4'h2: begin // address
                    if (addr_num_lines_reg == 4'b0001) //x1
                        flash_data_oe = 4'b1101;
                    else  // x2, x4: set all to 1 - output
                        flash_data_oe = 4'b1111;     
                end
                4'h4: begin // write data 
                    if (data_num_lines_reg == 4'b0001) //x1
                        flash_data_oe = 4'b1101;
                    else  // x2, x4: set all to 1 - output
                        flash_data_oe = 4'b1111;     
                end
                default: begin // dummy
                    flash_data_oe = 4'b1111;     
                end
            endcase // type_of_transaction
        end
        else begin 
            if (state == ST_RECEIVE) begin 
                if (data_num_lines_reg == 4'b0001)
                    flash_data_oe = 4'b1101;
                else if (data_num_lines_reg == 4'b0010)
                    flash_data_oe = 4'b1100;
                else
                    flash_data_oe = 4'b0000;
            end
        end
    end



    always_ff @(posedge clk) begin
        flash_clk_reg      <= flash_clk;
        oe_reg             <= oe_wire;
        flash_data_out_reg <= flash_data_out;
        flash_clk_neg_reg  <= flash_clk_neg;
        flash_data_oe_reg  <= flash_data_oe;
    end

    //+--------------------------------------------------------------------------------
    //|      QSPI interfaces 
    //+--------------------------------------------------------------------------------
    // feed to asmiblock
// GPIO interface
//    intel_generic_serial_flash_interface_gpio #(
//        .NCS_LENGTH(NCS_LENGTH),
//        .DATA_LENGTH(DATA_LENGTH)
//    ) dedicated_interface (
//        .atom_ports_dclk(flash_clk_reg),
//        .atom_ports_ncs(ncs_reg),
//        .atom_ports_oe(oe_reg),
//        .atom_ports_dataout(flash_data_out_reg),
//        .atom_ports_dataoe(flash_data_oe_reg),
//        .atom_ports_datain(flash_data_in),
//        
//        .qspi_pins_dclk(qspi_pins_dclk),
//        .qspi_pins_ncs(qspi_pins_ncs),
//        .qspi_pins_data(qspi_pins_data));

    // original generated code used the module above to instantiate tri-state drivers
    // we want the tri-state drivers in the top level of our design to allow muxing of multiple SPI interfaces, so we pass the data_out, data_oe, and data_in ports up to the top level
    assign qspi_pins_data_out = flash_data_out_reg;
    assign qspi_pins_data_oe = flash_data_oe_reg;
    assign flash_data_in = qspi_pins_data_in;
    assign qspi_pins_dclk = flash_clk_reg;          // original code allowed this output to be tri-stated, we don't need that functionality
    assign qspi_pins_ncs = ncs_reg;                 // original code allowed this output to be tri-stated, we don't need that functionality

endmodule 


module adapter_8_1 (
                    // Interface: in
                    output reg           in_ready,
                    input                in_valid,
                    input [8-1 : 0]      in_data,
                    input [12-1 : 0]     in_channel,
                    input                in_startofpacket,
                    input                in_endofpacket,
                    // Interface: out
                    input                out_ready,
                    output reg           out_valid,
                    output reg [3:0]     out_data,
                    output reg [12-1: 0] out_channel,
                    output reg           out_startofpacket,
                    output reg           out_endofpacket,

                    // Interface: clk
                    input                clk,
                    // Interface: reset
                    input                reset_n

);


    assign out_data[1] = 1'b1;
    assign out_data[2] = 1'b1;
    assign out_data[3] = 1'b1;
   // ---------------------------------------------------------------------
   //| Signal Declarations
   // ---------------------------------------------------------------------
    reg [12-1:0]                         state_read_addr;
    wire [3-1:0]                         state_from_memory;
    reg [3-1:0]                          state;
    reg [3-1:0]                          new_state;
    reg [3-1:0]                          state_d1;
    
    
    reg                                  in_ready_d1;
    reg [12-1:0]                         mem_readaddr; 
    reg [12-1:0]                         mem_readaddr_d1;
    reg                                  a_ready;
    reg                                  a_valid;
    reg [12-1:0]                         a_channel;
    reg [1-1:0]                          a_data0; 
    reg [1-1:0]                          a_data1; 
    reg [1-1:0]                          a_data2; 
    reg [1-1:0]                          a_data3; 
    reg [1-1:0]                          a_data4; 
    reg [1-1:0]                          a_data5; 
    reg [1-1:0]                          a_data6; 
    reg [1-1:0]                          a_data7; 
    reg                                  a_startofpacket;
    reg                                  a_endofpacket;
    reg                                  a_empty;
    reg                                  a_error;
    reg                                  b_ready;
    reg                                  b_valid;
    reg [12-1:0]                         b_channel;
    reg [1-1:0]                          b_data;
    reg                                  b_startofpacket; 
    wire                                 b_startofpacket_wire; 
    reg                                  b_endofpacket; 
    reg                                  b_empty;   
    reg                                  b_error; 
    reg                                  mem_write0;
    reg [1-1:0]                          mem_writedata0;
    wire [1-1:0]                         mem_readdata0;
    wire                                 mem_waitrequest0;
    reg [1-1:0]                          mem0[0:0];
    reg                                  mem_write1;
    reg [1-1:0]                          mem_writedata1;
    wire [1-1:0]                         mem_readdata1;
    wire                                 mem_waitrequest1;
    reg [1-1:0]                          mem1[0:0];
    reg                                  mem_write2;
    reg [1-1:0]                          mem_writedata2;
    wire [1-1:0]                         mem_readdata2;
    wire                                 mem_waitrequest2;
    reg [1-1:0]                          mem2[0:0];
    reg                                  mem_write3;
    reg [1-1:0]                          mem_writedata3;
    wire [1-1:0]                         mem_readdata3;
    wire                                 mem_waitrequest3;
    reg [1-1:0]                          mem3[0:0];
    reg                                  mem_write4;
    reg [1-1:0]                          mem_writedata4;
    wire [1-1:0]                         mem_readdata4;
    wire                                 mem_waitrequest4;
    reg [1-1:0]                          mem4[0:0];
    reg                                  mem_write5;
    reg [1-1:0]                          mem_writedata5;
    wire [1-1:0]                         mem_readdata5;
    wire                                 mem_waitrequest5;
    reg [1-1:0]                          mem5[0:0];
    reg                                  mem_write6;
    reg [1-1:0]                          mem_writedata6;
    wire [1-1:0]                         mem_readdata6;
    wire                                 mem_waitrequest6;
    reg [1-1:0]                          mem6[0:0];
    reg                                  sop_mem_writeenable;
    reg                                  sop_mem_writedata;
    wire                                 mem_waitrequest_sop; 
    wire                                 state_waitrequest;
    reg                                  state_waitrequest_d1; 
    reg [8-1:0]                          in_empty = 0;
    reg [1-1:0]                          out_empty;
    
    reg                                  in_error = 0;
    reg                                  out_error; 
    
    
    reg [3-1:0]                          state_register;
    reg                                  sop_register; 
    reg                                  error_register;
    reg [1-1:0]                          data0_register;
    reg [1-1:0]                          data1_register;
    reg [1-1:0]                          data2_register;
    reg [1-1:0]                          data3_register;
    reg [1-1:0]                          data4_register;
    reg [1-1:0]                          data5_register;
    reg [1-1:0]                          data6_register;
    
    // ---------------------------------------------------------------------
   //| Input Register Stage
   // ---------------------------------------------------------------------
   always @(posedge clk or negedge reset_n) begin
      if (!reset_n) begin
          a_valid         <= 0;
          a_channel       <= 0;
          a_data0         <= 0;
          a_data1         <= 0;
          a_data2         <= 0;
          a_data3         <= 0;
          a_data4         <= 0;
          a_data5         <= 0;
          a_data6         <= 0;
          a_data7         <= 0;
          a_startofpacket <= 0;
          a_endofpacket   <= 0;
          a_empty         <= 0; 
          a_error         <= 0;
      end else begin
         if (in_ready) begin
             a_valid         <= in_valid;
             a_channel       <= in_channel;
             a_error         <= in_error;
             a_data0         <= in_data[7:7];
             a_data1         <= in_data[6:6];
             a_data2         <= in_data[5:5];
             a_data3         <= in_data[4:4];
             a_data4         <= in_data[3:3];
             a_data5         <= in_data[2:2];
             a_data6         <= in_data[1:1];
             a_data7         <= in_data[0:0];
             a_startofpacket <= in_startofpacket;
             a_endofpacket   <= in_endofpacket;
             a_empty         <= 0;
             if (in_endofpacket)
                 a_empty <= in_empty;
         end
      end
   end // always @ (posedge clk or negedge reset_n)
    
    always @* begin 
        state_read_addr  = a_channel;
        if (in_ready)
            state_read_addr  = in_channel;
    end
   

   // ---------------------------------------------------------------------
   //| State & Memory Keepers
   // ---------------------------------------------------------------------
   always @(posedge clk or negedge reset_n) begin
      if (!reset_n) begin
          in_ready_d1          <= 0;
          state_d1             <= 0;
          mem_readaddr_d1      <= 0;
          state_waitrequest_d1 <= 0;
      end else begin
          in_ready_d1          <= in_ready;
          state_d1             <= state;
          mem_readaddr_d1      <= mem_readaddr;
          state_waitrequest_d1 <= state_waitrequest;
      end
   end
   
   always @(posedge clk or negedge reset_n) begin
      if (!reset_n) begin
          state_register <= 0;
          sop_register   <= 0;
          data0_register <= 0;
          data1_register <= 0;
          data2_register <= 0;
          data3_register <= 0;
          data4_register <= 0;
          data5_register <= 0;
          data6_register <= 0;
      end else begin
          state_register <= new_state;
          if (sop_mem_writeenable)
              sop_register <= sop_mem_writedata;
          if (mem_write0)
              data0_register <= mem_writedata0;
          if (mem_write1)
              data1_register <= mem_writedata1;
          if (mem_write2)
              data2_register <= mem_writedata2;
          if (mem_write3)
              data3_register <= mem_writedata3;
          if (mem_write4)
              data4_register <= mem_writedata4;
          if (mem_write5)
              data5_register <= mem_writedata5;
          if (mem_write6)
              data6_register <= mem_writedata6;
      end
   end
   
    assign state_from_memory = state_register;
    assign b_startofpacket_wire = sop_register;
    assign mem_readdata0 = data0_register;
    assign mem_readdata1 = data1_register;
    assign mem_readdata2 = data2_register;
    assign mem_readdata3 = data3_register;
    assign mem_readdata4 = data4_register;
    assign mem_readdata5 = data5_register;
    assign mem_readdata6 = data6_register;
    
    // ---------------------------------------------------------------------
    //| State Machine
    // ---------------------------------------------------------------------
    always @* begin
        b_ready              = (out_ready || ~out_valid);
        a_ready              = 0;
        b_data               = 0;
        b_valid              = 0;
        b_channel            = a_channel;
        b_error              = a_error;
        state                = state_from_memory;
        new_state            = state;
        mem_write0           = 0;
        mem_writedata0       = a_data0;
        mem_write1           = 0;
        mem_writedata1       = a_data0;
        mem_write2           = 0;
        mem_writedata2       = a_data0;
        mem_write3           = 0;
        mem_writedata3       = a_data0;
        mem_write4           = 0;
        mem_writedata4       = a_data0;
        mem_write5           = 0;
        mem_writedata5       = a_data0;
        mem_write6           = 0;
        mem_writedata6       = a_data0;
        sop_mem_writeenable  = 0;
        b_endofpacket        = a_endofpacket;
        b_startofpacket      = 0;
        b_endofpacket        = 0;
        b_empty              = 0;
        
        case (state) 
            0 : begin
                b_data[0:0] = a_data0;
                b_startofpacket = a_startofpacket;
                if (out_ready || ~out_valid) begin
                    if (a_valid) begin
                        b_valid = 1;
                        new_state = state+1'b1;
                        if (a_endofpacket && (a_empty >= 7) ) begin
                            new_state      = 0;
                            b_empty        = a_empty - 7;
                            b_endofpacket  = 1;
                            a_ready        = 1;
                        end
                    end
                end
            end
            1 : begin
                b_data[0:0] = a_data1;
                b_startofpacket = 0;
                if (out_ready || ~out_valid) begin
                    if (a_valid) begin
                        b_valid = 1;
                        new_state = state+1'b1;
                        if (a_endofpacket && (a_empty >= 6) ) begin
                            new_state      = 0;
                            b_empty        = a_empty - 6;
                            b_endofpacket  = 1;
                            a_ready        = 1;
                        end
                    end
                end
            end
            2 : begin
                b_data[0:0] = a_data2;
                b_startofpacket = 0;
                if (out_ready || ~out_valid) begin
                    if (a_valid) begin
                        b_valid = 1;
                        new_state = state+1'b1;
                        if (a_endofpacket && (a_empty >= 5) ) begin
                            new_state      = 0;
                            b_empty        = a_empty - 5;
                            b_endofpacket  = 1;
                            a_ready        = 1;
                        end
                  end
               end
            end
            3 : begin
                b_data[0:0] = a_data3;
                b_startofpacket = 0;
                if (out_ready || ~out_valid) begin
                    if (a_valid) begin
                        b_valid = 1;
                        new_state = state+1'b1;
                        if (a_endofpacket && (a_empty >= 4) ) begin
                            new_state      = 0;
                            b_empty        = a_empty - 4;
                            b_endofpacket  = 1;
                            a_ready        = 1;
                        end
                    end
                end
            end
            4 : begin
                b_data[0:0]      = a_data4;
                b_startofpacket  = 0;
                if (out_ready || ~out_valid) begin
                    if (a_valid) begin
                        b_valid = 1;
                        new_state = state+1'b1;
                        if (a_endofpacket && (a_empty >= 3) ) begin
                            new_state      = 0;
                            b_empty        = a_empty - 3;
                            b_endofpacket  = 1;
                            a_ready        = 1;
                        end
                    end
                end
            end
            5 : begin
                b_data[0:0]      = a_data5;
                b_startofpacket  = 0;
                if (out_ready || ~out_valid) begin
                    if (a_valid) begin
                        b_valid = 1;
                        new_state = state+1'b1;
                        if (a_endofpacket && (a_empty >= 2) ) begin
                            new_state      = 0;
                            b_empty        = a_empty - 2;
                            b_endofpacket  = 1;
                            a_ready        = 1;
                        end
                    end
                end
            end
            6 : begin
                b_data[0:0]      = a_data6;
                b_startofpacket  = 0;
                if (out_ready || ~out_valid) begin
                    if (a_valid) begin
                        b_valid    = 1;
                        new_state  = state+1'b1;
                        if (a_endofpacket && (a_empty >= 1) ) begin
                            new_state      = 0;
                            b_empty        = a_empty - 1'b1;
                            b_endofpacket  = 1;
                            a_ready        = 1;
                        end
                    end
                end
            end
            7 : begin
                b_data[0:0]      = a_data7;
                b_startofpacket  = 0;
                if (out_ready || ~out_valid) begin
                    a_ready = 1;
                    if (a_valid) begin
                        b_valid    = 1;
                        new_state  = 0;
                        if (a_endofpacket && (a_empty >= 0) ) begin
                         new_state      = 0;
                         b_empty        = a_empty - 0;
                         b_endofpacket  = 1;
                         a_ready        = 1;
                     end
                  end
               end
            end
        endcase

        in_ready      = (a_ready || ~a_valid);
        mem_readaddr  = in_channel;
        if (~in_ready)
            mem_readaddr   = mem_readaddr_d1;
             
        sop_mem_writedata  = 0;
        if (a_valid)
            sop_mem_writedata  = a_startofpacket;
        if (b_ready && b_valid && b_startofpacket)
            sop_mem_writeenable  = 1;
    end

   // ---------------------------------------------------------------------
   //| Output Register Stage
   // ---------------------------------------------------------------------
   always @(posedge clk or negedge reset_n) begin
      if (!reset_n) begin
          out_valid         <= 0;
          out_data[0]       <= 0;
          out_channel       <= 0;
          out_startofpacket <= 0;
          out_endofpacket   <= 0;
          out_empty         <= 0;
          out_error         <= 0;
      end else begin
         if (out_ready || ~out_valid) begin
             out_valid         <= b_valid;
             out_data[0]       <= b_data;
             out_channel       <= b_channel; 
             out_startofpacket <= b_startofpacket;
             out_endofpacket   <= b_endofpacket;
             out_empty         <= b_empty;
             out_error         <= b_error;
         end
      end 
   end 
endmodule

module adapter_8_2 (
                    // Interface: in
                    output reg           in_ready,
                    input                in_valid,
                    input [8-1 : 0]      in_data,
                    input [12-1 : 0]     in_channel,
                    input                in_startofpacket,
                    input                in_endofpacket,
                    // Interface: out
                    input                out_ready,
                    output reg           out_valid,
                    output reg [3: 0]    out_data,
                    output reg [12-1: 0] out_channel,
                    output reg           out_startofpacket,
                    output reg           out_endofpacket,

                    // Interface: clk
                    input                clk,
                    // Interface: reset
                    input                reset_n

);


    assign out_data[2]  = 1'b1;
    assign out_data[3]  = 1'b1;
    // ---------------------------------------------------------------------
   //| Signal Declarations
   // ---------------------------------------------------------------------
    reg [12-1:0]                         state_read_addr;
    wire [3-1:0]                         state_from_memory;
    reg [3-1:0]                          state;
    reg [3-1:0]                          new_state;
    reg [3-1:0]                          state_d1;
    
    reg                                  in_ready_d1;
    reg [12-1:0]                         mem_readaddr; 
    reg [12-1:0]                         mem_readaddr_d1;
    reg                                  a_ready;
    reg                                  a_valid;
    reg [12-1:0]                         a_channel;
    reg [1-1:0]                          a_data0; 
    reg [1-1:0]                          a_data1; 
    reg [1-1:0]                          a_data2; 
    reg [1-1:0]                          a_data3; 
    reg [1-1:0]                          a_data4; 
    reg [1-1:0]                          a_data5; 
    reg [1-1:0]                          a_data6; 
    reg [1-1:0]                          a_data7; 
    reg                                  a_startofpacket;
    reg                                  a_endofpacket;
    reg                                  a_empty;
    reg                                  a_error;
    reg                                  b_ready;
    reg                                  b_valid;
    reg [12-1:0]                         b_channel;
    reg [2-1:0]                          b_data;
    reg                                  b_startofpacket; 
    wire                                 b_startofpacket_wire; 
    reg                                  b_endofpacket; 
    reg                                  b_empty;   
    reg                                  b_error; 
    reg                                  mem_write0;
    reg [1-1:0]                          mem_writedata0;
    wire [1-1:0]                         mem_readdata0;
    wire                                 mem_waitrequest0;
    reg [1-1:0]                          mem0[0:0];
    reg                                  mem_write1;
    reg [1-1:0]                          mem_writedata1;
    wire [1-1:0]                         mem_readdata1;
    wire                                 mem_waitrequest1;
    reg [1-1:0]                          mem1[0:0];
    reg                                  mem_write2;
    reg [1-1:0]                          mem_writedata2;
    wire [1-1:0]                         mem_readdata2;
    wire                                 mem_waitrequest2;
    reg [1-1:0]                          mem2[0:0];
    reg                                  mem_write3;
    reg [1-1:0]                          mem_writedata3;
    wire [1-1:0]                         mem_readdata3;
    wire                                 mem_waitrequest3;
    reg [1-1:0]                          mem3[0:0];
    reg                                  mem_write4;
    reg [1-1:0]                          mem_writedata4;
    wire [1-1:0]                         mem_readdata4;
    wire                                 mem_waitrequest4;
    reg [1-1:0]                          mem4[0:0];
    reg                                  mem_write5;
    reg [1-1:0]                          mem_writedata5;
    wire [1-1:0]                         mem_readdata5;
    wire                                 mem_waitrequest5;
    reg [1-1:0]                          mem5[0:0];
    reg                                  mem_write6;
    reg [1-1:0]                          mem_writedata6;
    wire [1-1:0]                         mem_readdata6;
    wire                                 mem_waitrequest6;
    reg [1-1:0]                          mem6[0:0];
    reg                                  sop_mem_writeenable;
    reg                                  sop_mem_writedata;
    wire                                 mem_waitrequest_sop; 
    
    wire                                 state_waitrequest;
    reg                                  state_waitrequest_d1; 
    
    reg [8-1:0]                          in_empty = 0;
    reg [2-1:0]                          out_empty;
    
    reg                                  in_error = 0;
    reg                                  out_error; 
        
    reg [3-1:0]                          state_register;
    reg                                  sop_register; 
    reg                                  error_register;
    reg [1-1:0]                          data0_register;
    reg [1-1:0]                          data1_register;
    reg [1-1:0]                          data2_register;
    reg [1-1:0]                          data3_register;
    reg [1-1:0]                          data4_register;
    reg [1-1:0]                          data5_register;
    reg [1-1:0]                          data6_register;
    
   // ---------------------------------------------------------------------
   //| Input Register Stage
   // ---------------------------------------------------------------------
   always @(posedge clk or negedge reset_n) begin
      if (!reset_n) begin
          a_valid         <= 0;
          a_channel       <= 0;
          a_data0         <= 0;
          a_data1         <= 0;
          a_data2         <= 0;
          a_data3         <= 0;
          a_data4         <= 0;
          a_data5         <= 0;
          a_data6         <= 0;
          a_data7         <= 0;
          a_startofpacket <= 0;
          a_endofpacket   <= 0;
          a_empty         <= 0; 
          a_error         <= 0;
      end else begin
         if (in_ready) begin
             a_valid         <= in_valid;
             a_channel       <= in_channel;
             a_error         <= in_error;
             a_data0         <= in_data[7:7];
             a_data1         <= in_data[6:6];
             a_data2         <= in_data[5:5];
             a_data3         <= in_data[4:4];
             a_data4         <= in_data[3:3];
             a_data5         <= in_data[2:2];
             a_data6         <= in_data[1:1];
             a_data7         <= in_data[0:0];
             a_startofpacket <= in_startofpacket;
             a_endofpacket   <= in_endofpacket;
             a_empty         <= 0;
             if (in_endofpacket)
                 a_empty <= in_empty;
         end
      end 
   end
    
    always @* begin 
        state_read_addr  = a_channel;
        if (in_ready)
            state_read_addr  = in_channel;
    end
   

   // ---------------------------------------------------------------------
   //| State & Memory Keepers
   // ---------------------------------------------------------------------
   always @(posedge clk or negedge reset_n) begin
      if (!reset_n) begin
          in_ready_d1          <= 0;
          state_d1             <= 0;
          mem_readaddr_d1      <= 0;
          state_waitrequest_d1 <= 0;
      end else begin
          in_ready_d1          <= in_ready;
          state_d1             <= state;
          mem_readaddr_d1      <= mem_readaddr;
          state_waitrequest_d1 <= state_waitrequest;
      end
   end
   
   always @(posedge clk or negedge reset_n) begin
      if (!reset_n) begin
          state_register <= 0;
          sop_register   <= 0;
          data0_register <= 0;
          data1_register <= 0;
          data2_register <= 0;
          data3_register <= 0;
          data4_register <= 0;
          data5_register <= 0;
          data6_register <= 0;
      end else begin
          state_register <= new_state;
          if (sop_mem_writeenable)
              sop_register <= sop_mem_writedata;
          if (mem_write0)
              data0_register <= mem_writedata0;
          if (mem_write1)
              data1_register <= mem_writedata1;
          if (mem_write2)
              data2_register <= mem_writedata2;
          if (mem_write3)
              data3_register <= mem_writedata3;
          if (mem_write4)
              data4_register <= mem_writedata4;
          if (mem_write5)
              data5_register <= mem_writedata5;
          if (mem_write6)
              data6_register <= mem_writedata6;
      end
   end
   
    assign state_from_memory = state_register;
    assign b_startofpacket_wire = sop_register;
    assign mem_readdata0 = data0_register;
    assign mem_readdata1 = data1_register;
    assign mem_readdata2 = data2_register;
    assign mem_readdata3 = data3_register;
    assign mem_readdata4 = data4_register;
    assign mem_readdata5 = data5_register;
    assign mem_readdata6 = data6_register;
    
    // ---------------------------------------------------------------------
    //| State Machine
    // ---------------------------------------------------------------------
    always @* begin
        b_ready              = (out_ready || ~out_valid);
        a_ready              = 0;
        b_data               = 0;
        b_valid              = 0;
        b_channel            = a_channel;
        b_error              = a_error;
        state                = state_from_memory;
        new_state            = state;
        mem_write0           = 0;
        mem_writedata0       = a_data0;
        mem_write1           = 0;
        mem_writedata1       = a_data0;
        mem_write2           = 0;
        mem_writedata2       = a_data0;
        mem_write3           = 0;
        mem_writedata3       = a_data0;
        mem_write4           = 0;
        mem_writedata4       = a_data0;
        mem_write5           = 0;
        mem_writedata5       = a_data0;
        mem_write6           = 0;
        mem_writedata6       = a_data0;
        sop_mem_writeenable  = 0;
        b_endofpacket        = a_endofpacket;
        b_startofpacket      = 0;
        b_endofpacket        = 0;
        b_empty              = 0;
        case (state) 
            0 : begin
                b_data[1:1]      = a_data0;
                b_data[0:0]      = a_data1;
                b_startofpacket  = a_startofpacket;
                if (out_ready || ~out_valid) begin
                    if (a_valid) begin
                        b_valid    = 1;
                        new_state  = state+1'b1;
                        if (a_endofpacket && (a_empty >= 6) ) begin
                            new_state      = 0;
                            b_empty        = a_empty - 6;
                            b_endofpacket  = 1;
                            a_ready        = 1;
                        end
                    end
                end
            end
            1 : begin
                b_data[1:1]      = a_data2;
                b_data[0:0]      = a_data3;
                b_startofpacket  = 0;
                if (out_ready || ~out_valid) begin
                    if (a_valid) begin
                        b_valid    = 1;
                        new_state  = state+1'b1;
                        if (a_endofpacket && (a_empty >= 4) ) begin
                            new_state      = 0;
                            b_empty        = a_empty - 4;
                            b_endofpacket  = 1;
                            a_ready        = 1;
                        end
                    end
                end
            end
            2 : begin
                b_data[1:1]      = a_data4;
                b_data[0:0]      = a_data5;
                b_startofpacket  = 0;
                if (out_ready || ~out_valid) begin
                    if (a_valid) begin
                        b_valid    = 1;
                        new_state  = state+1'b1;
                        if (a_endofpacket && (a_empty >= 2) ) begin
                            new_state      = 0;
                            b_empty        = a_empty - 2;
                            b_endofpacket  = 1;
                            a_ready        = 1;
                        end
                    end
                end
            end
            3 : begin
                b_data[1:1]      = a_data6;
                b_data[0:0]      = a_data7;
                b_startofpacket  = 0;
                if (out_ready || ~out_valid) begin
                    a_ready = 1;
                    if (a_valid) begin
                        b_valid    = 1;
                        new_state  = state+1'b1;
                        if (a_endofpacket && (a_empty >= 0) ) begin
                            new_state      = 0;
                            b_empty        = a_empty - 0;
                            b_endofpacket  = 1;
                            a_ready        = 1;
                        end
                    end
                end
            end
            4 : begin
                b_data[1:1]      = a_data0;
                b_data[0:0]      = a_data1;
                b_startofpacket  = 0;
                if (out_ready || ~out_valid) begin
                    if (a_valid) begin
                        b_valid    = 1;
                        new_state  = state+1'b1;
                        if (a_endofpacket && (a_empty >= 6) ) begin
                            new_state      = 0;
                            b_empty        = a_empty - 6;
                            b_endofpacket  = 1;
                            a_ready        = 1;
                        end
                    end
                end
            end
            5 : begin
                b_data[1:1]      = a_data2;
                b_data[0:0]      = a_data3;
                b_startofpacket  = 0;
                if (out_ready || ~out_valid) begin
                    if (a_valid) begin
                        b_valid    = 1;
                        new_state  = state+1'b1;
                        if (a_endofpacket && (a_empty >= 4) ) begin
                            new_state      = 0;
                            b_empty        = a_empty - 4;
                            b_endofpacket  = 1;
                            a_ready        = 1;
                        end
                    end
                end
            end
            6 : begin
                b_data[1:1]      = a_data4;
                b_data[0:0]      = a_data5;
                b_startofpacket  = 0;
                if (out_ready || ~out_valid) begin
                    if (a_valid) begin
                        b_valid = 1;
                        new_state = state+1'b1;
                        if (a_endofpacket && (a_empty >= 2) ) begin
                            new_state      = 0;
                            b_empty        = a_empty - 2;
                            b_endofpacket  = 1;
                            a_ready        = 1;
                        end
                    end
                end
            end
         7 : begin
             b_data[1:1]      = a_data6;
             b_data[0:0]      = a_data7;
             b_startofpacket  = 0;
             if (out_ready || ~out_valid) begin
                 a_ready = 1;
                 if (a_valid) begin
                     b_valid    = 1;
                     new_state  = 0;
                     if (a_endofpacket && (a_empty >= 0) ) begin
                         new_state      = 0;
                         b_empty        = a_empty - 0;
                         b_endofpacket  = 1;
                         a_ready        = 1;
                     end
                 end
             end
         end
        endcase
        in_ready      = (a_ready || ~a_valid);
        mem_readaddr  = in_channel;
        if (~in_ready)
            mem_readaddr   = mem_readaddr_d1;
        sop_mem_writedata  = 0;
        if (a_valid)
            sop_mem_writedata = a_startofpacket;
        if (b_ready && b_valid && b_startofpacket)
            sop_mem_writeenable  = 1;
    end

    // ---------------------------------------------------------------------
    //| Output Register Stage
    // ---------------------------------------------------------------------
    always @(posedge clk or negedge reset_n) begin
        if (!reset_n) begin
            out_valid         <= 0;
            out_data[1:0]     <= 0;
            out_channel       <= 0;
            out_startofpacket <= 0;
            out_endofpacket   <= 0;
            out_empty         <= 0;
            out_error         <= 0;
        end else begin
            if (out_ready || ~out_valid) begin
                out_valid         <= b_valid;
                out_data[1:0]     <= b_data;
                out_channel       <= b_channel; 
                out_startofpacket <= b_startofpacket;
                out_endofpacket   <= b_endofpacket;
                out_empty         <= b_empty;
                out_error         <= b_error;
            end
        end 
    end 
endmodule

module adapter_8_4 (
                    // Interface: in
                    output reg           in_ready,
                    input                in_valid,
                    input [8-1 : 0]      in_data,
                    input [12-1 : 0]     in_channel,
                    input                in_startofpacket,
                    input                in_endofpacket,
                    // Interface: out
                    input                out_ready,
                    output reg           out_valid,
                    output reg [4-1: 0]  out_data,
                    output reg [12-1: 0] out_channel,
                    output reg           out_startofpacket,
                    output reg           out_endofpacket,

                    // Interface: clk
                    input                clk,
                    // Interface: reset
                    input                reset_n

);

    // ---------------------------------------------------------------------
    //| Signal Declarations
    // ---------------------------------------------------------------------
    reg [12-1:0]                         state_read_addr;
    wire [3-1:0]                         state_from_memory;
    reg [3-1:0]                          state;
    reg [3-1:0]                          new_state;
    reg [3-1:0]                          state_d1;
    
    reg                                  in_ready_d1;
    reg [12-1:0]                         mem_readaddr; 
    reg [12-1:0]                         mem_readaddr_d1;
    reg                                  a_ready;
    reg                                  a_valid;
    reg [12-1:0]                         a_channel;
    reg [1-1:0]                          a_data0; 
    reg [1-1:0]                          a_data1; 
    reg [1-1:0]                          a_data2; 
    reg [1-1:0]                          a_data3; 
    reg [1-1:0]                          a_data4; 
    reg [1-1:0]                          a_data5; 
    reg [1-1:0]                          a_data6; 
    reg [1-1:0]                          a_data7; 
    reg                                  a_startofpacket;
    reg                                  a_endofpacket;
    reg                                  a_empty;
    reg                                  a_error;
    reg                                  b_ready;
    reg                                  b_valid;
    reg [12-1:0]                         b_channel;
    reg [4-1:0]                          b_data;
    reg                                  b_startofpacket; 
    wire                                 b_startofpacket_wire; 
    reg                                  b_endofpacket; 
    reg                                  b_empty;   
    reg                                  b_error; 
    reg                                  mem_write0;
    reg [1-1:0]                          mem_writedata0;
    wire [1-1:0]                         mem_readdata0;
    wire                                 mem_waitrequest0;
    reg [1-1:0]                          mem0[0:0];
    reg                                  mem_write1;
    reg [1-1:0]                          mem_writedata1;
    wire [1-1:0]                         mem_readdata1;
    wire                                 mem_waitrequest1;
    reg [1-1:0]                          mem1[0:0];
    reg                                  mem_write2;
    reg [1-1:0]                          mem_writedata2;
    wire [1-1:0]                         mem_readdata2;
    wire                                 mem_waitrequest2;
    reg [1-1:0]                          mem2[0:0];
    reg                                  mem_write3;
    reg [1-1:0]                          mem_writedata3;
    wire [1-1:0]                         mem_readdata3;
    wire                                 mem_waitrequest3;
    reg [1-1:0]                          mem3[0:0];
    reg                                  mem_write4;
    reg [1-1:0]                          mem_writedata4;
    wire [1-1:0]                         mem_readdata4;
    wire                                 mem_waitrequest4;
    reg [1-1:0]                          mem4[0:0];
    reg                                  mem_write5;
    reg [1-1:0]                          mem_writedata5;
    wire [1-1:0]                         mem_readdata5;
    wire                                 mem_waitrequest5;
    reg [1-1:0]                          mem5[0:0];
    reg                                  mem_write6;
    reg [1-1:0]                          mem_writedata6;
    wire [1-1:0]                         mem_readdata6;
    wire                                 mem_waitrequest6;
    reg [1-1:0]                          mem6[0:0];
    reg                                  sop_mem_writeenable;
    reg                                  sop_mem_writedata;
    wire                                 mem_waitrequest_sop; 
    
    wire                                 state_waitrequest;
    reg                                  state_waitrequest_d1; 
    
    reg [8-1:0]                          in_empty = 0;
    reg [4-1:0]                          out_empty;
    
    reg                                  in_error = 0;
    reg                                  out_error; 
    
    
    reg [3-1:0]                          state_register;
    reg                                  sop_register; 
    reg                                  error_register;
    reg [1-1:0]                          data0_register;
    reg [1-1:0]                          data1_register;
    reg [1-1:0]                          data2_register;
    reg [1-1:0]                          data3_register;
    reg [1-1:0]                          data4_register;
    reg [1-1:0]                          data5_register;
    reg [1-1:0]                          data6_register;
    
    // ---------------------------------------------------------------------
    //| Input Register Stage
    // ---------------------------------------------------------------------
    always @(posedge clk or negedge reset_n) begin
        if (!reset_n) begin
            a_valid         <= 0;
            a_channel       <= 0;
            a_data0         <= 0;
            a_data1         <= 0;
            a_data2         <= 0;
            a_data3         <= 0;
            a_data4         <= 0;
            a_data5         <= 0;
            a_data6         <= 0;
            a_data7         <= 0;
            a_startofpacket <= 0;
            a_endofpacket   <= 0;
            a_empty         <= 0; 
            a_error         <= 0;
        end else begin
            if (in_ready) begin
                a_valid         <= in_valid;
                a_channel       <= in_channel;
                a_error         <= in_error;
                a_data0         <= in_data[7:7];
                a_data1         <= in_data[6:6];
                a_data2         <= in_data[5:5];
                a_data3         <= in_data[4:4];
                a_data4         <= in_data[3:3];
                a_data5         <= in_data[2:2];
                a_data6         <= in_data[1:1];
                a_data7         <= in_data[0:0];
                a_startofpacket <= in_startofpacket;
                a_endofpacket   <= in_endofpacket;
                a_empty         <= 0;
                if (in_endofpacket)
                    a_empty <= in_empty;
            end
        end 
    end
    
    always @* begin 
        state_read_addr  = a_channel;
        if (in_ready)
            state_read_addr  = in_channel;
    end
   

    // ---------------------------------------------------------------------
    //| State & Memory Keepers
    // ---------------------------------------------------------------------
    always @(posedge clk or negedge reset_n) begin
        if (!reset_n) begin
            in_ready_d1          <= 0;
            state_d1             <= 0;
            mem_readaddr_d1      <= 0;
            state_waitrequest_d1 <= 0;
        end else begin
            in_ready_d1          <= in_ready;
            state_d1             <= state;
            mem_readaddr_d1      <= mem_readaddr;
            state_waitrequest_d1 <= state_waitrequest;
        end
    end
    
    always @(posedge clk or negedge reset_n) begin
        if (!reset_n) begin
            state_register <= 0;
            sop_register   <= 0;
            data0_register <= 0;
            data1_register <= 0;
            data2_register <= 0;
            data3_register <= 0;
            data4_register <= 0;
            data5_register <= 0;
            data6_register <= 0;
        end else begin
            state_register <= new_state;
            if (sop_mem_writeenable)
                sop_register <= sop_mem_writedata;
            if (mem_write0)
                data0_register <= mem_writedata0;
            if (mem_write1)
                data1_register <= mem_writedata1;
            if (mem_write2)
                data2_register <= mem_writedata2;
            if (mem_write3)
                data3_register <= mem_writedata3;
            if (mem_write4)
                data4_register <= mem_writedata4;
            if (mem_write5)
                data5_register <= mem_writedata5;
            if (mem_write6)
                data6_register <= mem_writedata6;
        end
    end
    
    assign state_from_memory = state_register;
    assign b_startofpacket_wire = sop_register;
    assign mem_readdata0 = data0_register;
    assign mem_readdata1 = data1_register;
    assign mem_readdata2 = data2_register;
    assign mem_readdata3 = data3_register;
    assign mem_readdata4 = data4_register;
    assign mem_readdata5 = data5_register;
    assign mem_readdata6 = data6_register;
   
    // ---------------------------------------------------------------------
    //| State Machine
    // ---------------------------------------------------------------------
    always @* begin
        b_ready              = (out_ready || ~out_valid);
        a_ready              = 0;
        b_data               = 0;
        b_valid              = 0;
        b_channel            = a_channel;
        b_error              = a_error;
      
        state                = state_from_memory;
      
        new_state            = state;
        mem_write0           = 0;
        mem_writedata0       = a_data0;
        mem_write1           = 0;
        mem_writedata1       = a_data0;
        mem_write2           = 0;
        mem_writedata2       = a_data0;
        mem_write3           = 0;
        mem_writedata3       = a_data0;
        mem_write4           = 0;
        mem_writedata4       = a_data0;
        mem_write5           = 0;
        mem_writedata5       = a_data0;
        mem_write6           = 0;
        mem_writedata6       = a_data0;
        sop_mem_writeenable  = 0;

        b_endofpacket        = a_endofpacket;
      
        b_startofpacket      = 0;
      
        b_endofpacket        = 0;
        b_empty              = 0;
        
   case (state) 
       0 : begin
           b_data[3:3]      = a_data0;
           b_data[2:2]      = a_data1;
           b_data[1:1]      = a_data2;
           b_data[0:0]      = a_data3;
           b_startofpacket  = a_startofpacket;
           if (out_ready || ~out_valid) begin
               if (a_valid) begin
                   b_valid    = 1;
                   new_state  = state+1'b1;
                   if (a_endofpacket && (a_empty >= 4) ) begin
                       new_state      = 0;
                       b_empty        = a_empty - 4;
                       b_endofpacket  = 1;
                       a_ready        = 1;
                   end
               end
           end
       end
       1 : begin
           b_data[3:3]      = a_data4;
           b_data[2:2]      = a_data5;
           b_data[1:1]      = a_data6;
           b_data[0:0]      = a_data7;
           b_startofpacket  = 0;
           if (out_ready || ~out_valid) begin
               a_ready = 1;
               if (a_valid) begin
                   b_valid    = 1;
                   new_state  = state+1'b1;
                   if (a_endofpacket && (a_empty >= 0) ) begin
                       new_state      = 0;
                       b_empty        = a_empty - 0;
                       b_endofpacket  = 1;
                       a_ready        = 1;
                   end
               end
           end
       end
       2 : begin
           b_data[3:3]      = a_data0;
           b_data[2:2]      = a_data1;
           b_data[1:1]      = a_data2;
           b_data[0:0]      = a_data3;
           b_startofpacket  = 0;
           if (out_ready || ~out_valid) begin
               if (a_valid) begin
                   b_valid    = 1;
                   new_state  = state+1'b1;
                   if (a_endofpacket && (a_empty >= 4) ) begin
                       new_state      = 0;
                       b_empty        = a_empty - 4;
                       b_endofpacket  = 1;
                       a_ready        = 1;
                   end
               end
           end
       end
       3 : begin
           b_data[3:3]      = a_data4;
           b_data[2:2]      = a_data5;
           b_data[1:1]      = a_data6;
           b_data[0:0]      = a_data7;
           b_startofpacket  = 0;
           if (out_ready || ~out_valid) begin
               a_ready = 1;
               if (a_valid) begin
                   b_valid    = 1;
                   new_state  = state+1'b1;
                   if (a_endofpacket && (a_empty >= 0) ) begin
                       new_state      = 0;
                       b_empty        = a_empty - 0;
                       b_endofpacket  = 1;
                       a_ready        = 1;
                   end
               end
           end
       end
       4 : begin
           b_data[3:3]      = a_data0;
           b_data[2:2]      = a_data1;
           b_data[1:1]      = a_data2;
           b_data[0:0]      = a_data3;
           b_startofpacket  = 0;
           if (out_ready || ~out_valid) begin
               if (a_valid) begin
                   b_valid    = 1;
                   new_state  = state+1'b1;
                   if (a_endofpacket && (a_empty >= 4) ) begin
                       new_state      = 0;
                       b_empty        = a_empty - 4;
                       b_endofpacket  = 1;
                       a_ready        = 1;
                   end
               end
           end
       end
       5 : begin
           b_data[3:3]      = a_data4;
           b_data[2:2]      = a_data5;
           b_data[1:1]      = a_data6;
           b_data[0:0]      = a_data7;
           b_startofpacket  = 0;
           if (out_ready || ~out_valid) begin
               a_ready = 1;
               if (a_valid) begin
                   b_valid    = 1;
                   new_state  = state+1'b1;
                   if (a_endofpacket && (a_empty >= 0) ) begin
                       new_state      = 0;
                       b_empty        = a_empty - 0;
                       b_endofpacket  = 1;
                       a_ready        = 1;
                   end
               end
           end
       end
       6 : begin
           b_data[3:3]      = a_data0;
           b_data[2:2]      = a_data1;
           b_data[1:1]      = a_data2;
           b_data[0:0]      = a_data3;
           b_startofpacket  = 0;
           if (out_ready || ~out_valid) begin
               if (a_valid) begin
                   b_valid    = 1;
                   new_state  = state+1'b1;
                   if (a_endofpacket && (a_empty >= 4) ) begin
                       new_state      = 0;
                       b_empty        = a_empty - 4;
                       b_endofpacket  = 1;
                       a_ready        = 1;
                   end
               end
           end
       end
       7 : begin
           b_data[3:3]      = a_data4;
           b_data[2:2]      = a_data5;
           b_data[1:1]      = a_data6;
           b_data[0:0]      = a_data7;
           b_startofpacket  = 0;
           if (out_ready || ~out_valid) begin
               a_ready = 1;
               if (a_valid) begin
                   b_valid    = 1;
                   new_state  = 0;
                   if (a_endofpacket && (a_empty >= 0) ) begin
                       new_state      = 0;
                       b_empty        = a_empty - 0;
                       b_endofpacket  = 1;
                       a_ready        = 1;
                   end
               end
           end
       end
       
   endcase
        
        in_ready      = (a_ready || ~a_valid);
        mem_readaddr  = in_channel;
        if (~in_ready)
            mem_readaddr   = mem_readaddr_d1;
        
        sop_mem_writedata  = 0;
        if (a_valid)
            sop_mem_writedata  = a_startofpacket;
        if (b_ready && b_valid && b_startofpacket)
            sop_mem_writeenable = 1;
    end

    // ---------------------------------------------------------------------
    //| Output Register Stage
    // ---------------------------------------------------------------------
    always @(posedge clk or negedge reset_n) begin
        if (!reset_n) begin
            out_valid         <= 0;
            out_data          <= 0;
            out_channel       <= 0;
            out_startofpacket <= 0;
            out_endofpacket   <= 0;
            out_empty         <= 0;
            out_error         <= 0;
        end else begin
            if (out_ready || ~out_valid) begin
                out_valid         <= b_valid;
                out_data          <= b_data;
                out_channel       <= b_channel; 
                out_startofpacket <= b_startofpacket;
                out_endofpacket   <= b_endofpacket;
                out_empty         <= b_empty;
                out_error         <= b_error;
                         end
        end 
    end 
endmodule




module demultiplexer
(
 // -------------------
 // Sink
 // -------------------
 input [1-1 : 0]      sink_valid,
 input [8-1 : 0]      sink_data, // ST_DATA_W=8
 input [3-1 : 0]      sink_channel, // ST_CHANNEL_W=3
 input                sink_startofpacket,
 input                sink_endofpacket,
 output               sink_ready,

    // -------------------
    // Sources 
    // -------------------
 output reg           src0_valid,
 output reg [8-1 : 0] src0_data, // ST_DATA_W=8
 output reg [3-1 : 0] src0_channel, // ST_CHANNEL_W=3
 output reg           src0_startofpacket,
 output reg           src0_endofpacket,
 input                src0_ready,

 output reg           src1_valid,
 output reg [8-1 : 0] src1_data, // ST_DATA_W=8
 output reg [3-1 : 0] src1_channel, // ST_CHANNEL_W=3
 output reg           src1_startofpacket,
 output reg           src1_endofpacket,
 input                src1_ready,

 output reg           src2_valid,
 output reg [8-1 : 0] src2_data, // ST_DATA_W=8
 output reg [3-1 : 0] src2_channel, // ST_CHANNEL_W=3
 output reg           src2_startofpacket,
 output reg           src2_endofpacket,
 input                src2_ready,


    // -------------------
    // Clock & Reset
    // -------------------
    (*altera_attribute = "-name MESSAGE_DISABLE 15610" *) // setting message suppression on clk
 input                clk,
    (*altera_attribute = "-name MESSAGE_DISABLE 15610" *) // setting message suppression on reset
 input                reset

);

    localparam NUM_OUTPUTS = 3;
    wire [NUM_OUTPUTS - 1 : 0] ready_vector;
    
    // -------------------
    // Demux
    // -------------------
    always @* begin
        src0_data           = sink_data;
        src0_startofpacket  = sink_startofpacket;
        src0_endofpacket    = sink_endofpacket;
        src0_channel        = sink_channel >> NUM_OUTPUTS;

        src0_valid          = sink_channel[0] && sink_valid;

        src1_data           = sink_data;
        src1_startofpacket  = sink_startofpacket;
        src1_endofpacket    = sink_endofpacket;
        src1_channel        = sink_channel >> NUM_OUTPUTS;

        src1_valid          = sink_channel[1] && sink_valid;

        src2_data           = sink_data;
        src2_startofpacket  = sink_startofpacket;
        src2_endofpacket    = sink_endofpacket;
        src2_channel        = sink_channel >> NUM_OUTPUTS;

        src2_valid          = sink_channel[2] && sink_valid;
    end

    // -------------------
    // Backpressure
    // -------------------
    assign ready_vector[0] = src0_ready;
    assign ready_vector[1] = src1_ready;
    assign ready_vector[2] = src2_ready;

    assign sink_ready = |(sink_channel & ready_vector);

endmodule

module demultiplexer_7_channel
(
 // -------------------
 // Sink
 // -------------------
 input [1-1 : 0]      sink_valid,
 input [8-1 : 0]      sink_data, // ST_DATA_W=8
 input [7-1 : 0]      sink_channel, // ST_CHANNEL_W=7
 input                sink_startofpacket,
 input                sink_endofpacket,
 output               sink_ready,

    // -------------------
    // Sources 
    // -------------------
 output reg           src0_valid,
 output reg [8-1 : 0] src0_data, // ST_DATA_W=8
 output reg [7-1 : 0] src0_channel, // ST_CHANNEL_W=7
 output reg           src0_startofpacket,
 output reg           src0_endofpacket,
 input                src0_ready,

 output reg           src1_valid,
 output reg [8-1 : 0] src1_data, // ST_DATA_W=8
 output reg [7-1 : 0] src1_channel, // ST_CHANNEL_W=7
 output reg           src1_startofpacket,
 output reg           src1_endofpacket,
 input                src1_ready,

 output reg           src2_valid,
 output reg [8-1 : 0] src2_data, // ST_DATA_W=8
 output reg [7-1 : 0] src2_channel, // ST_CHANNEL_W=7
 output reg           src2_startofpacket,
 output reg           src2_endofpacket,
 input                src2_ready,


    // -------------------
    // Clock & Reset
    // -------------------
    (*altera_attribute = "-name MESSAGE_DISABLE 15610" *) // setting message suppression on clk
 input                clk,
    (*altera_attribute = "-name MESSAGE_DISABLE 15610" *) // setting message suppression on reset
 input                reset

);

    localparam NUM_OUTPUTS = 3;
    wire [NUM_OUTPUTS - 1 : 0] ready_vector;

    // -------------------
    // Demux
    // -------------------
    always @* begin
        src0_data           = sink_data;
        src0_startofpacket  = sink_startofpacket;
        src0_endofpacket    = sink_endofpacket;
        src0_channel        = sink_channel >> NUM_OUTPUTS;

        src0_valid          = sink_channel[0] && sink_valid;

        src1_data           = sink_data;
        src1_startofpacket  = sink_startofpacket;
        src1_endofpacket    = sink_endofpacket;
        src1_channel        = sink_channel >> NUM_OUTPUTS;

        src1_valid          = sink_channel[1] && sink_valid;

        src2_data           = sink_data;
        src2_startofpacket  = sink_startofpacket;
        src2_endofpacket    = sink_endofpacket;
        src2_channel        = sink_channel >> NUM_OUTPUTS;

        src2_valid          = sink_channel[2] && sink_valid;
        
    end

    // -------------------
    // Backpressure
    // -------------------
    assign ready_vector[0] = src0_ready;
    assign ready_vector[1] = src1_ready;
    assign ready_vector[2] = src2_ready;

    assign sink_ready = |(sink_channel & {{4{1'b0}},{ready_vector[NUM_OUTPUTS - 1 : 0]}});

endmodule

module demultiplexer_9_channels
(
 // -------------------
 // Sink
 // -------------------
 input [1-1 : 0]      sink_valid,
 input [8-1 : 0]      sink_data, // ST_DATA_W=8
 input [9-1 : 0]      sink_channel, // ST_CHANNEL_W=9
 input                sink_startofpacket,
 input                sink_endofpacket,
 output               sink_ready,

 // -------------------
 // Sources 
 // -------------------
 output reg           src0_valid,
 output reg [8-1 : 0] src0_data, // ST_DATA_W=8
 output reg [9-1 : 0] src0_channel, // ST_CHANNEL_W=9
 output reg           src0_startofpacket,
 output reg           src0_endofpacket,
 input                src0_ready,

 output reg           src1_valid,
 output reg [8-1 : 0] src1_data, // ST_DATA_W=8
 output reg [9-1 : 0] src1_channel, // ST_CHANNEL_W=9
 output reg           src1_startofpacket,
 output reg           src1_endofpacket,
 input                src1_ready,

 output reg           src2_valid,
 output reg [8-1 : 0] src2_data, // ST_DATA_W=8
 output reg [9-1 : 0] src2_channel, // ST_CHANNEL_W=9
 output reg           src2_startofpacket,
 output reg           src2_endofpacket,
 input                src2_ready,


    // -------------------
    // Clock & Reset
    // -------------------
    (*altera_attribute = "-name MESSAGE_DISABLE 15610" *) // setting message suppression on clk
 input                clk,
    (*altera_attribute = "-name MESSAGE_DISABLE 15610" *) // setting message suppression on reset
 input                reset

);

    localparam NUM_OUTPUTS = 3;
    wire [NUM_OUTPUTS - 1 : 0] ready_vector;

    // -------------------
    // Demux
    // -------------------
    always @* begin
        src0_data          = sink_data;
        src0_startofpacket = sink_startofpacket;
        src0_endofpacket   = sink_endofpacket;
        src0_channel       = sink_channel >> NUM_OUTPUTS;

        src0_valid         = sink_channel[0] && sink_valid;

        src1_data          = sink_data;
        src1_startofpacket = sink_startofpacket;
        src1_endofpacket   = sink_endofpacket;
        src1_channel       = sink_channel >> NUM_OUTPUTS;

        src1_valid         = sink_channel[1] && sink_valid;

        src2_data          = sink_data;
        src2_startofpacket = sink_startofpacket;
        src2_endofpacket   = sink_endofpacket;
        src2_channel       = sink_channel >> NUM_OUTPUTS;

        src2_valid         = sink_channel[2] && sink_valid;

    end

    // -------------------
    // Backpressure
    // -------------------
    assign ready_vector[0] = src0_ready;
    assign ready_vector[1] = src1_ready;
    assign ready_vector[2] = src2_ready;

    assign sink_ready = |(sink_channel & {{6{1'b0}},{ready_vector[NUM_OUTPUTS - 1 : 0]}});

endmodule

module demultiplexer_12_channels
(
 // -------------------
 // Sink
 // -------------------
 input [1-1 : 0]       sink_valid,
 input [8-1 : 0]       sink_data, // ST_DATA_W=8
 input [12-1 : 0]      sink_channel, // ST_CHANNEL_W=12
 input                 sink_startofpacket,
 input                 sink_endofpacket,
 output                sink_ready,

 // -------------------
 // Sources 
 // -------------------
 output reg            src0_valid,
 output reg [8-1 : 0]  src0_data, // ST_DATA_W=8
 output reg [12-1 : 0] src0_channel, // ST_CHANNEL_W=12
 output reg            src0_startofpacket,
 output reg            src0_endofpacket,
 input                 src0_ready,

 output reg            src1_valid,
 output reg [8-1 : 0]  src1_data, // ST_DATA_W=8
 output reg [12-1 : 0] src1_channel, // ST_CHANNEL_W=12
 output reg            src1_startofpacket,
 output reg            src1_endofpacket,
 input                 src1_ready,

 output reg            src2_valid,
 output reg [8-1 : 0]  src2_data, // ST_DATA_W=8
 output reg [12-1 : 0] src2_channel, // ST_CHANNEL_W=12
 output reg            src2_startofpacket,
 output reg            src2_endofpacket,
 input                 src2_ready,


 // -------------------
 // Clock & Reset
 // -------------------
 (*altera_attribute = "-name MESSAGE_DISABLE 15610" *) // setting message suppression on clk
 input                 clk,
 (*altera_attribute = "-name MESSAGE_DISABLE 15610" *) // setting message suppression on reset
 input                 reset
 
);

    localparam NUM_OUTPUTS = 3;
    wire [NUM_OUTPUTS - 1 : 0] ready_vector;

    // -------------------
    // Demux
    // -------------------
    always @* begin
        src0_data           = sink_data;
        src0_startofpacket  = sink_startofpacket;
        src0_endofpacket    = sink_endofpacket;
        src0_channel        = sink_channel >> NUM_OUTPUTS;

        src0_valid          = sink_channel[0] && sink_valid;

        src1_data           = sink_data;
        src1_startofpacket  = sink_startofpacket;
        src1_endofpacket    = sink_endofpacket;
        src1_channel        = sink_channel >> NUM_OUTPUTS;

        src1_valid          = sink_channel[1] && sink_valid;

        src2_data           = sink_data;
        src2_startofpacket  = sink_startofpacket;
        src2_endofpacket    = sink_endofpacket;
        src2_channel        = sink_channel >> NUM_OUTPUTS;

        src2_valid          = sink_channel[2] && sink_valid;
        
    end

    // -------------------
    // Backpressure
    // -------------------
    assign ready_vector[0] = src0_ready;
    assign ready_vector[1] = src1_ready;
    assign ready_vector[2] = src2_ready;

    assign sink_ready = |(sink_channel & {{9{1'b0}},{ready_vector[NUM_OUTPUTS - 1 : 0]}});

endmodule

module clock_devider (
                      // Inputs
                      clk,
                      reset,
                      enable_clk,
                      
                      // Outputs
                      new_clk,

                      rising_edge,
                      falling_edge,
                      
                      middle_of_high_level,
                      middle_of_low_level
);

/*****************************************************************************
 *                           Parameter Declarations                          *
 *****************************************************************************/

parameter COUNTER_BITS  = 10;
parameter COUNTER_INC   = 10'h001;

/*****************************************************************************
 *                             Port Declarations                             *
 *****************************************************************************/

    // Inputs
    input               clk;
    input               reset;
    input               enable_clk;
    
    // Outputs
    output  reg         new_clk;
    output  reg         rising_edge;
    output  reg         falling_edge;
    output  reg         middle_of_high_level;
    output  reg         middle_of_low_level;

/*****************************************************************************
 *                           Constant Declarations                           *
 *****************************************************************************/

/*****************************************************************************
 *                 Internal wires and registers Declarations                 *
 *****************************************************************************/

// Internal Wires

// Internal Registers
    reg [COUNTER_BITS:1] clk_counter;

// State Machine Registers

/*****************************************************************************
 *                         Finite State Machine(s)                           *
 *****************************************************************************/


/*****************************************************************************
 *                             Sequential logic                              *
 *****************************************************************************/

    always @(posedge clk) begin
        if (reset)
            clk_counter <= 'h0;
        else if (enable_clk)
            clk_counter <= clk_counter + 'h1;
    end
    
    always @(posedge clk) begin
        if (reset)
            new_clk <= 1'b0;
        else
            new_clk <= clk_counter[COUNTER_BITS];
    end

    always @(posedge clk) begin
        if (reset)
            rising_edge <= 1'b0;
        else
            rising_edge <= (clk_counter[COUNTER_BITS] ^ new_clk) & ~new_clk;
    end

    always @(posedge clk) begin
        if (reset)
            falling_edge <= 1'b0;
        else
            falling_edge <= (clk_counter[COUNTER_BITS] ^ new_clk) & new_clk;
    end

/*
always @(posedge clk)
begin
    if (reset)
        middle_of_high_level <= 1'b0;
    else
        middle_of_high_level <= 
            clk_counter[COUNTER_BITS] & 
            ~clk_counter[(COUNTER_BITS - 1)] &
            (&(clk_counter[(COUNTER_BITS - 2):1]));
end

always @(posedge clk)
begin
    if (reset)
        middle_of_low_level <= 1'b0;
    else
        middle_of_low_level <= 
            ~clk_counter[COUNTER_BITS] & 
            ~clk_counter[(COUNTER_BITS - 1)] &
            (&(clk_counter[(COUNTER_BITS - 2):1]));
end
*/


/*****************************************************************************
 *                            Combinational logic                            *
 *****************************************************************************/

// Output Assignments

// Internal Assignments

/*****************************************************************************
 *                              Internal Modules                             *
 *****************************************************************************/

endmodule

module clk_div 
#( 
    parameter WIDTH = 5, // Width of the register required
    parameter N     = 6  // We will divide by 12 for example in this case
)
(
    input        clk,
    input        reset,
    input [4:0]  divisor,
    input        pol,
    input        enable,
    output logic clk_out,
    output logic falling_edge_trigger,
    output logic rising_edge,
    output logic falling_edge
); 
    logic [WIDTH-1:0] cnt_reg;
    logic [WIDTH-1:0] cnt_nxt;
    logic             clk_track;
    logic             cnt_equal;
    logic             clk_track_d;
    logic             enable_d;
    assign cnt_equal  = (cnt_nxt == divisor);
    
    always @(posedge clk or posedge reset)  begin
        if (reset) begin
            cnt_reg   <= '0;
            clk_track <= pol;
        end
        else if (enable) begin
            if (cnt_equal) begin
                cnt_reg   <= '0;
                clk_track <= ~clk_track;
            end
            else 
                cnt_reg <= cnt_nxt;
        end
        else begin
            //cnt_reg   <= '0;
            cnt_reg   <= divisor - 5'h1;
            clk_track <= pol;
        end
    end // always @ (posedge clk or posedge reset)

    // Edges detection
    always @(posedge clk or posedge reset)  begin
        if (reset) begin
            clk_track_d <= 0;
            enable_d <= '0;
        end
        else begin
            clk_track_d <= clk_track;
            enable_d <= enable;
        end
    end
    logic rising_edge_wire;         
    logic falling_edge_wire;        
    logic falling_edge_trigger_wire;
    logic clk_out_wire;

    always_comb begin
        rising_edge           = ~clk_track_d & clk_track;
        falling_edge          = clk_track_d & ~clk_track;
        falling_edge_trigger  = enable_d ? (clk_track & cnt_equal) : 1'b0;
    end
    assign cnt_nxt = cnt_reg + 1'b1;
    //assign clk_out_wire = enable ? clk_track : pol;
    assign clk_out = enable ? clk_track : pol;
    //assign clk_out = clk_track;
/*
    always_ff @(posedge clk) begin
        clk_out  <= clk_out_wire;
        rising_edge         <= rising_edge_wire;
        falling_edge           <= falling_edge_wire;
        falling_edge_trigger    <=falling_edge_trigger_wire;
    end
*/
endmodule




