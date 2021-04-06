//////////////////////////////////////////////////////////////////////////////////
/*!

    \brief    <b>%SysCheck </b>
    \file     SysCheck.v
    \details    <b>Image of the Block:</b>
                \image html SysCheck.png

                 <b>Description:</b> \n
                The module contain all logic needed for SysCheck logic\n\n

                Checks if the CPU ID and Package IDs match the correct way in the platform.\n

                The true table was provide by PAS 0.6v

                \image html truetable.png

    \brief  <b>Last modified</b>
            $Date:   Jan 19, 2018 $
            $Author:  David.bolanos@intel.com $
            Project         : Wilson City RP
            Group           : BD
    \version
             20160609 \b  David.bolanos@intel.com - File creation\n
             20181901 \b  David.bolanos@intel.com - Mofify to adapt to Wilson RP, leverage from Neon city\n
			 20180223 \b  jorge.juarez.campos@intel.com - Added support for STP Test Card.\n
             20181129 \b  jorge.juarez.campos@intel.com - Modified to match with latest PAS docuemnt

    \copyright Intel Proprietary -- Copyright \htmlonly <script>document.write(new Date().getFullYear())</script> \endhtmlonly Intel -- All rights reserved
*/
//////////////////////////////////////////////////////////////////////////////////
module SysCheck
(
    //% Clock input
    input           iClk,
    //% Asynchronous reset
    input           iRst,
    //% Socket occupied
    input   [1:0]   invCPUSktOcc,
    //% CPU0 Processor ID
    input   [1:0]   ivProcIDCPU1,
    //% CPU1 Processor ID
    input   [1:0]   ivProcIDCPU2,
    //% CPU0 Package ID
    input   [2:0]   ivPkgIDCPU1,
    //% CPU1 Package ID
    input   [2:0]   ivPkgIDCPU2,
    //% interposer present
    input   [1:0]   ivIntr,
    //% Auxiliar Power Ready
    input   		iAuxPwrDone,
    //% System validation Ok
    output          oSysOk,
    //% CPU Mismatch - priority
    output          oCPUMismatch,
    //% MCP Clock Control
    output          oMCPSilicon,
    //% Socket Removed
    output          oSocketRemoved,
	output 			oICX
);
localparam ICX	= 2'b00;
localparam CPX	= 2'b01;
localparam STP	= 2'b11;

localparam NON_MCP	= 3'b000;
localparam XCC_CPX4	= 3'b001;
localparam CPX6_CPU	= 3'b010;
localparam STP_CPU	= 3'b111;

reg         rSysOk;
reg         rProcIDErr;
reg         rProcPkgErr;
reg         rXCC_CPX4;
reg	rSocketRemoved;
reg	rICX;

wire	wSocket1Removed;
wire	wSocket2Removed;

wire	wSocket1RemovedLatch;
wire	wSocket2RemovedLatch;

EdgeDetector #(
	.EDGE                   ( 1'b1 )
) mSocket1Removed(
	.iClk                   ( iClk ),
	.iRst                   ( !iRst ),
	.iSignal                ( invCPUSktOcc[0] ),
	.oEdgeDetected          ( wSocket1RemovedLatch )
);

FF_SR mLatchOut1(
	.iClk                   ( iClk ),
	.iRst                   ( !iRst ),
	.iSet                   ( wSocket1RemovedLatch && iAuxPwrDone),
	.oQ                     ( wSocket1Removed )
);

EdgeDetector #(
	.EDGE                   ( 1'b1 )
)mSocket2Removed(
	.iClk                   ( iClk ),
	.iRst                   ( !iRst ),
	.iSignal                ( invCPUSktOcc[1] ),
	.oEdgeDetected          ( wSocket2RemovedLatch )
);

FF_SR mLatchOut2(
	.iClk                   ( iClk ),
	.iRst                   ( !iRst ),
	.iSet                   ( wSocket2RemovedLatch && iAuxPwrDone),
	.oQ                     ( wSocket2Removed )
);

