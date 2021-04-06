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


`timescale 1ps / 1ps
`default_nettype none

module jtag_lock_controller
    (
    clk_in,
    resetn,
    lock_unlock_n,
    tck_external,
    tdi_external,
    tms_external,
    tdo_external
);

    input   clk_in, tck_external, tdi_external, tms_external, resetn, lock_unlock_n;
    output  tdo_external;

    wire tck_core, tdi_core, tms_core, tdo_core, tdi_ser_lock, tdi_ser_unlock, jtag_core_en_out, lock_unlock_edge_detect;


    reg [4:0] counter;
    reg tdi_ser, tck_out, tck_en, tms, load_tdi, tdi_en, tdo_en, jtag_core_en, tms_out, tdi_out, start_lock, start_unlock;
    reg input_shifter;
    
    assign tck_core = tck_en ? !clk_in : 1'b0;
    assign tdi_core = tdi_en ? tdi_ser : 1'b0;
    assign tms_core = tms; 
    assign jtag_core_en_out = jtag_core_en;
    
    
    always_ff @ (posedge clk_in or negedge resetn) begin
        if (!resetn) begin
            input_shifter <= 1'b1;
            start_lock <= 1'b0;
            start_unlock <= 1'b0;
        end else begin
            input_shifter <= lock_unlock_n;
            if (lock_unlock_edge_detect) begin
                start_lock <= lock_unlock_n;
                start_unlock <= !lock_unlock_n;
            end
        end
    end
    assign lock_unlock_edge_detect = (input_shifter != lock_unlock_n);
    
    always_comb begin   
        if (start_lock)
            tdi_ser <= tdi_ser_lock;
        else if (start_unlock)
            tdi_ser <= tdi_ser_unlock;
        else
            tdi_ser <= 1'b0;
    end
        
    typedef enum {  
        state_reset,
        state_jtag_idle,
        state_jtag_select_dr,
        state_jtag_select_ir,
        state_jtag_capture_ir,
        state_jtag_capture_dr,
        state_jtag_shift_ir,
        state_jtag_shift_dr,
        state_load_lock,
        state_load_unlock,
        state_do_lock,
        state_do_unlock,
        state_jtag_exit1_ir,
        state_jtag_exit1_dr,
        state_jtag_update_ir,
        state_jtag_update_dr,
        state_jtag_backto_idle,
        state_complete
    } state_t; 
    state_t state, next_state;
    
    parameter 
    tdi_par_lock = 10'h101,
    tdi_par_unlock = 10'h041;

always_ff @ (posedge clk_in or negedge resetn) begin
    if (!resetn) begin
         state <= state_reset;
          counter <= 5'b0;
     end else begin
            state <= next_state;
           if (state == state_do_lock || state == state_do_unlock ) begin
               counter <= counter + 5'b00001;
           end else begin
               counter <= 'b0;
           end
     end
end
        
