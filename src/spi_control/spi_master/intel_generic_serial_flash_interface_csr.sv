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

`timescale 1 ns / 1 ns
module intel_generic_serial_flash_interface_csr #(
        parameter ADD_W                   = 6,
        //parameter CHIP_SELECT_BYPASS      = 0,
        parameter DEFAULT_VALUE_REG_0     = 32'b00000000000000000000000100000001,
        parameter DEFAULT_VALUE_REG_1     = 32'b00000000000000000000000000000011,
        parameter DEFAULT_VALUE_REG_2     = 32'b00000000000000000000000000000000,
        parameter DEFAULT_VALUE_REG_3     = 32'b00000000000000000000000000000000,
        parameter DEFAULT_VALUE_REG_4     = 32'b00000000000000000000000000000000,
        parameter DEFAULT_VALUE_REG_5     = 32'b00000000000000000000000000000011,
        parameter DEFAULT_VALUE_REG_6     = 32'b00000000000000000111000000000010,
        parameter DEFAULT_VALUE_REG_7     = 32'b00000000000000000001100000000101
    ) (
        input [ADD_W-1:0]   csr_addr, 
        input               csr_rd, 
        output logic [31:0] csr_rddata, 
        input               csr_wr, 
        input [31:0]        csr_wrdata, 
        output logic        csr_waitrequest, 
        output logic        csr_rddatavalid, 
        input               clk, 
        input               reset, 
        output logic [1:0]  cmd_channel, 
        output logic        cmd_eop, 
        input               cmd_ready, 
        output logic        cmd_sop, 
        output logic [31:0] cmd_data, 
        output logic        cmd_valid, 
        input [1:0]         rsp_channel, 
        input [31:0]        rsp_data, 
        input               rsp_eop, 
        output logic        rsp_ready, 
        input               rsp_sop, 
        input               rsp_valid,
        //input [3:0]         in_chip_select,
        output logic [31:0] addr_bytes_csr,
        output logic [3:0]  chip_select,
        output logic [1:0]  op_type, 
        output logic [1:0]  wr_addr_type,
        output logic [1:0]  wr_data_type,
        output logic [1:0]  rd_addr_type,
        output logic [1:0]  rd_data_type,
        output logic [7:0]  wr_en_opcode,
        output logic [7:0]  polling_opcode,
        output logic [2:0]  polling_bit,
        output logic [7:0]  wr_opcode,
        output logic [7:0]  rd_opcode,
        output logic [4:0]  rd_dummy_cycles,
        output logic        is_4bytes_addr_xip,
        output logic        qspi_interface_en,
        output logic [4:0]  baud_rate_divisor,
        output logic [7:0]  cs_delay_setting,
        output logic [3:0]  read_capture_delay

    );

    // +-------------------------------------------------------------------------------------------
    // | Internal signals for address decode
    // +-------------------------------------------------------------------------------------------
    //logic                           csr_control;
    //logic                           csr_clk_baud_rate;
    //logic                           csr_delay_setting;
    //logic                           csr_rd_capturing;
    logic                           csr_op_protocol;
    logic                           csr_rd_inst;
    logic                           csr_wr_inst;
    logic                           csr_flash_cmd_setting;
    logic                           csr_flash_cmd_control;
    logic                           csr_flash_cmd_addr;
    logic                           csr_flash_cmd_wr_data_0;
    logic                           csr_flash_cmd_wr_data_1;

    //logic                           wr_csr_control;
    //logic                           wr_csr_clk_baud_rate;
    //logic                           wr_csr_delay_setting;
    //logic                           wr_csr_rd_capturing;
    logic                           wr_csr_op_protocol;
    logic                           wr_csr_rd_inst;
    logic                           wr_csr_wr_inst;
    logic                           wr_csr_flash_cmd_setting;
    logic                           wr_csr_flash_cmd_control;
    logic                           wr_csr_flash_cmd_addr;
    logic                           wr_csr_flash_cmd_wr_data_0;
    logic                           wr_csr_flash_cmd_wr_data_1;

    // These registers to store the user input 
    logic [31:0]                    csr_control_data;
    logic [31:0]                    csr_clk_baud_rate_data;
    logic [31:0]                    csr_delay_setting_data;
    logic [31:0]                    csr_rd_capturing_data;
    logic [1:0]                     csr_op_protocol_data_17_16;
    logic [1:0]                     csr_op_protocol_data_13_12;
    logic [1:0]                     csr_op_protocol_data_9_8;
    logic [1:0]                     csr_op_protocol_data_5_4;
    logic [1:0]                     csr_op_protocol_data_1_0;
    logic [12:0]                    csr_rd_inst_data;
    logic [18:0]                    csr_wr_inst_data;
    // Not support those indirect yet
    //logic                           csr_indirect_rd_setting;
    //logic                           csr_indirect_rd_addr;
    //logic                           csr_indirect_rd_control;
    //logic                           csr_indirect_wr_setting;
    //logic                           csr_indirect_wr_addr;
    //logic                           csr_indirect_wr_control;
    //logic                           wr_csr_indirect_rd_setting;
    //logic                           wr_csr_indirect_rd_addr;
    //logic                           wr_csr_indirect_rd_control;
    //logic                           wr_csr_indirect_wr_setting;
    //logic                           wr_csr_indirect_wr_addr;
    //logic                           wr_csr_indirect_wr_control;
    //logic                           rd_csr_indirect_rd_setting;
    //logic                           rd_csr_indirect_rd_addr;
    //logic                           rd_csr_indirect_rd_control;
    //logic                           rd_csr_indirect_wr_setting;
    //logic                           rd_csr_indirect_wr_addr;
    //logic                           rd_csr_indirect_wr_control;
    //logic [31:0]                    csr_indirect_rd_setting_data;
    //logic [31:0]                    csr_indirect_rd_addr_data;
    //logic [31:0]                    csr_indirect_rd_control_data;
    //logic [31:0]                    csr_indirect_wr_setting_data;
    //logic [31:0]                    csr_indirect_wr_addr_data;
    //logic [31:0]                    csr_indirect_wr_control_data;
    logic [20:0]                    csr_flash_cmd_setting_data;
    logic [31:0]                    csr_flash_cmd_addr_data;
    logic [31:0]                    csr_flash_cmd_wr_data_0_data;
    logic [31:0]                    csr_flash_cmd_wr_data_1_data;
    logic [31:0]                    csr_flash_cmd_rd_data_0_data;
    logic [31:0]                    csr_flash_cmd_rd_data_1_data;
    
    logic                           csr_waitrequest_local;
    logic [ADD_W-1:0]               avl_addr;
    logic                           avl_rd;        
    logic                           avl_wr;          
    logic [31:0]                    avl_wrdata;
    logic [31:0]                    avl_rddata;
    logic [31:0]                    avl_rddata_local;
    logic                           avl_waitrequest;
    logic                           avl_rddatavalid;
    logic                           avl_rddatavalid_local;
    logic                           flash_operation;
    logic                           flash_operation_reg;
    logic [31:0]                    header_information;
    logic [31:0]                    header_to_sent;
    logic [7:0]                     opcode;
    logic [3:0]                     numb_data;
    //logic [2:0]                     numb_addr;
    logic [4:0]                     numb_dummy;
    logic                           is_4bytes_addr;
    logic                           has_addr;
    logic                           has_dummy;
    logic                           has_data_in;
    logic                           has_data_out;
    logic                           more_than_4bytes_data;
    logic                           baud_rate_all_zero;
    logic                           baud_rate_larger_16;
    logic [31:0]                    rdata_comb;
    logic [3:0]                     local_chip_select;
    // State machine
    typedef enum bit [4:0]
    {
     ST_IDLE         = 5'b00001,
     ST_SEND_HEADER  = 5'b00010,
     ST_SEND_DATA_0  = 5'b00100,
     ST_SEND_DATA_1  = 5'b01000,
     ST_WAIT_RSP     = 5'b10000
     } t_state;
    t_state state, next_state;

    // Just to make it easy to read
    assign avl_addr         = csr_addr;
    assign avl_wr           = csr_wr;
    assign avl_rd           = csr_rd;
    assign avl_wrdata       = csr_wrdata;
    assign csr_waitrequest  = avl_waitrequest;
    assign csr_rddatavalid  = avl_rddatavalid;
    assign csr_rddata       = avl_rddata;

     
    // +-------------------------------------------------------------------------------------------
    // | Access CSR decoding logic
    // +-------------------------------------------------------------------------------------------
    always_comb begin
        //csr_control              = (csr_addr == 6'd0);
        //csr_clk_baud_rate        = (csr_addr == 6'd1);
        //csr_delay_setting        = (csr_addr == 6'd2);
        //csr_rd_capturing         = (csr_addr == 6'd3);
        csr_op_protocol          = (csr_addr == 6'd4);
        csr_rd_inst              = (csr_addr == 6'd5);
        csr_wr_inst              = (csr_addr == 6'd6);
        csr_flash_cmd_setting    = (csr_addr == 6'd7);
        csr_flash_cmd_control    = (csr_addr == 6'd8);
        csr_flash_cmd_addr       = (csr_addr == 6'd9);
        csr_flash_cmd_wr_data_0  = (csr_addr == 6'd10);
        csr_flash_cmd_wr_data_1  = (csr_addr == 6'd11);
        //csr_indirect_rd_setting  = (csr_addr == 6'd14);
        //csr_indirect_rd_addr     = (csr_addr == 6'd15);
        //csr_indirect_rd_control  = (csr_addr == 6'd16);
        //csr_indirect_wr_setting  = (csr_addr == 6'd17);
        //csr_indirect_wr_addr     = (csr_addr == 6'd18);
        //csr_indirect_wr_control  = (csr_addr == 6'd19);
    end
    // |
    // +-------------------------------------------------------------------------------------------
    
    // +-------------------------------------------------------------------------------------------
    // | CSR write and read transaction 
    // +-------------------------------------------------------------------------------------------
    always_comb begin
        //wr_csr_control              = avl_wr && !avl_waitrequest && csr_control;
        //wr_csr_clk_baud_rate        = avl_wr && !avl_waitrequest && csr_clk_baud_rate ;
        //wr_csr_delay_setting        = avl_wr && !avl_waitrequest && csr_delay_setting;
        //wr_csr_rd_capturing         = avl_wr && !avl_waitrequest && csr_rd_capturing;
        wr_csr_op_protocol          = avl_wr && !avl_waitrequest && csr_op_protocol;
        wr_csr_rd_inst              = avl_wr && !avl_waitrequest && csr_rd_inst;
        wr_csr_wr_inst              = avl_wr && !avl_waitrequest && csr_wr_inst;

        //rd_csr_indirect_rd_setting  = avl_rd && !avl_waitrequest && csr_indirect_rd_setting;
        //wr_csr_indirect_rd_setting  = avl_wr && !avl_waitrequest && csr_indirect_rd_setting;
        //rd_csr_indirect_rd_addr     = avl_rd && !avl_waitrequest && csr_indirect_rd_addr;
        //wr_csr_indirect_rd_addr     = avl_wr && !avl_waitrequest && csr_indirect_rd_addr;
        //rd_csr_indirect_rd_control  = avl_rd && !avl_waitrequest && csr_indirect_rd_control;
        //wr_csr_indirect_rd_control  = avl_wr && !avl_waitrequest && csr_indirect_rd_control && (avl_wrdata == 32'h1);;
        //rd_csr_indirect_wr_setting  = avl_rd && !avl_waitrequest && csr_indirect_wr_setting;
        //wr_csr_indirect_wr_setting  = avl_wr && !avl_waitrequest && csr_indirect_wr_setting;
        //rd_csr_indirect_wr_addr     = avl_rd && !avl_waitrequest && csr_indirect_wr_addr;
        //wr_csr_indirect_wr_addr     = avl_wr && !avl_waitrequest && csr_indirect_wr_addr;
        //rd_csr_indirect_wr_control  = avl_rd && !avl_waitrequest && csr_indirect_wr_control;
        //wr_csr_indirect_wr_control  = avl_wr && !avl_waitrequest && csr_indirect_wr_control && (avl_wrdata == 32'h1);;
        
        wr_csr_flash_cmd_setting    = avl_wr && !avl_waitrequest && csr_flash_cmd_setting;
        wr_csr_flash_cmd_control    = avl_wr && !avl_waitrequest && csr_flash_cmd_control && (avl_wrdata == 32'h1);
        wr_csr_flash_cmd_addr       = avl_wr && !avl_waitrequest && csr_flash_cmd_addr;
        wr_csr_flash_cmd_wr_data_0  = avl_wr && !avl_waitrequest && csr_flash_cmd_wr_data_0;
        wr_csr_flash_cmd_wr_data_1  = avl_wr && !avl_waitrequest && csr_flash_cmd_wr_data_1;
    end
    // |
    // +-------------------------------------------------------------------------------------------

    
    // +-------------------------------------------------------------------------------------------
    // | Build the header that contain all infomration for that flash command
    // | -> this is the format that is used by ASMI2, dont change or else downstream component will be upset
    // | 32'b[reserved_bits][data_bytes_bin][dummy_bytes_bin][has_dummy][has_data_out][has_data_in][4bytes_addr][has_addr][opcode_bin]
    // | -> For this generic spi component, the information is get from the control_register "csr_flash_cmd_setting_data" with format like this
    // | bit    - field 
    // | 31:20	Reserved
    // | 19:16	Number of dummy cycles
    // | 15:12	Number of data bytes 
    // | 11	    Data type
    // | 10:8	Number of address bytes 
    // | 7:0	Opcode

    // +-------------------------------------------------------------------------------------------
    // These are default setting for some registers upon reset. User can set these vua UI
    //DEFAULT_VALUE_REG_0  {Control Register}
    //DEFAULT_VALUE_REG_1  {Clock Baudrate Register}
    //DEFAULT_VALUE_REG_2  {Chip Select Delay Register}
    //DEFAULT_VALUE_REG_3  {Read Capturing Register}
    //DEFAULT_VALUE_REG_4  {Protocol Settings Register}
    //DEFAULT_VALUE_REG_5  {Read Instruction Register}
    //DEFAULT_VALUE_REG_6  {Write Instruction Register}
    //DEFAULT_VALUE_REG_7  {Flash Command Setting Register}

    // +-------------------------------------------------------------------------------------------
    // | Operation selection: There are many CSR access, some talk to the device, some do not.
    // +-------------------------------------------------------------------------------------------
    assign flash_operation  = wr_csr_flash_cmd_control;
    
    always_ff @(posedge clk or posedge reset) begin
        if (reset)
            flash_operation_reg <= '0;
        else
            flash_operation_reg <= flash_operation;
    end
    // | 
    // +-------------------------------------------------------------------------------------------
    // Control register
    /* hard-code the control data register
    always_ff @(posedge clk or posedge reset) begin
        if (reset)
            csr_control_data <= DEFAULT_VALUE_REG_0; // default, turn on the IP and select first chip select.
        else begin
            // if bit 0 is 0, means disable qspi output then do not record the chip select
            // if bit 0 is 1, recored the chip select value
            if (wr_csr_control) begin
                csr_control_data <= avl_wrdata;
                if (avl_wrdata[0] == 1)
                    csr_control_data[7:4] <= avl_wrdata[7:4];
            end
        end // else: !if(reset)
    end // always_ff @
    */
    assign csr_control_data = 32'h00000101;     // 4B addressing, CS=0, block is enabled
    always_comb begin
        is_4bytes_addr_xip  = csr_control_data[8];
        // enable signal
        qspi_interface_en   = csr_control_data[0];
        // If turn on chip select, then  together with sending the value to downsteam component
        // send this value to export chip select interface so that the XIP can use
        local_chip_select   = csr_control_data[7:4];
    end
    // Chip select: normaly it is regsiter base controller, only for ASMI backward compatibility
    // to ease the implemenation, the chip select is direct from user. this will overwrite from the controller register.
    assign chip_select = local_chip_select;
    
    // Flash command setting register
    always_ff @(posedge clk or posedge reset) begin
        if (reset)
            // at reset, set up the command setting register to do a read status
            csr_flash_cmd_setting_data <= DEFAULT_VALUE_REG_7[20:0];
        else begin
            if (wr_csr_flash_cmd_setting)
                csr_flash_cmd_setting_data <= avl_wrdata[20:0];
        end
    end // always_ff @
 
    always_ff @(posedge clk) begin
        opcode                <= csr_flash_cmd_setting_data[7:0];
        //numb_addr             <= csr_flash_cmd_setting_data[10:8];
        numb_data             <= csr_flash_cmd_setting_data[15:12];
        numb_dummy            <= csr_flash_cmd_setting_data[20:16];
        has_addr              <= (csr_flash_cmd_setting_data[10:8] == 3'h0) ? 1'b0 : 1'b1;
        is_4bytes_addr        <= (csr_flash_cmd_setting_data[10:8] == 3'h4) ? 1'b1 : 1'b0;
        has_dummy             <= (csr_flash_cmd_setting_data[20:16] == 4'h0) ? 1'b0 : 1'b1;
        more_than_4bytes_data <= (csr_flash_cmd_setting_data[15:12] > 4'd4) ? 1'b1 : 1'b0;
        if (csr_flash_cmd_setting_data[15:12] == 4'h0) begin
            has_data_in  <= 1'b0;
            has_data_out <= 1'b0;
        end
        else begin
            if (csr_flash_cmd_setting_data[11] == 1'b0) begin
                has_data_in  <= 1'b1;
                has_data_out <= 1'b0;
            end
            else begin
                has_data_in  <= 1'b0;
                has_data_out <= 1'b1;
            end
        end // else: !if(csr_flash_cmd_setting_data[15:12] == 4'h0)
    end // always_ff @

    // reconstruct the header to send downstream
    //assign header_to_sent ={1'b0, csr_control_data[7:4], 5'h0, numb_data, numb_dummy, has_dummy, has_data_out, has_data_in, is_4bytes_addr, has_addr, opcode};
    assign header_to_sent ={1'b0, chip_select, 5'h0, numb_data, numb_dummy, has_dummy, has_data_out, has_data_in, is_4bytes_addr, has_addr, opcode};
    // baud-rate register, only register the value from 1 - 16
    // other than that, ignore

    /* hard code baud rate register 
    assign baud_rate_all_zero   = !(|avl_wrdata[4:0]);
    assign baud_rate_larger_16  = (avl_wrdata[4] == 1'b1) && (|avl_wrdata[3:0] == 1'b1);
    always_ff @(posedge clk or posedge reset) begin
        if (reset)
            csr_clk_baud_rate_data <= DEFAULT_VALUE_REG_1;
        else begin
            if (wr_csr_clk_baud_rate) begin
                if (baud_rate_all_zero || baud_rate_larger_16)
                    csr_clk_baud_rate_data <= csr_clk_baud_rate_data;
                else 
                    csr_clk_baud_rate_data <= {27'h0, avl_wrdata[4:0]};
            end
        end // else: !if(reset)
    end // always_ff @
    */
    assign csr_clk_baud_rate_data = 32'h00000001;       // set baud rate divisor to divide by 2
    // assign the value diviosor to the outout
    assign baud_rate_divisor  = csr_clk_baud_rate_data[4:0];

    /* hard code delay setting
    // delay setting
    always_ff @(posedge clk or posedge reset) begin
        if (reset)
            csr_delay_setting_data <= DEFAULT_VALUE_REG_2;
        else begin
            if (wr_csr_delay_setting)
                csr_delay_setting_data <= {24'h0, avl_wrdata[7:0]};
        end
    end // always_ff @
    */
    assign csr_delay_setting_data = 32'h00000000;
    // assign CS delay to output
    assign cs_delay_setting  = csr_delay_setting_data[7:0];

    /*
    // Read delay capturing register
    always_ff @(posedge clk or posedge reset) begin
        if (reset)
            csr_rd_capturing_data <= DEFAULT_VALUE_REG_3;
        else begin
            if (wr_csr_rd_capturing)
                csr_rd_capturing_data <= {28'h0, avl_wrdata[3:0]};
        end
    end // always_ff @
    */
    assign csr_rd_capturing_data = 32'h00000000;
    // assign CS delay to output
    assign read_capture_delay = csr_rd_capturing_data[3:0];
    
    
    always_ff @(posedge clk or posedge reset) begin
        if (reset) begin 
            csr_flash_cmd_addr_data      <= '0;
            csr_flash_cmd_wr_data_0_data <= '0;
            csr_flash_cmd_wr_data_1_data <= '0;
        end
        else begin
            if (wr_csr_flash_cmd_addr)
                csr_flash_cmd_addr_data <= avl_wrdata;
            if (wr_csr_flash_cmd_wr_data_0)
                csr_flash_cmd_wr_data_0_data <= avl_wrdata;
            if (wr_csr_flash_cmd_wr_data_1)
                csr_flash_cmd_wr_data_1_data <= avl_wrdata;
        end
    end // always_ff @
    // Protocol setting register: data line for opcode, address or data
    always_ff @(posedge clk or posedge reset) begin
        if (reset) begin
            csr_op_protocol_data_17_16[1:0] <= 2'b00;
            csr_op_protocol_data_13_12[1:0] <= 2'b00;
            csr_op_protocol_data_9_8[1:0]   <= 2'b00;
            csr_op_protocol_data_5_4[1:0]   <= 2'b00;
            csr_op_protocol_data_1_0[1:0]   <= 2'b00;
            //csr_op_protocol_data <= DEFAULT_VALUE_REG_4;
        end else begin
            if (wr_csr_op_protocol) begin
                csr_op_protocol_data_17_16[1:0] <= avl_wrdata[17:16];
                csr_op_protocol_data_13_12[1:0] <= avl_wrdata[13:12];
                csr_op_protocol_data_9_8[1:0]   <= avl_wrdata[9:8];
                csr_op_protocol_data_5_4[1:0]   <= avl_wrdata[5:4];
                csr_op_protocol_data_1_0[1:0]   <= avl_wrdata[1:0];
            end
        end
    end // always_ff @
    // set the protocol setting to the qspi interface component
    always_comb begin
        op_type       = csr_op_protocol_data_1_0; 
        wr_addr_type  = csr_op_protocol_data_5_4;
        wr_data_type  = csr_op_protocol_data_9_8;
        rd_addr_type  = csr_op_protocol_data_13_12;
        rd_data_type  = csr_op_protocol_data_17_16;
    end
    
    // Write instruction register
    // Default, it polls flasg status register (for Micron), other flash user can overwrite this.
    always_ff @(posedge clk or posedge reset) begin
        if (reset)
            csr_wr_inst_data <= DEFAULT_VALUE_REG_6[18:0];
            //csr_wr_inst_data <= 32'h00007002;
        else begin
            if (wr_csr_wr_inst)
                csr_wr_inst_data <= avl_wrdata[18:0];
        end
    end // always_ff @
    
    // Read instruction register
    always_ff @(posedge clk or posedge reset) begin
        if (reset)
            csr_rd_inst_data <= DEFAULT_VALUE_REG_5[12:0];
            //csr_rd_inst_data <= 32'h00000003;
        else begin
            if (wr_csr_rd_inst)
                csr_rd_inst_data <= avl_wrdata[12:0];
        end
    end // always_ff @

    // pass to the xip controller
    always_comb begin    
        // fix this is write enable
        wr_en_opcode      = 8'h06;
        // not used for now
        polling_bit       = csr_wr_inst_data[18:16];
        polling_opcode    = csr_wr_inst_data[15:8];
        wr_opcode         = csr_wr_inst_data[7:0];
        
        rd_opcode         = csr_rd_inst_data[7:0];
        rd_dummy_cycles   = csr_rd_inst_data[12:8];
    end
    
    // read data counter
    logic [1:0] read_data_cnt;
    always_ff @(posedge clk or posedge reset) begin
        if (reset) 
            read_data_cnt <= '0; 
        else begin
            if (state == ST_IDLE)
                read_data_cnt <= '0; 
            else begin
                if ((state == ST_WAIT_RSP) && rsp_valid)
                    read_data_cnt <= read_data_cnt + 2'h1;
            end
        end // else: !if(reset)
    end // always_ff @

    // pass the read data to each allocated register :read_data_0 and read_data_1
    always_ff @(posedge clk or posedge reset) begin
        if (reset) begin
            csr_flash_cmd_rd_data_0_data <= '0;
            csr_flash_cmd_rd_data_1_data <= '0;
        end
        else begin
            if (state == ST_WAIT_RSP) begin
                if (read_data_cnt == 2'h0)
                    csr_flash_cmd_rd_data_0_data <= rsp_data;
                if (read_data_cnt == 2'h1)
                    csr_flash_cmd_rd_data_1_data <= rsp_data;
            end
        end // else: !if(reset)
    end // always_ff @
    
    // | 
    // +-------------------------------------------------------------------------------------------

    // +-------------------------------------------------------------------------------------------
    // | Output 
    // +-------------------------------------------------------------------------------------------
    // | If the operation need address, then set value from write data to the address byte interface
    assign addr_bytes_csr = has_addr ? csr_flash_cmd_addr_data : 32'h0;

    // | Fix channel from csr controller is 2'b01 - 2'b10 for xip controller
    //assign cmd_channel    = 2'b01;
    assign cmd_channel  = 2'b10;
    // | Alwasy take in response, avalon master has no backpressure on data so it is safe
    // assign rsp_ready     = 1;
    // | Wait request 
    // assert waitrequest when in reset
    logic hold_waitrequest;
    always_ff @(posedge clk or posedge reset) begin
        if (reset) 
            hold_waitrequest <= 1'h1; 
        else 
            hold_waitrequest <= 1'h0; 
    end
    assign avl_waitrequest  = !(state == ST_IDLE) || flash_operation_reg || hold_waitrequest;
    assign avl_rddatavalid  = avl_rddatavalid_local;
    assign avl_rddata       = avl_rddata_local;

    // combinatorial read data signal declaration
    always_ff @(posedge clk or posedge reset) begin
        if (reset) 
            avl_rddata_local[31:0] <= 32'h0; 
        else 
            avl_rddata_local[31:0] <= rdata_comb[31:0];
    end

    // read data is always returned on the next cycle
    always_ff @(posedge clk or posedge reset) begin
        if (reset) 
            avl_rddatavalid_local <= 1'b0; 
        else 
            avl_rddatavalid_local <= avl_rd && !avl_waitrequest;
    end

    // Avalon readdata logic
    always_comb begin
        rdata_comb = '0;
        if (avl_rd) begin 
            case (avl_addr)
                6'd0: begin // control register
                    rdata_comb  = csr_control_data;
                end
                6'd1: begin // clock baund rate
                    rdata_comb  = csr_clk_baud_rate_data;
                end
                6'd2: begin // delay setting 
                    rdata_comb  = csr_delay_setting_data;
                end
                6'd3: begin // read capturing 
                    rdata_comb  = csr_rd_capturing_data;
                end
                6'd4: begin // operation protocol
                    rdata_comb  = {14'h0000, csr_op_protocol_data_17_16[1:0], 2'b00, csr_op_protocol_data_13_12[1:0], 2'b00, csr_op_protocol_data_9_8[1:0], 2'b00, csr_op_protocol_data_5_4[1:0], 2'b00, csr_op_protocol_data_1_0[1:0]};
                end
                6'd5: begin // read instruction
                    rdata_comb  = {18'h00000, csr_rd_inst_data[12:0]};
                end
                6'd6: begin // write instruction
                    rdata_comb  = {13'h0000, csr_wr_inst_data[18:0]};
                end
                6'd7: begin // flash command setting
                    rdata_comb  = {11'h0000, csr_flash_cmd_setting_data[20:0]};
                end
                6'd8: begin // flash command control
                    rdata_comb  = '0;
                end
                6'd9: begin // flash command address
                    rdata_comb  = csr_flash_cmd_addr_data;
                end
                6'd10: begin // flash command write data 0
                    rdata_comb  = csr_flash_cmd_wr_data_0_data;
                end
                6'd11: begin // flash command write data 1
                    rdata_comb  = csr_flash_cmd_wr_data_1_data;
                end
                6'd12: begin // flash command read data 0
                    rdata_comb  = csr_flash_cmd_rd_data_0_data;
                end
                6'd13: begin // flash command read data 1
                    rdata_comb  = csr_flash_cmd_rd_data_1_data;
                end
                6'd14: begin // IER
                    rdata_comb  = '0;
                end
                6'd15: begin // ISR
                    rdata_comb  = '0;
                end
                6'd16: begin // indirect read setting
                    //rdata_comb  = csr_indirect_rd_setting_data;
                    rdata_comb  = '0;
                end
                6'd17: begin // indirect read address
                    //rdata_comb  = csr_indirect_rd_addr_data;
                    rdata_comb  = '0;
                end
                6'd18: begin // indirect read control 
                    //rdata_comb  = csr_indirect_rd_control_data;
                    rdata_comb  = '0;
                end
                6'd19: begin // indirect write setting
                    //rdata_comb  = csr_indirect_wr_setting_data;
                    rdata_comb  = '0;
                end
                6'd20: begin // indirect write address
                    //rdata_comb  = csr_indirect_wr_addr_data;
                    rdata_comb  = '0;
                end
                6'd21: begin // indirect write control
                    //rdata_comb  = csr_indirect_wr_control_data;
                    rdata_comb  = '0;
                end
                default: begin 
                    rdata_comb  = '0;
                end
            endcase
        end
    end
    // | 
    // +-------------------------------------------------------------------------------------------
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
                if (flash_operation_reg)
                    next_state = ST_SEND_HEADER;
            end
            ST_SEND_HEADER: begin 
                next_state = ST_SEND_HEADER;
                if (cmd_ready) begin
                    if (has_data_in)
                        next_state  = ST_SEND_DATA_0;
                    else
                        next_state  = ST_WAIT_RSP;
                end
            end
            ST_SEND_DATA_0: begin
                next_state  = ST_SEND_DATA_0;
                if (cmd_ready) begin 
                    if (more_than_4bytes_data)
                        next_state  = ST_SEND_DATA_1;
                    else
                        next_state  = ST_WAIT_RSP;
                end
            end
            
            ST_SEND_DATA_1: begin
                next_state  = ST_SEND_DATA_1;
                if (cmd_ready)
                    next_state  = ST_WAIT_RSP;
            end
            
            ST_WAIT_RSP : begin
                next_state  = ST_WAIT_RSP;
                if (rsp_valid && rsp_ready && rsp_eop)
                    next_state  = ST_IDLE;
            end
        endcase // case (state)
    end // always_comb
    
    // +--------------------------------------------------
    // | State Machine: state outputs
    // +--------------------------------------------------
    always_comb begin
        cmd_valid       = '0;
        cmd_data        = '0;
        cmd_sop         = '0;
        cmd_eop         = '0;
        rsp_ready       = '0;
        case (state)
            ST_IDLE: begin
                cmd_valid       = '0;
                cmd_data        = '0;
                cmd_sop         = '0;
                cmd_eop         = '0;
                rsp_ready       = '0;
            end
            ST_SEND_HEADER: begin 
                cmd_valid       = 1'b1;
                rsp_ready       = '0;
                // overwrite the chip select value
                cmd_data        = header_to_sent;
                cmd_sop         = 1'b1;
                if (has_data_in)
                    cmd_eop         = '0;
                else
                    cmd_eop         = 1'b1;
            end 
            ST_SEND_DATA_0: begin 
                cmd_valid       = 1'b1;
                cmd_data        = csr_flash_cmd_wr_data_0_data;
                cmd_sop         = '0;
                cmd_eop         = more_than_4bytes_data ? 1'b0 : 1'b1;
                rsp_ready       = '0;
            end
            ST_SEND_DATA_1: begin 
                cmd_valid       = 1'b1;
                cmd_data        = csr_flash_cmd_wr_data_1_data;
                cmd_sop         = '0;
                cmd_eop         = 1'b1;
                rsp_ready       = '0;
            end
            ST_WAIT_RSP : begin 
                cmd_valid       = '0;
                cmd_data        = '0;
                cmd_sop         = '0;
                cmd_eop         = '0;
                rsp_ready       = 1'b1;
            end
        endcase // case (state)
    end
endmodule

