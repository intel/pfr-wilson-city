///////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Intel Corporation
//
// Permission is hereby granted, free of charge, to any person obtaining a
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
///////////////////////////////////////////////////////////////////////////////

/**********************************************************************/
// Description : The following module performs SHA2 hashing operation in
//		 256/384/512 modes.
/**********************************************************************/

module sha_unit (
	clk,	// SHA IP clock
        rst,	// Active low asynchronous reset
        start,	// Indicates the start of a new hash operation
	input_valid, 	// Indicates the input data is valid
        win,	// Input data/message
	hash_size,	// Indicates the size of hash --> 00 (Invalid); 01 (SHA-256); 10 (SHA-384); 11 (SHA-512)
        output_valid,	// Output signal indicating a valid hash value
        hout,	// Output hash
	h_init_256 //Output hash for SHA256 - remains constant after output valid
        );

input clk, rst, start;
input input_valid;
input [1:0] hash_size;
input [1023:0] win;

output output_valid;
output [255:0] h_init_256;
output [511:0] hout;

wire [1023:0] sha_win;
wire [511:0] sha_hout;

assign sha_win[1023:960] = (hash_size[1]==1'b0) ? {32'h0,win[511:480]} : win[1023:960];
assign sha_win[959:896 ] = (hash_size[1]==1'b0) ? {32'h0,win[479:448]} : win[959:896 ];
assign sha_win[895:832 ] = (hash_size[1]==1'b0) ? {32'h0,win[447:416]} : win[895:832 ];
assign sha_win[831:768 ] = (hash_size[1]==1'b0) ? {32'h0,win[415:384]} : win[831:768 ];
assign sha_win[767:704 ] = (hash_size[1]==1'b0) ? {32'h0,win[383:352]} : win[767:704 ];
assign sha_win[703:640 ] = (hash_size[1]==1'b0) ? {32'h0,win[351:320]} : win[703:640 ];
assign sha_win[639:576 ] = (hash_size[1]==1'b0) ? {32'h0,win[319:288]} : win[639:576 ];
assign sha_win[575:512 ] = (hash_size[1]==1'b0) ? {32'h0,win[287:256]} : win[575:512 ];
assign sha_win[511:448 ] = (hash_size[1]==1'b0) ? {32'h0,win[255:224]} : win[511:448 ];
assign sha_win[447:384 ] = (hash_size[1]==1'b0) ? {32'h0,win[223:192]} : win[447:384 ];
assign sha_win[383:320 ] = (hash_size[1]==1'b0) ? {32'h0,win[191:160]} : win[383:320 ];
assign sha_win[319:256 ] = (hash_size[1]==1'b0) ? {32'h0,win[159:128]} : win[319:256 ];
assign sha_win[255:192 ] = (hash_size[1]==1'b0) ? {32'h0,win[127:96 ]} : win[255:192 ];
assign sha_win[191:128 ] = (hash_size[1]==1'b0) ? {32'h0,win[95:64  ]} : win[191:128 ];
assign sha_win[127:64  ] = (hash_size[1]==1'b0) ? {32'h0,win[63:32  ]} : win[127:64  ];
assign sha_win[63:0    ] = (hash_size[1]==1'b0) ? {32'h0,win[31:0   ]} : win[63:0    ];

assign hout[511:384] = (hash_size[1]==1'b0) ? 128'h0            : ((hash_size[0]==1'b0) ? 128'h0            : sha_hout[511:384]);
assign hout[383:256] = (hash_size[1]==1'b0) ? 128'h0            : ((hash_size[0]==1'b0) ? sha_hout[511:384] : sha_hout[383:256]);
assign hout[255:224] = (hash_size[1]==1'b0) ? sha_hout[479:448] : ((hash_size[0]==1'b0) ? sha_hout[383:352] : sha_hout[255:224]);
assign hout[223:192] = (hash_size[1]==1'b0) ? sha_hout[415:384] : ((hash_size[0]==1'b0) ? sha_hout[351:320] : sha_hout[223:192]);
assign hout[191:160] = (hash_size[1]==1'b0) ? sha_hout[351:320] : ((hash_size[0]==1'b0) ? sha_hout[319:288] : sha_hout[191:160]);
assign hout[159:128] = (hash_size[1]==1'b0) ? sha_hout[287:256] : ((hash_size[0]==1'b0) ? sha_hout[287:256] : sha_hout[159:128]);
assign hout[127:96 ] = (hash_size[1]==1'b0) ? sha_hout[223:192] : ((hash_size[0]==1'b0) ? sha_hout[255:224] : sha_hout[127:96 ]);
assign hout[95:64  ] = (hash_size[1]==1'b0) ? sha_hout[159:128] : ((hash_size[0]==1'b0) ? sha_hout[223:192] : sha_hout[95:64  ]);
assign hout[63:32  ] = (hash_size[1]==1'b0) ? sha_hout[95:64  ] : ((hash_size[0]==1'b0) ? sha_hout[191:160] : sha_hout[63:32  ]);
assign hout[31:0   ] = (hash_size[1]==1'b0) ? sha_hout[31:0   ] : ((hash_size[0]==1'b0) ? sha_hout[159:128] : sha_hout[31:0   ]);



sha_top sha_inst (
        .clk(clk),
        .rst(rst),
        .start(start),
	.input_valid(input_valid),
        .win(sha_win),
	.hash_size(hash_size),
        .output_valid(output_valid),
	.h_init_256(h_init_256),
        .hout(sha_hout)
        );

endmodule
