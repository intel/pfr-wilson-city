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


//***Design Example for volatile key clear and security mode verification****//
//***in MAX 10 through internal JTAG interface (JTAG access from core)**//
		
module JTAG_Lock_Unlock_wysiwyg (
	jtag_core_en, 
	tck_core,
	tdi_core,
	tdo_ignore,
	tdo_core,
	tms_core,
	tck_ignore,
	tdi_ignore,
	tms_ignore
);

input 	jtag_core_en, tck_core, tdi_core, tms_core, tck_ignore, tdi_ignore, tms_ignore;
output	tdo_ignore, tdo_core;

// Define WYSIWYG atoms
//
// WYSIWYG atoms to access JTAG from core

fiftyfivenm_jtag JTAG
(
      .clkdruser(),
		.corectl(jtag_core_en),
		.runidleuser(),
		.shiftuser(),
		.tck(tck_ignore),
		.tckcore(tck_core),
		.tckutap(),
		.tdi(tdi_ignore),
		.tdicore(tdi_core),
		.tdiutap(),
		.tdo(tdo_ignore),
		.tdocore(tdo_core),
		.tdouser(),
		.tdoutap(),
		.tms(tms_ignore),
		.tmscore(tms_core),
		.tmsutap(),
		.updateuser(),
		.usr1user(),
		.ntdopinena()
);


endmodule
