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

// Address protector module
// This module contains conditions for all the addresses that the BMC and PCH slaves are allowed to write to
// These conditions currently reflect the PFR HAS version 0.96
// Edit this file if the SMBus mailbox register map changes

`timescale 1 ps / 1 ps
`default_nettype none

module address_protector ( 
	                       input [7:0] pch_address,
	                       output      pch_cmd_valid,

	                       input [7:0] bmc_address,
	                       output      bmc_cmd_valid
	                       );

	// Signals is true if the address is in an allowable range whitelist 
	// The signals are only considered in the top level for writes, all reads are valid

	// update conditions to reflect the allowable ranges for the PCH
	assign pch_cmd_valid = (
							pch_address == 8'h9  ||
							pch_address == 8'hA  ||
							pch_address == 8'hB  ||
							pch_address == 8'h10 ||
							pch_address == 8'h11 ||
							pch_address == 8'h14 ||
							pch_address[7:6] == 2'b10/*(pch_address >= 8'h80 && pch_address <= 8'hBF)*/
							);
								     
	// update conditions to reflect the allowable write ranges for the BMC					     
	assign bmc_cmd_valid = (
							bmc_address == 8'h9  ||
							bmc_address == 8'hA  ||
							bmc_address == 8'hB  ||
							bmc_address == 8'h13 ||
							bmc_address == 8'h15 ||
							bmc_address[7:6] == 2'b11 // (bmc_address >= 8'hC0 && bmc_address <= 8'hFF)
							);



endmodule
