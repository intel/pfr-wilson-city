
//////////////////////////////////////////////////////////////////////////////////
/*!

    \brief    <b>%BMC Sequence Control </b>
    \file     Bmc_Seq.v
    \details    <b>Image of the Block:</b>
                \image html Bmc_Seq.png

                 <b>Description:</b> \n

                Ths module control the power secuence for BMC VR's, fault condition of each VR and SRST#.
                This module does not depend on Mstr_Seq.v  to enable the VR's
                the BMC VR's are enabled on S5 state.\n




    \brief  <b>Last modified</b>
            $Date:   Jun 19, 2018 $
            $Author:  David.bolanos@intel.com $
            Project            : Wilson City RP
            Group            : BD
    \version
             20160509 \b  David.bolanos@intel.com - File creation\n
             20181901 \b  David.bolanos@intel.com - Modify to adapt to Wilson City, leverage from Mehlow\n

    \copyright Intel Proprietary -- Copyright \htmlonly <script>document.write(new Date().getFullYear())</script> \endhtmlonly Intel -- All rights reserved
*/
//////////////////////////////////////////////////////////////////////////////////


module Bmc_Seq
(
	input			iClk,//%Clock input
	input			iRst_n,//%Reset enable on high
	input 			i1mSCE, //% 1 mS Clock Enable
	input           iGoOutFltSt,//% Go out fault state.

	input 			PWRGD_P1V1_BMC_AUX,//% BMC VR PWRGD P1V1
	input 			PWRGD_P3V3_AUX,//% P3V3_AUX VR PWRGD
	input 			PWRGD_PCH_P1V8,//% PCH VR Enable P1V8
	input           FM_SLP_SUS_N,//% SLP_SUS

	input 			RST_DEDI_BUSY_PLD_N, //% Dediprog Detection Support

	output reg		FM_BMC_P2V5_AUX_EN,//% BMC VR Enable P2V5
	output reg      RST_SRST_BMC_N , //% SRST#
    output reg      oBmcPwrgd,//% PWRGD of all BMC VR's

    output reg      oBmcPwrFlt//% Fault BMC VR's

);
//////////////////////////////////////////////////////////////////////////////////
// Parameters
//////////////////////////////////////////////////////////////////////////////////

	parameter  LOW =1'b0;
	parameter  HIGH=1'b1;

//////////////////////////////////////////////////////////////////////////////////
// Internal Signals
//////////////////////////////////////////////////////////////////////////////////
	wire wPwrgd_All_VRs_dly_2m;
	wire wPSLP_SUS_dly_1m;
	reg  rFM_SLP_SUS_RSM_RST_N_FF1, rSLP_SUS_N_Fall;
	reg  rPwrgdBmcP1V1_ff1;
	reg  rBmcPwrFltP1V1;
	reg [2:0]rCurrState;

localparam IDLE=0;
localparam ENABLE_VR = 1;
localparam DEASSERT_RST = 2;
localparam BMC_DONE = 3;
localparam BMC_FLT = 4;

//////////////////////////////////////////////////////////////////////////////////
// Secuencial Logic
//////////////////////////////////////////////////////////////////////////////////

always @ ( posedge iClk, negedge iRst_n) begin
	if ( !iRst_n ) begin
			rCurrState <= IDLE;
		end
	else begin
		case (rCurrState)
			ENABLE_VR: begin
				rCurrState <= (wPwrgd_All_VRs_dly_2m ) ? DEASSERT_RST :ENABLE_VR ;
			end
			DEASSERT_RST: begin
				rCurrState <= BMC_DONE ;
			end
			BMC_DONE: begin
				rCurrState <= (PWRGD_P1V1_BMC_AUX && FM_SLP_SUS_N) ? BMC_DONE : BMC_FLT ;
			end
			BMC_FLT: begin
				rCurrState <= iGoOutFltSt ? IDLE : BMC_FLT ;
			end
			default: begin//IDLE
				rCurrState <= wPSLP_SUS_dly_1m ? ENABLE_VR : IDLE ;
			end
		endcase
	end
end

