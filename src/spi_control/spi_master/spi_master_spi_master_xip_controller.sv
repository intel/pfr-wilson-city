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

`timescale 1 ps / 1 ps
module spi_master_spi_master_xip_controller #(
    parameter ADDR_WIDTH            = 32
) (
    input [ADDR_WIDTH-1:0]  mem_addr,        
    input          		    mem_rd,          
    output logic [31:0]     mem_rddata,      
    input          		    mem_wr,          
    input [31:0] 		    mem_wrdata,      
    output logic            mem_waitrequest, 
    output logic            mem_rddatavalid, 
    input [3:0]             mem_byteenable,
    input [6:0]  		    mem_burstcount,
    
    input                   clk,             
    input                   reset,
    
    output logic [1:0]      cmd_channel,     
    output logic            cmd_eop,         
    input                   cmd_ready,       
    output logic            cmd_sop,         
    output logic [31:0]     cmd_data,        
    output logic            cmd_valid,
    
    input [1:0]             rsp_channel,       
    input [31:0]            rsp_data,        
    input                   rsp_eop,         
    output logic            rsp_ready,       
    input                   rsp_sop,       
    input                   rsp_valid,
    input [7:0]             polling_opcode,
    input [2:0]             polling_bit,
    input [7:0]             wr_opcode,
    input [7:0]             rd_opcode,
    input [4:0]             rd_dummy_cycles,
    output logic [1:0]      xip_trans_type,
    input [7:0]             wr_en_opcode,
    input                   is_4bytes_addr_xip,

    input [3:0]             chip_select,

    output logic [31:0]     addr_bytes_xip   
);
    
    typedef enum logic [13:0]
    {STATE_IDLE, STATE_STATUS_CMD, STATE_STATUS_RSP, STATE_WRENABLE_CMD, STATE_WRENABLE_RSP,
        STATE_WR_CMD, STATE_WR_DATA, STATE_POLL_CMD, STATE_POLL_RSP, 
        STATE_READ_NVCR_CMD, STATE_READ_NVCR_RSP, STATE_READ_CMD, STATE_READ_DATA, 
        STATE_COMPLETE} current_t;
    current_t current_state, next_state;

    logic                   mem_rd_combi;
    logic                   mem_wr_combi;
    logic [ADDR_WIDTH-1:0]  mem_addr_reg;
    logic [6:0]             mem_burstcount_reg;
    logic [3:0]             mem_byteenable_reg;
    logic [31:0]            mem_write_data_reg;
    
    logic [8:0]             data_bytes_num;
    logic [4:0]             dummy_bytes_num;
    
    logic [31:0]            fifo_in_data_wire;
    
    logic [2:0]             wr_data_val;
        
    logic [6:0]             word_num;
    logic [6:0]             byte_num;
    
    logic                   busy;
    
    logic [31:0]            fifo_in_data;
    logic                   fifo_in_valid;
    logic                   fifo_in_ready;
    logic                   fifo_in_sop;
    logic                   fifo_in_eop;
    
    logic [31:0]            fifo_out_data;
    logic                   fifo_out_valid;
    logic                   fifo_out_ready;
    logic                   fifo_out_sop;
    logic                   fifo_out_eop;
    
    logic                   cmd_eop_combi;    
    logic                   cmd_sop_combi;
    logic [31:0]            cmd_data_combi;
    logic                   cmd_valid_combi;
    
    logic                   cmd_eop_reg;    
    logic                   cmd_sop_reg;
    logic [31:0]            cmd_data_reg;
    logic                   cmd_valid_reg;

logic read_has_dummy;

// *********************************************************************************   
//      Array of predefinef command header
//      32'b[reserved_bit][chip_select][data_bytes_bin][dummy_bytes_bin][has_dummy][has_data_out][has_data_in][4bytes_addr][has_addr][opcode_bin]
// *********************************************************************************
    logic [12:0]   header_mem [0: 5];  

    // read_status
    assign header_mem[0]    = {1'b0, 1'b1, 1'b0, 1'b0, 1'b0, 8'h05};

    // write_enable
    assign header_mem[1]    = {1'b0, 1'b0, 1'b0, 1'b0, 1'b0, 8'h06};
    
    // quad_write, dual_write, write
    assign header_mem[2]    = {1'b0, 1'b0, 1'b1, is_4bytes_addr_xip, 1'b1, wr_opcode};  

    // read_status, read_flag_status
    assign header_mem[3]  = {1'b0, 1'b1, 1'b0, 1'b0, 1'b0, polling_opcode};

    // read_nvcr to retrieve dummy clock value
    assign header_mem[4]    = {1'b0, 1'b1, 1'b0, 1'b0, 1'b0, 8'hB5};
    
    // quad_fast_read, dual_fast_read, fast_read, read
    assign header_mem[5]    = {read_has_dummy, 1'b1, 1'b0, is_4bytes_addr_xip, 1'b1, rd_opcode};


// *********************************************************************************   
//      RTL State Machine: State transition
// *********************************************************************************
// STATE_IDLE           : Idle state that wait for Read/Write.
//                          If Write operation is detected, and is a burst, store
//                             all data into fifp then goes to STATE_BURST_DATA.
//                          If Read operation is detected, go to STATE_READ_NVCR_CMD.
// STATE_STATUS_CMD     : Send read_status command packet, go to STATE_STATUS_RSP.
// STATE_STATUS_RSP     : Read response to check system is busy or not.
//                          If busy, go back to STATE_STATUS_CMD.
//                          Else, go to STATE_WRENABLE_CMD.
// STATE_WRENABLE_CMD   : Send write_enable command packet, go to STATE_WR_CMD.
// STATE_WR_CMD         : Send write command packet, go to STATE_WR_DATA.
// STATE_WR_DATA        : Send all address/data in fifo, go to STATE_POLL_CMD.
// STATE_POLL_CMD       : Send read_status command packet, go to STATE_POLL_RSP.
// STATE_POLL_RSP       : Read response to check the polling status.
//                          If complete, go to STATE_COMPLETE.
//                          Else, go to STATE_POLL_CMD.
// STATE_READ_NVCR_CMD  : Send read_nvcr command packet, go to STATE_READ_NVCR_RSP.
// STATE_READ_NVCR_RSP  : Read response to check dummy clock value, go to STATE_READ_CMD.
// STATE_READ_CMD       : Send read command packet, go to STATE_READ_DATA.
// STATE_READ_DATA      : Check rsp_eop and go to STATE_COMPLETE.
// STATE_COMPLETE       : All operation is completed, going back to STATE_IDLE.
        
    always_comb begin
        next_state  = STATE_IDLE;
        
        case (current_state)
            STATE_IDLE: begin
                next_state  = STATE_IDLE;
                if (mem_rd_combi) begin
                    next_state      = STATE_READ_CMD;
                end          
                else if (fifo_in_valid && fifo_in_eop)
                    // if the last data has been stored infifo, then move to next state
                    next_state = STATE_STATUS_CMD;
            end
           
            STATE_STATUS_CMD: begin
                next_state  = STATE_STATUS_CMD;
                if (cmd_ready)
                    next_state  = STATE_STATUS_RSP;
            end
            
            STATE_STATUS_RSP: begin
                next_state  = STATE_STATUS_RSP;
                if (rsp_valid && rsp_eop) begin
                    if (~busy)
                        next_state  = STATE_WRENABLE_CMD;
                    else
                        next_state  = STATE_STATUS_CMD;
                end
            end
            
            STATE_WRENABLE_CMD: begin
                next_state  = STATE_WRENABLE_CMD;
                if (cmd_ready)
                    next_state  = STATE_WRENABLE_RSP;
            end

            STATE_WRENABLE_RSP: begin
                next_state  = STATE_WRENABLE_RSP;
                if (rsp_valid && rsp_eop)
                    next_state  = STATE_WR_CMD;
            end
            
            STATE_WR_CMD: begin
                next_state  = STATE_WR_CMD;
                if (cmd_ready)
                    next_state  = STATE_WR_DATA;
            end
            
            STATE_WR_DATA: begin
                next_state  = STATE_WR_DATA;
                if (rsp_valid && rsp_eop)
                    next_state  = STATE_POLL_CMD;
            end
            
            STATE_POLL_CMD: begin
                next_state  = STATE_POLL_CMD;
                if (cmd_ready)
                    next_state  = STATE_POLL_RSP;
            end
            
            STATE_POLL_RSP: begin
                next_state  = STATE_POLL_RSP;
                if (rsp_valid && rsp_eop) begin
                    if (~busy)
                        next_state  = STATE_COMPLETE;
                    else
                        next_state  = STATE_POLL_CMD;
                end
            end
            
            STATE_READ_NVCR_CMD: begin
                next_state  = STATE_READ_NVCR_CMD;
                if (cmd_ready)
                    next_state  = STATE_READ_NVCR_RSP;
            end
            
            STATE_READ_NVCR_RSP: begin
                next_state  = STATE_READ_NVCR_RSP;
                if (rsp_valid && rsp_eop)
                    next_state  = STATE_READ_CMD;
            end
            
            STATE_READ_CMD: begin
                next_state  = STATE_READ_CMD;
                if (cmd_ready)
                    next_state  = STATE_READ_DATA;
            end
            
            STATE_READ_DATA: begin
                next_state  = STATE_READ_DATA;
                if (rsp_valid && rsp_eop)
                    next_state  = STATE_COMPLETE;
            end
            
            STATE_COMPLETE: begin
                next_state  = STATE_IDLE;
            end
        endcase // case (current_state)
    end // always_comb
    
// *********************************************************************************   
//      RTL State Machine: State Output
// *********************************************************************************
    assign rsp_ready  = 1'b1;// Avalon master has no backpressure on response, connect to 1
    always_comb begin
        if (current_state == STATE_WR_DATA)
            fifo_out_ready  = cmd_ready;
        else
            fifo_out_ready  = '0;
    end
    // xip_trans_type:
    // it is used to indicate what kinf of transfer, since the XIP mainly send write and read but 
    // it does need to send flash command - read status, nvcr ... 
    // this is used to tell the command generator select correct data line
    // 00 -> normal flash command
    // 01 -> write command
    // 10 -> read command
    always_comb begin
        cmd_valid_combi     = '0;
        cmd_data_combi      = '0;
        cmd_sop_combi       = '0;
        cmd_eop_combi       = '0;
        xip_trans_type      = '0;
        case (current_state)
            STATE_IDLE: begin
                cmd_sop_combi    = '0;
                cmd_eop_combi    = '0;
                cmd_valid_combi  = '0;
                cmd_data_combi   = '0;
                xip_trans_type   = 0;
            end
            
            STATE_STATUS_CMD: begin
                cmd_data_combi   = {1'b0, chip_select, 9'b1, 5'b0, header_mem[0]};
                cmd_sop_combi    = 1'b1;
                cmd_eop_combi    = 1'b1;
                cmd_valid_combi  = 1'b1;
                xip_trans_type   = '0;
            end
            
            STATE_STATUS_RSP: begin      
                cmd_sop_combi    = '0;
                cmd_eop_combi    = '0;
                cmd_valid_combi  = '0;
                cmd_data_combi   = '0;
            end
            
            STATE_WRENABLE_CMD: begin    
                cmd_data_combi   = {1'b0, chip_select, 9'b0, 5'b0, header_mem[1]};
                cmd_sop_combi    = 1'b1;
                cmd_eop_combi    = 1'b1;
                cmd_valid_combi  = 1'b1;
                xip_trans_type   = '0;
            end

            STATE_WRENABLE_RSP: begin    
                cmd_sop_combi    = '0;
                cmd_eop_combi    = '0;
                cmd_valid_combi  = '0;
                cmd_data_combi   = '0;
                xip_trans_type   = 0;
            end 
            
            STATE_WR_CMD: begin
                cmd_data_combi   = {1'b0, chip_select, data_bytes_num, 5'b1, header_mem[2]};
                cmd_sop_combi    = 1'b1;
                cmd_eop_combi    = '0;
                cmd_valid_combi  = 1'b1;
                xip_trans_type   = 2'b01;
            end
            
            STATE_WR_DATA: begin
                // this stage, those command signals are used from the fifo
                cmd_sop_combi    = '0;
                cmd_eop_combi    = '0;
                cmd_valid_combi  = '0;
                cmd_data_combi   = '0;
                xip_trans_type   = 2'b01;
            end
            
            STATE_POLL_CMD: begin
                cmd_data_combi  = {1'b0, chip_select, 9'b1, 5'b0, header_mem[3]};
                cmd_sop_combi   = 1'b1;
                cmd_eop_combi   = 1'b1;
                cmd_valid_combi = 1'b1;
                xip_trans_type   = '0;
            end
            
            STATE_POLL_RSP: begin
                cmd_sop_combi    = '0;
                cmd_eop_combi    = '0;
                cmd_valid_combi  = '0;
                cmd_data_combi   = '0;
                xip_trans_type   = 0;
            end
            
            STATE_READ_NVCR_CMD: begin
                cmd_data_combi  = {1'b0, chip_select, 9'h2, 5'b0, header_mem[4]};
                cmd_sop_combi   = 1'b1;
                cmd_eop_combi   = 1'b1;
                cmd_valid_combi = 1'b1;
                xip_trans_type   = 0;
            end
            
            STATE_READ_NVCR_RSP: begin
                cmd_sop_combi    = '0;
                cmd_eop_combi    = '0;
                cmd_valid_combi  = '0;
                cmd_data_combi   = '0;
                xip_trans_type   = 0;
            end
            
            STATE_READ_CMD: begin
                cmd_data_combi  = {1'b0, chip_select, data_bytes_num, dummy_bytes_num, header_mem[5]};
                cmd_sop_combi   = 1'b1;
                cmd_eop_combi   = 1'b1;
                cmd_valid_combi = 1'b1;
                xip_trans_type   = 2'b10;
            end
            
            STATE_READ_DATA: begin
                cmd_sop_combi    = '0;
                cmd_eop_combi    = '0;
                cmd_valid_combi  = '0;
                cmd_data_combi   = '0;
                xip_trans_type   = 2'b10;
            end
                        
            STATE_COMPLETE: begin
                cmd_sop_combi    = '0;
                cmd_eop_combi    = '0;
                cmd_valid_combi  = '0;
                cmd_data_combi   = '0;
                xip_trans_type   = 0;
            end
            
        endcase
        end // always_comb
    
    always_ff @(posedge clk or posedge reset) begin
        if (reset)
            current_state <= STATE_IDLE;
        else
            current_state <= next_state;
    end
    

// *********************************************************************************   
//      Avalon-MM signals
// *********************************************************************************
// enabling read or write operation
    assign mem_rd_combi  = mem_rd && ~mem_waitrequest;
    // For write burst, byteenable should all be 1, 
    // for non-bursting write, it can have different value (avalon spec)
    assign mem_wr_combi = mem_wr && ~mem_waitrequest && (mem_byteenable != 4'h0);
    
    // register user address, not timing critical
    always_ff @(posedge clk or posedge reset) begin
        if (reset) 
            mem_addr_reg    <= '0;
        else begin
            if ((mem_rd_combi || mem_wr_combi) && current_state == STATE_IDLE) 
                mem_addr_reg    <= mem_addr;
        end
    end
    
    // register user burst count
    always_ff @(posedge clk or posedge reset) begin
        if (reset) 
            mem_burstcount_reg  <= '0;
        else begin
            if ((mem_rd_combi || mem_wr_combi) && current_state == STATE_IDLE) 
                mem_burstcount_reg  <= mem_burstcount;
        end
    end
    
    /*
    // register user byteenable
    always_ff @(posedge clk or posedge reset) begin
        if (reset) 
            mem_byteenable_reg  <= '0;
        else begin
            if (mem_wr_combi && current_state == STATE_IDLE) begin
                if (mem_burstcount == 7'b1)
                    mem_byteenable_reg  <= mem_byteenable;
                else
                    mem_byteenable_reg  <= 4'b1111;
            end
            else if (mem_rd_combi && current_state == STATE_IDLE)
                // For read, all byteenalble assumed to be 1.
                mem_byteenable_reg  <= 4'b1111;
        end
    end
    */
    assign mem_byteenable_reg = 4'b1111;  
    
    // register user write data
    always_ff @(posedge clk or posedge reset) begin
        if (reset) 
            mem_write_data_reg  <= '0;
        else begin
            if (mem_wr_combi) 
                mem_write_data_reg  <= mem_wrdata;
        end
    end
    
    // Wait request signals, make sure to assert it when reset is high
    logic hold_waitrequest;
    always_ff @(posedge clk or posedge reset) begin
   		if (reset) 
   			hold_waitrequest <= 1'h1; 
   		else 
   			hold_waitrequest <= 1'h0; 
   	end
    assign mem_waitrequest  = !(current_state ==  STATE_IDLE) || hold_waitrequest;

// *********************************************************************************   
//      Process Data for Command Generator
// *********************************************************************************
     
    // manage write data according to byteenable
    always_comb begin
        case (mem_byteenable_reg)
            4'b1111: begin
                fifo_in_data_wire = mem_write_data_reg;
                addr_bytes_xip   = {mem_addr_reg[ADDR_WIDTH-3:0], 2'b0}; 
            end
                
            4'b0011: begin
                fifo_in_data_wire = {16'b0, mem_write_data_reg[15:0]};
                addr_bytes_xip   = {mem_addr_reg[ADDR_WIDTH-3:0], 2'b0}; 
            end
            
            4'b1100: begin
                fifo_in_data_wire = {16'b0, mem_write_data_reg[31:16]};
                addr_bytes_xip   = {mem_addr_reg[ADDR_WIDTH-3:0], 2'b0} + 32'h2; 
            end
            
            4'b0001: begin
                fifo_in_data_wire = {24'b0, mem_write_data_reg[7:0]};
                addr_bytes_xip   = {mem_addr_reg[ADDR_WIDTH-3:0], 2'b0}; 
            end
            
            4'b0010: begin
                fifo_in_data_wire = {24'b0, mem_write_data_reg[15:8]};
                addr_bytes_xip   = {mem_addr_reg[ADDR_WIDTH-3:0], 2'b0} + 32'h1; 
            end
            
            4'b0100: begin
                fifo_in_data_wire = {24'b0, mem_write_data_reg[23:16]};
                addr_bytes_xip   = {mem_addr_reg[ADDR_WIDTH-3:0], 2'b0} + 32'h2; 
            end
            
            4'b1000: begin
                fifo_in_data_wire = {24'b0, mem_write_data_reg[31:24]};
                addr_bytes_xip   = {mem_addr_reg[ADDR_WIDTH-3:0], 2'b0} + 32'h3; 
            end
            
            default: begin
                fifo_in_data_wire   = '0;
                addr_bytes_xip   = {mem_addr_reg[ADDR_WIDTH-3:0], 2'b0}; 
            end
        endcase
    end // always_comb
    
    // This logic to calculate the number of byte need transfer in case of non-bursting write
    logic [8:0] bytes_num_single_burst;
    logic       is_burst_reg;
    
    assign bytes_num_single_burst = mem_byteenable_reg[0] + mem_byteenable_reg[1] + mem_byteenable_reg[2] + mem_byteenable_reg[3];
    // Register this signal for later use when sending write data
    always_ff @(posedge clk or posedge reset) begin
        if (reset)
            is_burst_reg <= '0;
        else begin
            if ((fifo_in_valid && fifo_in_eop) || mem_rd_combi) begin 
                if (mem_burstcount > 7'h1)
                    is_burst_reg <= 1'b1;
                else
                    is_burst_reg <= 1'b0;
            end
        end
    end // always_ff @
    
    // number of data that the controller need, when bursting it is simple, convert from burstcount in words
    // to byte, for non-bursting use the byteenable.
    always_comb begin
        if (is_burst_reg) 
            data_bytes_num = {mem_burstcount_reg, 2'b00};
        else 
            data_bytes_num = bytes_num_single_burst;
    end
    
    // Store user input data in fifo for write operation
    logic [6:0] internal_burstcount;
    logic [6:0] burstcount_register;
    logic       sop_enable;
    logic     mem_data_eop;
    
    always @(posedge clk, posedge reset) begin
        if (reset) begin
            sop_enable <= 1'b1;
        end
        else begin
            if (mem_wr_combi && !mem_waitrequest) begin
                sop_enable <= 1'b0;
                if (mem_data_eop)
                    sop_enable <= 1'b1;
            end
        end
    end // always @ (posedge clk, posedge reset)
    assign mem_data_eop = fifo_in_eop;

    always_comb begin
        internal_burstcount = mem_burstcount;
        
        if (~sop_enable) begin
            internal_burstcount = burstcount_register;
        end
    end
    
    always @(posedge clk, posedge reset) begin
        if (reset) begin
            burstcount_register <= '0;
        end 
        else begin
            burstcount_register <= burstcount_register;
            if (sop_enable)
                burstcount_register <= mem_burstcount - 7'h1;
            else if (mem_wr_combi)
                burstcount_register <= burstcount_register - 7'h1;
        end
    end // always @ (posedge clk, posedge reset)
    
    // Connect those control signals to the fifo
    assign fifo_in_eop     = (internal_burstcount == 7'h1);
    assign fifo_in_sop     = sop_enable;
    assign fifo_in_data    = mem_wrdata; // the data is direct from avalon write data
    assign fifo_in_valid   = fifo_in_ready && mem_wr_combi && !mem_waitrequest;
    
    wire wire_fifo_out_data; 
    assign fifo_out_data = {31'h0, wire_fifo_out_data}; 
    avst_fifo #(
		.SYMBOLS_PER_BEAT    (1),
		.BITS_PER_SYMBOL     (1),
		.FIFO_DEPTH          (0),
		.CHANNEL_WIDTH       (0),
		.ERROR_WIDTH         (0),
		.USE_PACKETS         (1),
		.USE_FILL_LEVEL      (0),
		.EMPTY_LATENCY       (3),
		.USE_MEMORY_BLOCKS   (1),
		.USE_STORE_FORWARD   (0),
		.USE_ALMOST_FULL_IF  (0),
		.USE_ALMOST_EMPTY_IF (0)
	) avst_fifo_inst (
		.clk               (clk),                    
		.reset             (reset),            
		.in_data           (fifo_in_data[0]),                    
		.in_valid          (fifo_in_valid),                   
		.in_ready          (fifo_in_ready),                   
		.in_startofpacket  (fifo_in_sop),           
		.in_endofpacket    (fifo_in_eop),             
		.out_data          (wire_fifo_out_data),                   
		.out_valid         (fifo_out_valid),                  
		.out_ready         (fifo_out_ready),                  
		.out_startofpacket (fifo_out_sop),         
		.out_endofpacket   (fifo_out_eop));
        

// *********************************************************************************   
//      Command
// *********************************************************************************
	// Fix channel from csr controller is 2'b01 - 2'b10 for xip controller
	//assign cmd_channel = 2'b10;
    assign cmd_channel = 2'b01;

    always_comb begin
        cmd_eop_reg    = cmd_eop_combi;
        cmd_sop_reg    = cmd_sop_combi;
        cmd_data_reg   = cmd_data_combi;
        cmd_valid_reg  = cmd_valid_combi;
    end
            
    // Mux cmd output data between avst fifo and header
    always_comb begin
        //if (current_state == STATE_WR_DATA && ~(rsp_valid && rsp_eop)) begin
        if (current_state == STATE_WR_DATA) begin
            cmd_eop    = fifo_out_eop;
            cmd_sop    = '0;
            // If this is burst, read data from the fifo, if non-bursting which depend on byteenable
            // use the data that has been realignned
            // Note that the original data still goes in fifo, just pop out as usual
            // but do not use them to send the downstream component.
            cmd_data   = is_burst_reg ? fifo_out_data : fifo_in_data_wire;
            cmd_valid  = fifo_out_valid;
        end
        else begin
            cmd_eop    = cmd_eop_reg;
            cmd_sop    = cmd_sop_reg;
            cmd_data   = cmd_data_reg;
            cmd_valid  = cmd_valid_reg;
            
        end
    end // always_comb
        
    
// *********************************************************************************   
//      Response
// *********************************************************************************
    // Check Status Register/Flag Status Register to confirm device currently is not busy or complete the write operation
    always_comb begin
        busy = '0;
        if (current_state == STATE_STATUS_RSP && rsp_valid && rsp_eop)
            busy = rsp_data[0]; 
        else if (current_state == STATE_POLL_RSP && rsp_valid && rsp_eop)
            if (polling_opcode == 8'h70) // if this is flash status
                busy = ~rsp_data[7];
            else // status register
                busy = rsp_data[0];
    end

    // the dummy cycles is get from the CSR, which user is supposed to set up at read instruction register
    assign dummy_bytes_num = rd_dummy_cycles;
    assign read_has_dummy = (dummy_bytes_num == '0) ? 1'b0 : 1'b1;

    // Output data from read operation
    always_ff @(posedge clk or posedge reset) begin
        if (reset) begin
            mem_rddata      <= '0;
            mem_rddatavalid <= '0;
        end
        else begin
            if (current_state == STATE_READ_DATA) begin
                mem_rddata      <= rsp_data;
                mem_rddatavalid <= rsp_valid;
            end
            else begin
                mem_rddatavalid <= '0;
            end
        end
    end
    
endmodule