always @(posedge iClk or negedge iRst )
begin
	if (!iRst) begin
		rSysOk			<= 1'b0;
		rProcIDErr		<= 1'b0;
		rProcPkgErr		<= 1'b0;
		rXCC_CPX4		<= 1'b0;
		rSocketRemoved	<= 1'b0;
		rICX			<= 1'b0;
	end
	else begin
		rProcIDErr	<= 1'b0;
		rProcPkgErr	<= 1'b0;
		rXCC_CPX4	<= 1'b0;
		rICX		<= 1'b0;
		if ( invCPUSktOcc[0] == 1'b0 ) begin
			casex ({invCPUSktOcc[1], ivIntr})
				3'b1_x1: begin
					rProcIDErr	<= 1'b0;
					rProcPkgErr	<= 1'b0;
				end   //With interposer. Only CPU1 Present
				3'b0_11: begin
					rProcIDErr	<= 1'b0;
					rProcPkgErr	<= 1'b0;
				end   //With interposer on both CPUs present
				3'b1_x0: begin //No interposer. Only CPU1 Present
					// PROC ID + PKG ID valid
					case ({ivProcIDCPU1, ivPkgIDCPU1})
						{ICX, NON_MCP}:		begin
							rProcIDErr	<= 1'b0;
							rProcPkgErr	<= 1'b0;
							rXCC_CPX4	<= 1'b0;
							rICX		<= 1'b1;
						end     //ICX-HCC/ICX-LCC (Non-MCP)
						{ICX, XCC_CPX4}:	begin
							rProcIDErr	<= 1'b0;
							rProcPkgErr	<= 1'b0;
							rXCC_CPX4	<= 1'b1;
							rICX		<= 1'b1;
						end     //ICX-XCC (MCP required)
						{CPX, XCC_CPX4}:	begin
							rProcIDErr	<= 1'b0;
							rProcPkgErr	<= 1'b0;
							rXCC_CPX4	<= 1'b1;
						end     //ICX-4 (MCP enabled)
						{STP, STP_CPU}:		begin
							rProcIDErr	<= 1'b0;
							rProcPkgErr	<= 1'b0;
							rXCC_CPX4	<= 1'b1;
						end     //STP (MCP enabled)
						default:			begin
							rProcIDErr	<= 1'b1;
							rProcPkgErr	<= 1'b1;
							rXCC_CPX4	<= 1'b0;
						end     //All other are invalid
					endcase
				end
				3'b0_00:	begin//No interposer
					// PROC ID + PKG ID valid
					case ({ivProcIDCPU2,ivProcIDCPU1,ivPkgIDCPU2,ivPkgIDCPU1})
						{ICX, ICX, NON_MCP, NON_MCP}:	begin
							rProcIDErr	<= 1'b0;
							rProcPkgErr	<= 1'b0;
							rXCC_CPX4	<= 1'b0;
							rICX		<= 1'b1;
						end//ICX-XCC/ICX-HCC/ICX-LCC (Non-MCP)
						{ICX, ICX, NON_MCP, XCC_CPX4}:	begin
							rProcIDErr	<= 1'b0;
							rProcPkgErr	<= 1'b0;
							rXCC_CPX4	<= 1'b1;
							rICX		<= 1'b1;
						end     //ICX-XCC/ICX-HCC/ICX-LCC (MCP Enabled)
						{ICX, ICX, XCC_CPX4, NON_MCP}:	begin
							rProcIDErr	<= 1'b0;
							rProcPkgErr	<= 1'b0;
							rXCC_CPX4	<= 1'b1;
							rICX		<= 1'b1;
						end     //ICX-XCC/ICX-HCC/ICX-LCC (MCP Enabled)
						{ICX, ICX, XCC_CPX4, XCC_CPX4}:	begin
							rProcIDErr	<= 1'b0;
							rProcPkgErr	<= 1'b0;
							rXCC_CPX4	<= 1'b1;
							rICX		<= 1'b1;
						end     //ICX-XCC/ICX-HCC/ICX-LCC (MCP Enabled)
						{CPX, CPX, XCC_CPX4, XCC_CPX4}:	begin
							rProcIDErr	<= 1'b0;
							rProcPkgErr	<= 1'b0;
							rXCC_CPX4	<= 1'b1;
						end     //CPX-4 (MCP Enabled)
						{STP, STP, STP_CPU, STP_CPU}:	begin
							rProcIDErr	<= 1'b0;
							rProcPkgErr	<= 1'b0;
							rXCC_CPX4	<= 1'b1;
						end     //STP Test Card (MCP Enabled)
						default:						begin
							rProcIDErr	<= 1'b1;
							rProcPkgErr	<= 1'b1;
							rXCC_CPX4	<= 1'b0;
						end     //All other are invalid
					endcase
				end
				default:	begin
					rProcIDErr	<= 1'b1;
					rProcPkgErr	<= 1'b1;
					rXCC_CPX4	<= 1'b0;
				end
			endcase
		end
		rSysOk			<= !rProcIDErr && !rProcPkgErr;//No PKG and PROC ID error and at least CPU0 present
		rSocketRemoved	<= (wSocket1Removed || wSocket2Removed) && (iRst);
	end
end

assign oSysOk			= rSysOk;
assign oCPUMismatch		= rProcIDErr || rProcPkgErr;
assign oMCPSilicon		= rXCC_CPX4;
assign oSocketRemoved	= rSocketRemoved;
assign oICX				= rICX;

endmodule