always @ (rCurrState) begin
	case (rCurrState)
		default: begin//IDLE
			FM_BMC_P2V5_AUX_EN              <= LOW;
			RST_SRST_BMC_N                  <= LOW;
			oBmcPwrgd                       <= LOW;
			oBmcPwrFlt                      <= LOW;
		end
		ENABLE_VR: begin
			FM_BMC_P2V5_AUX_EN              <= HIGH;
			RST_SRST_BMC_N                  <= LOW;
			oBmcPwrgd                       <= LOW;
			oBmcPwrFlt                      <= LOW;
		end
		DEASSERT_RST: begin
			FM_BMC_P2V5_AUX_EN              <= HIGH;
			RST_SRST_BMC_N                  <= HIGH;
			oBmcPwrgd                       <= LOW;
			oBmcPwrFlt                      <= LOW;
		end
		BMC_DONE: begin
			FM_BMC_P2V5_AUX_EN              <= HIGH;
			RST_SRST_BMC_N                  <= HIGH;
			oBmcPwrgd                       <= HIGH;
			oBmcPwrFlt                      <= LOW;
		end
		BMC_FLT: begin
			FM_BMC_P2V5_AUX_EN              <= LOW;
			RST_SRST_BMC_N                  <= LOW;
			oBmcPwrgd                       <= LOW;
			oBmcPwrFlt                      <= HIGH;
		end
	endcase
end

/*always @ ( posedge iClk)
	begin
		if (  !iRst_n  )
			begin
				rFM_SLP_SUS_RSM_RST_N_FF1       <= HIGH;
				oBmcPwrFlt                      <= LOW;
				rSLP_SUS_N_Fall                 <= LOW;

				oBmcPwrgd                       <= LOW;

				FM_BMC_P2V5_AUX_EN              <= LOW;

				RST_SRST_BMC_N                  <= LOW;

				rPwrgdBmcP1V1_ff1               <= LOW;
				rBmcPwrFltP1V1                  <= LOW;
			end
		else if ( iGoOutFltSt )
			begin
				oBmcPwrFlt                      <= LOW;
				rBmcPwrFltP1V1                  <= LOW;
			end
		else
			begin
				oBmcPwrFlt                      <= (PWRGD_P3V3_AUX && (rBmcPwrFltP1V1)) ? HIGH: oBmcPwrFlt;  //detect BMC VR fault
				rFM_SLP_SUS_RSM_RST_N_FF1       <= FM_SLP_SUS_N;
				rSLP_SUS_N_Fall                 <= ~FM_SLP_SUS_N && rFM_SLP_SUS_RSM_RST_N_FF1;

				oBmcPwrgd                       <= PWRGD_P1V1_BMC_AUX; // last VR for BMC are ready.
				FM_BMC_P2V5_AUX_EN              <= (PWRGD_P3V3_AUX && PWRGD_PCH_P1V8 && (wPSLP_SUS_dly_1m));
				rPwrgdBmcP1V1_ff1               <= PWRGD_P1V1_BMC_AUX;

				rBmcPwrFltP1V1                  <= (FM_BMC_P2V5_AUX_EN && !PWRGD_P1V1_BMC_AUX && rPwrgdBmcP1V1_ff1) ? HIGH: rBmcPwrFltP1V1;

				RST_SRST_BMC_N                  <= (rSLP_SUS_N_Fall && oBmcPwrgd) ? LOW : (wPwrgd_All_VRs_dly_2m && RST_DEDI_BUSY_PLD_N);// The RST need be enable before of turn off the BMC
			end
	end*/



//////////////////////////////////////////////////////////////////////
// Instances
//////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////
//% 2ms Delay for power UP  RST_SRST_BMC_N
//
SignalValidationDelay#
(
    .VALUE                  ( 1'b1 ),
    .TOTAL_BITS             ( 2'd2 ),
    .POL                    ( 1'b1 )
)mRST_SRST_BMC_N
(
    .iClk                   ( iClk ),
    .iRst                   ( ~iRst_n ),
    .iCE                    ( i1mSCE ),
    .ivMaxCnt               ( 2'd2 ),        //2ms delay
    .iStart                 ( PWRGD_P1V1_BMC_AUX ),
    .oDone                  ( wPwrgd_All_VRs_dly_2m )
);

//% 1ms Delay for power down  FM_SLP_SUS_N
//
SignalValidationDelay#
(
    .VALUE                  ( 1'b0 ),
    .TOTAL_BITS             ( 1'd1 ),
    .POL                    ( 1'b0 )
)mSLP_SUS
(
    .iClk                   ( iClk ),
    .iRst                   ( ~iRst_n ),
    .iCE                    ( i1mSCE ),
    .ivMaxCnt               ( 1'd1 ),        //1ms delay
    .iStart                 ( FM_SLP_SUS_N ),
    .oDone                  ( wPSLP_SUS_dly_1m )
);

endmodule
