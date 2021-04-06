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

// This is a fifo used in the SMbus mailbox
// This fifo supports a enqueue operation and a dequeue operation 
// as well as a non-destructive read that returns the last value enqueud. 
// This option happens whenever both enqueue and dequeue are not asserted
// 

`timescale 1 ps / 1 ps
`default_nettype none

module fifo #(
		// the width of data in and out
		parameter DATA_WIDTH = 8,
		// the number of spaces in the fifo. Internally the fifo actually implements a fifo of data_depth + 1
		// set by default to use 1 M9K
		parameter DATA_DEPTH = 1023 
		) (
		input  logic 					clk,
		input  logic 					resetn,
		// input data to the fifo
		input  logic [DATA_WIDTH-1:0] 	data_in,
		// output data from the fifo
		output logic [DATA_WIDTH-1:0] 	data_out,
		input  logic 					clear,
		input  logic 					enqueue,
		input  logic 					dequeue

		);

// have one more space in the FIFO than requestsed to simplify the logic for full and empty
// because the fifo is implemented as a ram, one more word in the ram usually has no addtional cost
// the exception would be if the size of the required a second M9K
// bitwidth of the read and write pointers
localparam PTR_WIDTH = $clog2(DATA_DEPTH + 1);
localparam [PTR_WIDTH-1:0] ACTUAL_DATA_DEPTH = DATA_DEPTH[PTR_WIDTH-1:0] + 1'b1;

// read and write pointers, used to implement a fifo in a ring buffer configuration
logic [PTR_WIDTH-1:0] read_ptr;
logic [PTR_WIDTH-1:0] write_ptr;

// empty and full signals used to check if it is valid to enqueue or dequeue values
logic empty;
logic full;

// inputs to the single port ram
logic wren;
logic [PTR_WIDTH-1:0] address;
logic [DATA_WIDTH-1:0] q;

// next read valid, used to determine if data_out should be driven by the ram or 0 to signify an empty fifo
logic next_read_valid;


assign empty = (read_ptr == write_ptr);
assign full = (read_ptr == 0 && write_ptr == ACTUAL_DATA_DEPTH - 1'b1) || (read_ptr == write_ptr + 1'b1);
assign wren = enqueue && !full;

// increment read pointer if the fifo is not empty and a dequeue command is issued
always_ff @(posedge clk or negedge resetn) begin
	if (!resetn) begin
		read_ptr <= {PTR_WIDTH{1'b0}};
	end
	else begin
		if (dequeue && !empty) begin
			read_ptr <= (read_ptr == ACTUAL_DATA_DEPTH - 1'b1) ? '0 : read_ptr + 1'b1;
		end
		if (clear) begin
			read_ptr <= {PTR_WIDTH{1'b0}};
		end
	end
end

// Increment write pointer if the fifo is not full and the enqueue connamnd is issued
always_ff @(posedge clk or negedge resetn) begin
	if (!resetn) begin
		write_ptr <= {PTR_WIDTH{1'b0}};
	end
	else begin
		if (enqueue && !full) begin
			write_ptr <= (write_ptr == ACTUAL_DATA_DEPTH - 1'b1) ? '0 : write_ptr + 1'b1;
		end
		if (clear) begin
			write_ptr <= {PTR_WIDTH{1'b0}};
		end
	end
end

// logic for if the next read is valid
// if the next read is invalid because the fifo is empty data_out is 0
// registering next_read_valid gets around the case where the fifo is empty because we just read the only value inside it
assign data_out = (next_read_valid | !empty) ? q : 8'b0;
always_ff @(posedge clk or negedge resetn) begin
	if (!resetn) begin
		next_read_valid <= 1'b0;
	end
	else begin
		if (dequeue && !empty) begin
			next_read_valid <= 1'b1;
		end
		else begin
			next_read_valid <= 1'b0;
		end
	end
end

// address block
// address is write_ptr - 1 by deault to allow us to read the last value enqueued
// otherwise checks if a enqueue or dequeue is valid and sets the address to either the read or write pointer
always_comb begin
	address = (write_ptr == 0) ? ACTUAL_DATA_DEPTH - 1'b1 : write_ptr - 1'b1;
	if (enqueue && !full) begin
		address = write_ptr;
	end
	else if (dequeue && !empty) begin
		address = read_ptr;
	end
end

// single port ram, used to implement the fifo's array
sp_ram #(
	.NUMWORDS(ACTUAL_DATA_DEPTH),
	.ADDRESS_WIDTH(PTR_WIDTH),
	.DATA_WIDTH(DATA_WIDTH)
	) u_sp_ram (
	.clock(clk),
	.address(address),
	.data(data_in),
	.wren(wren),
	.q(q)
	);

endmodule
