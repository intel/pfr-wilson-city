/////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Intel Corporation
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the
// "Software"),
// to deal in the Software without restriction, including without
// limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
/////////////////////////////////////////////////////////////////////////////////


/**********************************************************************/
// Description : SHA message scheduler module including 16x64-bit message 
//		 registers and performs 1 round of expansion per cycle.
/**********************************************************************/


module sha_scheduler_top (
        clk,
        rst,
	start,
	input_valid,
        cnt,
	hash_size,
        win,
        w_out
        );

input clk, rst, start;
input input_valid;
input [1:0] hash_size;	
input [6:0] cnt;
input [1023:0] win;
output [63:0] w_out;

reg [63:0] wreg[15:0];
wire [63:0] msg_out;

wire [63:0] w16,w7;
wire [63:0] w15,w2;

//############### Combinational Logic ######

assign w16 = wreg[15];
assign w15 = wreg[14];
assign w7 = wreg[6];
assign w2 = wreg[1];

assign w_out = wreg[15];

sha_scheduler msg_sch_inst (
	.W_t_2(w2),
	.W_t_7(w7),
	.W_t_15(w15),
	.W_t_16(w16),
	.hash_size(hash_size[1]),
	.W_t(msg_out)
	);

//############### Registers ################

always @(posedge clk or negedge rst)
begin

        if(rst==1'b0)
        begin
        
                wreg[15] <= 64'h0;
                wreg[14] <= 64'h0;
                wreg[13] <= 64'h0;
                wreg[12] <= 64'h0;
                wreg[11] <= 64'h0;
                wreg[10] <= 64'h0;
                wreg[9]  <= 64'h0;
                wreg[8]  <= 64'h0;
                wreg[7]  <= 64'h0;
                wreg[6]  <= 64'h0;
                wreg[5]  <= 64'h0;
                wreg[4]  <= 64'h0;
                wreg[3]  <= 64'h0;
                wreg[2]  <= 64'h0;
                wreg[1]  <= 64'h0;
                wreg[0]  <= 64'h0;
        
        end
        
        else
        begin
        


                if (start==1'b1 || input_valid==1'b1)
                begin
                
                        wreg[15] <= win[1023:960];
                        wreg[14] <= win[959:896 ];
                        wreg[13] <= win[895:832 ];
                        wreg[12] <= win[831:768 ];
                        wreg[11] <= win[767:704 ];
                        wreg[10] <= win[703:640 ];
                        wreg[9 ] <= win[639:576 ];
                        wreg[8 ] <= win[575:512 ];
                        wreg[7 ] <= win[511:448 ];
                        wreg[6 ] <= win[447:384 ];
                        wreg[5 ] <= win[383:320 ];
                        wreg[4 ] <= win[319:256 ];
                        wreg[3 ] <= win[255:192 ];
                        wreg[2 ] <= win[191:128 ];
                        wreg[1 ] <= win[127:64  ];
                        wreg[0 ] <= win[63:0    ];

                end

                
                else 
                begin
                
                        wreg[1]  <= wreg[0] ;     
                        wreg[2]  <= wreg[1] ;     
                        wreg[3]  <= wreg[2] ;     
                        wreg[4]  <= wreg[3] ;     
                        wreg[5]  <= wreg[4] ;     
                        wreg[6]  <= wreg[5] ;     
                        wreg[7]  <= wreg[6] ;     
                        wreg[8]  <= wreg[7] ;     
                        wreg[9]  <= wreg[8] ;     
                        wreg[10] <= wreg[9] ;     
                        wreg[11] <= wreg[10];     
                        wreg[12] <= wreg[11];     
                        wreg[13] <= wreg[12];     
                        wreg[14] <= wreg[13];     
                        wreg[15] <= wreg[14];     
                        
                        wreg[0]  <= msg_out;
               
                  
 
                end
                
                
        end

end 

endmodule
