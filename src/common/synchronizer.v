/////////////////////////////////////////////////////////////////////////////////
//// Copyright (c) 2021 Intel Corporation
////
//// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the
//"Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included
//in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
//OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
//THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
/////////////////////////////////////////////////////////////////////////////////



module synchronizer #(
	parameter USE_INIT_0 = 0,
    parameter WIDTH = 16,
	parameter STAGES = 3 // should not be less than 2
)
(
	input clk_in,arst_in,
	input clk_out,arst_out,
	
	input [WIDTH-1:0] dat_in,
	output [WIDTH-1:0] dat_out	
);

// launch register
reg [WIDTH-1:0] d /* synthesis preserve */;
always @(posedge clk_in or posedge arst_in) begin
	if (arst_in) d <= 0;
	else d <= dat_in;
end

// capture registers
reg [STAGES*WIDTH-1:0] c /* synthesis preserve */;
always @(posedge clk_out or posedge arst_out) begin
	if (arst_out) c <= 0;
	else c <= {c[(STAGES-1)*WIDTH-1:0],d};
end

initial begin
   if (USE_INIT_0 == 1) begin
      d = 0;
      c = 0;
   end
end


assign dat_out = c[STAGES*WIDTH-1:(STAGES-1)*WIDTH];

endmodule
