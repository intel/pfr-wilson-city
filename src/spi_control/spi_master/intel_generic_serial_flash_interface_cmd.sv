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
// $Revision: #3 $
// $Date: 2018/01/15 $
// $Author: tgngo $
//altera message_off 10036 10858 10230 10030 10034

`timescale 1 ps / 1 ps
module intel_generic_serial_flash_interface_cmd (
        input               clk,
        input               reset,

        input [1:0]         in_cmd_channel,
        input               in_cmd_eop,
        output logic        in_cmd_ready,
        input               in_cmd_sop,
        input [31:0]        in_cmd_data,
        input               in_cmd_valid,

        output logic [8:0]  out_cmd_channel,
        output logic        out_cmd_eop,
        input               out_cmd_ready,
        output logic        out_cmd_sop,
        output logic [7:0]  out_cmd_data,
        output logic        out_cmd_valid,

        input [7:0]         in_rsp_data,
        output logic        in_rsp_ready,
        input               in_rsp_valid,

        output logic [1:0]  out_rsp_channel,
        output logic [31:0] out_rsp_data,
        output logic        out_rsp_eop,
        input               out_rsp_ready,
        output logic        out_rsp_sop,
        output logic        out_rsp_valid,

        input [31:0]        addr_bytes_csr,
        input [31:0]        addr_bytes_xip,
        // Control signals to qspi interface component
        output logic [4:0]  dummy_cycles,
        output logic [3:0]  chip_select,
        output logic        require_rdata,
        
        input [1:0]         xip_trans_type,
        output logic [3:0]  op_num_lines,
        output logic [3:0]  addr_num_lines,
        output logic [3:0]  data_num_lines,
        input [1:0]         op_type, 
        input [1:0]         wr_addr_type,
        input [1:0]         wr_data_type,
        input [1:0]         rd_addr_type,
        input [1:0]         rd_data_type

    );

    // State machine
    typedef enum bit [7:0]
    {
        ST_IDLE             = 8'b00000001,
        ST_SEND_OPCODE      = 8'b00000010,
        ST_SEND_ADDR        = 8'b00000100,
        ST_SEND_DATA        = 8'b00001000,
        ST_WAIT_RSP         = 8'b00010000,
        ST_WAIT_BUFFER      = 8'b00100000,
        ST_SEND_DUMMY_RSP   = 8'b01000000,
        ST_COMPLETE         = 8'b10000000
     } t_state;
    t_state state, next_state;

    // +--------------------------------------------------
    // | Internal Signals
    // +--------------------------------------------------
    logic [3:0][7:0]    addr_mem;
    logic [1:0]         addr_cnt;
    logic [1:0]         addr_cnt_next;
    logic [31:0]        header_information;
    logic [1:0]         in_cmd_channel_reg;
    logic               has_addr;
    logic               is_4bytes_addr;
    logic               has_data_in;
    logic               has_data_out;
    logic               has_data_in_wire;
    logic               has_data_out_wire;
    logic [7:0]         opcode;
    logic [4:0]         numb_dummy_cycles;
    logic [3:0]         chip_select_reg;
    logic [3:0]         chip_select_wire;
    
    logic [8:0]         numb_data_bytes;
    logic               in_rsp_ready_adapt;
    logic [7:0]         csr_addr_0, csr_addr_1, csr_addr_2, csr_addr_3;
    logic [7:0]         xip_addr_0, xip_addr_1, xip_addr_2, xip_addr_3;
    
    logic [2:0]         buffer_cnt;
    logic [2:0]         buffer_cnt_next;
    logic               buffer_cnt_done;
    logic [1:0]         numb_addr_bytes;
    logic               addr_cnt_done;
    logic [7:0]         data_in_cnt;
    logic [7:0]         data_in_cnt_next;
    logic               data_in_cnt_done;
    logic [4:0]         dummy_cnt;
    logic [4:0]         dummy_cnt_next;
    logic               dummy_cnt_done;
    logic [7:0]         data_out_cnt;
    logic [7:0]         data_out_cnt_next;
    logic               data_out_cnt_done;
    logic [7:0]         numb_data_bytes_in_cnt;
    logic [7:0]         numb_data_bytes_in_next;
    logic               last_word_detect;
    logic               need_data_in;
    logic               need_data_out;
    
    // +--------------------------------------------------
    // | Decode Header
    // +--------------------------------------------------
    // |
    // register the header first
    always_ff @(posedge clk or posedge reset) begin
        if (reset) begin
            header_information  <= '0;
            in_cmd_channel_reg  <= '0;
        end
        else begin 
            if (in_cmd_valid && in_cmd_sop && in_cmd_ready) begin
                header_information <= in_cmd_data;
                in_cmd_channel_reg <= in_cmd_channel;
            end
        end
    end
    assign opcode             = header_information[7:0];
    assign has_addr           = header_information[8];
    assign is_4bytes_addr     = header_information[9];
    assign has_data_in        = header_information[10];
    assign has_data_out       = header_information[11];
    //assign has_dummy          = header_information[12];
    assign numb_dummy_cycles  = header_information[17:13];
    assign numb_data_bytes    = header_information[26:18];
    assign chip_select        = header_information[30:27];
    assign has_data_in_wire   = header_information[10];
    assign has_data_out_wire  = header_information[11];

    // Process address byte
    assign csr_addr_0     = addr_bytes_csr[7:0];
    assign csr_addr_1     = addr_bytes_csr[15:8];
    assign csr_addr_2     = addr_bytes_csr[23:16];
    assign csr_addr_3     = addr_bytes_csr[31:24];
    assign xip_addr_0     = addr_bytes_xip[7:0];
    assign xip_addr_1     = addr_bytes_xip[15:8];
    assign xip_addr_2     = addr_bytes_xip[23:16];
    assign xip_addr_3     = addr_bytes_xip[31:24];
    assign dummy_cycles   = numb_dummy_cycles;
    // Note that swap the address byte.
    always_ff @(posedge clk or posedge reset) begin
        if (reset)
            addr_mem <= '{{8{1'b0}}, {8{1'b0}}, {8{1'b0}}, {8{1'b0}}};
        else begin 
            if (in_cmd_valid && in_cmd_sop && in_cmd_ready) begin
                // 2'b01 is the channel for XIP controller
                // 2'b10 is the channel for CST controller
                if (in_cmd_channel == 2'b10)
                    addr_mem <= {csr_addr_3, csr_addr_2, csr_addr_1, csr_addr_0};
                else
                    addr_mem <= {xip_addr_3, xip_addr_2, xip_addr_1, xip_addr_0};
            end
        end
    end
    // +----------------------------------------------------------------------------------
    // | Encode type of transaction and data lines used for opcode, address, data ...
    // +----------------------------------------------------------------------------------
    always_ff @(posedge clk or posedge reset) begin
        if (reset) begin
            op_num_lines       <= '0;
            addr_num_lines     <= '0;
            data_num_lines     <= '0;
        end
        else begin 
            if (in_cmd_valid && in_cmd_sop && in_cmd_ready) begin
                if ((in_cmd_channel == 2'b10) || ((in_cmd_channel == 2'b01) && (xip_trans_type == 2'b00))) begin 
                // if come from csr controller, or from XIP but with flash command, not write and read
                    if (op_type == 2'b01) begin 
                        op_num_lines    <= 4'b0010;
                        addr_num_lines  <= 4'b0010;
                        data_num_lines  <= 4'b0010;
                    end 
                    else if (op_type == 2'b10) begin 
                        op_num_lines    <= 4'b0100;
                        addr_num_lines  <= 4'b0100;
                        data_num_lines  <= 4'b0100;
                    end
                    else begin // work for both x1 and default case
                        op_num_lines    <= 4'b0001;
                        addr_num_lines  <= 4'b0001;
                        data_num_lines  <= 4'b0001;
                    end
                end
                else begin // if from xip then need to check this is write or read and set accordingly
                    if (op_type == 2'b01)
                        op_num_lines    <= 4'b0010;
                    else if (op_type == 2'b10)
                        op_num_lines    <= 4'b0100;
                    else // work for both x1 and default case
                        op_num_lines    <= 4'b0001;

                    if (xip_trans_type == 2'b01) begin // this is write command
                        if (wr_addr_type == 2'b01)
                            addr_num_lines    <= 4'b0010;
                        else if (wr_addr_type == 2'b10)
                            addr_num_lines    <= 4'b0100;
                        else // work for both x1 and default case
                            addr_num_lines    <= 4'b0001;

                        if (wr_data_type == 2'b01)
                            data_num_lines    <= 4'b0010;
                        else if (wr_data_type == 2'b10)
                            data_num_lines    <= 4'b0100;
                        else // work for both x1 and default case
                            data_num_lines    <= 4'b0001;
                    end
                    else begin // this is read command
                        if (rd_addr_type == 2'b01)
                            addr_num_lines    <= 4'b0010;
                        else if (rd_addr_type == 2'b10)
                            addr_num_lines    <= 4'b0100;
                        else // work for both x1 and default case
                            addr_num_lines    <= 4'b0001;

                        if (rd_data_type == 2'b01)
                            data_num_lines    <= 4'b0010;
                        else if (rd_data_type == 2'b10)
                            data_num_lines    <= 4'b0100;
                        else // work for both x1 and default case
                            data_num_lines    <= 4'b0001;
                    end
                end
            end
        end
    end // always_ff @
    
    // +--------------------------------------------------
    // | Address bytes counter
    // +--------------------------------------------------
    assign numb_addr_bytes = is_4bytes_addr ? 2'h3 : 2'h2;
    always_ff @(posedge clk or posedge reset) begin
        if (reset)
            addr_cnt <= '0;
        else begin
            if (state == ST_SEND_OPCODE)
                addr_cnt <= numb_addr_bytes;
            else
                addr_cnt <= addr_cnt_next;
        end
    end

    assign addr_cnt_done = ((addr_cnt == 0) & out_cmd_valid & out_cmd_ready);

    always_comb begin 
        addr_cnt_next = addr_cnt;
        if ((state == ST_SEND_ADDR) & out_cmd_valid & out_cmd_ready) begin
            addr_cnt_next = addr_cnt_next - 2'h1;
            if (addr_cnt_done)
                addr_cnt_next = numb_addr_bytes;
        end
    end
    
    // +--------------------------------------------------
    // | Dummy bytes counter
    // +--------------------------------------------------
    /*
    always_ff @(posedge clk or posedge reset) begin
        if (reset)
            dummy_cnt <= '0;
        else 
            dummy_cnt <= dummy_cnt_next;
     end

    assign dummy_cnt_done = ((dummy_cnt == (numb_dummy_cycles - 4'h1)) & out_cmd_valid & out_cmd_ready);

    always_comb begin 
        dummy_cnt_next = dummy_cnt;
        if ((state == ST_SEND_DUMMY) & out_cmd_valid & out_cmd_ready) begin
            dummy_cnt_next = dummy_cnt_next + 4'h1;
            if (dummy_cnt_done)
                dummy_cnt_next = '0;
        end
    end
*/
    // +--------------------------------------------------
    // | Buffer counter: just count few cycles then return
    // | fake response, make sure nothing sent in
    // +--------------------------------------------------
    always_ff @(posedge clk or posedge reset) begin
        if (reset)
            buffer_cnt <= '0;
        else
            buffer_cnt <= buffer_cnt_next;
    end
    assign buffer_cnt_done = (buffer_cnt == 3'b111);
   
    always_comb begin
        buffer_cnt_next = buffer_cnt;
        if (state == ST_WAIT_BUFFER) begin
            buffer_cnt_next = buffer_cnt_next + 3'h1;
            if (buffer_cnt_done)
                buffer_cnt_next = '0;
        end
    end

    // +--------------------------------------------------
    // | Write Data bytes counter
    // +--------------------------------------------------
    
    always_ff @(posedge clk or posedge reset) begin
        if (reset)
            data_in_cnt <= '0;
        else
            data_in_cnt <= data_in_cnt_next;
    end
    assign data_in_cnt_done = ((data_in_cnt == (numb_data_bytes - 4'h1)) & out_cmd_valid & out_cmd_ready);
    always_comb begin 
        data_in_cnt_next = data_in_cnt;
        if ((state == ST_SEND_DATA) & out_cmd_valid & out_cmd_ready) begin
            data_in_cnt_next = data_in_cnt_next + 4'h1;
            if (data_in_cnt_done)
                data_in_cnt_next = '0;
        end
    end
    
    always_ff @(posedge clk or posedge reset) begin
        if (reset)
            numb_data_bytes_in_cnt <= '0;
        else
            numb_data_bytes_in_cnt <= numb_data_bytes_in_next;
    end

    always_comb begin 
        numb_data_bytes_in_next = numb_data_bytes_in_cnt;
        if (state != ST_IDLE) begin 
            if (has_data_in & in_cmd_valid & in_cmd_ready)
                numb_data_bytes_in_next = numb_data_bytes_in_next + 7'h4;
        end
        else
            numb_data_bytes_in_next = '0;
    end


    // +--------------------------------------------------
    // | Write Data bytes counter
    // +--------------------------------------------------
    always_ff @(posedge clk or posedge reset) begin
        if (reset)
            data_out_cnt <= '0;
        else
            data_out_cnt <= data_out_cnt_next;
    end
    assign data_out_cnt_done = ((data_out_cnt == (numb_data_bytes - 4'h1)) & in_rsp_valid & in_rsp_ready);
    always_comb begin 
        data_out_cnt_next = data_out_cnt;
        if ((state == ST_WAIT_RSP) & in_rsp_valid & in_rsp_ready) begin
            data_out_cnt_next = data_out_cnt_next + 4'h1;
            if (data_out_cnt_done)
                data_out_cnt_next = '0;
        end
    end

    logic [31:0]    adap_in_cmd_data;
    logic           adap_in_cmd_valid;
    logic           adap_in_cmd_ready;
    logic [7:0]     adap_out_cmd_data;
    logic           adap_out_cmd_valid;
    logic           adap_out_cmd_ready;
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
                if (in_cmd_valid && in_cmd_sop)
                    next_state = ST_SEND_OPCODE;
            end
            ST_SEND_OPCODE: begin
                next_state  = ST_SEND_OPCODE;
                if (out_cmd_ready) begin
                    if (has_addr)
                        next_state = ST_SEND_ADDR;
                    else if (has_data_in)
                        next_state = ST_SEND_DATA;
                    else if (has_data_out)
                        next_state = ST_WAIT_RSP;
                    else
                        next_state = ST_WAIT_BUFFER;
                end
            end
            ST_SEND_ADDR: begin
                next_state = ST_SEND_ADDR;
                if (addr_cnt_done) begin 
                    if (has_data_in)
                        next_state = ST_SEND_DATA;
                    else if (has_data_out)
                        next_state = ST_WAIT_RSP;
                    else
                        next_state = ST_WAIT_BUFFER;     
                end
            end
            ST_SEND_DATA: begin
                next_state = ST_SEND_DATA;
                if (data_in_cnt_done) begin 
                    if (has_data_out)
                        next_state = ST_WAIT_RSP;
                    else
                        next_state = ST_WAIT_BUFFER;
                end
            end
            ST_WAIT_BUFFER: begin
                next_state = ST_WAIT_BUFFER;
                if (buffer_cnt_done)
                    next_state = ST_SEND_DUMMY_RSP;
            end
            ST_SEND_DUMMY_RSP: begin
                next_state = ST_SEND_DUMMY_RSP;
                if (out_rsp_ready)
                    next_state = ST_IDLE;
            end
            ST_WAIT_RSP: begin
                next_state = ST_WAIT_RSP;
                if (data_out_cnt_done)
                   next_state = ST_COMPLETE;
            end
            ST_COMPLETE: begin
                next_state = ST_COMPLETE;
                if (out_rsp_valid && out_rsp_eop)
                   next_state = ST_IDLE;
            end
        endcase // case (state)
    end // always_comb

    // +--------------------------------------------------
    // | State Machine: state outputs
    // +--------------------------------------------------
    logic out_cmd_sop_each_stage;
    logic out_cmd_eop_each_stage;
    logic [3:0] out_cmd_channel_wire;
    always_comb begin
        out_cmd_valid           = '0;
        out_cmd_data            = '0;
        out_cmd_sop             = '0;
        out_cmd_eop             = '0;
        out_cmd_sop_each_stage  = '0;
        out_cmd_eop_each_stage  = '0;
        in_cmd_ready            = '0;
        out_cmd_channel_wire    = '0;
        case (state)
            ST_IDLE: begin
                out_cmd_valid           = '0;
                out_cmd_data            = '0;
                out_cmd_sop             = '0;
                out_cmd_eop             = '0;
                in_cmd_ready            = 1'b1;
                out_cmd_sop_each_stage  = '0;
                out_cmd_eop_each_stage  = '0;
            end
            ST_SEND_OPCODE: begin 
                out_cmd_valid           = 1'b1;
                out_cmd_data            = opcode;
                out_cmd_sop             = 1'b1;
                out_cmd_eop             = !has_addr & !has_data_in;
                in_cmd_ready            = '0;
                out_cmd_channel_wire    = 4'b0001;
                out_cmd_sop_each_stage  = 1'b1;
                out_cmd_eop_each_stage  = 1'b1;
            end
            ST_SEND_ADDR: begin 
                out_cmd_valid           = 1'b1;
                out_cmd_data            = addr_mem[addr_cnt];
                out_cmd_sop             = '0;
                out_cmd_eop             = !has_data_in & addr_cnt_done;
                in_cmd_ready            = adap_in_cmd_ready && !last_word_detect;
                out_cmd_channel_wire    = 4'b0010;
                out_cmd_sop_each_stage  = (addr_cnt == numb_addr_bytes);
                out_cmd_eop_each_stage  = addr_cnt_done;
            end
            ST_SEND_DATA: begin 
                out_cmd_valid           = adap_out_cmd_valid;
                out_cmd_data            = adap_out_cmd_data;
                out_cmd_sop             = '0;
                out_cmd_eop             = data_in_cnt_done;
                // only allow data in if not last word.
                in_cmd_ready            = adap_in_cmd_ready && !last_word_detect;
                out_cmd_channel_wire    = 4'b0100;
                out_cmd_sop_each_stage  = (data_in_cnt == '0);
                out_cmd_eop_each_stage  = data_in_cnt_done;
            end
            ST_WAIT_BUFFER: begin
                out_cmd_valid           = '0;
                out_cmd_data            = '0;
                out_cmd_sop             = '0;
                out_cmd_eop             = '0;
                in_cmd_ready            = '0;
                out_cmd_channel_wire    = 4'b0000;
                out_cmd_sop_each_stage  = '0;
                out_cmd_eop_each_stage  = '0;
            end
            ST_SEND_DUMMY_RSP: begin 
                out_cmd_valid           = '0;
                out_cmd_data            = 8'h0;
                out_cmd_sop             = '0;
                out_cmd_eop             = '0;
                in_cmd_ready            = '0;
                out_cmd_channel_wire    = 4'b0000;
                out_cmd_sop_each_stage  = '0;
                out_cmd_eop_each_stage  = '0;
            end
            ST_WAIT_RSP: begin 
                out_cmd_valid           = '0;
                out_cmd_data            = 8'h0;
                out_cmd_sop             = '0;
                out_cmd_eop             = '0;
                in_cmd_ready            = '0;
                out_cmd_channel_wire    = 4'b0000;
                out_cmd_sop_each_stage  = '0;
                out_cmd_eop_each_stage  = '0;
            end
            ST_COMPLETE: begin 
                out_cmd_valid           = '0;
                out_cmd_data            = 8'h0;
                out_cmd_sop             = '0;
                out_cmd_eop             = '0;
                in_cmd_ready            = '0;
                out_cmd_channel_wire    = 4'b0000;
                out_cmd_sop_each_stage  = '0;
                out_cmd_eop_each_stage  = '0;
            end

        endcase // case (state)
    end
    
    logic out_cmd_eop_final;
    assign out_cmd_eop_final = out_cmd_eop;
    assign out_cmd_channel    = {out_cmd_eop_final, out_cmd_sop_each_stage, out_cmd_eop_each_stage, in_cmd_channel_reg, out_cmd_channel_wire};

    // +--------------------------------------------------
    // | Output mapping
    // +--------------------------------------------------
    //assign out_cmd_channel  = in_cmd_channel;
    //assign in_cmd_ready   = out_cmd_ready;
    assign in_rsp_ready     = in_rsp_ready_adapt;
    assign require_rdata    = (state == ST_IDLE) ? 1'b0 : has_data_out;

    logic adapt_8_32_sop;
    logic adapt_8_32_eop;
    logic adapt_8_32_valid;
    logic [31:0] adapt_8_32_data;

    logic internal_rsp_valid; // this is fake response
    logic internal_rsp_sop; // this is fake response
    logic internal_rsp_eop; // this is fake response
    assign internal_rsp_valid = (state == ST_SEND_DUMMY_RSP);
    assign internal_rsp_sop = (state == ST_SEND_DUMMY_RSP);
    assign internal_rsp_eop = (state == ST_SEND_DUMMY_RSP);
    assign out_rsp_channel  = in_cmd_channel_reg;
    assign out_rsp_valid    = internal_rsp_valid | adapt_8_32_valid;
    assign out_rsp_sop      = internal_rsp_sop | adapt_8_32_sop;
    assign out_rsp_eop      = internal_rsp_eop | adapt_8_32_eop;
    // If the adapter has data then use this or else zero
    assign out_rsp_data     = adapt_8_32_valid ? adapt_8_32_data : 32'h0;

    assign adap_in_cmd_data     = in_cmd_sop ? 32'h0 : in_cmd_data;
    //assign adap_in_cmd_valid    = in_cmd_sop ? 1'h0 : in_cmd_valid;

    assign adap_out_cmd_ready   = (state == ST_SEND_DATA) & out_cmd_ready;
    assign adap_in_cmd_valid    = ((state == ST_SEND_ADDR) || (state == ST_SEND_DATA)) ? in_cmd_valid : 1'b0;


    logic sop_enable;
    logic in_rsp_sop;
    logic in_rsp_eop;

    assign in_rsp_sop    = sop_enable;
    assign in_rsp_eop    = data_out_cnt_done;
    always @(posedge clk) begin
        if (reset) begin
            sop_enable <= 1'b1;
        end
        else begin
            if (in_rsp_valid && in_rsp_ready) begin
                sop_enable <= 1'b0;
                if (in_rsp_eop)
                    sop_enable <= 1'b1;
            end
        end
    end


    assign need_data_in = in_cmd_sop ? has_data_in_wire : has_data_in;
    assign need_data_out = in_cmd_sop ? has_data_out_wire : has_data_out;

    always @(posedge clk) begin
        if (reset) begin
            last_word_detect <= 1'b0;
        end
        else begin
            if (in_cmd_valid && in_cmd_ready && in_cmd_eop && (need_data_in || need_data_out)) 
                last_word_detect <= 1'b1;
            if ((state == ST_SEND_DUMMY_RSP) || (state == ST_COMPLETE))
                last_word_detect <= 1'b0;
        end
    end

    // +--------------------------------------------------
    // | 32 bits to 8 bits adapter - for command
    // +--------------------------------------------------    
    logic adapter_rst;
    //assign adapter_en = !(state == ST_SEND_DATA) && !((state == ST_SEND_ADDR) && (addr_cnt == '0));
    assign adapter_rst = (state == ST_IDLE);
        data_adapter_32_8 data_adapter_32_8_inst (
        .clk               (clk),           
        .reset             (reset || adapter_rst),     
        .in_data           (adap_in_cmd_data),           
        .in_valid          (adap_in_cmd_valid),          
        .in_ready          (adap_in_cmd_ready),          
        .out_data          (adap_out_cmd_data),          
        .out_valid         (adap_out_cmd_valid),         
        .out_ready         (adap_out_cmd_ready)
    );

    // +--------------------------------------------------
    // | 8 bits to 32 bits adapter - for response
    // +--------------------------------------------------    
    data_adapter_8_32 data_adapter_8_32_inst(
        .clk               (clk),           
        .reset             (reset || adapter_rst),
        //.reset             (reset || !(state == ST_WAIT_RSP)),
        .in_data           (in_rsp_data),
        .in_valid          (in_rsp_valid && (state == ST_WAIT_RSP)),
        .in_ready          (in_rsp_ready_adapt),          
        .in_startofpacket  (in_rsp_sop),
        .in_endofpacket    (in_rsp_eop),
        .out_data          (adapt_8_32_data),          
        .out_valid         (adapt_8_32_valid),         
        .out_ready         (out_rsp_ready),
        .out_startofpacket (adapt_8_32_sop),
        .out_endofpacket   (adapt_8_32_eop),
        .out_empty         ( )
    );


endmodule


module data_adapter_32_8 (
// Interface: in
 output reg         in_ready,
 input              in_valid,
 input [32-1 : 0]    in_data,
 // Interface: out
 input                out_ready,
 output reg           out_valid,
 output reg [8-1: 0]  out_data,

  // Interface: clk
 input              clk,
 // Interface: reset
 input              reset

);



   // ---------------------------------------------------------------------
   //| Signal Declarations
   // ---------------------------------------------------------------------
   reg         state_read_addr;
   wire [2-1:0]   state_from_memory;
   reg  [2-1:0]   state;
   reg  [2-1:0]   new_state;
   reg  [2-1:0]   state_d1;
    
   reg            in_ready_d1;
   reg            mem_readaddr; 
   reg            mem_readaddr_d1;
   reg            a_ready;
   reg            a_valid;
   reg            a_channel;
   reg [8-1:0]    a_data0; 
   reg [8-1:0]    a_data1; 
   reg [8-1:0]    a_data2; 
   reg [8-1:0]    a_data3; 
   reg            a_startofpacket;
   reg            a_endofpacket;
   reg            a_empty;
   reg            a_error;
   reg            b_ready;
   reg            b_valid;
   reg            b_channel;
   reg  [8-1:0]   b_data;
   reg            b_startofpacket; 
   wire           b_startofpacket_wire; 
   reg            b_endofpacket; 
   reg            b_empty;   
   reg            b_error; 
   reg            mem_write0;
   reg  [8-1:0]   mem_writedata0;
   wire [8-1:0]   mem_readdata0;
   wire           mem_waitrequest0;
   reg  [8-1:0]   mem0[0:0];
   reg            mem_write1;
   reg  [8-1:0]   mem_writedata1;
   wire [8-1:0]   mem_readdata1;
   wire           mem_waitrequest1;
   reg  [8-1:0]   mem1[0:0];
   reg            mem_write2;
   reg  [8-1:0]   mem_writedata2;
   wire [8-1:0]   mem_readdata2;
   wire           mem_waitrequest2;
   reg  [8-1:0]   mem2[0:0];
   reg            sop_mem_writeenable;
   reg            sop_mem_writedata;
   wire           mem_waitrequest_sop; 

   wire           state_waitrequest;
   reg            state_waitrequest_d1; 

   reg            in_channel = 0;
   reg            out_channel;

   reg in_startofpacket = 0;
   reg in_endofpacket   = 0;
   reg out_startofpacket;
   reg out_endofpacket;

   reg  [4-1:0] in_empty = 0;
   reg  [1-1:0] out_empty;

   reg in_error = 0;
   reg out_error; 


   reg  [2-1:0]   state_register;
   reg            sop_register; 
   reg            error_register;
   reg  [8-1:0]   data0_register;
   reg  [8-1:0]   data1_register;
   reg  [8-1:0]   data2_register;

   // ---------------------------------------------------------------------
   //| Input Register Stage
   // ---------------------------------------------------------------------
   always @(posedge clk or posedge reset) begin
      if (reset) begin
         a_valid   <= 0;
         a_channel <= 0;
         a_data0   <= 0;
         a_data1   <= 0;
         a_data2   <= 0;
         a_data3   <= 0;
         a_startofpacket <= 0;
         a_endofpacket   <= 0;
         a_empty <= 0; 
         a_error <= 0;
      end else begin
         if (in_ready) begin
            a_valid   <= in_valid;
            a_channel <= in_channel;
            a_error   <= in_error;
            a_data3 <= in_data[31:24];
            a_data2 <= in_data[23:16];
            a_data1 <= in_data[15:8];
            a_data0 <= in_data[7:0];
            a_startofpacket <= in_startofpacket;
            a_endofpacket   <= in_endofpacket;
            a_empty         <= 0; 
            if (in_endofpacket)
               a_empty <= in_empty;
         end
      end 
   end

   always @* begin 
      state_read_addr = a_channel;
      if (in_ready)
         state_read_addr = in_channel;
   end
   

   // ---------------------------------------------------------------------
   //| State & Memory Keepers
   // ---------------------------------------------------------------------
   always @(posedge clk or posedge reset) begin
      if (reset) begin
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
   
   always @(posedge clk or posedge reset) begin
      if (reset) begin
         state_register <= 0;
         sop_register   <= 0;
         data0_register <= 0;
         data1_register <= 0;
         data2_register <= 0;
      end else begin
         state_register <= new_state;
         if (sop_mem_writeenable)
            sop_register   <= sop_mem_writedata;
         if (mem_write0)
            data0_register <= mem_writedata0;
         if (mem_write1)
            data1_register <= mem_writedata1;
         if (mem_write2)
            data2_register <= mem_writedata2;
         end
      end
   
      assign state_from_memory = state_register;
      assign b_startofpacket_wire = sop_register;
      assign mem_readdata0 = data0_register;
      assign mem_readdata1 = data1_register;
      assign mem_readdata2 = data2_register;
   
   // ---------------------------------------------------------------------
   //| State Machine
   // ---------------------------------------------------------------------
   always @* begin

      
   b_ready = (out_ready || ~out_valid);

   a_ready   = 0;
   b_data    = 0;
   b_valid   = 0;
   b_channel = a_channel;
   b_error   = a_error;
      
   state = state_from_memory;
      
   new_state           = state;
   mem_write0          = 0;
   mem_writedata0      = a_data0;
   mem_write1          = 0;
   mem_writedata1      = a_data0;
   mem_write2          = 0;
   mem_writedata2      = a_data0;
   sop_mem_writeenable = 0;

   b_endofpacket = a_endofpacket;
      
   b_startofpacket = 0;
      
   b_empty = 0;
       
   case (state) 
            0 : begin
            b_data[7:0] = a_data0;
            b_startofpacket = a_startofpacket;
            if (out_ready || ~out_valid) begin
               if (a_valid) begin
                  b_valid = 1;
                  new_state = state+1'b1;
                     if (a_endofpacket && (a_empty >= 3) ) begin
                        new_state = 0;
                        b_empty = a_empty - 3;
                        b_endofpacket = 1;
                        a_ready = 1;
                     end
                  end
               end
            end
         1 : begin
            b_data[7:0] = a_data1;
            b_startofpacket = 0;
            if (out_ready || ~out_valid) begin
               if (a_valid) begin
                  b_valid = 1;
                  new_state = state+1'b1;
                     if (a_endofpacket && (a_empty >= 2) ) begin
                        new_state = 0;
                        b_empty = a_empty - 2;
                        b_endofpacket = 1;
                        a_ready = 1;
                     end
                  end
               end
            end
         2 : begin
            b_data[7:0] = a_data2;
            b_startofpacket = 0;
            if (out_ready || ~out_valid) begin
               if (a_valid) begin
                  b_valid = 1;
                  new_state = state+1'b1;
                     if (a_endofpacket && (a_empty >= 1) ) begin
                        new_state = 0;
                        b_empty = a_empty - 1'b1;
                        b_endofpacket = 1;
                        a_ready = 1;
                     end
                  end
               end
            end
         3 : begin
            b_data[7:0] = a_data3;
            b_startofpacket = 0;
            if (out_ready || ~out_valid) begin
            a_ready = 1;
               if (a_valid) begin
                  b_valid = 1;
                  new_state = 0;
                     if (a_endofpacket && (a_empty >= 0) ) begin
                        new_state = 0;
                        b_empty = a_empty - 0;
                        b_endofpacket = 1;
                        a_ready = 1;
                     end
                  end
               end
            end

   endcase

      in_ready = (a_ready || ~a_valid);

      mem_readaddr = in_channel; 
      if (~in_ready)
         mem_readaddr = mem_readaddr_d1;

      
      sop_mem_writedata = 0;
      if (a_valid)
         sop_mem_writedata = a_startofpacket;
      if (b_ready && b_valid && b_startofpacket)
         sop_mem_writeenable = 1;

   end


   // ---------------------------------------------------------------------
   //| Output Register Stage
   // ---------------------------------------------------------------------
   always @(posedge clk or posedge reset) begin
      if (reset) begin
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

 
module data_adapter_8_32 (
 // Interface: in
 output reg         in_ready,
 input              in_valid,
 input [8-1 : 0]    in_data,
 input              in_startofpacket,
 input              in_endofpacket,
 // Interface: out
 input                out_ready,
 output reg           out_valid,
 output reg [32-1: 0]  out_data,
 output reg           out_startofpacket,
 output reg           out_endofpacket,
 output reg [2-1 : 0] out_empty,

  // Interface: clk
 input              clk,
 // Interface: reset
 input              reset

);



   // ---------------------------------------------------------------------
   //| Signal Declarations
   // ---------------------------------------------------------------------
   reg         state_read_addr;
   wire [2-1:0]   state_from_memory;
   reg  [2-1:0]   state;
   reg  [2-1:0]   new_state;
   reg  [2-1:0]   state_d1;
    
   reg            in_ready_d1;
   reg            mem_readaddr; 
   reg            mem_readaddr_d1;
   reg            a_ready;
   reg            a_valid;
   reg            a_channel;
   reg [8-1:0]    a_data0; 
   reg            a_startofpacket;
   reg            a_endofpacket;
   reg            a_empty;
   reg            a_error;
   reg            b_ready;
   reg            b_valid;
   reg            b_channel;
   reg  [32-1:0]   b_data;
   reg            b_startofpacket; 
   wire           b_startofpacket_wire; 
   reg            b_endofpacket; 
   reg  [2-1:0]   b_empty;
   reg            b_error; 
   reg            mem_write0;
   reg  [8-1:0]   mem_writedata0;
   wire [8-1:0]   mem_readdata0;
   wire           mem_waitrequest0;
   reg  [8-1:0]   mem0[0:0];
   reg            mem_write1;
   reg  [8-1:0]   mem_writedata1;
   wire [8-1:0]   mem_readdata1;
   wire           mem_waitrequest1;
   reg  [8-1:0]   mem1[0:0];
   reg            mem_write2;
   reg  [8-1:0]   mem_writedata2;
   wire [8-1:0]   mem_readdata2;
   wire           mem_waitrequest2;
   reg  [8-1:0]   mem2[0:0];
   reg            sop_mem_writeenable;
   reg            sop_mem_writedata;
   wire           mem_waitrequest_sop; 

   wire           state_waitrequest;
   reg            state_waitrequest_d1; 

   reg            in_channel = 0;
   reg            out_channel;


   reg  [1-1:0] in_empty = 0;

   reg in_error = 0;
   reg out_error; 

   wire           error_from_mem;
   reg            error_mem_writedata;
   reg          error_mem_writeenable;

   reg  [2-1:0]   state_register;
   reg            sop_register; 
   reg            error_register;
   reg  [8-1:0]   data0_register;
   reg  [8-1:0]   data1_register;
   reg  [8-1:0]   data2_register;

   // ---------------------------------------------------------------------
   //| Input Register Stage
   // ---------------------------------------------------------------------
   always @(posedge clk or posedge reset) begin
      if (reset) begin
         a_valid   <= 0;
         a_channel <= 0;
         a_data0   <= 0;
         a_startofpacket <= 0;
         a_endofpacket   <= 0;
         a_empty <= 0; 
         a_error <= 0;
      end else begin
         if (in_ready) begin
            a_valid   <= in_valid;
            a_channel <= in_channel;
            a_error   <= in_error;
            a_data0 <= in_data[7:0];
            a_startofpacket <= in_startofpacket;
            a_endofpacket   <= in_endofpacket;
            a_empty         <= 0; 
            if (in_endofpacket)
               a_empty <= in_empty;
         end
      end 
   end

   always @* begin 
      state_read_addr = in_channel;
   end
   

   // ---------------------------------------------------------------------
   //| State & Memory Keepers
   // ---------------------------------------------------------------------
   always @(posedge clk or posedge reset) begin
      if (reset) begin
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
   
   always @(posedge clk or posedge reset) begin
      if (reset) begin
         state_register <= 0;
         sop_register   <= 0;
         data0_register <= 0;
         data1_register <= 0;
         data2_register <= 0;
         error_register <= 0;
      end else begin
         state_register <= new_state;
         if (sop_mem_writeenable)
            sop_register   <= sop_mem_writedata;
         if (a_valid)
            error_register <= error_mem_writedata;
         if (mem_write0)
            data0_register <= mem_writedata0;
         if (mem_write1)
            data1_register <= mem_writedata1;
         if (mem_write2)
            data2_register <= mem_writedata2;
         end
      end
   
      assign state_from_memory = state_register;
      assign b_startofpacket_wire = sop_register;
      assign mem_readdata0 = data0_register;
      assign mem_readdata1 = data1_register;
      assign mem_readdata2 = data2_register;
      assign error_from_mem = error_register;
   
   // ---------------------------------------------------------------------
   //| State Machine
   // ---------------------------------------------------------------------
   always @* begin

      
   b_ready = (out_ready || ~out_valid);

   a_ready   = 0;
   b_data    = 0;
   b_valid   = 0;
   b_channel = a_channel;
   b_error   = a_error;
      
   state = state_from_memory;
   if (~in_ready_d1)
      state = state_d1;
         
   error_mem_writedata = error_from_mem | a_error;
   if (state == 0)
      error_mem_writedata = a_error;
   b_error = error_mem_writedata;
      
   new_state           = state;
   mem_write0          = 0;
   mem_writedata0      = a_data0;
   mem_write1          = 0;
   mem_writedata1      = a_data0;
   mem_write2          = 0;
   mem_writedata2      = a_data0;
   sop_mem_writeenable = 0;

   b_endofpacket = a_endofpacket;
      
   b_startofpacket = 0;
      
   b_startofpacket = b_startofpacket_wire;
   b_empty = 0;
       
   case (state) 
            0 : begin
               mem_writedata0 = a_data0;
               //b_data[31:24] = a_data0;
               b_data[7:0] = a_data0;
               if (out_ready || ~out_valid) begin
                  a_ready = 1;
                  if (a_valid) 
                  begin
                     new_state = state+1'b1;
                     mem_write0 = 1;
                     sop_mem_writeenable = 1;
                     if (a_endofpacket) begin
                        b_empty = a_empty+3;
                        b_valid = 1;
                        b_startofpacket = a_startofpacket;
                        new_state = 0;
                     end
                  end
               end
            end
         1 : begin
               mem_writedata0 = mem_readdata0;
               //b_data[31:24] = mem_readdata0;
               b_data[7:0] = mem_readdata0;
               mem_writedata1 = a_data0;
               //b_data[23:16] = a_data0;
               b_data[15:8] = a_data0;
               if (out_ready || ~out_valid) begin
                  a_ready = 1;
                  if (a_valid) 
                  begin
                     new_state = state+1'b1;
                     mem_write0 = 1;
                     mem_write1 = 1;
                     if (a_endofpacket) begin
                        b_empty = a_empty+2;
                        b_valid = 1;
                        new_state = 0;
                     end
                  end
               end
            end
         2 : begin
               mem_writedata0 = mem_readdata0;
               //b_data[31:24] = mem_readdata0;
               b_data[7:0] = mem_readdata0;
               mem_writedata1 = mem_readdata1;
               //b_data[23:16] = mem_readdata1;
               b_data[15:8] = mem_readdata1;
               mem_writedata2 = a_data0;
               //b_data[15:8] = a_data0;
               b_data[23:16] = a_data0;
               if (out_ready || ~out_valid) begin
                  a_ready = 1;
                  if (a_valid) 
                  begin
                     new_state = state+1'b1;
                     mem_write0 = 1;
                     mem_write1 = 1;
                     mem_write2 = 1;
                     if (a_endofpacket) begin
                        b_empty = a_empty+1;
                        b_valid = 1;
                        new_state = 0;
                     end
                  end
               end
            end
         3 : begin
               //b_data[31:24] = mem_readdata0;
               //b_data[23:16] = mem_readdata1;
               //b_data[15:8] = mem_readdata2;
               //b_data[7:0] = a_data0;
               b_data[7:0] = mem_readdata0;
               b_data[15:8] = mem_readdata1;
               b_data[23:16] = mem_readdata2;
               b_data[31:24] = a_data0;
               if (out_ready || ~out_valid) begin
                  a_ready = 1;
                  if (a_valid) 
                  begin
                     new_state = 0;
                     b_valid = 1;
                     if (a_endofpacket) begin
                        b_empty = a_empty+0;
                        b_valid = 1;
                        new_state = 0;
                     end
                  end
               end
            end

   endcase

      in_ready = (a_ready || ~a_valid);

      mem_readaddr = in_channel; 
      if (~in_ready)
         mem_readaddr = mem_readaddr_d1;

      
      sop_mem_writedata = 0;
      if (a_valid)
         sop_mem_writedata = a_startofpacket;
      if (b_ready && b_valid && b_startofpacket)
         sop_mem_writeenable = 1;

   end


   // ---------------------------------------------------------------------
   //| Output Register Stage
   // ---------------------------------------------------------------------
   always @(posedge clk or posedge reset) begin
      if (reset) begin
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
