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

`timescale 1 ps / 1 ps
`default_nettype none


module crypto256_top #(
    // Should the ECDSA block be used? If not, it is empty and the crypto block
    // is SHA only
    parameter USE_ECDSA_BLOCK = 1,
    parameter ECDSA_AUTHENTICATION_RESULT = 0   // This parameter allow user to choose the return result of ECDSA authentication when USE_ECDSA_BLOCK is set to 0.
                                                // 0 means ECDSA authentication return FAIL, 1 means ECDSA authentication return PASS
)(
    input wire clk,
    input wire areset,
    
    input wire [3:0] csr_address,
    output wire csr_waitrequest,
    input wire csr_read,
    input wire csr_write,
    output logic [31:0] csr_readdata,
    input wire [31:0] csr_writedata
    
);

localparam CRYPTO_LENGTH = 256;
localparam KEY_LENGTH = 2*256;
localparam SIGNATURE_LENGTH = 2*256;

// CSR Interface
// Word Address           | Description
//----------------------------------------------------------------------
// 0x01                   | 0: ECDSA + SHA start (WO)
//                        | 1: ECDSA + SHA done (RO, cleared on go)
//                        | 2: ECDSA signature good (RO, cleared on go)
//                        | 3 : Reserved
//                        | 4 : SHA-only start (WO)
//                        | 5 : SHA done (Cleared on start)
//                        | 31:6 : Reserved
//----------------------------------------------------------------------
// 0x02                   | Data (WO)
//----------------------------------------------------------------------
// 0x03                   | Data length in bytes, must be 64 byte aligned (lower 6 bits = 0) (WO)
//----------------------------------------------------------------------
// 0x08-0x0f              | 256 bit data register for SHA result (RO)
//                        | address 0x08 is the lsbs (31:0), address 0x0f is the msbs (255:224)


// NOTE that we do 'sloppy' address decoding in many cases, so writing to any undefined or read-only address is not allowed
localparam CSR_CSR_ADDRESS = 4'd1;
localparam CSR_SHA_DATA_ADDRESS = 4'd2;
localparam CSR_SHA_DATA_LENGTH_ADDRESS = 4'd3;

///////////////////////////////////////
// Parameter checking
//
// Generate an error if any illegal parameter settings or combinations are used
///////////////////////////////////////
initial /* synthesis enable_verilog_initial_construct */
begin
    if (USE_ECDSA_BLOCK != 0 && USE_ECDSA_BLOCK != 1) 
        $fatal(1, "%s:%0d illegal parameterization, expecting USE_ECDSA_BLOCK = 0 or 1", `__FILE__, `__LINE__);
end
    
    
// SHA processing FSM
typedef enum {
    SHA_WAIT_SHA_START,
    SHA_ACCEPT_DATA,
    SHA_ADD_PADDING_BLOCK,
    SHA_WAIT_DONE,
    SHA_DONE,

    EC_ACCEPT_DATA,

    ECDSA_WRITE_AX_INSTR,
    ECDSA_WRITE_AX,
    ECDSA_WRITE_AY_INSTR,
    ECDSA_WRITE_AY,
    ECDSA_WRITE_BX_INSTR,
    ECDSA_WRITE_BX,
    ECDSA_WRITE_BY_INSTR,
    ECDSA_WRITE_BY,
    ECDSA_WRITE_P_INSTR,
    ECDSA_WRITE_P,
    ECDSA_WRITE_A_INSTR,
    ECDSA_WRITE_A,
    ECDSA_WRITE_N_INSTR,
    ECDSA_WRITE_N,
    ECDSA_WRITE_R_INSTR,
    ECDSA_WRITE_R,
    ECDSA_WRITE_S_INSTR,
    ECDSA_WRITE_S,
    ECDSA_WRITE_E_INSTR,
    ECDSA_WRITE_E,
    
    ECDSA_WRITE_VALIDATE_INSTR,

    ECDSA_WAIT_DONE,
    ECDSA_DONE

} sha_state_t;
sha_state_t sha_state;
sha_state_t next_ec_state;


// Input block to the SHA and EC
reg [511:0] crypto_data_in;

