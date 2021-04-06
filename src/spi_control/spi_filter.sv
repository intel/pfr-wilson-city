//=========================================================
//
//Copyright 2021 Intel Corporation
//
//Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
//
//The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
//
//THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
//
//===================


// spi_filter
//
// This module implements SPI filtering.  Incoming traffic on the SPI bus is monitored, and the 'o_spi_disable_cs'
// signal is asserted if a command that is not permitted is detected.  This signal is used to de-assert then
// SPI CSn signal from being asserted.
//
// There is a short list of permitted commands.  Most commands will be 'blocked' after the command byte has been
// sent, but before additional bytes required for the completeion of the command can be sent.  Some commands (such
// as die erase) will be blocked before the full command byte has been received, by necessity.
//
// Additionally, each 16kB section of FLASH memory has a bit stored in a local array that indicates whether write
// and erase commands are permitted to that section.  Any attempt to write to or erase an address that does not have
// write permission enabled will be blocked.  The local array that stores the write enable bit for each 16kB chunks
// of FLASH memory is written to by an AVMM interface.

`timescale 1 ps / 1 ps
`default_nettype none

module spi_filter #(
    parameter           ENABLE_IBB_DETECT   = 0,                            // when enabled, include circuitry to detect access to the IBB sector of flash as defined by the IBB_ADDRESS_MSBS parameter
    parameter [31:16]   IBB_ADDRESS_MSBS    = 16'h0000,                     // 16 msbs out of a 32 bit SPI address that indicate an access to the IBB sector
    parameter           FLASH_ADDRESS_BITS  = 27,                           // number of BYTE-based address bits supported by the FLASH device (27 bits = 128 MBytes = 1 Gbit)
    parameter           ENABLE_COMMAND_LOG  = 0,                            // set to 1 to enable logging of blocked commands
    parameter           ENABLE_3B_ADDR      = 1,                            // set to 1 to enable special support for 3 Byte address mode and detection of entrace to 4 Byte address mode
    parameter           USE_BMC_SPECIFIC_COMMANDS = 0,                      // set to 1 to enable commands specifically for the BMC but not the PCH.

    // the following parameters must all be left at their default values, they should not be modified
    parameter           AVMM_ADDRESS_BITS   = FLASH_ADDRESS_BITS-14-5       // -14 because we divide the FLASH into 16kB chunks, -5 because each 32 bit word provides info for 32 16kB chunks
) (
    input  wire                             clock,                          // system clock, avmm interface is synchronous with this clock
    input  wire                             i_resetn,                       // global reset signal, must deassert synchronous with clock
    
    // AVMM slave interface to access the write enable mask memory (note that reads from this memory are not supported) - synchronous with clock
    input  wire                             i_avmm_write,
    input  wire  [AVMM_ADDRESS_BITS-1:0]    i_avmm_address,
    input  wire  [31:0]                     i_avmm_writedata,
    
    // GPIO control and satus bits from/to the NIOS processor
    input  wire                             i_filter_disable,               // when asserted, all filtering functions are disabled, so all SPI commands are allowed
    output logic                            o_ibb_access_detected,          // asserted any time an address matching IBB_ADDRESS_MSBS is detected, stays asserted until i_clear_ibb_detected is asserted
    input  wire                             i_clear_ibb_detected,           // clears the o_ibb_access_detected bit
    input  wire                             i_addr_mode_set_3b,             // when asserted, forces the internal state of the filter into 3 Byte Addressing mode, the filter leaves this state when the ENTER_4B_ADDR_MODE command is detected (host should set this high then low again to enter 3B mode)
    output logic [31:0]                     o_filtered_command_info,        // 31 - 1=4-byte addressing mode, 0=3-byte addressing mode; 30:14 - address bits 30:14 of filtered command (invalid when Bit 13 == 1); 13 - 1=illegal command, 0=illegal write/erase region; 12:8- number of filtered commands; 7:0 - command that was filtered

    // SPI signals - synchronous with clk_spi_sclk
    input  wire                             clk_spi_sclk,                   // clock from the SPI interface, used to clock internal SPI filtering logic
    input  wire  [3:0]                      i_spi_data,                     // SPI data bits
    input  wire                             i_spi_csn,                      // active low chip select from the SPI master device
    output logic                            o_spi_disable_cs                // asserted by this block to 'block' the spi csn signal from being asserted to the external FLASH device
);

    localparam M9K_X1_ADDRESS_BITS              = FLASH_ADDRESS_BITS-14;    // one bit per 16kB block in the FLASH (16kB = 14 address bits)
    localparam NUM_M9K_BLOCKS                   = M9K_X1_ADDRESS_BITS > 13 ? 2**(M9K_X1_ADDRESS_BITS-13) : 1;   // number of M9K blocks required to implement the write enable mask, one M9K is 8k deep (13 address bits)

    // permitted SPI commands
    localparam [7:0] SPI_CMD_PAGE_PROGRAM           = 8'h02;
    localparam [7:0] SPI_CMD_READ_SLOW              = 8'h03;
    localparam [7:0] SPI_CMD_READ_SLOW_4BYTE        = 8'h13;
    localparam [7:0] SPI_CMD_WRITE_DISABLE          = 8'h04;
    localparam [7:0] SPI_CMD_READ_STATUS_REG        = 8'h05;
    localparam [7:0] SPI_CMD_WRITE_ENABLE           = 8'h06;
    localparam [7:0] SPI_CMD_READ_FAST              = 8'h0B;
    localparam [7:0] SPI_CMD_PAGE_PROGRAM_4BYTE     = 8'h12;
    localparam [7:0] SPI_CMD_ERASE_4K               = 8'h20;
    localparam [7:0] SPI_CMD_DUAL_OUT_FAST_RD       = 8'h3B;        // this command was not originally permitted in the HAS, but the BMC seems to use it during the boot sequence
    localparam [7:0] SPI_CMD_DUAL_OUT_FAST_RD_4BYTE = 8'h3C;
    localparam [7:0] SPI_CMD_READ_SFDP              = 8'h5A;
    localparam [7:0] SPI_CMD_QUAD_OUT_FAST_RD       = 8'h6B;
    localparam [7:0] SPI_READ_FLAG_STATUS_REGISTER  = 8'h70;
    localparam [7:0] SPI_CMD_READ_ID                = 8'h9F;
    localparam [7:0] SPI_CMD_ENTER_4B_ADDR_MODE     = 8'hB7;
    localparam [7:0] SPI_CMD_CLEAR_FLR              = 8'h50;
    localparam [7:0] SPI_CMD_SECTOR_ERASE       = 8'hD8;
    localparam [7:0] SPI_CMD_4B_SECTOR_ERASE       = 8'hDC;
    // the following commands were originally listed as permitted in the HAS, but agreement was reached to remove them
    //localparam [7:0] SPI_CMD_DUAL_INOUT_FAST_RD   = 8'hBB;
    //localparam [7:0] SPI_CMD_EXIT_4B_ADDR_MODE  = 8'hE9;      // this command MUST be blocked, as the filter does not support checking addresses in 3B address mode

    localparam NUM_PERMITTED_SPI_COMMANDS_DEFAULT           = 17;
    localparam [NUM_PERMITTED_SPI_COMMANDS_DEFAULT-1:0][7:0]  PERMITTED_SPI_COMMANDS_DEFAULT = 
        {   SPI_CMD_READ_SLOW_4BYTE     ,
            SPI_CMD_DUAL_OUT_FAST_RD    ,
            SPI_CMD_DUAL_OUT_FAST_RD_4BYTE,
            SPI_READ_FLAG_STATUS_REGISTER,
            SPI_CMD_CLEAR_FLR           ,
            SPI_CMD_PAGE_PROGRAM_4BYTE  ,
            SPI_CMD_ENTER_4B_ADDR_MODE  ,
            SPI_CMD_PAGE_PROGRAM        ,
            SPI_CMD_ERASE_4K            ,
            SPI_CMD_READ_SLOW           ,
            SPI_CMD_WRITE_DISABLE       ,
            SPI_CMD_READ_STATUS_REG     ,
            SPI_CMD_WRITE_ENABLE        ,
            SPI_CMD_READ_FAST           ,
            SPI_CMD_READ_SFDP           ,
            SPI_CMD_QUAD_OUT_FAST_RD    ,
            SPI_CMD_READ_ID             
        };

    // modify this if the BMC needs to have specific commands enabled that can't be enabled in general
    localparam NUM_PERMITTED_SPI_COMMANDS_BMC_SPECIFIC = 2;
    localparam [NUM_PERMITTED_SPI_COMMANDS_BMC_SPECIFIC-1:0][7:0]  PERMITTED_SPI_COMMANDS_BMC_SPECIFIC = 
    {
	// The Sector erase commands send an address and the sector containing that address is erased
	// We have changed the PFM for the BMC so that each page in a sector will have the same write permissions
	// This ensures that we can have sector erase that should have been blocked go through when we filter the address for this command
        SPI_CMD_SECTOR_ERASE,
        SPI_CMD_4B_SECTOR_ERASE
    };

    localparam NUM_PERMITTED_SPI_COMMANDS = (USE_BMC_SPECIFIC_COMMANDS) ? (NUM_PERMITTED_SPI_COMMANDS_DEFAULT + NUM_PERMITTED_SPI_COMMANDS_BMC_SPECIFIC) : NUM_PERMITTED_SPI_COMMANDS_DEFAULT;                                               
    
    localparam [(NUM_PERMITTED_SPI_COMMANDS_DEFAULT + NUM_PERMITTED_SPI_COMMANDS_BMC_SPECIFIC)-1:0][7:0]  PERMITTED_SPI_COMMANDS = { PERMITTED_SPI_COMMANDS_BMC_SPECIFIC,PERMITTED_SPI_COMMANDS_DEFAULT };
    
    // These indices are for the PERMITTED_SPI_COMMAND array, where 0 is the last entry (SPI_CMD_READ_ID). These indices are
    // used to identify the opcodes for specific commands.
    localparam SPI_CMD_PAGE_PROGRAM_INDEX = 9;
    localparam SPI_CMD_ERASE_4K_INDEX = 8;
    localparam SPI_CMD_ENTER_4B_ADDR_MODE_INDEX = 10;
    localparam SPI_CMD_PAGE_PROGRAM_4BYTE_INDEX = 11;


    // Indexes for BMC specific commands that we need to check the address for
    localparam SPI_CMD_SECTOR_ERASE_INDEX = NUM_PERMITTED_SPI_COMMANDS_DEFAULT + 1;
    localparam SPI_CMD_4B_SECTOR_ERASE_INDEX = NUM_PERMITTED_SPI_COMMANDS_DEFAULT + 0;
    
    // 1-byte SPI commands that must be blocked before the command completes
    // The purpose of enumerating these commands here is to verify that all forbidden 1-byte commands can be distinguished from all permitted commands with fewer than EARLY_COMPARE_COMMAND_BITS (see below) bits
    // This list currently includes all 1-byte commands for the Macronix MX25L51245G and the Micron MT25QL01GBBB
    localparam [7:0] SPI_CMD_ERASE_FAST_BOOT    = 8'h18;
    localparam [7:0] SPI_CMD_WRITE_SECURITY_REG = 8'h2F;
    localparam [7:0] SPI_CMD_PER_30             = 8'h30;
    localparam [7:0] SPI_CMD_ENTER_QUAD_IO      = 8'h35;
    localparam [7:0] SPI_CMD_FACTORY_MODE_EN    = 8'h41;
    localparam [7:0] SPI_CMD_CHIP_ERASE_60      = 8'h60;
    localparam [7:0] SPI_CMD_RESET_ENABLE       = 8'h66;
    //localparam [7:0] SPI_CMD_WRITE_PROTECT_SEL  = 8'h68;    // this command appears to have the same function as the permitted command WRITE_DISABLE (h04), and it matches the QUAD_OUT_FAST_READ command up to the 6th bit, so removing it from the forbidden list
    localparam [7:0] SPI_CMD_PES_75             = 8'h75;
    localparam [7:0] SPI_CMD_PER_7A             = 8'h7A;
    localparam [7:0] SPI_CMD_GANG_BLOCK_LOCK    = 8'h7E;
    //localparam [7:0] SPI_CMD_GANG_BLOCK_UNLOCK  = 8'h98;    // don't really need to block this as long as GANG_BLOCK_LOCK is blocked
    //localparam [7:0] SPI_CMD_RESET_MEMORY       = 8'h99;    // don't really need to block this as long as RESET_ENABLE (h66) command is blocked
    //localparam [7:0] SPI_CMD_INTERFACE_ACTIV    = 8'h9B;    // part of a multi-command sequence, probably OK not to block this one?
    localparam [7:0] SPI_CMD_WRITE_GLOBAL_FRZ   = 8'hA6;
    localparam [7:0] SPI_CMD_EXIT_DPD           = 8'hAB;
    localparam [7:0] SPI_CMD_PES_B0             = 8'hB0;
    localparam [7:0] SPI_CMD_ENTER_SECURE_OTP   = 8'hB1;
    localparam [7:0] SPI_CMD_ENTER_DPD          = 8'hB9;
    localparam [7:0] SPI_CMD_SET_BURST_LEN      = 8'hC0;
    localparam [7:0] SPI_CMD_EXIT_SECURE_OTP    = 8'hC1;
    localparam [7:0] SPI_CMD_CHIP_ERASE_C7      = 8'hC7;
    localparam [7:0] SPI_CMD_ERASE_NV_LOCK_BITS = 8'hE4;
    localparam [7:0] SPI_CMD_EXIT_4B_ADDR_MODE  = 8'hE9;
    localparam [7:0] SPI_CMD_EXIT_QUAD_IO       = 8'hF5;
    
    localparam NUM_FORBIDDEN_ONEBYTE_SPI_COMMANDS   = 21;
    localparam [NUM_FORBIDDEN_ONEBYTE_SPI_COMMANDS-1:0][7:0]  FORBIDDEN_ONEBYTE_SPI_COMMANDS = 
        {   SPI_CMD_ERASE_FAST_BOOT     ,
            SPI_CMD_WRITE_SECURITY_REG  ,
            SPI_CMD_PER_30              ,
            SPI_CMD_ENTER_QUAD_IO       ,
            SPI_CMD_FACTORY_MODE_EN     ,
            SPI_CMD_CHIP_ERASE_60       ,
            SPI_CMD_RESET_ENABLE        ,
            SPI_CMD_PES_75              ,
            SPI_CMD_PER_7A              ,
            SPI_CMD_GANG_BLOCK_LOCK     ,
            SPI_CMD_WRITE_GLOBAL_FRZ    ,
            SPI_CMD_EXIT_DPD            ,
            SPI_CMD_PES_B0              ,
            SPI_CMD_ENTER_SECURE_OTP    ,
            SPI_CMD_ENTER_DPD           ,
            SPI_CMD_SET_BURST_LEN       ,
            SPI_CMD_EXIT_SECURE_OTP     ,
            SPI_CMD_CHIP_ERASE_C7       ,
            SPI_CMD_ERASE_NV_LOCK_BITS  ,
            SPI_CMD_EXIT_4B_ADDR_MODE   ,
            SPI_CMD_EXIT_QUAD_IO       
        };  

    // number of bits required to distinguish all permitted commands from all forbidden one byte commands
    // setting this value to 5 means that no forbidden 1-byte command is identcial to any permitted command in the most significat 5 bits of the command (7:3)
    // if more permitted/forbidden commands are added in the future, this number may have to increase
    // increasing this number beyond 6 will reqire re-architecting the block, as currently there is a 1 clock delay before CS can be de-asserted 
    // with the current set, we can distinguish all permitted commands from all forbidden 1-byte commands with only 5 bits, so setting this number to 5
    localparam EARLY_COMPARE_COMMAND_BITS = 6;          
        
    ///////////////////////////////////////
    // Parameter checking
    //
    // Generate an error if any illegal parameter settings or combinations are used
    ///////////////////////////////////////
    initial /* synthesis enable_verilog_initial_construct */
    begin
        if (FLASH_ADDRESS_BITS < 13)    // minimum supported address space is 2 16kB chunks (would never have a FLASH device this small anyway)
            $fatal(1, "Illegal parameterization: expecting FLASH_ADDRESS_BITS >= 13");
        if (NUM_M9K_BLOCKS > 8)         // if this block is required to work with very large FLASH devices, consider enforcing that write enable segments be aligned to 64kB or larger blocks, rather than 16kB, to reduce the on-board RAM requirements for this block
            $fatal(1, "Illegal parameterization: FLASH_ADDRESS_BITS is too high, will require > 8 M9K blocks to implement write enable mask memory");
        if (AVMM_ADDRESS_BITS != FLASH_ADDRESS_BITS-14-5)
            $fatal(1, "Illegal parameterization: AVMM_ADDRESS_BITS should always be left with the default assignment");
        // verify that the EARLY_COMPARE_COMMAND_BITS are adequate to distinguish all permitted commands from all forbidden one byte commands
        for (int i=0; i<NUM_FORBIDDEN_ONEBYTE_SPI_COMMANDS; i++) begin
            for (int j=0; j<NUM_PERMITTED_SPI_COMMANDS; j++) begin
                if (FORBIDDEN_ONEBYTE_SPI_COMMANDS[i][7:8-EARLY_COMPARE_COMMAND_BITS] == PERMITTED_SPI_COMMANDS[j][7:8-EARLY_COMPARE_COMMAND_BITS])
                    $fatal(1, "Illegal parameterization: EARLY_COMPARE_COMMAND_BITS not large enough to distinguish all permitted SPI commands from all forbidden one-byte SPI commands");
            end
        end
    end
        
    
    ///////////////////////////////////////
    // spi monitoring circuit
    ///////////////////////////////////////
    
    // Internal signal to reset the SPI monitor block.  
    // This signal is used to asynchronously reset the state of the SPI monitoring circuit.  It must be applied asynchronously, since
    // the clock this circuit runs on (clk_spi_sclk) only toggles during 'active' phases on the SPI interface.
    // The reset is not synchronized to the clock and provides no protection against metastability.  However, we know that when i_resetn
    // is applied, we are in T-1 and so external devices are held in reset and thus clk_spi_sclk will not be toggling, and we also know
    // that when spi_csn deasserts clk_spi_sclk will not be toggling, thus it is safe to not provide synchronization on this signal.
    logic                                   spi_mon_resetn;
    assign spi_mon_resetn = i_resetn & ~i_spi_csn;
    
    // signals to connect to the write enable RAM
    logic                                   write_enable_mask_mem_dout;
    
    // monitor and capture incoming command and address
    logic [5:0]                             bit_count;                  // count each bit as it is received in an SPI command
    logic                                   command_ready;              // asserted after the first 8 bits have been received (the first 8 bits of an SPI sequence are the command byte)
    logic [7:0]                             command;                    // command received, used only for logging purposes
    logic [NUM_PERMITTED_SPI_COMMANDS-1:0]  permitted_command_match_n;  // each bit represents whether the command bits that have been received so far match the command at the same index in the PERMITTED_SPI_COMMANDS array (active low)
    logic                                   spi_16k_address_ready;      // asserted after all bits in the spi_16kb_address register have been received (ie all but the final 14 address bits)
    logic [M9K_X1_ADDRESS_BITS-1:0]         spi_16k_addr;               // spi command address in units of 16kB 'chunks' of memory, used during write and erase commands to determine if the command needs to be blocked due to the chunk being write only
    logic                                   write_enable_ready;         // asserted a few clock cycles after the 16kB address is valid, indicates the output of the write enable RAM is valid and can be used to mask disallowed write commands
    logic                                   block_write_command;        // asserted when a write or erase command has been received in an address range that does not allow write permission
    logic                                   address_mode_4b;            // current state of the SPI FLASH device interface, 3B address mode (0) or 4B address mode (1)

    always_ff @(posedge clk_spi_sclk or negedge spi_mon_resetn) begin
        if (~spi_mon_resetn) begin

            bit_count                   <= '0;
            command_ready               <= '0;
            command                     <= '0;
            permitted_command_match_n   <= '0;      // before any bits are received, all permitted commands are (potentially) a match, thus initialize this (active low) array to all 0's
            spi_16k_address_ready       <= '0;
            spi_16k_addr                <= '0;
            write_enable_ready          <= '0;
            o_spi_disable_cs            <= '0;

        end else begin
        
            // count the number of bits received during this SPI command, and set status bits at key count values
            if (bit_count != 6'b111111) begin
                bit_count <= bit_count + 6'd1;      // increment the counter, but do not let it wrap around (wrapping could create problems for the blocked command logging circuit)
            end
            if (bit_count == 6'd7) command_ready <= '1;                 // command consists of the first 8 bits received
            if (bit_count == 6'd25) spi_16k_address_ready <= '1;        // first 8 bits are command, next 32 bits are address, but we ignore the 14 lsbs of address to thet the 16kB address, thus 8+32-14 = 26 bits are required
            if (bit_count == 6'd27) write_enable_ready <= '1;           // allow two clock cycles for spi_16k_address to propogate through the write enable RAM and produce a valid output
            
            // compare each new bit as it comes in against each permitted command and determine if we still have a match
            for (int permitted_command_index = 0; permitted_command_index < NUM_PERMITTED_SPI_COMMANDS; permitted_command_index++) begin

                // if the full command has already been received, or if we already have found a bit that doesn't match, then further input bits are ignored
                if (command_ready | permitted_command_match_n[permitted_command_index]) begin
                
                    permitted_command_match_n[permitted_command_index] <= permitted_command_match_n[permitted_command_index];   // hold the current match value until we are reset and the next command starts

                // all bits so far have matched, and we have not received the full command, so keep comparing incoming bits against the expected value
                end else begin
                
                    if (i_spi_data[0] == PERMITTED_SPI_COMMANDS[permitted_command_index][3'h7 - bit_count[2:0]]) begin     // latest bit is also a match
                        permitted_command_match_n[permitted_command_index] <= '0;
                    end else begin                                                                                  // latest bit is not a match, set this match_n bit to 1
                        permitted_command_match_n[permitted_command_index] <= '1;
                    end
                    
                end
                
            end
            
            // shift each new bit into the address register until all required address bits have been received
            if (~spi_16k_address_ready) begin
                spi_16k_addr <= {spi_16k_addr[M9K_X1_ADDRESS_BITS-2:0], i_spi_data[0]};
            end
            
            // capture the command byte, for logging purposes
            if (~command_ready) begin
                command <= {command[6:0], i_spi_data[0]};
            end
            
            // determine when we need to 'block' the CSn signal
            // if filtering is enabled and we have no match with any permitted command, or this is a write command that needs to be blocked, then assert o_spi_disable_cs, leave it asserted until reset
            if ( ( (permitted_command_match_n == '1) || (block_write_command == '1) ) && (i_filter_disable == '0) ) begin
                o_spi_disable_cs <= '1;
            end
            
        end

    end

    generate
        if (USE_BMC_SPECIFIC_COMMANDS) begin
            always_ff @(posedge clk_spi_sclk or negedge spi_mon_resetn) begin
                if (~spi_mon_resetn) begin
                    block_write_command <= '0;
                end
                else begin
                    // determine if an illegal write command is occurring
                    if( write_enable_ready ) begin      // output from the write enable ram is valid
                        if ( (permitted_command_match_n[SPI_CMD_PAGE_PROGRAM_INDEX] == '0) || (permitted_command_match_n[SPI_CMD_ERASE_4K_INDEX] == '0) || (permitted_command_match_n[SPI_CMD_PAGE_PROGRAM_4BYTE_INDEX] == '0 ) || (permitted_command_match_n[SPI_CMD_SECTOR_ERASE_INDEX] == '0) || (permitted_command_match_n[SPI_CMD_4B_SECTOR_ERASE_INDEX] == '0)) begin     // this is a write command or an erase command
                            block_write_command <= ~write_enable_mask_mem_dout | ~address_mode_4b;      // block all write/erase commands when we are not in 4 byte address mode
                        end
                    end
                end
            end
        end
        else begin
            always_ff @(posedge clk_spi_sclk or negedge spi_mon_resetn) begin
                if (~spi_mon_resetn) begin
                    block_write_command <= '0;
                end
                else begin
                    // determine if an illegal write command is occurring
                    if( write_enable_ready ) begin      // output from the write enable ram is valid
                        if ( (permitted_command_match_n[SPI_CMD_PAGE_PROGRAM_INDEX] == '0) || (permitted_command_match_n[SPI_CMD_ERASE_4K_INDEX] == '0) || (permitted_command_match_n[SPI_CMD_PAGE_PROGRAM_4BYTE_INDEX] == '0 )) begin     // this is a write command or an erase command
                            block_write_command <= ~write_enable_mask_mem_dout | ~address_mode_4b;      // block all write/erase commands when we are not in 4 byte address mode
                        end
                    end
                end
            end
        end
    endgenerate
    
    // detect when the IBB address block is being accessed
    generate
        if (ENABLE_IBB_DETECT) begin
            
            logic ibb_addr_detected;          // asserted when an address in the IBB block is being accessed
            logic ibb_access_detectedn;
    
            always_ff @(posedge clk_spi_sclk or negedge spi_mon_resetn) begin
                if (~spi_mon_resetn) begin
                    ibb_addr_detected <= '0;
                end else begin
                    if (spi_16k_address_ready) begin
                        if (spi_16k_addr[M9K_X1_ADDRESS_BITS-1:2] == IBB_ADDRESS_MSBS[FLASH_ADDRESS_BITS-1:16]) begin    // spi_16k_addr lsb is 32-bit address bit 14, bit selections are done to properly 'align' the address bits
                            ibb_addr_detected <= '1;
                        end
                    end
                end
            end

            // this is a clock domain crossing, use ibb_addr_detected (generated on the clk_spi_sclk domain) to asynchronously clear the signal
            always_ff @(posedge clock or posedge ibb_addr_detected) begin
                if (ibb_addr_detected) begin
                    ibb_access_detectedn <= '0;
                end else begin
                    if (i_clear_ibb_detected) ibb_access_detectedn <= '1;
                end
            end

            // add metastability hardening to the multi-clock domain signal
            altera_std_synchronizer #(
                .depth(2)
            ) ibb_access_detectedn_synchronizer (
                .clk(clock), 
                .reset_n(1'b1), 
                .din(~ibb_access_detectedn), 
                .dout(o_ibb_access_detected)
            );            
            
        
        end else begin      // IBB Detection disabled
        
            assign o_ibb_access_detected = '0;
            
        end
    endgenerate

    generate
        if (ENABLE_3B_ADDR) begin

            // determine if we are in 3 Byte address mode or 4 Byte address mode (all write commands are blocked if we are in 3B address mode)
            always_ff @(posedge clk_spi_sclk or posedge i_addr_mode_set_3b) begin
                if (i_addr_mode_set_3b) begin
                    address_mode_4b <= '0;
                end else begin
                    if ((permitted_command_match_n[SPI_CMD_ENTER_4B_ADDR_MODE_INDEX] == '0) &&              // all bits of command received so far match 'ENTER_4B_ADDR_MODE' command
                        (bit_count == 6'd7) &&                                                              // we have received 7 of the 8 bits of the command
                        (i_spi_data[0] == PERMITTED_SPI_COMMANDS[SPI_CMD_ENTER_4B_ADDR_MODE_INDEX][0])      // the 8th bit also matches the 'ENTER_4B_ADDR_MODE' command
                    ) begin
                        address_mode_4b <= '1;          // we have received the 'ENTER_4B_ADDR_MODE' command, so switch internal state into 4B address mode
                    end
                end
            end
            
        end else begin
        
            // assume we are always in 4B address mode, local SPI master will have to guarantee this before exiting T-1 state
            assign address_mode_4b = '1;
        
        end
    endgenerate
    
    // capture the most recent filtered SPI command, and increment the counter tracking filtered commands
    generate
        if (ENABLE_COMMAND_LOG) begin

            logic spi_disable_cs_dly;                   // use this to determine the first clock cycle when o_spi_disable_cs is asserted
            
            always_ff @(posedge clk_spi_sclk or negedge i_resetn) begin
                if (~i_resetn) begin
                    o_filtered_command_info <= '0;
                    spi_disable_cs_dly <= '0;
                end else begin
                    spi_disable_cs_dly <= o_spi_disable_cs; 
                    if ( o_spi_disable_cs & ~spi_disable_cs_dly & command_ready ) begin         // this will be true for 1 clock cycle each time we filter out a command due to an illegal write address
                        o_filtered_command_info[31]                         <= address_mode_4b;             // internal filter state thinks we are in 4B (1) or 3B (0) address mode (all write commands are blocked in 3B address mode)
                        o_filtered_command_info[30:FLASH_ADDRESS_BITS]      <= '0;                          // unused address bits, set them to 0
                        o_filtered_command_info[FLASH_ADDRESS_BITS-1:14]    <= spi_16k_addr;                // captured address of the current transaction (will be invalid if we are blocking because of the command instead of blocking write to a certain address)
                        o_filtered_command_info[13]                         <= ~block_write_command;        // will be 1 when command was blocked because of command byte, 0 when blocked because of write/erase to write protected region
                        o_filtered_command_info[12:8]                       <= o_filtered_command_info[12:8] + 5'b00001;    // free-running counter, will roll over when it reaches maximum value
                        o_filtered_command_info[7:0]                        <= command;                     // command byte of command that was filtered
                    end else if ( o_spi_disable_cs && bit_count == 6'd7 ) begin                 // this will be true as the final bit of a blocked command is accepted
                        o_filtered_command_info[31]                         <= address_mode_4b;             // internal filter state thinks we are in 4B (1) or 3B (0) address mode (all write commands are blocked in 3B address mode)
                        o_filtered_command_info[30:FLASH_ADDRESS_BITS]      <= '0;                          // unused address bits, set them to 0
                        o_filtered_command_info[FLASH_ADDRESS_BITS-1:14]    <= spi_16k_addr;                // captured address of the current transaction (will be invalid if we are blocking because of the command instead of blocking write to a certain address)
                        o_filtered_command_info[13]                         <= ~block_write_command;        // will be 1 when command was blocked because of command byte, 0 when blocked because of write/erase to write protected region
                        o_filtered_command_info[12:8]                       <= o_filtered_command_info[12:8] + 5'b00001;    // free-running counter, will roll over when it reaches maximum value
                        o_filtered_command_info[7:0]                        <= {command[6:0], i_spi_data[0]}; // command byte of command that was filtered (must grab the last bit 'live' from the input, since we don't know if there will be any more clock cycles
                    end
                end
            end
            
        end else begin
        
            assign o_filtered_command_info = '0;
            
        end
    endgenerate
                
    altsyncram #(
        .address_aclr_b                     ( "NONE"                   ),
        .address_reg_b                      ( "CLOCK1"                 ),
        //.clock_enable_input_a               ( "BYPASS"                 ), // these 'BYPASS' settings are causing problems in simulation, should be fine with default values as all clken ports are tied-off to 1
        //.clock_enable_input_b               ( "BYPASS"                 ),
        //.clock_enable_output_b              ( "BYPASS"                 ),
        //.intended_device_family            .( "MAX 10"                 ),  // this setting shouldn't be needed, will come from the project - leaving it out makes this more generic
        .lpm_type                           ( "altsyncram"             ),
        .numwords_a                         ( 2**AVMM_ADDRESS_BITS     ),
        .numwords_b                         ( 2**M9K_X1_ADDRESS_BITS   ),
        .operation_mode                     ( "DUAL_PORT"              ),
        .outdata_aclr_b                     ( "NONE"                   ),
        .outdata_reg_b                      ( "CLOCK1"                 ),  // set to 'UNREGISTERED' for no output reg
        .power_up_uninitialized             ( "FALSE"                  ),
        .ram_block_type                     ( "AUTO"                   ),  // was set to 'M9K', changing to 'AUTO' to prevent any family incompatibility issues in the future
        .read_during_write_mode_mixed_ports ( "DONT_CARE"              ),
        .widthad_a                          ( AVMM_ADDRESS_BITS        ),
        .widthad_b                          ( M9K_X1_ADDRESS_BITS      ),
        .width_a                            ( 32                       ),
        .width_b                            ( 1                        ),
        .width_byteena_a                    ( 1                        )
    ) cmd_enable_mem (
        .address_a      ( i_avmm_address                ),
        .address_b      ( spi_16k_addr                  ),
        .clock0         ( clock                         ),
        .data_a         ( i_avmm_writedata              ),
        .data_b         ( '0                            ),
        .wren_a         ( i_avmm_write                  ),
        .wren_b         ( '0                            ),
        .q_a            (                               ),
        .q_b            ( write_enable_mask_mem_dout    ),
        .aclr0          ( '0                            ),
        .aclr1          ( '0                            ),
        .addressstall_a ( '0                            ),
        .addressstall_b ( '0                            ),
        .byteena_a      ( '1                            ),
        .byteena_b      ( '1                            ),
        .clock1         ( clk_spi_sclk                  ),
        .clocken0       ( '1                            ),
        .clocken1       ( '1                            ),
        .clocken2       ( '1                            ),
        .clocken3       ( '1                            ),
        .eccstatus      (                               ),
        .rden_a         ( '1                            ),
        .rden_b         ( '1                            )
    );

endmodule
