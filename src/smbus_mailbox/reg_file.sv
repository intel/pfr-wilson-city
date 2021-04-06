/////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Intel Corporation
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
/////////////////////////////////////////////////////////////////////////////////

// This is the register file that is accessed by the SMBus mailbox
// This module has 3 AVMM interfaces going into it. The PCH_* and BMC_* are SMBus interfaces that have been translated to a AVMM like interace
// This module supports single clock cycle reads so that the m0 AVMM interface does not require a readdatavalid signal
// reads and writes from the AVMM interface are independant of the reads and writes from the SMBus mailbox interfaces
// The SMBus interfaces must share access to the register file so they are arbitrated using the waitrequest signal
// the SMBus interfaces have restrictions on the addresses they can write, but the AVMM interface has full access

`timescale 1 ps / 1 ps
`default_nettype none


module reg_file (
                 input logic         clk,
                 input logic         resetn,

                 // AVMM interface, controlled by the embedded NIOS
                 // The data width of AVM master is 32 bits but the register file is 8 so the uppoer 24 bits are tied to 0
                 input logic         m0_read,
                 input logic         m0_write,
                 input logic  [7:0]  m0_writedata,
                 output logic [31:0] m0_readdata,
                 input logic  [7:0]  m0_address,
                 output logic        m0_readdatavalid,

                 // PCH/CPU interface. This is translated from SMBus to AVMM using i2c_slave.sv
                 input logic         pch_read,
                 input logic         pch_write,
                 input logic  [7:0]  pch_writedata,
                 output logic [31:0] pch_readdata,
                 input logic  [7:0]  pch_address,
                 output logic        pch_readdatavalid,
                 output logic        pch_waitrequest,
                 output logic        pch_invalid_cmd,

                 // BMC interface. This is translated from SMBus to AVMM using i2c_slave.sv
                 input logic         bmc_read,
                 input logic         bmc_write,
                 input logic  [7:0]  bmc_writedata,
                 output logic [31:0] bmc_readdata,
                 input logic  [7:0]  bmc_address,
                 output logic        bmc_readdatavalid,
                 output logic        bmc_waitrequest,
                 output logic        bmc_invalid_cmd

                 );


    // Signals for keeping track of when data is ready
    // used to determine if pch_readdata should be fed by the fifo output or the dual port ram output and when to set pch_readatavalid
    logic                            pch_readdatavalid_reg;
    logic                            pch_readdatavalid_next;

    // used to determine if BMC_readata should be fed by the fifo output or the dual port ram output and when to set bmc_readatavalid
    logic                            bmc_readdatavalid_reg;
    logic                            bmc_readdatavalid_next;
    
    // used to determine when data from the fifo is read
    // fifo data out is muxed with the output of the ram
    logic                            fifo_readvalid_reg;
    logic                            fifo_readvalid_next;

    // output of port a of the dual port ram
    logic [7:0]                      ram_porta_readdata;

    // inputs of port b of the dual port ram
    logic                            ram_portb_wren;
    logic [7:0]                      ram_portb_writedata;
    logic [7:0]                      ram_portb_address;

    // output of port b of the dual port ram
    logic [7:0]                      ram_portb_readdata;

    // used to tell if the pch or bmc is being served now
    // if a transaction is not being served, waitrequest is asserted 
    logic                            pch_transaction_active;
    logic                            bmc_transaction_active;

    // output of the address protection module, high if the address can be written to and low otherwise
    // signal is ignored for write transactions
    logic                            pch_address_writable;
    logic                            bmc_address_writable;

    // input to the fifo, all interfaces share a single fifo and if both the Nios and te pch/bmc try to use it at the same time it can get corrupted
    // the fifo should only be used in the factory durring provisioning so we are guaranteed a "nice" behavior
    logic [7:0]                      fifo_data_in;
    logic                            fifo_dequeue;
    logic                            fifo_enqueue;
    logic                            fifo_clear;

    // output of the fifo
    logic [7:0]                      fifo_data_out;



    // we need readdatavalid for the SMBus slaves to know when to grab data
    //assign pch_readdatavalid_next = pch_read & ~pch_waitrequest;
    //assign bmc_readdatavalid_next = bmc_read & ~bmc_waitrequest;

    //delays 1 clock cycle while waiting for the ram or fifo to respond to the read request
    always_ff @ (posedge clk or negedge resetn) begin
        if (~resetn) begin
            pch_readdatavalid <= 1'b0;
            bmc_readdatavalid <= 1'b0;
        end else begin
            pch_readdatavalid <= pch_read & ~pch_waitrequest;
            bmc_readdatavalid <= bmc_read & ~bmc_waitrequest;
        end
    end

    always_ff @ (posedge clk or negedge resetn) begin
        if (~resetn) begin
            m0_readdatavalid <= 1'b0;
        end
        else begin
            m0_readdatavalid <= m0_read;
        end
    end

    always_ff @ (posedge clk or negedge resetn) begin
        if (~resetn) begin
            fifo_readvalid_reg <= 1'b0;
        end
        else begin
            fifo_readvalid_reg <= fifo_readvalid_next;
        end
    end

    // assert waitrequest on the bus that is not currently being served
    // only relevant if 2 transactions come in simultaneously
    // priority goes pch -> bmc -> other bus
    always_comb begin
        bmc_waitrequest = 1'b0;
        pch_waitrequest = 1'b0;
        if (pch_read | pch_write) begin
            bmc_waitrequest = 1'b1;
        end
        else if (bmc_read | bmc_write) begin
            pch_waitrequest = 1'b1;
        end
    end

    // when read or write == 1 and waitrequest == 0 and the address conforms to the rules we are serving that transaction 
    assign pch_transaction_active = (pch_read || (pch_write && pch_address_writable)) && ~pch_waitrequest;
    assign bmc_transaction_active = (bmc_read || (bmc_write && bmc_address_writable)) && ~bmc_waitrequest;



    // pulse if command is invalid and SMBus slave should send a nack
    // signal is routed back up to the SMBus slave module
    always_comb begin
        if (pch_write & ~pch_waitrequest & ~pch_address_writable) begin
            pch_invalid_cmd = 1'b1;
        end
        else begin
            pch_invalid_cmd = 1'b0;
        end
    end

    always_comb begin
        if (bmc_write & ~bmc_waitrequest & ~bmc_address_writable) begin
            bmc_invalid_cmd = 1'b1;
        end
        else begin
            bmc_invalid_cmd = 1'b0;
        end
    end

    always_comb begin
        if (pch_transaction_active) begin
            ram_portb_address = pch_address;
            ram_portb_writedata = pch_writedata;
            ram_portb_wren = pch_write;
        end
        else if (bmc_transaction_active) begin
            ram_portb_address = bmc_address;
            ram_portb_writedata = bmc_writedata;
            ram_portb_wren = bmc_write;
        end
        else begin
            ram_portb_address = bmc_address;
            ram_portb_writedata = bmc_writedata;
            ram_portb_wren = 1'b0;
        end
    end

    always_comb begin
        pch_readdata[31:8] = 24'h0;       
        if (pch_readdatavalid && fifo_readvalid_reg) begin
            pch_readdata[7:0] = fifo_data_out;
        end
        else begin
            pch_readdata[7:0] = ram_portb_readdata; 
        end

    end

    always_comb begin
        bmc_readdata[31:8] = 24'b0;        
        if (bmc_readdatavalid && fifo_readvalid_reg) begin
            bmc_readdata[7:0] = fifo_data_out;
        end
        else begin
            bmc_readdata[7:0] = ram_portb_readdata;
        end
    end

    always_comb begin
        fifo_readvalid_next = 1'b0;
        if (
            (m0_read && (m0_address == platform_defs_pkg::WRITE_FIFO_ADDR || m0_address == platform_defs_pkg::READ_FIFO_ADDR))
            ||(pch_read && pch_transaction_active && (pch_address == platform_defs_pkg::WRITE_FIFO_ADDR || pch_address == platform_defs_pkg::READ_FIFO_ADDR)) 
            ||(bmc_read && bmc_transaction_active && (bmc_address == platform_defs_pkg::WRITE_FIFO_ADDR || bmc_address == platform_defs_pkg::READ_FIFO_ADDR))
            ) 
        begin
            fifo_readvalid_next = 1'b1;
        end
    end

    // mux driving m0_readdata
    // driven by RAM unless the read was from the FIFO
    always_comb begin
        m0_readdata[31:8] = 24'h0;
        if (m0_readdatavalid && fifo_readvalid_reg) begin
            m0_readdata[7:0] = fifo_data_out;
        end
        else begin
            m0_readdata[7:0] = ram_porta_readdata;
        end
    end

    // logic for determining the push and pop signals going into the fifo
    // data_in is driven by m0_writedata by default and is only driven by the pch/bmc when they read or write to 0x0B of 0x0C
    always_comb begin
        fifo_data_in = m0_writedata;
        fifo_enqueue = 1'b0;
        fifo_dequeue = 1'b0;
        if (m0_address == platform_defs_pkg::WRITE_FIFO_ADDR && m0_write) begin
            fifo_enqueue = 1'b1;
        end 
        else if (m0_address == platform_defs_pkg::READ_FIFO_ADDR && m0_read) begin
            fifo_dequeue = 1'b1;
        end
        else if (pch_address == platform_defs_pkg::WRITE_FIFO_ADDR && pch_write && pch_transaction_active) begin
            fifo_enqueue = 1'b1;
            fifo_data_in = pch_writedata;
        end
        else if (pch_address == platform_defs_pkg::READ_FIFO_ADDR && pch_read && pch_transaction_active) begin
            fifo_dequeue = 1'b1;
        end
        else if (bmc_address == platform_defs_pkg::WRITE_FIFO_ADDR && bmc_write && bmc_transaction_active) begin
            fifo_enqueue = 1'b1;
            fifo_data_in = bmc_writedata;
        end
        else if (bmc_address == platform_defs_pkg::READ_FIFO_ADDR && bmc_read && bmc_transaction_active) begin
            fifo_dequeue = 1'b1;
        end
    end

    // Clears the fifo if bits [2] or [1] are set in address 0xA
    // This is managed in hardware to simplifiy the software requirments of the NIOS
    always_comb begin
        fifo_clear = 1'b0;
        if (m0_address == platform_defs_pkg::COMMAND_TRIGGER_ADDR && m0_write && (|m0_writedata[2:1])
            || bmc_address == platform_defs_pkg::COMMAND_TRIGGER_ADDR && bmc_write && bmc_transaction_active && (|bmc_writedata[2:1])
            || pch_address == platform_defs_pkg::COMMAND_TRIGGER_ADDR && pch_write && pch_transaction_active && (|pch_writedata[2:1])) begin
            fifo_clear = 1'b1;
        end
    end

    // pass the addresses from the SMBus slaves in to determine if they are allowed to write to those addresses
    // conditions can be edited inside platform_defs_pkg.sv
    assign pch_address_writable = platform_defs_pkg::pch_mailbox_writable_address(pch_address);

    assign bmc_address_writable = platform_defs_pkg::bmc_mailbox_writable_address(bmc_address);

    // use a Dual port ram to act as the register file
    dp_ram dp_ram_0 (
                     .address_a(m0_address),
                     .address_b(ram_portb_address),
                     .clock(clk),
                     .data_a(m0_writedata),
                     .data_b(ram_portb_writedata),
                     .wren_a(m0_write),
                     .wren_b(ram_portb_wren),
                     .q_a(ram_porta_readdata),
                     .q_b(ram_portb_readdata)

                     );

    // Fifo 
    // anything less than 8x1023 will result in using 1 M9k
    fifo #(
            .DATA_WIDTH(8),
            .DATA_DEPTH(platform_defs_pkg::SMBUS_MAILBOX_FIFO_DEPTH)
            ) 
    u_fifo 
        (
            .clk(clk),
            .resetn(resetn),
            .data_in(fifo_data_in),
            .data_out(fifo_data_out),
            .clear(fifo_clear),
            .enqueue(fifo_enqueue),
            .dequeue(fifo_dequeue)
        );


endmodule