always_comb begin
    case (state)

    state_reset: begin
        if (lock_unlock_edge_detect)
            next_state = state_jtag_idle;
        else
            next_state = state_reset;
    end
    state_jtag_idle:       next_state = state_jtag_select_dr;   
    state_jtag_select_dr:  next_state = state_jtag_select_ir;   
    state_jtag_select_ir:  next_state = state_jtag_capture_ir;  
    state_jtag_capture_ir: next_state = state_jtag_shift_ir;
    state_jtag_shift_ir: begin
        if (start_lock)
            begin
                              next_state = state_load_lock;
            end
        else if (start_unlock)
            begin
                              next_state = state_load_unlock;
            end
        else
            begin
                              next_state = state_reset;
            end
        end
    
    state_load_lock:       next_state = state_do_lock;  
    state_do_lock:  begin
        if (counter >= 5'b00111)
            next_state = state_jtag_exit1_ir;
        else
           next_state = state_do_lock;
    end 
    state_load_unlock:    next_state = state_do_unlock; 
    state_do_unlock:    begin
        if (counter >= 5'b00111)
            next_state = state_jtag_exit1_ir;
        else
            next_state = state_do_unlock;
    end
    
    state_jtag_exit1_ir:  next_state = state_jtag_update_ir;    
    state_jtag_update_ir: next_state = state_jtag_backto_idle;  
    state_jtag_backto_idle: next_state = state_complete;    
    state_complete: next_state = state_reset;
    
    default: begin
        next_state <= state_reset;
    end
    
    endcase
end 

//control signals for TCK_CORE, TMS_CORE, TDI_CORE and TDO_CORE
always_comb begin
    case (state)
    state_reset: 
    begin
        jtag_core_en = 1'b0;    
        tck_en = 1'b1;
        tms = 1'b1;
        load_tdi = 1'b1;
        tdi_en = 1'b0;
        tdo_en = 1'b0;
    end

    state_jtag_idle:
    begin
        jtag_core_en = 1'b1;
        tck_en = 1'b1;
        tms = 1'b0;
        load_tdi = 1'b1;
        tdi_en = 1'b0;
        tdo_en = 1'b0;
    end

    state_jtag_select_dr:
    begin
        jtag_core_en = 1'b1;
        tck_en = 1'b1;
        tms = 1'b1;
        load_tdi = 1'b1;
        tdi_en = 1'b0;
        tdo_en = 1'b0;
    end
    
    state_jtag_select_ir:
    begin
        jtag_core_en = 1'b1;
        tck_en = 1'b1;
        tms = 1'b1;
        load_tdi = 1'b1;
        tdi_en = 1'b0;
        tdo_en = 1'b0;
    end
    
    state_jtag_capture_ir:
    begin
        jtag_core_en = 1'b1;
        tck_en = 1'b1;
        tms = 1'b0;
        load_tdi = 1'b1;
        tdi_en = 1'b0;
        tdo_en = 1'b0;
    end
    
    state_jtag_shift_ir:
    begin
        jtag_core_en = 1'b1;
        tck_en = 1'b1;
        tms = 1'b0;
        load_tdi = 1'b1;
        tdi_en = 1'b0;
        tdo_en = 1'b0;
    end
    
    state_load_lock:
    begin
        jtag_core_en = 1'b1;
        tck_en = 1'b1;
        tms = 1'b0;
        load_tdi = 1'b0;
        tdi_en = 1'b1;
        tdo_en = 1'b0;
    end

    state_do_lock:
    begin
        jtag_core_en = 1'b1;
        tck_en = 1'b1;
        tms = 1'b0;
        load_tdi = 1'b0;
        tdi_en = 1'b1;
        tdo_en = 1'b0;
    end

    state_load_unlock:
    begin
        jtag_core_en = 1'b1;
        tck_en = 1'b1;
        tms = 1'b0;
        load_tdi = 1'b0;
        tdi_en = 1'b1;
        tdo_en = 1'b0;
    end

    state_do_unlock:
    begin
        jtag_core_en = 1'b1;
        tck_en = 1'b1;
        tms = 1'b0;
        load_tdi = 1'b0;
        tdi_en = 1'b1;
        tdo_en = 1'b0;
    end

    state_jtag_exit1_ir:
    begin
        jtag_core_en = 1'b1;
        tck_en = 1'b1;
        tms = 1'b1;
        load_tdi = 1'b0;
        tdi_en = 1'b1;
        tdo_en = 1'b0;
    end

    state_jtag_update_ir:
    begin
        jtag_core_en = 1'b1;
        tck_en = 1'b1;
        tms = 1'b1;
        load_tdi = 1'b1;
        tdi_en = 1'b0;
        tdo_en = 1'b0;
    end

    state_jtag_backto_idle:
    begin
        jtag_core_en = 1'b1;
        tck_en = 1'b1;
        tms = 1'b0;
        load_tdi = 1'b1;
        tdi_en = 1'b0;
        tdo_en = 1'b0;
    end

    state_complete: 
    begin
        jtag_core_en = 1'b1;
        tck_en = 1'b1;
        tms = 1'b0;
        load_tdi = 1'b1;
        tdi_en = 1'b0;
        tdo_en = 1'b0;
    end
    
    default: //same as settings for state_reset
    begin
        jtag_core_en = 1'b0;
        tck_en = 1'b1;
        tms = 1'b1;
        load_tdi = 1'b1;
        tdi_en = 1'b0;
        tdo_en = 1'b0;  
    end
    endcase
end

lpm_shiftreg tdi_reg_lock (
        .clock(clk_in),
        .enable(1'b1),
        .load(load_tdi),
        .data(tdi_par_lock),
        .shiftout(tdi_ser_lock)
    );
        defparam
        tdi_reg_lock.lpm_type = "LPM_SHIFTREG",
        tdi_reg_lock.lpm_width = 10,
        tdi_reg_lock.lpm_direction = "LEFT";

lpm_shiftreg tdi_reg_unlock (
        .clock(clk_in),
        .enable(1'b1),
        .load(load_tdi),
        .data(tdi_par_unlock),
        .shiftout(tdi_ser_unlock)
    );
        defparam
        tdi_reg_unlock.lpm_type = "LPM_SHIFTREG",
        tdi_reg_unlock.lpm_width = 10,
        tdi_reg_unlock.lpm_direction = "LEFT";

JTAG_Lock_Unlock_wysiwyg    jtag_atom(
    .jtag_core_en(jtag_core_en_out),
    .tck_core(tck_core),
    .tdi_core(tdi_core),
    .tms_core(tms_core),
    .tck_ignore(tck_external),
    .tdi_ignore(tdi_external),
    .tms_ignore(tms_external),
    .tdo_ignore(tdo_external),
    .tdo_core(tdo_core)
);

endmodule
