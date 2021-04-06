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


// timer_bank
//
// This module implements the a bank of 20ms timers. Each word represents an
// independent timer. The timer value is bits 19:0. The start/stop bit is
// bit 28.

`timescale 1 ps / 1 ps
`default_nettype none

module timer_bank (
	input wire clk,
	input wire i20msCE,
	input wire areset,

	input wire [2:0] avmm_address,
	input wire avmm_read,
	input wire avmm_write,
	output logic [31:0] avmm_readdata,
	input wire [31:0] avmm_writedata
);
	localparam NUM_TIMERS = 3;
	localparam CEIL_LOG2_NUM_TIMERS = $clog2(NUM_TIMERS);
	localparam TIMER_WIDTH = 20;

	reg [TIMER_WIDTH-1:0] timers [NUM_TIMERS-1:0];
	reg [NUM_TIMERS-1:0] timer_active;

	reg [1:0] edge_tracker_20msCE;

	// Track the rising edge of the 20msCE. Since this is
	// synchronous to clk, we don't need extra synchronization
	always_ff @(posedge clk or posedge areset) begin
		if (areset) begin
			edge_tracker_20msCE <= 2'b0;
		end
		else begin
			edge_tracker_20msCE <= {edge_tracker_20msCE[0], i20msCE};
		end
	end

	// Implement the timers. Each timer is independely controlled using
	// timer_active. Decrement on the timer only occurs when the 20ms
	// pulse has occurred. 
	//
	// AVMM write is also controlled in this process
	always_ff @(posedge clk or posedge areset) begin
		if (areset) begin
			for (integer i = 0; i < NUM_TIMERS; i++) begin
				timers[i] <= {TIMER_WIDTH{1'b0}};
				timer_active[i] <= 1'b0;
			end
		end
		else begin
			if (edge_tracker_20msCE == 2'b01) begin
				// Active transition on 20ms clock enable
				for (integer i = 0; i < NUM_TIMERS; i++) begin
					if (timer_active[i] && (timers[i] != {TIMER_WIDTH{1'b0}})) begin
						timers[i] <= timers[i] - 1'b1;
					end
				end
			end

			// AVMM Write will overwrite any timer activity from above
			if (avmm_write) begin
				timers[avmm_address[CEIL_LOG2_NUM_TIMERS-1:0]] <= avmm_writedata[TIMER_WIDTH-1:0];
				timer_active[avmm_address[CEIL_LOG2_NUM_TIMERS-1:0]] <= avmm_writedata[28];
			end

		end
	end


	// AVMM read interface
	always_comb begin
		if (avmm_read) begin
			avmm_readdata <= {3'b0, timer_active[avmm_address[CEIL_LOG2_NUM_TIMERS-1:0]], {(28-TIMER_WIDTH){1'b0}}, timers[avmm_address[CEIL_LOG2_NUM_TIMERS-1:0]]};
		end
		else begin
			avmm_readdata = 32'bX;
		end
	end

endmodule