// Instantiate SHA block
///////////////////////////////////////////////////////////////////////////////

// Reset for the SHA
reg sha_reset;

// Output of SHA valid
reg sha_output_valid;

// Input block valid
reg sha_input_valid;

// Signal start of new SHA operation
reg sha_input_start;

// Output SHA
wire [CRYPTO_LENGTH-1:0] sha_out;

// SHA processing done
reg sha_done;

// Length of data in bytes to SHA
reg [31:0] sha_data_length;


sha_unit u_sha (
    .clk(clk),
    .rst(!(areset || sha_reset)),
    .start(sha_input_start),
    .input_valid(sha_input_valid),
    .win({512'b0, crypto_data_in}),
    .hash_size(2'h1), // 2'h2 for 384 2'h1 for 256
    .output_valid(sha_output_valid),
    .hout(),
    .h_init_256(sha_out)
);


///////////////////////////////////////////////////////////////////////////////


// Instantiate EC block
///////////////////////////////////////////////////////////////////////////////

// Track pending SHA processing
reg pending_sha;

// 32-bit word slice counter from 512-bit block
reg [3:0] sha_data_slice;

wire crypto_done;

wire ecdsa_block_done;
wire ecdsa_block_busy;

wire ecdsa_block_dout_valid;
wire [255:0] ecdsa_block_cx;

wire [255:0] ecdsa_block_din;
wire ecdsa_block_din_valid;

reg ecdsa_block_sw_reset;
reg ecdsa_block_start;
wire ecdsa_block_instr_valid;
logic [4:0] ecdsa_block_instr;

reg [31:0] working_counter;

generate
    if (USE_ECDSA_BLOCK == 1)
    begin
        // Instantiate the ECDSA block
        ecdsa256_top u_ecdsa (
            .ecc_done(ecdsa_block_done),
            .ecc_busy(ecdsa_block_busy),
            .dout_valid(ecdsa_block_dout_valid),
            .cx_out(ecdsa_block_cx),
            .clk(clk),
            .resetn(!areset),
            .sw_reset(ecdsa_block_sw_reset),
            .ecc_start(ecdsa_block_start),
            .din_valid(ecdsa_block_din_valid),
            .ins_valid(ecdsa_block_instr_valid),
            .data_in(ecdsa_block_din),
            .ecc_ins(ecdsa_block_instr)
        );
    end
    else begin
        assign ecdsa_block_done = 1'b1;
        assign ecdsa_block_busy = 1'b0;
        assign ecdsa_block_dout_valid = 1'b0;
        assign ecdsa_block_cx = (ECDSA_AUTHENTICATION_RESULT == 1) ? 256'b0 : 256'b1;
    end
endgenerate

// Data-in is the crypto data in, except when writing in (E
// which is the sha
assign ecdsa_block_din = (sha_state == ECDSA_WRITE_E) ? sha_out : crypto_data_in[511:256];

// Data-in valid is when we are delivering any of the instructions with payloads
assign ecdsa_block_din_valid = (
    (sha_state == ECDSA_WRITE_AX) ||
    (sha_state == ECDSA_WRITE_AY) ||
    (sha_state == ECDSA_WRITE_BX) ||
    (sha_state == ECDSA_WRITE_BY) ||
    (sha_state == ECDSA_WRITE_P) || 
    (sha_state == ECDSA_WRITE_A) || 
    (sha_state == ECDSA_WRITE_N) || 
    (sha_state == ECDSA_WRITE_R) || 
    (sha_state == ECDSA_WRITE_S) || 
    (sha_state == ECDSA_WRITE_E)
);

// Decode states into the ECDSA instructions
always_comb begin
    case (sha_state)
        ECDSA_WRITE_AX_INSTR : ecdsa_block_instr = 5'd1;
        ECDSA_WRITE_AY_INSTR : ecdsa_block_instr = 5'd2;
        ECDSA_WRITE_BX_INSTR : ecdsa_block_instr = 5'd3;
        ECDSA_WRITE_BY_INSTR : ecdsa_block_instr = 5'd4;
        ECDSA_WRITE_P_INSTR  : ecdsa_block_instr = 5'd5;
        ECDSA_WRITE_A_INSTR  : ecdsa_block_instr = 5'd6;
        ECDSA_WRITE_N_INSTR  : ecdsa_block_instr = 5'd16;
        ECDSA_WRITE_R_INSTR  : ecdsa_block_instr = 5'd17;
        ECDSA_WRITE_S_INSTR  : ecdsa_block_instr = 5'd19;
        ECDSA_WRITE_E_INSTR  : ecdsa_block_instr = 5'd18;
        ECDSA_WRITE_VALIDATE_INSTR : ecdsa_block_instr = 5'd15;
        default ecdsa_block_instr = 5'bx;
    endcase
end

assign ecdsa_block_instr_valid = (
    (sha_state == ECDSA_WRITE_AX_INSTR) ||
    (sha_state == ECDSA_WRITE_AY_INSTR) ||
    (sha_state == ECDSA_WRITE_BX_INSTR) ||
    (sha_state == ECDSA_WRITE_BY_INSTR) ||
    (sha_state == ECDSA_WRITE_P_INSTR) || 
    (sha_state == ECDSA_WRITE_A_INSTR) || 
    (sha_state == ECDSA_WRITE_N_INSTR) || 
    (sha_state == ECDSA_WRITE_R_INSTR) || 
    (sha_state == ECDSA_WRITE_S_INSTR) || 
    (sha_state == ECDSA_WRITE_E_INSTR) || 
    (sha_state == ECDSA_WRITE_VALIDATE_INSTR)
);

///////////////////////////////////////////////////////////////////////////////



// Avalon-MM CSR
///////////////////////////////////////////////////////////////////////////////
wire ready_to_accept_sha_data;

// Handle writes for generic register file data. Data to SHA handled in the SHA
// FSM
always_ff @(posedge clk or posedge areset) begin
    if (areset) begin
        sha_data_length <= 32'b0;
    end
    else begin
        if (csr_write && csr_address[1:0] == CSR_SHA_DATA_LENGTH_ADDRESS[1:0]) begin
            // Enforce that only multiples of 128 are accepted
            sha_data_length <= {csr_writedata[31:6], 6'b0};
        end
    end
end

// Read access to CSR interface
reg ecdsa_good;



always_comb begin
    if (~csr_address[3]) begin                              // 'sloppy' decode of the one readable status register
        csr_readdata = {
                        26'b0, 
                        sha_done,
                        1'b0, //SHA start
                        1'b0, // Reserved
                        ecdsa_good,
                        crypto_done,
                        1'b0 // Crypto start
                        };    
    end else begin                                          // decode of addresses 0x08-0x0f (address bit 3 = 1)
        // convert read data to little-endian
        csr_readdata[7:0]   = sha_out[32*csr_address[2:0] + 24 +: 8];
        csr_readdata[15:8]  = sha_out[32*csr_address[2:0] + 16 +: 8];
        csr_readdata[23:16] = sha_out[32*csr_address[2:0] + 8  +: 8];
        csr_readdata[31:24] = sha_out[32*csr_address[2:0] + 0  +: 8];
    end
end

// Generate wait request to only occur when:
//  In reset
//  Writing to SHA_DATA_ADDRESS but we cannot accept more data. This is either because we are full, or adding padding
//  Note the 'sloppy' address decoding to save logic, we only look at the one lsb of the address to determine if we are writing to the SHA_DATA register(0x02) as opposed to the length(0x03) or control(0x01) register
assign ready_to_accept_sha_data = !(
            ((sha_state == SHA_ACCEPT_DATA) && (csr_address[0] == CSR_SHA_DATA_ADDRESS[0]) && (sha_data_slice == 4'b1111) && (pending_sha == 1'b1)) ||
            ((sha_state == SHA_WAIT_DONE) && (pending_sha == 1'b1) && (csr_address[0] == CSR_SHA_DATA_ADDRESS[0])) ||
            ((sha_state == EC_ACCEPT_DATA) && (pending_sha == 1'b1) && (csr_address[0] == CSR_SHA_DATA_ADDRESS[0])) || // Don't accept data until SHA is done
            ((sha_state == SHA_ADD_PADDING_BLOCK) && (csr_address[0] == CSR_SHA_DATA_ADDRESS[0])) ||
            // Don't accept data in any of the EC states except ACCEPT_DATA
            ((sha_state == ECDSA_WRITE_AX_INSTR) && (csr_address[0] == CSR_SHA_DATA_ADDRESS[0])) ||
            ((sha_state == ECDSA_WRITE_AX) && (csr_address[0] == CSR_SHA_DATA_ADDRESS[0])) ||
            ((sha_state == ECDSA_WRITE_AY_INSTR) && (csr_address[0] == CSR_SHA_DATA_ADDRESS[0])) ||
            ((sha_state == ECDSA_WRITE_AY) && (csr_address[0] == CSR_SHA_DATA_ADDRESS[0])) ||
            ((sha_state == ECDSA_WRITE_BX_INSTR) && (csr_address[0] == CSR_SHA_DATA_ADDRESS[0])) ||
            ((sha_state == ECDSA_WRITE_BX) && (csr_address[0] == CSR_SHA_DATA_ADDRESS[0])) ||
            ((sha_state == ECDSA_WRITE_BY_INSTR) && (csr_address[0] == CSR_SHA_DATA_ADDRESS[0])) ||
            ((sha_state == ECDSA_WRITE_BY) && (csr_address[0] == CSR_SHA_DATA_ADDRESS[0])) ||
            ((sha_state == ECDSA_WRITE_P_INSTR) && (csr_address[0] == CSR_SHA_DATA_ADDRESS[0])) ||
            ((sha_state == ECDSA_WRITE_P) && (csr_address[0] == CSR_SHA_DATA_ADDRESS[0])) ||
            ((sha_state == ECDSA_WRITE_A_INSTR) && (csr_address[0] == CSR_SHA_DATA_ADDRESS[0])) ||
            ((sha_state == ECDSA_WRITE_A) && (csr_address[0] == CSR_SHA_DATA_ADDRESS[0])) ||
            ((sha_state == ECDSA_WRITE_N_INSTR) && (csr_address[0] == CSR_SHA_DATA_ADDRESS[0])) ||
            ((sha_state == ECDSA_WRITE_N) && (csr_address[0] == CSR_SHA_DATA_ADDRESS[0])) ||
            ((sha_state == ECDSA_WRITE_R_INSTR) && (csr_address[0] == CSR_SHA_DATA_ADDRESS[0])) ||
            ((sha_state == ECDSA_WRITE_R) && (csr_address[0] == CSR_SHA_DATA_ADDRESS[0])) ||
            ((sha_state == ECDSA_WRITE_S_INSTR) && (csr_address[0] == CSR_SHA_DATA_ADDRESS[0])) ||
            ((sha_state == ECDSA_WRITE_S) && (csr_address[0] == CSR_SHA_DATA_ADDRESS[0])) ||
            ((sha_state == ECDSA_WRITE_E_INSTR) && (csr_address[0] == CSR_SHA_DATA_ADDRESS[0])) ||
            ((sha_state == ECDSA_WRITE_E) && (csr_address[0] == CSR_SHA_DATA_ADDRESS[0])) ||
            ((sha_state == ECDSA_WRITE_VALIDATE_INSTR) && (csr_address[0] == CSR_SHA_DATA_ADDRESS[0])) ||
            ((sha_state == ECDSA_WAIT_DONE) && (csr_address[0] == CSR_SHA_DATA_ADDRESS[0]))
            );

assign csr_waitrequest = areset ||
        (csr_write && !ready_to_accept_sha_data);

///////////////////////////////////////////////////////////////////////////////


// Crypto FSM
//
// Marshall data from the data register into the input words required to
// interact with the SHA and EC cores. 
//
// In SHA mode :
//   0) Expect the data length to be written
//   1) Wait for SHA start (CSR bit 4)
//   2) Accept SHA data until data length
//   3) Create reaquired padding word. This is a simplified word since
//      the input must be a multiple of 1024.
//   4) Accept expected SHA words
//   5) DONE: Set SHA_DONE and Match SHA is good
//
// In EC+SHA Mode
//   0) Expect the data length to be written
//   1) Wait for EC and SHA start (CSR bit 0). If bit 4 is also written then
//      ignore bit 4 and use only bit 0 starting EC+SHA.
//   2) Accept SHA data until data length
//   3) Create reaquired padding word. This is a simplified word since
//      the input must be a multiple of 1024.
//   4) Accept expected SHA words
//   5) Accept AX
//   6) Accept AY
//   7) Accept BX
//   8) Accept BY
//   9) Accept P
//   10) Accept A
//   11) Accept N
//   12) Accept R
//   13) Accept S
//   14) DONE: Set EC_DONE and Good EC is good

///////////////////////////////////////////////////////////////////////////////
reg [31:0] sha_data_length_remaining;
reg [31:0] sha_data_in_current_block;
reg sha_start_on_next_data;
reg ec_after_sha_mode;

always_ff @(posedge clk or posedge areset) begin
    if (areset) begin
        sha_reset <= 1'b0;
        sha_data_slice <= 4'b0;
        crypto_data_in <= 512'b0;
        sha_input_valid <= '0;
        sha_start_on_next_data <= 1'b0;
        sha_input_start <= 1'b0;
        pending_sha <= 1'b0;
        sha_data_length_remaining <= 32'b0;
        sha_data_in_current_block <= 32'b0;
        ec_after_sha_mode <= 1'b0;

        ecdsa_block_sw_reset <= 1'b0;

        ecdsa_good <= 1'b0;
        ecdsa_block_start <= 1'b0;


        sha_state <= SHA_WAIT_SHA_START;
        next_ec_state <= SHA_WAIT_SHA_START;
    end
    else begin
        sha_reset <= 1'b0;
        ecdsa_block_sw_reset <= 1'b0;
        sha_input_start <= 1'b0;
        sha_input_valid <= 1'b0;
        pending_sha <= (pending_sha & sha_output_valid) ? 1'b0 : pending_sha;
        sha_start_on_next_data <= sha_start_on_next_data;
        sha_data_slice <= sha_data_slice;
        ecdsa_block_start <= 1'b0;


        case (sha_state)
            SHA_WAIT_SHA_START, SHA_DONE, ECDSA_DONE :
                begin
                    if (csr_write && (csr_address == CSR_CSR_ADDRESS) && (csr_writedata[0] || csr_writedata[4])) begin
                        sha_start_on_next_data <= 1'b1;
                        sha_data_slice <= 4'b0; // Reset slice counter on start request
                        
                        // Load the counters based on the data length
                        sha_data_length_remaining <= sha_data_length;
                        sha_data_in_current_block <= sha_data_length;

                        // Reset the SHA and EC
                        sha_reset <= 1'b1;
                        ecdsa_block_sw_reset <= 1'b1;

                        ec_after_sha_mode <= csr_writedata[0];

                        // Reset EC good
                        ecdsa_good <= 1'b0;


                        // Next is to accept SHA data
                        sha_state <= SHA_ACCEPT_DATA;
                    end
                end
            SHA_ACCEPT_DATA :
                begin
                    if (csr_write && (csr_address[0] == CSR_SHA_DATA_ADDRESS[0]) && !csr_waitrequest) begin     // 'sloppy' address decode, only look at the lsb
                        // Accept a data word
                        // Store as big endian and account for Nios little endian alt_u32 read/write
                        crypto_data_in[32*(16-sha_data_slice)-8 +: 8] <= csr_writedata[7:0];
                        crypto_data_in[32*(16-sha_data_slice)-16 +: 8] <= csr_writedata[15:8];
                        crypto_data_in[32*(16-sha_data_slice)-24 +: 8] <= csr_writedata[23:16];
                        crypto_data_in[32*(16-sha_data_slice)-32 +: 8] <= csr_writedata[31:24];
                        
                        sha_data_slice <= sha_data_slice + 1'b1; // This will wrap back to 0 when the whole 512-bit word is written
                        sha_data_length_remaining <= sha_data_length_remaining - 32'd4;

                        // If the block is full then set the data valid. If this is also the first word, then
                        // also set the start
                        if (sha_data_slice == 4'b1111) begin
                            sha_input_valid <= 1'b1;
                            pending_sha <= 1'b1;
                            sha_data_in_current_block <= sha_data_in_current_block - 32'd64; // 512/8
    
                            if (sha_start_on_next_data) begin
                                sha_input_start <= 1'b1;
                                sha_start_on_next_data <= 1'b0;
                            end
                        end
                        
                        // If we have written all bytes, then write the padding in
                        // Since data is always a multiple of 1024, we add a complete
                        // padding block
                        if (sha_data_length_remaining == 32'd4) begin
                            sha_state <= SHA_ADD_PADDING_BLOCK;
                        end
                        else begin
                            sha_state <= SHA_ACCEPT_DATA;
                        end
                        
                    end
                end
            SHA_ADD_PADDING_BLOCK :
                begin
                    if (pending_sha == 1'b0) begin
                        // Middle 0 padding
                        crypto_data_in[510:64] <= 447'b0;
            
                        // Put 1'b1 in the first bit after the data
                        crypto_data_in[32'd511] <= 1'b1;
                        
                        // Write in the length
                        crypto_data_in[63:0] <= {29'b0, sha_data_length, 3'b0};
                        
                        // Mark the block for writing
                        sha_input_valid <= 1'b1;
                        pending_sha <= 1'b1;
                        
                        if (ec_after_sha_mode) begin
                            ecdsa_block_start <= 1'b1;

                            sha_data_slice <= 4'b0;
                            // Next state is to accept AY data
                            sha_state <= EC_ACCEPT_DATA;
                            next_ec_state <= ECDSA_WRITE_AX_INSTR;
                        end
                        else begin
                            // Reset the data slice counter and accept the accepted data
                            sha_data_slice <= 4'b0; 
                            sha_state <= SHA_WAIT_DONE;
                        end
                    end
                end
            SHA_WAIT_DONE :      // wait for the SHA calculation to finish
                begin
                    if (pending_sha) begin
                        sha_state <= SHA_WAIT_DONE;
                    end
                    else begin
                        sha_state <= SHA_DONE;
                    end
                end
            EC_ACCEPT_DATA :
                begin
                    if (csr_write && (csr_address[0] == CSR_SHA_DATA_ADDRESS[0]) && !csr_waitrequest) begin     // 'sloppy' address decode, only look at the lsb
                        // Accept a data word
                        // Store as big endian and account for Nios little endian alt_u32 read/write
                        crypto_data_in[32*(16-sha_data_slice)-8 +: 8] <= csr_writedata[7:0];
                        crypto_data_in[32*(16-sha_data_slice)-16 +: 8] <= csr_writedata[15:8];
                        crypto_data_in[32*(16-sha_data_slice)-24 +: 8] <= csr_writedata[23:16];
                        crypto_data_in[32*(16-sha_data_slice)-32 +: 8] <= csr_writedata[31:24];

                        sha_data_slice <= sha_data_slice + 1'b1;

                        // Expected EC is 256-bit (i.e. 8 32-bit words)
                        if (sha_data_slice == 4'b0111) begin
                            sha_state <= next_ec_state;
                        end
                    end
                end
            // Write in the AX instruction then the AX data
            ECDSA_WRITE_AX_INSTR:
                begin
                    sha_state <= ECDSA_WRITE_AX;
                end
            ECDSA_WRITE_AX:
                begin
                    sha_data_slice <= 4'b0; 
                    sha_state <= EC_ACCEPT_DATA;
                    next_ec_state <= ECDSA_WRITE_AY_INSTR;
                end
            // Write in the AY instruction then the AY data
            ECDSA_WRITE_AY_INSTR:
                begin
                    sha_state <= ECDSA_WRITE_AY;
                end
            ECDSA_WRITE_AY:
                begin
                    sha_data_slice <= 4'b0; 
                    sha_state <= EC_ACCEPT_DATA;
                    next_ec_state <= ECDSA_WRITE_BX_INSTR;
                end
            // Write in the BX instruction then the BX data
            ECDSA_WRITE_BX_INSTR:
                begin
                    sha_state <= ECDSA_WRITE_BX;
                end
            ECDSA_WRITE_BX:
                begin
                    sha_data_slice <= 4'b0; 
                    sha_state <= EC_ACCEPT_DATA;
                    next_ec_state <= ECDSA_WRITE_BY_INSTR;
                end
            // Write in the BY instruction then the BY data
            ECDSA_WRITE_BY_INSTR:
                begin
                    sha_state <= ECDSA_WRITE_BY;
                end
            ECDSA_WRITE_BY:
                begin
                    sha_data_slice <= 4'b0; 
                    sha_state <= EC_ACCEPT_DATA;
                    next_ec_state <= ECDSA_WRITE_P_INSTR;
                end
            // Write in the P instruction then the P data
            ECDSA_WRITE_P_INSTR:
                begin
                    sha_state <= ECDSA_WRITE_P;
                end
            ECDSA_WRITE_P:
                begin
                    sha_data_slice <= 4'b0; 
                    sha_state <= EC_ACCEPT_DATA;
                    next_ec_state <= ECDSA_WRITE_A_INSTR;
                end
            // Write in the A instruction then the A data
            ECDSA_WRITE_A_INSTR:
                begin
                    sha_state <= ECDSA_WRITE_A;
                end
            ECDSA_WRITE_A:
                begin
                    sha_data_slice <= 4'b0; 
                    sha_state <= EC_ACCEPT_DATA;
                    next_ec_state <= ECDSA_WRITE_N_INSTR;
                end
            // Write in the N instruction then the N data
            ECDSA_WRITE_N_INSTR:
                begin
                    sha_state <= ECDSA_WRITE_N;
                end
            ECDSA_WRITE_N:
                begin
                    sha_data_slice <= 4'b0; 
                    sha_state <= EC_ACCEPT_DATA;
                    next_ec_state <= ECDSA_WRITE_R_INSTR;
                end
            // Write in the R instruction then the R data
            ECDSA_WRITE_R_INSTR:
                begin
                    sha_state <= ECDSA_WRITE_R;
                end
            ECDSA_WRITE_R:
                begin
                    sha_data_slice <= 4'b0; 
                    sha_state <= EC_ACCEPT_DATA;
                    next_ec_state <= ECDSA_WRITE_S_INSTR;
                end
            // Write in the S instruction then the S data
            ECDSA_WRITE_S_INSTR:
                begin
                    sha_state <= ECDSA_WRITE_S;
                end
            ECDSA_WRITE_S:
                begin
                    sha_state <= ECDSA_WRITE_E_INSTR;
                end

            // Write in the E instruction then the E data
            ECDSA_WRITE_E_INSTR:
                begin
                    sha_state <= ECDSA_WRITE_E;
                end
            ECDSA_WRITE_E:
                begin
                    sha_state <= ECDSA_WRITE_VALIDATE_INSTR;
                end
                
            ECDSA_WRITE_VALIDATE_INSTR:
                begin
                    sha_state <= ECDSA_WAIT_DONE;
                    working_counter <= 32'b0;
                end

            ECDSA_WAIT_DONE:
                begin
                    working_counter <= working_counter + 1'b1;
                    if (ecdsa_block_done) begin
                        // Do we also need to look at dout_valid? Seems like it is defined to be 1
                        // when done is 1.
                        sha_state <= ECDSA_DONE;
                        
                        ecdsa_good <= (ecdsa_block_cx == 0);
                    end
                end

            default: sha_state <= SHA_WAIT_SHA_START;
        endcase
    end
end

always_ff @(posedge clk or posedge areset) begin
    if (areset)
        sha_done <= 1'b0;
    else
        sha_done <= (sha_state == SHA_DONE);
end

// crypto_done status is always 1 when ECDSA BLOCK is disabled
assign crypto_done = (sha_state == ECDSA_DONE) || (USE_ECDSA_BLOCK == 0);


//ERR_SHA_NEW_WORD_WITH_PENDING_SHA :
//   assert property (@(posedge clk) disable iff (areset) (!pending_sha |=> sha_input_start)) else $error;



endmodule
