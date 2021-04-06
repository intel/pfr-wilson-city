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

// smbus_filtered_relay
//
// This module implements an SMBus relay between a single master and multiple
// slave devices, with filtering to enable only whitelisted commands to be sent 
// to each slave.
//
// The module uses clock stretching on the interface from the SMBus master
// to allow time for the slave to respond with ACK and read data.
//
// Required files:
//      async_input_filter.sv


`timescale 1 ps / 1 ps
`default_nettype none

module smbus_filtered_relay #(
    parameter FILTER_ENABLE         = 1,            // Set to 1 to enable SMBus command filtering, 0 to allow all commands
    parameter RELAY_ALL_ADDRESSES   = 0,            // set to 1 to allow commands to any address to pass through the relay (FILTER_ENABLE must be set to 0 in this mode)
    parameter CLOCK_PERIOD_PS       = 10000,        // period of the input 'clock', in picoseconds (default 10,000 ps = 100 MHz)
    parameter BUS_SPEED_KHZ         = 100,          // bus speed of the slowest device on the slave SMBus interface, in kHz (must be 100, 400, or 1000)
    parameter NUM_RELAY_ADDRESSES   = 1,            // number of addresses to pass through the relay, must be >= 1
    parameter IGNORED_ADDR_BITS     = 0,            // number of SMBus Address least significant bits to ignore when determining which relay address is being addressed
    parameter [NUM_RELAY_ADDRESSES:1][6:0] SMBUS_RELAY_ADDRESS = {(NUM_RELAY_ADDRESSES){7'h0}}
) (
    input  wire                            clock,              // master clock for this block
    input  wire                            i_resetn,           // master reset, must be de-asserted synchronously with clock
    input  wire                            i_block_disable,    // when asserted, disables this entire block such that it drives no SMBus signals
    input  wire                            i_filter_disable,   // when asserted, disables command filtering
    
    // Master SMBus interface signals
    // This module acts as a slave on this bus, and accepts commands from an external SMBus Master
    // Each signal on this bus (scl and sda) has an input that comes from the device pin, and an output that turns on a tri-state
    // driver which causes that pin to be driven to 0 by this device.
    input  wire                            ia_master_scl,      // asynchronous input from the SCL pin of the master interface
    output logic                           o_master_scl_oe,    // when asserted, drive the SCL pin of the master interface low
    input  wire                            ia_master_sda,      // asynchronous input from the SDA pin of the master interface
    output logic                           o_master_sda_oe,    // when asserted, drive the SDA pin of the master interface low

    // Slave SMBus interface signals
    // This module acts as a master on this bus, forwarding commands from the SMBus master
    // Each signal on this bus (scl and sda) has an input that comes from the device pin, and an output that turns on a tri-state
    // driver which causes that pin to be driven to 0 by this device.
    input  wire                            ia_slave_scl,       // asynchronous input from the SCL pin of the master interface
    output logic                           o_slave_scl_oe,     // when asserted, drive the SCL pin of the master interface low
    input  wire                            ia_slave_sda,       // asynchronous input from the SDA pin of the master interface
    output logic                           o_slave_sda_oe,     // when asserted, drive the SDA pin of the master interface low    

    // AVMM slave interface to access the command enable memory (note that reads from this memory are not supported)
    input  wire                            i_avmm_write,
    input  wire [7:0]                      i_avmm_address,
    input  wire [31:0]                     i_avmm_writedata
);

    ///////////////////////////////////////
    // Parameter checking
    //
    // Generate an error if any illegal parameter settings or combinations are used
    ///////////////////////////////////////
    initial /* synthesis enable_verilog_initial_construct */
    begin
        if (FILTER_ENABLE != 0 && FILTER_ENABLE != 1) 
            $fatal(1, "Illegal parameterization: expecting FILTER_ENABLE = 0 or 1");
        if (RELAY_ALL_ADDRESSES != 0 && RELAY_ALL_ADDRESSES != 1) 
            $fatal(1, "Illegal parameterization: expecting RELAY_ALL_ADDRESSES = 0 or 1");
        if (RELAY_ALL_ADDRESSES == 1 && FILTER_ENABLE == 1) 
            $fatal(1, "Illegal parameterization: FILTER_ENABLE must be 0 if RELAY_ALL_ADDRESSES is 1");
        if (CLOCK_PERIOD_PS < 10000 || CLOCK_PERIOD_PS > 50000)     // This is somewhat arbitrary, assuming a clock between 20 MHz (for minimum 20x over-sampling) and 100 MHz
            $fatal(1, "Illegal parameterization: requre 10000 <= CLOCK_PERIOD_PS <= 50000");
        if (BUS_SPEED_KHZ != 100 && BUS_SPEED_KHZ != 400 && BUS_SPEED_KHZ != 1000) 
            $fatal(1, "Illegal parameterization: expecting BUS_SPEED_KHZ = 100, 400, or 1000");
        if (IGNORED_ADDR_BITS < 0 || IGNORED_ADDR_BITS > 6)
            $fatal(1, "Illegal parameterization: expecting 0 <= IGNORED_ADDR_BITS <= 6");
        if (NUM_RELAY_ADDRESSES <= 0 || NUM_RELAY_ADDRESSES > 32)
            $fatal(1, "Illegal parameterization: require NUM_RELAY_ADDRESSES > 0 and NUM_RELAY_ADDRESSES <= 32");
        if (NUM_RELAY_ADDRESSES <= 32 && NUM_RELAY_ADDRESSES > 16)
            $fatal(1, "Illegal parameterization: NUM_RELAY_ADDRESSES values > 16 and NUM_RELAY_ADDRESSES <= 32 have not been fully tested. Users are recommended to perform additional verification on this block before deploying this feature.");
        for (int i=1; i<=NUM_RELAY_ADDRESSES-1; i++) begin
            for (int j=i+1; j<=NUM_RELAY_ADDRESSES; j++) begin
                if (SMBUS_RELAY_ADDRESS[i][6:IGNORED_ADDR_BITS]==SMBUS_RELAY_ADDRESS[j][6:IGNORED_ADDR_BITS]) 
                    $fatal(1, "Illegal parameterization: all SMBUS_RELAY_ADDRESS entries must be unique (after IGNORED_ADDR_BITS lsbs are ignored)");
            end
        end
    end


    ///////////////////////////////////////
    // Calculate timing parameters
    // Uses timing parameters from the SMBus specification version 3.1
    ///////////////////////////////////////

    // minimum time to hold the SCLK signal low, in clock cycles
    // minimum time to hold SCLK high is less than this, but we will use this same number to simplify logic
    localparam int SCLK_HOLD_MIN_COUNT =
        BUS_SPEED_KHZ == 100 ? ((5000000 + CLOCK_PERIOD_PS - 1) / CLOCK_PERIOD_PS) :
        BUS_SPEED_KHZ == 400 ? ((1300000 + CLOCK_PERIOD_PS - 1) / CLOCK_PERIOD_PS) :
                               (( 500000 + CLOCK_PERIOD_PS - 1) / CLOCK_PERIOD_PS)  ;
    localparam int SCLK_HOLD_COUNTER_BIT_WIDTH = $clog2(SCLK_HOLD_MIN_COUNT)+1;             // counter will count from SCLK_HOLD_MIN_COUNT-1 downto -1

    localparam int NOISE_FILTER_MIN_CLOCK_CYCLES = ((50000 + CLOCK_PERIOD_PS - 1) / CLOCK_PERIOD_PS);           // SMBus spec says to filter out pulses smaller than 50ns
    localparam int NUM_FILTER_REGS = NOISE_FILTER_MIN_CLOCK_CYCLES >= 2 ? NOISE_FILTER_MIN_CLOCK_CYCLES : 2;    // always use at least 2 filter registers, to eliminate single-cycle pulses that might be caused during slow rising/falling edges




    ///////////////////////////////////////
    // Synchronize asynchronous SMBus input signals to clock and detect edges and start/stop conditions
    ///////////////////////////////////////

    // synchronized, metastability hardened versions of the scl and sda input signals for internal use in the rest of this module
    logic master_scl;
    logic master_sda;
    logic slave_scl;
    logic slave_sda;
    
    // detection of positive and negative edges, asserted synchronous with the first '1' or '0' on the master/slave scl signals defined below
    logic master_scl_posedge;
    logic master_scl_negedge;
    logic slave_scl_posedge ;
    logic slave_scl_negedge ;
    logic master_sda_posedge;
    logic master_sda_negedge;
    logic slave_sda_posedge ;
    logic slave_sda_negedge ;

    // SDA signals are delayed by 1 extra clock cycle, to provide a small amount of hold timing when sampling SDA on the rising edge of SCL
    async_input_filter #(
        .NUM_METASTABILITY_REGS (2),
        .NUM_FILTER_REGS        (NUM_FILTER_REGS)
    ) sync_master_scl_inst (
        .clock                  ( clock ),
        .ia_async_in            ( ia_master_scl ),
        .o_sync_out             ( master_scl ),
        .o_rising_edge          ( master_scl_posedge ),
        .o_falling_edge         ( master_scl_negedge )
    );

    async_input_filter #(
        .NUM_METASTABILITY_REGS (2),
        .NUM_FILTER_REGS        (NUM_FILTER_REGS+1)
    ) sync_master_sda_inst (
        .clock                  ( clock ),
        .ia_async_in            ( ia_master_sda ),
        .o_sync_out             ( master_sda ),
        .o_rising_edge          ( master_sda_posedge ),
        .o_falling_edge         ( master_sda_negedge )
    );
    
    async_input_filter #(
        .NUM_METASTABILITY_REGS (2),
        .NUM_FILTER_REGS        (NUM_FILTER_REGS)
    ) sync_slave_scl_inst (
        .clock                  ( clock ),
        .ia_async_in            ( ia_slave_scl ),
        .o_sync_out             ( slave_scl ),
        .o_rising_edge          ( slave_scl_posedge ),
        .o_falling_edge         ( slave_scl_negedge )
    );
    
    async_input_filter #(
        .NUM_METASTABILITY_REGS (2),
        .NUM_FILTER_REGS        (NUM_FILTER_REGS+1)
    ) sync_slave_sda_inst (
        .clock                  ( clock ),
        .ia_async_in            ( ia_slave_sda ),
        .o_sync_out             ( slave_sda ),
        .o_rising_edge          ( slave_sda_posedge ),
        .o_falling_edge         ( slave_sda_negedge )
    );
    
    
    // detect start and stop conditions on the master bus, asserted 1 clock cycle after the start/stop condition actually occurs
    logic master_start;
    logic master_stop;
    logic master_scl_dly;       // delayed version of master_scl by 1 clock cycle
    
    always_ff @(posedge clock or negedge i_resetn) begin
        if (!i_resetn) begin
            master_start        <= '0;
            master_stop         <= '0;
            master_scl_dly      <= '0;
        end else begin
            master_start        <= master_scl & master_sda_negedge;      // falling edge on SDA while SCL high is a start condition
            master_stop         <= master_scl & master_sda_posedge;      // rising edge on SDA while SCL high is a stop condition
            master_scl_dly      <= master_scl;
        end
    end
    

    ///////////////////////////////////////
    // Track the 'phase' of the master SMBus
    ///////////////////////////////////////
    
    enum {
        SMBMASTER_STATE_IDLE                ,
        SMBMASTER_STATE_START               ,
        SMBMASTER_STATE_MASTER_ADDR         ,
        SMBMASTER_STATE_SLAVE_ADDR_ACK      ,
        SMBMASTER_STATE_MASTER_CMD          ,
        SMBMASTER_STATE_SLAVE_CMD_ACK       ,
        SMBMASTER_STATE_MASTER_WRITE        ,
        SMBMASTER_STATE_SLAVE_WRITE_ACK     ,
        SMBMASTER_STATE_SLAVE_READ          ,
        SMBMASTER_STATE_MASTER_READ_ACK     ,
        SMBMASTER_STATE_STOP                ,
        SMBMASTER_STATE_STOP_THEN_START      
    }                               master_smbstate;                // current state of the master SMBus

    logic [3:0]                     master_bit_count;               // number of bits received in the current byte (starts at 0, counts up to 8)
    logic                           clear_master_bit_count;         // reset the master_bit_count to 0 when asserted
    logic [7:0]                     master_byte_in;                 // shift register to store incoming bits, shifted in starting at position 0 and shifting left (so first bit ends up as the msb)
    logic                           check_address_match;            // asserted (for one clock cycle) when a new address has first been received during the SLAVE_ADDRESS phase on the master bus
    logic                           assign_rdwr_bit;                // asserted (for one clock cycle) when read/write bit is received during the SLAVE_ADDRESS phase on the master bus
    logic                           clear_address_match;            // asserted when the address match mask needs to be cleared
    logic [NUM_RELAY_ADDRESSES:1]   address_match_mask;             // a bit is asserted when the incoming address matches the given element in the SMBUS_RELAY_ADDRESS array
    logic                           my_address;                     // asserted when the incoming command address matches ANY of the elements in the SMBUS_RELAY_ADDRESS array
    logic                           command_rd_wrn;                 // captured during the slave address phase, indicates if the current command is a READ (1) or WRITE (0) command
    logic                           command_whitelisted;            // asserted when the current command is on the 'whitelist' of allowed write commands (all read commands are allowed)
    logic                           master_read_nack;               // capture the ack/nack status from the master after a read data byte has been sent
    logic                           master_read_nack_valid;         // used to ensure master_read_nack is only captured on the first rising edge of clock after read data has been sent
    logic                           master_triggered_start;         // used to indicates that the next start condition is a repeated start

    
    enum {
        RELAY_STATE_IDLE                                    ,
        RELAY_STATE_START_WAIT_TIMEOUT                      ,
        RELAY_STATE_START_WAIT_MSCL_LOW                     ,
        RELAY_STATE_STOP_WAIT_SSCL_LOW                      ,
        RELAY_STATE_STOP_SSCL_LOW_WAIT_TIMEOUT              ,
        RELAY_STATE_STOP_SSDA_LOW_WAIT_SSCL_HIGH            ,
        RELAY_STATE_STOP_SSDA_LOW_SSCL_HIGH_WAIT_TIMEOUT    ,
        RELAY_STATE_STOP_SSCL_HIGH_WAIT_SSDA_HIGH           ,
        RELAY_STATE_STOP_SSDA_HIGH_SSCL_HIGH_WAIT_TIMEOUT   ,
        RELAY_STATE_STOP_RESET_TIMEOUT_COUNTER              ,
        RELAY_STATE_STOP_WAIT_SECOND_TIMEOUT                ,
        RELAY_STATE_STOP_SECOND_RESET_TIMEOUT_COUNTER       ,
        // in all 'MTOS' (Master to Slave) states, the master is driving the SDA signal to the slave, so SDA must be relayed from the master bus to the slave bus (or sometimes a 'captured' version of SDA)
        RELAY_STATE_MTOS_MSCL_LOW_WAIT_SSCL_LOW             ,
        RELAY_STATE_MTOS_SSCL_LOW_WAIT_TIMEOUT              ,
        RELAY_STATE_MTOS_WAIT_MSCL_HIGH                     ,
        RELAY_STATE_MTOS_WAIT_SSCL_HIGH                     ,
        RELAY_STATE_MTOS_SSCL_HIGH_WAIT_TIMEOUT             ,
        RELAY_STATE_MTOS_WAIT_MSCL_LOW                      ,
        // in all 'STOM' (Slave to Master) states, the slave is driving the SDA signal to the master, so SDA must be relayed from the slave bus to the master bus (or sometimes a 'captured' version of SDA)
        RELAY_STATE_STOM_MSCL_LOW_WAIT_SSCL_LOW             ,
        RELAY_STATE_STOM_SSCL_LOW_WAIT_TIMEOUT              ,
        RELAY_STATE_STOM_WAIT_SSCL_HIGH                     ,
        RELAY_STATE_STOM_SSCL_HIGH_WAIT_MSCL_HIGH           ,
        RELAY_STATE_STOM_SSCL_LOW_WAIT_MSCL_HIGH            ,
        RELAY_STATE_STOM_SSCL_HIGH_WAIT_MSCL_LOW            ,
        RELAY_STATE_STOM_SSCL_LOW_WAIT_MSCL_LOW
    }                                           relay_state;                    // current state of the relay between the master and slave busses
    logic [SCLK_HOLD_COUNTER_BIT_WIDTH-1:0]     slave_scl_hold_count;           // counter to determine how long to hold the slave scl signal low/high, counts down to -1 then waits to be restarted (so msb=1 indicates the counter has timed out)
    logic                                       slave_scl_hold_count_restart;   // combinatorial signal, reset the slave_scl_hold_count counter and start a new countdown
    logic                                       slave_scl_hold_count_timeout;   // combinatorial signal (copy of msb of slave_scl_hold_count), indicates the counter has reached it's timeout value and is not about to be restarted
    logic                                       slave_captured_sda;             // slave sda signal captured on a rising edge of slave scl
    
        
    
    always_ff @(posedge clock or negedge i_resetn) begin
        if (!i_resetn) begin
        
            master_smbstate             <= SMBMASTER_STATE_IDLE;
            master_bit_count            <= 4'h0;
            clear_master_bit_count      <= '0;
            master_byte_in              <= 8'h00;
            check_address_match         <= '0;
            assign_rdwr_bit             <= '0;
            clear_address_match         <= '0;
            address_match_mask          <= {NUM_RELAY_ADDRESSES{1'b0}};
            my_address                  <= '0;
            command_rd_wrn              <= '0;
            master_read_nack            <= '0;
            master_read_nack_valid      <= '0;
            master_triggered_start      <= '0;
            
        end else begin
        
            case ( master_smbstate )
                // IDLE state
                // This is the reset state.  Wait here until a valid START condition is detected on the master smbus
                SMBMASTER_STATE_IDLE: begin
                    if (master_start & ~i_block_disable) begin                  // only way to leave the idle state is if we detect a 'start' condition
                        master_smbstate <= SMBMASTER_STATE_START;
                    end
                    clear_master_bit_count <= '1;                               // hold the bit counter at 0 until we have exited the idle state
                end
                
                // START state
                // A start condition was detected on the master bus, we must stay here and clockstretch the master bus as required until the slave bus has 'caught up' and also issued a start condition
                SMBMASTER_STATE_START: begin
                    if (master_stop) begin                                      // start followed immediately by stop is strange, but not necessarily invalid
                        master_smbstate <= SMBMASTER_STATE_STOP;
                    end else begin
                        // once the master scl goes low after the start, we will hold it low, so we know we will stay in this state until the slave bus is ready to proceed
                        if (relay_state == RELAY_STATE_START_WAIT_TIMEOUT) begin        // relay has driven a 'start' condition to the slave bus, and is now 'synchronized' with the master bus
                            master_smbstate <= SMBMASTER_STATE_MASTER_ADDR;             // the first data bits to be received are the SMBus address and read/write bit
                        end
                    end
                    clear_master_bit_count <= '1;                               // hold the bit counter at 0
                end


                // MASTER_ADDR state
                // receive the 7 bit slave addres and the read/write bit
                // leave this state when all 8 bits have been received and the clock has been driven low again by the master
                SMBMASTER_STATE_MASTER_ADDR: begin
                    if (master_stop || master_start) begin                      // unexpected stop/start condition, ignore further master bus activity until we can issue a 'stop' condition on the slave bus
                        master_smbstate <= SMBMASTER_STATE_STOP;               
                    end else begin
                        if (master_bit_count == 4'h7 && master_scl_negedge == 1'b1 && !my_address) begin   // we have received the address, proceed to STOP is its an invalid address
                            master_smbstate <= SMBMASTER_STATE_STOP;            // not our address, send a 'stop' condition on the slave bus and stop forwarding from the master until the next 'start'
                        end
                        else if (master_bit_count == 4'h8 && master_scl_negedge == 1'b1) begin   // we have received all 8 data bits, time for the ACK. When we reach here, we know that the address has matched.
                            if (!(master_triggered_start && (command_rd_wrn == 1'b0))) begin  // to block write operation after repeated start
                                master_smbstate <= SMBMASTER_STATE_SLAVE_ADDR_ACK;
                            end else begin
                                master_smbstate <= SMBMASTER_STATE_STOP;
                            end
                        end
                        clear_master_bit_count <= '0;                           // do not reset the bit counter in this state, it will be reset during the next ACK state
                    end
                end
                
                // SLAVE_ADDR_ACK state
                // we may be driving the master_scl signal during this state to clock-stretch while awaiting an ACK from the slave bus
                // always enter this state after an scl falling edge
                // leave this state when the ack/nack bit has been sent and the clock has gone high then been drivien low again by the master
                SMBMASTER_STATE_SLAVE_ADDR_ACK: begin
                    if (master_stop || master_start) begin                      // unexpected stop/start condition, ignore further master bus activity until we can issue a 'stop' condition on the slave bus
                        master_smbstate <= SMBMASTER_STATE_STOP;
                    end else begin
                        if (master_scl_negedge) begin                           // ack/nack sent on clock rising edge, then need to wait for clock to go low again before leaving this state so it's safe to stop driving sda
                            if (slave_captured_sda) begin                           // slave_captured_sda contains the ack (0) / nack (1) status of the last command, if we 'nack'ed send a stop on the slave bus and wait for a stop on the master bus
                                master_smbstate <= SMBMASTER_STATE_STOP;
                            end else if (command_rd_wrn) begin
                                master_smbstate <= SMBMASTER_STATE_SLAVE_READ;              // this is a read command, start sending data back from slave
                            end else begin
                                master_smbstate <= SMBMASTER_STATE_MASTER_CMD;              // receive the command on the next clock cycle
                            end
                        end
                    end
                    clear_master_bit_count <= '1;                               // hold the counter at 0 through this state, so it has the correct value at the start of the next state
                end
                        
                // MASTER_CMD state
                // receive the 8 bit command (the first data byte after the address is called the 'command')
                // always enter this state after an scl falling edge
                // leave this state when all 8 bits have been received and the clock has been driven low again by the master
                SMBMASTER_STATE_MASTER_CMD: begin
                    if (master_stop || master_start) begin                      // unexpected stop/start condition, ignore further master bus activity until we can issue a 'stop' condition on the slave bus
                        master_smbstate <= SMBMASTER_STATE_STOP;
                    end else begin
                        if (master_bit_count == 4'h8 && master_scl_negedge == 1'b1) begin   // we have received all 8 data bits, time for the ACK
                            master_smbstate <= SMBMASTER_STATE_SLAVE_CMD_ACK;
                        end
                    end
                    clear_master_bit_count <= '0;                           // do not reset the bit counter in this state, it will be reset during the next ACK state
                end

                // SLAVE_CMD_ACK or SLAVE_WRITE_ACK state
                // we may be driving the master_scl signal during this state to clock-stretch while awaiting an ACK from the slave bus
                // always enter this state after an scl falling edge
                // leave this state when the ack/nack bit has been sent and the clock has been drivien low again by the master 
                SMBMASTER_STATE_SLAVE_CMD_ACK,
                SMBMASTER_STATE_SLAVE_WRITE_ACK: begin
                    if (master_stop || master_start) begin                      // unexpected stop/start condition, ignore further master bus activity until we can issue a 'stop' condition on the slave bus
                        master_smbstate <= SMBMASTER_STATE_STOP;
                    end else begin
                        if (master_scl_negedge) begin                           // ack/nack sent on clock rising edge, then need to wait for clock to go low again before leaving this state so it's safe to stop driving sda
                            if (slave_captured_sda) begin                           // slave_captured_sda contains the ack (0) / nack (1) status of the last command, if we 'nack'ed send a stop on the slave bus and wait for a stop on the master bus
                                master_smbstate <= SMBMASTER_STATE_STOP;
                            end else begin
                                master_smbstate <= SMBMASTER_STATE_MASTER_WRITE;
                            end
                        end
                    end
                    clear_master_bit_count <= '1;                               // hold the counter at 0 through this state, so it has the correct value (0) at the start of the next state
                end

                // MASTER_WRITE state
                // receive a byte to write to the slave device
                // always enter this state after an scl falling edge
                // for non-whitelisted write commands, expect a 'restart' condition (which will allow a read command to begin) or abort the command by returning to the IDLE state
                // for whitelisted commands, allow the command to proceed
                SMBMASTER_STATE_MASTER_WRITE: begin
                    if (master_stop) begin                                      // unexpected stop condition, ignore further master bus activity until we can issue a 'stop' condition on the slave bus
                        master_smbstate <= SMBMASTER_STATE_STOP;
                    end else begin
                        if (command_whitelisted==1'b0 && i_filter_disable==1'b0 && master_bit_count == 4'h1 && master_scl_negedge == 1'b1) begin   // non-whitelisted command and no restart condition received, send a 'stop' condition on the slave bus and stop forwarding bits from the master bus
                            master_smbstate <= SMBMASTER_STATE_STOP;
                        end else if (master_start) begin                                // restart condition received, allow the command to proceed
                            master_smbstate <= SMBMASTER_STATE_START;
                        end else if (master_bit_count == 4'h8 && master_scl_negedge == 1'b1) begin   // we have received all 8 data bits for a whitelisted command, time for the ACK
                            master_smbstate <= SMBMASTER_STATE_SLAVE_WRITE_ACK;
                        end
                    end
                    clear_master_bit_count <= '0;                           // do not reset the bit counter in this state, it will be reset during the next ACK state
                end

                // SLAVE_READ state
                // receive a byte from the slave device and send it to the master
                // always enter this state after an scl falling edge
                SMBMASTER_STATE_SLAVE_READ: begin
                    if (master_stop || master_start) begin                      // unexpected stop/start condition, ignore further master bus activity until we can issue a 'stop' condition on the slave bus
                        master_smbstate <= SMBMASTER_STATE_STOP;
                    end else begin
                        if (master_bit_count == 4'h8 && master_scl_negedge == 1'b1) begin   // we have sent all 8 data bits, time for the ACK (from the master)
                            master_smbstate <= SMBMASTER_STATE_MASTER_READ_ACK;
                        end
                    end
                    clear_master_bit_count <= '0;                           // do not reset the bit counter in this state, it will be reset during the next ACK state
                end
                
                // MASTER_READ_ACK
                // always enter this state after an scl falling edge
                // leave this state when the ack/nack bit has been received and the clock has been drivien low again by the master 
                SMBMASTER_STATE_MASTER_READ_ACK: begin
                    if (master_stop || master_start) begin                      // unexpected stop/start condition, ignore further master bus activity until we can issue a 'stop' condition on the slave bus
                        master_smbstate <= SMBMASTER_STATE_STOP;
                    end else begin
                        if (master_scl_negedge) begin
                            if (~master_read_nack) begin                        // if we received an ack (not a nack) from the master, continue reading data from slave
                                master_smbstate <= SMBMASTER_STATE_SLAVE_READ;
                            end else begin                                      // on a nack, send a stop condition on the slave bus and wait for a stop on the master bus
                                master_smbstate <= SMBMASTER_STATE_STOP;
                            end
                        end
                    end
                    clear_master_bit_count <= '1;                               // hold the counter at 0 through this state, so it has the correct value (0) at the start of the next state
                end

                // STOP
                // Enter this state to indicate a STOP condition should be sent to the slave bus
                // Once the STOP has been sent on the slave bus, we return to the idle state and wait for another start condition
                // We can enter this state if a stop condition has been received on the master bus, or if we EXPECT a start condition on the master busses
                // We do not wait to actually see a stop condition on the master bus before issuing the stop on the slave bus and proceeding to the IDLE state
                SMBMASTER_STATE_STOP: begin
                    if (master_start) begin
                        master_smbstate <= SMBMASTER_STATE_STOP_THEN_START;     // we may receive a valid start condition while waiting to send a stop, we need to track that case
                    end else if (relay_state == RELAY_STATE_IDLE) begin
                        master_smbstate <= SMBMASTER_STATE_IDLE;
                    end
                    clear_master_bit_count <= '1;                               // hold the counter at 0 through this state, so it has the correct value (0) at the start of the next state
                end

                // STOP_THEN_START
                // While waiting to send a STOP on the slave bus, we receive a new start condition on the master busses
                // Must finish sending the stop condition on the slave bus, then send a start condition on the slave bus
                SMBMASTER_STATE_STOP_THEN_START: begin
                    if (relay_state == RELAY_STATE_IDLE || relay_state == RELAY_STATE_START_WAIT_TIMEOUT) begin
                        master_smbstate <= SMBMASTER_STATE_START;
                    end
                    clear_master_bit_count <= '1;                               // hold the counter at 0 through this state, so it has the correct value (0) at the start of the next state
                end

                
                default: begin                                                  // illegal state, should never get here
                    master_smbstate <= SMBMASTER_STATE_IDLE;
                    clear_master_bit_count <= '1;
                    // TODO flag some sort of error here?
                end
                
            endcase  

            // counter for bits received on the master bus
            // the master SMBus state machine will clear the counter at the appropriate times, otherwise it increments on every master scl rising edge
            if (clear_master_bit_count || master_start) begin       // need to reset the counter on a start condition to handle the repeated start (simpler than assigning clear signal in this one special case)
                master_bit_count <= 4'h0;
            end else begin
                if (master_scl_posedge) begin
                    master_bit_count <= master_bit_count + 4'b0001;
                end
            end
            
            // shift register to store incoming bits from the master bus
            // shifts on every clock edge regardless of bus state, rely on master_smbstate and master_bit_count to determine when a byte is fully formed
            if (master_scl_posedge) begin
                master_byte_in[7:0] <= {master_byte_in[6:0], master_sda};
            end
            
            // determine if the incoming address matches one of our relay addresses, and if this is a read or write command
            if ( (master_smbstate == SMBMASTER_STATE_MASTER_ADDR) && (master_scl_posedge) && (master_bit_count == 4'h6) ) begin
                check_address_match <= '1;
            end else begin
                check_address_match <= '0;
            end
                
            if ( (master_smbstate == SMBMASTER_STATE_MASTER_ADDR) && (master_scl_posedge) && (master_bit_count == 4'h7) ) begin // 8th bit is captured on the next clock cycle, so that's when it's safe to check the full byte contents
                assign_rdwr_bit <= '1;
            end else begin
                assign_rdwr_bit <= '0;
            end
            
            if ( (master_smbstate == SMBMASTER_STATE_IDLE) ) begin
                clear_address_match <= '1;
            end else begin
                clear_address_match <= '0;
            end
            
            if (clear_address_match) begin
                address_match_mask[NUM_RELAY_ADDRESSES:1] <= {NUM_RELAY_ADDRESSES{1'b0}};
            end else if (check_address_match) begin
                for (int i=1; i<= NUM_RELAY_ADDRESSES; i++) begin
                    if (master_byte_in[6:IGNORED_ADDR_BITS] == SMBUS_RELAY_ADDRESS[i][6:IGNORED_ADDR_BITS]) begin
                        address_match_mask[i] <= '1;
                    end else begin
                        address_match_mask[i] <= '0;
                    end
                end
            end 
            
            if (assign_rdwr_bit) begin
                command_rd_wrn <= master_byte_in[0];        // lsb of the byte indicates read (1) or write (0) command
            end
            my_address <= RELAY_ALL_ADDRESSES ? '1 : |address_match_mask;
            
            // capture the read data ACK/NACK from the master after read data has been sent
            // make sure to only capture ack/nack on the first rising edge of SCL using the master_read_nack_valid signal
            if (master_smbstate == SMBMASTER_STATE_MASTER_READ_ACK) begin
                if (master_scl_posedge) begin
                    if (!master_read_nack_valid) begin
                        master_read_nack <= master_sda;
                    end
                    master_read_nack_valid <= '1;
                end
            end else begin
                master_read_nack_valid <= '0;
                master_read_nack <= '1;
            end
            
            // repeated start can only be used to switch from write mode to read mode in SMBus protocol
            if (master_smbstate == SMBMASTER_STATE_MASTER_CMD) begin
                master_triggered_start <= '1;
            end else if ((master_smbstate == SMBMASTER_STATE_STOP) || (master_smbstate == SMBMASTER_STATE_IDLE)) begin            
                master_triggered_start <= '0;
            end                        
        end
        
    end
    
    // map address_match_mask (one-hot encoded) back to a binary representation to use as address inputs to the whitelist RAM
    logic [4:0]         address_match_index;    // (zero-based) index (in SMBUS_RELAY_ADDRESS array) that matches the received SMBus address (if there is no match, value is ignored so is not important)
    always_comb begin
        address_match_index = '0;
        for (int i=0; i<NUM_RELAY_ADDRESSES; i++) begin
            if (address_match_mask[i+1]) begin
                address_match_index |= i[4:0];      // use |= to avoid creating a priority encoder - we know only one bit in the address_match_mask array can be setting
            end
        end
    end

    
    ///////////////////////////////////////
    // Determine the state of the relay, and drive the slave and master SMBus signals based on the state of the master and slave busses
    ///////////////////////////////////////

    always_ff @(posedge clock or negedge i_resetn) begin
        if (!i_resetn) begin

            relay_state                     <= RELAY_STATE_IDLE;
            o_master_scl_oe                 <= '0;
            o_master_sda_oe                 <= '0;
            o_slave_scl_oe                  <= '0;
            o_slave_sda_oe                  <= '0;
            slave_scl_hold_count            <= {SCLK_HOLD_COUNTER_BIT_WIDTH{1'b0}};
            slave_captured_sda              <= '0;
            
        end else begin
        
            case ( relay_state )
            
                // IDLE state
                // waiting for a start condition on the master busses
                RELAY_STATE_IDLE: begin
                    if (master_smbstate == SMBMASTER_STATE_START) begin
                        relay_state <= RELAY_STATE_START_WAIT_TIMEOUT;
                    end
                    o_master_scl_oe <= '0;
                    o_master_sda_oe <= '0;
                    o_slave_scl_oe  <= '0;
                    o_slave_sda_oe  <= '0;
                end

                // START_WAIT_TIMEOUT
                // Start condition has been received on the master bus
                // Drive a start condition on the slave bus (SDA goes low while SCL is high) and wait for a timeout
                // If master SCL goes low during this state, hold it there to prvent further progress on the master bus until the slave bus can 'catch up'
                RELAY_STATE_START_WAIT_TIMEOUT: begin
                    if (slave_scl_hold_count_timeout) begin
                        relay_state <= RELAY_STATE_START_WAIT_MSCL_LOW;
                    end
                    o_master_scl_oe <= ~master_scl;             // if master SCL goes low, we hold it low
                    o_master_sda_oe <= '0; 
                    o_slave_scl_oe  <= '0;
                    o_slave_sda_oe  <= '1;                      // drive a '0' onto the slave SDA bus, to create a start condition
                end

                // START_WAIT_MSCL_LOW
                // Start condition has been received on the master bus
                // Continue to drive a start condition on the slave bus (SDA low while SCL is high) and wait for master scl to go low
                // If master SCL is low during this state, continue to hold it there to prvent further progress on the master bus until the slave bus can 'catch up'
                RELAY_STATE_START_WAIT_MSCL_LOW: begin
                    if (~master_scl) begin
                        relay_state <= RELAY_STATE_MTOS_MSCL_LOW_WAIT_SSCL_LOW;     // after a start, the master is driving the bus
                    end else if (master_smbstate == SMBMASTER_STATE_STOP) begin        // stop right after start may not be legal, but safter to handle this case anyway
                        relay_state <= RELAY_STATE_STOP_SSDA_LOW_SSCL_HIGH_WAIT_TIMEOUT;
                    end
                    o_master_scl_oe <= ~master_scl;             // if master SCL goes low, we hold it low
                    o_master_sda_oe <= '0; 
                    o_slave_scl_oe  <= '0;
                    o_slave_sda_oe  <= '1;                      // drive a '0' onto the slave SDA bus, to create a start condition
                end

                // MTOS_MSCL_LOW_WAIT_SSCL_LOW
                // This state exists to provide some hold time on the slave SDA signal, we drive slave SCL low and wait until we observe it low (which has several clock cycles of delay) before proceeding
                // While in this state, we hold the old value of o_slave_sda_oe
                // Clockstretch the master SCL (which is also low) during this state
                RELAY_STATE_MTOS_MSCL_LOW_WAIT_SSCL_LOW: begin
                    if (~slave_scl) begin
                        relay_state <= RELAY_STATE_MTOS_SSCL_LOW_WAIT_TIMEOUT;
                    end
                    o_master_scl_oe <= ~master_scl;             // we know master SCL is low here, we continue to hold it low
                    o_master_sda_oe <= '0; 
                    o_slave_scl_oe  <= '1;                      // we drive the slave SCL low
                    o_slave_sda_oe  <= o_slave_sda_oe;          // do not change the value of o_slave_sda_oe, we are providing some hold time on this signal after the falling edge of slave SCL
                end

                // MTOS_SSCL_LOW_WAIT_TIMEOUT
                // Master is driving the bus, SDA from the master bus to the slave bus
                // We are driving slave SCL low, wait for a timeout before allowing it to go high
                // Clockstretch the master SCL (which is also low) during this state
                RELAY_STATE_MTOS_SSCL_LOW_WAIT_TIMEOUT: begin
                    if (slave_scl_hold_count_timeout) begin
                        relay_state <= RELAY_STATE_MTOS_WAIT_MSCL_HIGH;
                    end
                    o_master_scl_oe <= ~master_scl;             // we know master SCL is low here, we continue to hold it low
                    o_master_sda_oe <= '0; 
                    o_slave_scl_oe  <= '1;                      // we drive the slave SCL low
                    o_slave_sda_oe  <= ~master_sda;             // drive the 'live' value from the master SDA onto the slave SDA
                end

                
                // MTOS_WAIT_MSCL_HIGH
                // Master is driving the bus, SDA from the master bus to the slave bus
                // Release master SCL and wait for it to go high, while continuing to hold slave scl low
                RELAY_STATE_MTOS_WAIT_MSCL_HIGH: begin
                    if (master_smbstate == SMBMASTER_STATE_STOP || master_smbstate == SMBMASTER_STATE_STOP_THEN_START) begin
                        relay_state <= RELAY_STATE_STOP_WAIT_SSCL_LOW;
                    end else begin
                        if (master_scl) begin
                            relay_state <= RELAY_STATE_MTOS_WAIT_SSCL_HIGH;
                        end
                    end
                    o_master_scl_oe <= '0;
                    o_master_sda_oe <= '0; 
                    o_slave_scl_oe  <= '1;                      // we drive the slave SCL low
                    o_slave_sda_oe  <= ~master_sda;             // drive the 'live' value from the master SDA onto the slave SDA
                end
                
                // MTOS_WAIT_SSCL_HIGH
                // Master is driving the bus, SDA from the master bus to the slave bus
                // Release slave SCL and wait for it to go high
                // If master SCL goes low again during this state, hold it there to prvent further progress on the master bus until the slave bus can 'catch up'
                RELAY_STATE_MTOS_WAIT_SSCL_HIGH: begin
                    if (slave_scl) begin
                        relay_state <= RELAY_STATE_MTOS_SSCL_HIGH_WAIT_TIMEOUT;
                    end
                    o_master_scl_oe <= ~master_scl;             // if master SCL goes low, we hold it low
                    o_master_sda_oe <= '0; 
                    o_slave_scl_oe  <= '0;
                    o_slave_sda_oe  <= ~master_byte_in[0];      // drive the most recent captured bit from the master sda onto the slave sda
                end

                // MTOS_SSCL_HIGH_WAIT_TIMEOUT
                // Master is driving the bus, SDA from the master bus to the slave bus
                // Slave SCL is high, wait for a timeout before allowing it to be driven low
                // If master SCL goes low again during this state, hold it there to prvent further progress on the master bus until the slave bus can 'catch up'
                RELAY_STATE_MTOS_SSCL_HIGH_WAIT_TIMEOUT: begin
                    if (slave_scl_hold_count_timeout) begin
                        relay_state <= RELAY_STATE_MTOS_WAIT_MSCL_LOW;
                    end
                    o_master_scl_oe <= ~master_scl;             // if master SCL goes low, we hold it low
                    o_master_sda_oe <= '0; 
                    o_slave_scl_oe  <= '0;
                    o_slave_sda_oe  <= ~master_byte_in[0];      // drive the most recent captured bit from the master sda onto the slave sda
                end


                // MTOS_WAIT_MSCL_LOW
                // Master is driving the bus, SDA from the master bus to the slave bus
                // Wait for master SCL to go low, then hold it low (clockstretch) and proceed to state where we drive the slave scl low
                RELAY_STATE_MTOS_WAIT_MSCL_LOW: begin
                    if (master_smbstate == SMBMASTER_STATE_START) begin
                        relay_state <= RELAY_STATE_START_WAIT_TIMEOUT;
                    end else if (master_smbstate == SMBMASTER_STATE_STOP || master_smbstate == SMBMASTER_STATE_STOP_THEN_START) begin
                        if (~master_byte_in[0]) begin       // we know slave SCL is high, if slave SDA is low proceed to the appropriate phase of the STOP states
                            relay_state <= RELAY_STATE_STOP_SSDA_LOW_SSCL_HIGH_WAIT_TIMEOUT;
                        end else begin                      // we need to first drive slave SCL low so we can drive slave SDA low then proceed to create the STOP condition
                            relay_state <= RELAY_STATE_STOP_WAIT_SSCL_LOW;
                        end
                    end else begin
                        if (~master_scl_dly) begin      // need to look at a delayed version of master scl, to give master_smbstate state machine time to update
                            // check if we are in a state where the slave is driving data back to the master bus
                            if (    (master_smbstate == SMBMASTER_STATE_SLAVE_ADDR_ACK) ||
                                    (master_smbstate == SMBMASTER_STATE_SLAVE_CMD_ACK) ||
                                    (master_smbstate == SMBMASTER_STATE_SLAVE_WRITE_ACK) ||
                                    (master_smbstate == SMBMASTER_STATE_SLAVE_READ)
                            ) begin
                                relay_state <= RELAY_STATE_STOM_MSCL_LOW_WAIT_SSCL_LOW;
                            end else begin
                                relay_state <= RELAY_STATE_MTOS_MSCL_LOW_WAIT_SSCL_LOW;
                            end
                        end
                    end
                    o_master_scl_oe <= ~master_scl;             // if master SCL is low, we hold it low
                    o_master_sda_oe <= '0; 
                    o_slave_scl_oe  <= '0;
                    o_slave_sda_oe  <= ~master_byte_in[0];      // drive the most recent captured bit from the master sda onto the slave sda
                end

                // STOM_MSCL_LOW_WAIT_SSCL_LOW
                // We start driving slave SCL low, wait to observe it actually going low before releasing the o_slave_sda_oe signal
                // This state exists to provide some hold timing on slave SDA after slave SCL goes low
                // Clockstretch the master SCL (which is also low) during this state
                RELAY_STATE_STOM_MSCL_LOW_WAIT_SSCL_LOW: begin
                    if (~slave_scl) begin
                        relay_state <= RELAY_STATE_STOM_SSCL_LOW_WAIT_TIMEOUT;
                    end
                    o_master_scl_oe <= ~master_scl;             // we know master SCL is low here, we continue to hold it low
                    o_master_sda_oe <= ~slave_sda;              // drive sda from the slave bus to the master bus
                    o_slave_scl_oe  <= '1;                      // we drive the slave SCL low
                    o_slave_sda_oe  <= o_slave_sda_oe;          // do not change the value of slave SDA, we are providing hold time on slave SDA in this state after falling edge of slave SCL
                end

                // STOM_SSCL_LOW_WAIT_TIMEOUT
                // Slave is driving data, SDA from the slave bus to the master
                // We are driving slave SCL low, wait for a timeout before allowing it to go high
                // Clockstretch the master SCL (which is also low) during this state
                RELAY_STATE_STOM_SSCL_LOW_WAIT_TIMEOUT: begin
                    if (slave_scl_hold_count_timeout) begin
                        relay_state <= RELAY_STATE_STOM_WAIT_SSCL_HIGH;
                    end
                    o_master_scl_oe <= ~master_scl;             // we know master SCL is low here, we continue to hold it low
                    o_master_sda_oe <= ~slave_sda;              // drive sda from the slave bus to the master bus
                    o_slave_scl_oe  <= '1;                      // we drive the slave SCL low
                    o_slave_sda_oe  <= '0;
                end

                // STOM_WAIT_SSCL_HIGH
                // Slave is driving data, SDA from the slave bus to the master
                // Release slave SCL and wait for it to go high
                // Continue to clockstretch (hold SCL low) on the master bus until we have received data from the slave bus
                RELAY_STATE_STOM_WAIT_SSCL_HIGH: begin
                    if (slave_scl) begin
                        relay_state <= RELAY_STATE_STOM_SSCL_HIGH_WAIT_MSCL_HIGH;
                    end
                    o_master_scl_oe <= ~master_scl;             // we know master SCL is low here, we continue to hold it low
                    o_master_sda_oe <= ~slave_sda;              // drive sda from the slave bus to the master bus
                    o_slave_scl_oe  <= '0;
                    o_slave_sda_oe  <= '0;
                end

                // STOM_SSCL_HIGH_WAIT_MSCL_HIGH
                // Slave is driving data, SDA from the slave bus to the master
                // Slave SCL is high, release master SCL and wait for it to go high
                // Continue to clockstretch (hold SCL low) on the master bus until we have received data from the slave bus
                RELAY_STATE_STOM_SSCL_HIGH_WAIT_MSCL_HIGH: begin
                    if (master_scl) begin
                        relay_state <= RELAY_STATE_STOM_SSCL_HIGH_WAIT_MSCL_LOW;
                    end else if (slave_scl_hold_count_timeout) begin
                        relay_state <= RELAY_STATE_STOM_SSCL_LOW_WAIT_MSCL_HIGH;
                    end
                    o_master_scl_oe <= '0;
                    o_master_sda_oe <= ~slave_captured_sda;     // drive master sda with the value captured on the rising edge of slave scl
                    o_slave_scl_oe  <= '0;
                    o_slave_sda_oe  <= '0;
                end
                
                // STOM_SSCL_LOW_WAIT_MSCL_HIGH
                // Slave is driving data, SDA from the slave bus to the master
                // Slave SCL has been high long enough that a timeout occurred, so need to drive it low again to comply with SMBus timing rules
                // Still waiting for the master SCL to go high so the master bus can 'catch up'
                RELAY_STATE_STOM_SSCL_LOW_WAIT_MSCL_HIGH: begin
                    if (master_scl) begin
                        relay_state <= RELAY_STATE_STOM_SSCL_LOW_WAIT_MSCL_LOW;
                    end
                    o_master_scl_oe <= '0;
                    o_master_sda_oe <= ~slave_captured_sda;     // drive master sda with the value captured on the rising edge of slave scl
                    o_slave_scl_oe  <= '1;                      // drive the slave SCL low to avoid a timing error from leaving scl high for too long
                    o_slave_sda_oe  <= '0;
                end
 
                // STOM_SSCL_HIGH_WAIT_MSCL_LOW
                // Slave is driving data, SDA from the slave bus to the master
                // Slave and master SCL are high, we are waiting for master SCL to go low
                RELAY_STATE_STOM_SSCL_HIGH_WAIT_MSCL_LOW: begin
                    if (master_smbstate == SMBMASTER_STATE_STOP || master_smbstate == SMBMASTER_STATE_STOP_THEN_START) begin
                        relay_state <= RELAY_STATE_STOP_WAIT_SSCL_LOW;
                    end else begin
                        if (~master_scl_dly) begin      // need to look at a delayed version of master scl, to give master_smbstate state machine time to update
                            if ( master_smbstate == SMBMASTER_STATE_SLAVE_READ ) begin      // only in the SLAVE_READ state do we have two SMBus cycles in a row where the slave drives data to the master
                                relay_state <= RELAY_STATE_STOM_SSCL_LOW_WAIT_TIMEOUT;
                            end else begin
                                relay_state <= RELAY_STATE_MTOS_SSCL_LOW_WAIT_TIMEOUT;
                            end
                        end
                    end
                    o_master_scl_oe <= '0;
                    o_master_sda_oe <= ~slave_captured_sda;     // drive master sda with the value captured on the rising edge of slave scl
                    o_slave_scl_oe  <= '0;
                    o_slave_sda_oe  <= '0;
                end

                // STOM_SSCL_LOW_WAIT_MSCL_LOW
                // Slave is driving data, SDA from the slave bus to the master
                // A timeout occured while waiting for master SCL to go high which forced us to drive slave SCL low
                // Master SCL has gone high, we are waiting for it to go low again
                RELAY_STATE_STOM_SSCL_LOW_WAIT_MSCL_LOW: begin
                    if (master_smbstate == SMBMASTER_STATE_STOP || master_smbstate == SMBMASTER_STATE_STOP_THEN_START) begin
                        relay_state <= RELAY_STATE_STOP_WAIT_SSCL_LOW;
                    end else begin
                        if (~master_scl_dly) begin      // need to look at a delayed version of master scl, to give master_smbstate state machine time to update
                            if ( master_smbstate == SMBMASTER_STATE_SLAVE_READ ) begin      // only in the SLAVE_READ state do we have two SMBus cycles in a row where the slave drives data to the master
                                relay_state <= RELAY_STATE_STOM_SSCL_LOW_WAIT_TIMEOUT;
                            end else begin
                                relay_state <= RELAY_STATE_MTOS_SSCL_LOW_WAIT_TIMEOUT;
                            end
                        end
                    end
                    o_master_scl_oe <= '0;
                    o_master_sda_oe <= ~slave_captured_sda;     // drive master sda with the value captured on the rising edge of slave scl
                    o_slave_scl_oe  <= '1;                      // continue to force slave SCL low
                    o_slave_sda_oe  <= '0;
                end

                // STOP_WAIT_SSCL_LOW
                // Sending a stop condition on the slave bus
                // Drive slave SCL low, wait to see it has gone low before driving SDA low (which happens in the next state)
                // Clockstretch the master bus if master SCL is driven low, to prevent the master bus from getting 'ahead' of the slave bus
                RELAY_STATE_STOP_WAIT_SSCL_LOW: begin
                    if (~slave_scl) begin
                        relay_state <= RELAY_STATE_STOP_SSCL_LOW_WAIT_TIMEOUT;
                    end
                    o_master_scl_oe <= ~master_scl;             // clockstretch on the master bus if master scl is driven low
                    o_master_sda_oe <= '0;
                    o_slave_scl_oe  <= '1;                      // we drive slave SCL low
                    o_slave_sda_oe  <= o_slave_sda_oe;          // provide hold time on slave SDA relative to slave_scl
                end

                // STOP_SSCL_LOW_WAIT_TIMEOUT
                // Sending a stop condition on the slave bus
                // Drive slave SCL low, and drive slave SDA low (after allowing for suitable hold time in the previous state)
                // Clockstretch the master bus if master SCL is driven low, to prevent the master bus from getting 'ahead' of the slave bus
                RELAY_STATE_STOP_SSCL_LOW_WAIT_TIMEOUT: begin
                    if (slave_scl_hold_count_timeout) begin
                        relay_state <= RELAY_STATE_STOP_SSDA_LOW_WAIT_SSCL_HIGH;
                    end
                    o_master_scl_oe <= ~master_scl;             // clockstretch on the master bus if master scl is driven low
                    o_master_sda_oe <= '0;
                    o_slave_scl_oe  <= '1;                      // we drive slave SCL low
                    o_slave_sda_oe  <= '1;
                end

                // STOP_SSDA_LOW_WAIT_SSCL_HIGH
                // Allow slave SCL to go high, confirm it has gone high before proceeding
                // Clockstretch on the master bus if master SCL goes low
                RELAY_STATE_STOP_SSDA_LOW_WAIT_SSCL_HIGH: begin
                    if (slave_scl) begin
                        relay_state <= RELAY_STATE_STOP_SSDA_LOW_SSCL_HIGH_WAIT_TIMEOUT;
                    end
                    o_master_scl_oe <= ~master_scl;             // clockstretch on the master bus if master scl is driven low
                    o_master_sda_oe <= '0;
                    o_slave_scl_oe  <= '0;
                    o_slave_sda_oe  <= '1;                      // hold slave SDA low, it will be released after we leave this state, creating the 'stop' condition on the bus
                end

                // STOP_SSDA_LOW_SSCL_HIGH_WAIT_TIMEOUT
                // Allow slave SCL to go high, then wait for a timeout before proceeding to next state
                // Clockstretch on the master bus if master SCL goes low
                RELAY_STATE_STOP_SSDA_LOW_SSCL_HIGH_WAIT_TIMEOUT: begin
                    if (slave_scl_hold_count_timeout) begin
                        relay_state <= RELAY_STATE_STOP_SSCL_HIGH_WAIT_SSDA_HIGH;
                    end
                    o_master_scl_oe <= ~master_scl;             // clockstretch on the master bus if master scl is driven low
                    o_master_sda_oe <= '0;
                    o_slave_scl_oe  <= '0;
                    o_slave_sda_oe  <= '1;                      // hold slave SDA low, it will be released after we leave this state, creating the 'stop' condition on the bus
                end
                
                // STOP_SSCL_HIGH_WAIT_SSDA_HIGH
                // Allow slave SDA to go high, wait to observe it high before proceeding to the next state
                // This rising edge on SDA while SCL is high is what creates the 'stop' condition on the bus
                // Clockstretch on the master bus if master SCL goes low
                RELAY_STATE_STOP_SSCL_HIGH_WAIT_SSDA_HIGH: begin
                    if (slave_sda) begin
                        relay_state <= RELAY_STATE_STOP_SSDA_HIGH_SSCL_HIGH_WAIT_TIMEOUT;
                    end
                    o_master_scl_oe <= ~master_scl;             // clockstretch on the master bus if master scl is driven low
                    o_master_sda_oe <= '0;
                    o_slave_scl_oe  <= '0;
                    o_slave_sda_oe  <= '0;
                end

                // STOP_SSDA_HIGH_SSCL_HIGH_WAIT_TIMEOUT
                // Stop condition has been sent, wait for a timeout before proceeding to next state
                // Clockstretch on the master bus if master SCL goes low
                RELAY_STATE_STOP_SSDA_HIGH_SSCL_HIGH_WAIT_TIMEOUT: begin
                    if (slave_scl_hold_count_timeout) begin
                        relay_state <= RELAY_STATE_STOP_RESET_TIMEOUT_COUNTER;
                    end
                    o_master_scl_oe <= ~master_scl;             // clockstretch on the master bus if master scl is driven low
                    o_master_sda_oe <= '0;
                    o_slave_scl_oe  <= '0;
                    o_slave_sda_oe  <= '0;
                end

                // STOP_RESET_TIMEOUT_COUNTER
                // We need TWO timeout counters after a stop condition, this state exists just to allow the counter to reset
                RELAY_STATE_STOP_RESET_TIMEOUT_COUNTER: begin
                    relay_state <= RELAY_STATE_STOP_WAIT_SECOND_TIMEOUT;
                    o_master_scl_oe <= ~master_scl;             // clockstretch on the master bus if master scl is driven low
                    o_master_sda_oe <= '0;
                    o_slave_scl_oe  <= '0;
                    o_slave_sda_oe  <= '0;
                end

                // STOP_WAIT_SECOND_TIMEOUT
                // Allow slave SCL to go high, then wait for a timeout before proceeding to next state
                // Clockstretch on the master bus if master SCL goes low
                RELAY_STATE_STOP_WAIT_SECOND_TIMEOUT: begin
                    if (slave_scl_hold_count_timeout) begin
                        if (master_smbstate == SMBMASTER_STATE_STOP_THEN_START) begin
                            relay_state <= RELAY_STATE_STOP_SECOND_RESET_TIMEOUT_COUNTER;
                        end else begin    
                            relay_state <= RELAY_STATE_IDLE;
                        end
                    end
                    o_master_scl_oe <= ~master_scl;             // clockstretch on the master bus if master scl is driven low
                    o_master_sda_oe <= '0;
                    o_slave_scl_oe  <= '0;
                    o_slave_sda_oe  <= '0;
                end

                // STOP_SECOND_RESET_TIMEOUT_COUNTER
                // need to reset the timeout counter before we proceed to the start state, which relies on the timeout counter
                RELAY_STATE_STOP_SECOND_RESET_TIMEOUT_COUNTER: begin
                    relay_state <= RELAY_STATE_START_WAIT_TIMEOUT;
                    o_master_scl_oe <= ~master_scl;             // clockstretch on the master bus if master scl is driven low
                    o_master_sda_oe <= '0;
                    o_slave_scl_oe  <= '0;
                    o_slave_sda_oe  <= '0;
                end

                
                default: begin
                    relay_state <= RELAY_STATE_IDLE;
                    o_master_scl_oe <= '0;             // clockstretch on the master bus if master scl is driven low
                    o_master_sda_oe <= '0;
                    o_slave_scl_oe  <= '0;
                    o_slave_sda_oe  <= '0;
                end

            endcase
            
            // capture data from the slave bus on clock rising edge
            if (slave_scl_posedge) begin
                slave_captured_sda <= slave_sda;
            end
            
            // counter to determine how long to hold slave_scl low or high 
            // used when creating 'artificial' scl clock pulses on slave while clock stretching the master during ack and read data phases
            // counter counts from some positive value down to -1, thus checking the highest bit (sign bit) is adequate to determine if the count is 'done'
            if (slave_scl_hold_count_restart) begin
                slave_scl_hold_count <= SCLK_HOLD_MIN_COUNT[SCLK_HOLD_COUNTER_BIT_WIDTH-1:0] - {{SCLK_HOLD_COUNTER_BIT_WIDTH-1{1'b0}}, 1'b1}; 
            end else if (!slave_scl_hold_count[SCLK_HOLD_COUNTER_BIT_WIDTH-1]) begin        // count down to -1 (first time msb goes high) and then stop
                slave_scl_hold_count <= slave_scl_hold_count - {{SCLK_HOLD_COUNTER_BIT_WIDTH-1{1'b0}}, 1'b1};
            end
        
        end
        
    end

    // when the msb of slave_scl_hold_count = 1, that indicates a negative number, which means the timeout has occurred
    assign slave_scl_hold_count_timeout = slave_scl_hold_count[SCLK_HOLD_COUNTER_BIT_WIDTH-1];
    
    // determine when to reset the counter based on the current relay state
    // creatre this signal with combinatorial logic so counter will be reset as we enter the next state
    // we never have two states in a row that both require this counter
    assign slave_scl_hold_count_restart = ( (relay_state == RELAY_STATE_START_WAIT_TIMEOUT) ||
                                            (relay_state == RELAY_STATE_MTOS_SSCL_LOW_WAIT_TIMEOUT) ||
                                            (relay_state == RELAY_STATE_MTOS_SSCL_HIGH_WAIT_TIMEOUT) ||
                                            (relay_state == RELAY_STATE_STOM_SSCL_LOW_WAIT_TIMEOUT) ||
                                            (relay_state == RELAY_STATE_STOM_SSCL_HIGH_WAIT_MSCL_HIGH) ||
                                            (relay_state == RELAY_STATE_STOP_SSCL_LOW_WAIT_TIMEOUT) ||
                                            (relay_state == RELAY_STATE_STOP_SSDA_LOW_SSCL_HIGH_WAIT_TIMEOUT) ||
                                            (relay_state == RELAY_STATE_STOP_SSDA_HIGH_SSCL_HIGH_WAIT_TIMEOUT) ||
                                            (relay_state == RELAY_STATE_STOP_WAIT_SECOND_TIMEOUT)
                                          ) ? '0 : '1;
                                          
                                          
    ///////////////////////////////////////
    // Instantiate an M9K block in simple dual port mode to store command enable whitelist information
    // The memory is only instantiated when filtering is enabled
    ///////////////////////////////////////
    generate 
        if (FILTER_ENABLE) begin    : gen_filter_enabled
            logic [7:0] command;    // current command coming from the Master SMBus.  The first byte after the I2C Address is the 'command' byte
            
				// capture the incoming command 
            always_ff @(posedge clock or negedge i_resetn) begin
                if (!i_resetn) begin
                    command <= 8'h00;
                end else begin
                    if ( (master_smbstate == SMBMASTER_STATE_MASTER_CMD) && (master_bit_count == 4'h8)  ) begin
                        command <= master_byte_in;
                    end else if (master_smbstate == SMBMASTER_STATE_IDLE) begin            // reset the command each time we return to the idle state
                        command <= 8'h00;
                    end
                end
            end

            altsyncram #(
                .address_aclr_b                     ( "NONE"                   ),
                .address_reg_b                      ( "CLOCK0"                 ),
                //.clock_enable_input_a               ( "BYPASS"                 ), // these 'BYPASS' settings are causing problems in simulation, should be fine with default values as all clken ports are tied-off to 1
                //.clock_enable_input_b               ( "BYPASS"                 ),
                //.clock_enable_output_b              ( "BYPASS"                 ),
                //.intended_device_family            .( "MAX 10"                 ),  // this setting shouldn't be needed, will come from the project - leaving it out makes this more generic
                .lpm_type                           ( "altsyncram"             ),
                .numwords_a                         ( 256                      ),
                .numwords_b                         ( 256*32                   ),   // output is 1 bit wide, so 32x as many words on port b
                .operation_mode                     ( "DUAL_PORT"              ),
                .outdata_aclr_b                     ( "NONE"                   ),
                .outdata_reg_b                      ( "CLOCK0"                 ),   // set to 'UNREGISTERED' for no output reg, 'CLOCK0' to use the output reg
                .power_up_uninitialized             ( "FALSE"                  ),
                .ram_block_type                     ( "AUTO"                   ),   // was set to 'M9K', changing to 'AUTO' to prevent any family incompatibility issues in the future
                .read_during_write_mode_mixed_ports ( "DONT_CARE"              ),
                .widthad_a                          ( 8                        ),
                .widthad_b                          ( 8+5                      ),   // output is only 1 bit wide, so 5 extra address bits are required
                .width_a                            ( 32                       ),
                .width_b                            ( 1                        ),
                .width_byteena_a                    ( 1                        )
            ) cmd_enable_mem (		
                .address_a      ( i_avmm_address            ),
                .address_b      ( {address_match_index[4:0],command[7:0]} ),        // msbs select which I2C device is being addressed, lsbs select the individual command
                .clock0         ( clock                     ),
                .data_a         ( i_avmm_writedata          ),
                .data_b         ( 1'b0                      ),
                .wren_a         ( i_avmm_write              ),
                .wren_b         ( 1'b0                      ),
                .q_a            (                           ),
                .q_b            ( command_whitelisted       ),
                .aclr0          ( 1'b0                      ),
                .aclr1          ( 1'b0                      ),
                .addressstall_a ( 1'b0                      ),
                .addressstall_b ( 1'b0                      ),
                .byteena_a      ( 1'b1                      ),
                .byteena_b      ( 1'b1                      ),
                .clock1         ( 1'b1                      ),
                .clocken0       ( 1'b1                      ),
                .clocken1       ( 1'b1                      ),
                .clocken2       ( 1'b1                      ),
                .clocken3       ( 1'b1                      ),
                .eccstatus      (                           ),
                .rden_a         ( 1'b1                      ),
                .rden_b         ( 1'b1                      )
            );
        end else begin              : gen_filter_disabled
            assign command_whitelisted = '1;           
        end
    endgenerate

endmodule





        
                
                
        
        

    
