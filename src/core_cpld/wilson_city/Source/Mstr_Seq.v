
//////////////////////////////////////////////////////////////////////////////////
/*!

    \brief    <b>%Master Sequence Control </b>
    \file     Mstr_Seq.v
    \details    <b>Image of the Block:</b>
                \image html Module-Mstr_Seq.PNG

                 <b>Description:</b> \n
                Master FSM (Finite State Machine) block is the KEY Module that controls system power state transition,
                synchronizing the VRs on/off and verifying that the PCH sends the appropriate signals to check that the sequence is correct.
                 The FSM is designed to comply with the Wilson City RP, Architecture diagrams <a href="https://sharepoint.amr.ith.intel.com/sites/WhitleyPlatform/commoncore_arch_wg/SitePages/Home.aspx" target="_blank"><b>Here</b></a>.\n

                The Mstr_Seq.v module has control of PSU_Seq.v, Mem_Seq.v, Cpu_Seq.v and MainVR_Seq.v for turn ON/OFF the VR's and also receive fault condition from each module.
                 The standby VR are controlled by Bmc_Seq.v and Pch_Seq.v and the Mstr_Seq.v receive the PWRGD for change Machine state from STBY to OFF. \n
                 The System verification related to valid CPU mix configurations this module use iSysOk to guarantee the system can turn on.\n

                This table contains each state of the FSM S5 -> S0 ->S5.

                State Hex  | State Name | Next State | Condition | Details
                -----------|------------|------------|-----------|-----------
                A          | ST_STBY    | ST_OFF     | RST_RSMRST_PCH_N & RST_SRST_BMC_N & wPwrgd_All_Aux =1 | Default State during AC-On. The system is waiting for all the STBY rails ready to trigger PCH RSMRST# as well as SRST to BMC.
                9          | ST_OFF (S5/S4)     | ST_PS      | wPwrEn=1  | Maps to S4/S5 system Status. All the STBY rails are ready. Waiting for Wake-up event to wait up to S0.
                7          | ST_PS      | ST_MAIN     | iPSUPwrgd=1 | PSU DC Enable signal has been asserted by PLD and waiting for PSU PS_PWROK.
                6          | ST_MAIN    | ST_MEM     | iMainVRPwrgd=1 | Main VR has been enable by PLD and wait iMainVRPwrgd=1 from MainVR_Seq.v
                5          | ST_MEM     | ST_CPU     | iMemPwrgd=1 | Memory VR has been enabled by PLD and PLD is waiting for Memory rails ready.
                4          | ST_CPU     | ST_SYSPWROK| iCpuPwrgd =1 | PLD has triggered the CPU Main rails enable sequence and waiting for the indication of all the CPU Main rail ready.
                3          | ST_SYSPWROK| ST_CPUPWRGD| PWRGD_SYS_PWROK=1 | An intermediate state to delay 100ms before asserting the SYS_PWROK and wait for Nevo Ready.
                2          | ST_CPUPWRGD| ST_RESET   | PWRGD_CPUPWRGD =1 | Waiting for CPU_PWRGD from PCH.
                1          | ST_RESET   | ST_DONE    | RST_PLTRST_N =1 | System is in Reset Status, Waiting for PLTRST de-assertion.
                0          | ST_DONE (S0)   | ST_SHUTDOWN| wPwrEn=0 | All the bring-up power sequence has been completed, and the BIOS shall start to execute since st_done.
                B          | ST_SHUTDOWN| ST_MAIN_OFF  | iCpuPwrgd=0 | System is in the shutdown process. Waiting for the indication of all CPU Power rails has been turned off.
				C          | ST_MAIN_OFF | ST_PS_OFF  | iMainVRPwrgd=0 | Waiting for the indication of all Main VR OFF.
                D          | ST_PS_OFF  | ST_S3      | FM_SLPS3_N=0| PSU is instructed to turn off and PLD is waiting for the assertion of SLPS3
                8          | ST_S3      | ST_OFF     | !FM_SLPS4_N && !iMemPwrgd = 1 | System in S3 Status with STBY and Memory rails On.
                F          | ST_FAULT   | ST_STBY    | oGoOutFltSt=1 | System in fault State due to various power failures and stay at fault status until the next power on command.

                \n\n
                For the complete flow including the fault condition, S3, shutdown, please review the next diagram:
                \image html Master-Seq.png



    \brief  <b>Last modified</b>
            $Date:   Jan 19, 2018 $
            $Author:  David.bolanos@intel.com $
            Project            : Wilson City
            Group            : BD
    \version
             20160709 \b  David.bolanos@intel.com - File creation\n
             20181901 \b  David.bolanos@intel.com - Mofify to adapt to Wilson RP, leverage from Mehlow\n
             20182803 \b  jorge.juarez.campos@intel.com - Added support for S3 State\n

    \copyright Intel Proprietary -- Copyright \htmlonly <script>document.write(new Date().getFullYear())</script> \endhtmlonly Intel -- All rights reserved
*/
//////////////////////////////////////////////////////////////////////////////////
module Mstr_Seq
(
	input		        iClk,//%Clock input 2 Mhz
	input		        iRst_n,//%Reset enable on low
	input               iForcePwrOn,//% Force the system to turn on (by only PCH Signals)
	input               iEnableGoOutFltSt,//% This signal enable the system go out Fault
	input               iEnableTimeOut,//% Enable the Timeout

    input               iSysOk, //% System validation Ok
    input               iDebugPLD_Valid, //% PLD validation Ok

	input               iMemPwrgd,//% Pwrgd of all Mem VR's
	input               iMemPwrFlt,//% Pwrgd fault of all Mem VR's

	input               iCpuPwrgd,//% Pwrgd of all Cpu VR's
	input               iCpuPwrFlt,//% Pwrgd fault of all Cpu VR's

	input               iBmcPwrgd,//% Pwrgd of all Bmc VR's
	input               iBmcPwrFlt,//% Pwrgd fault of all Bmc VR's

	input               iPchPwrgd,//% Pwrgd of all Pch VR's
	input               iPchPwrFlt,//% Pwrgd fault of all Pch VR's

	input               iPSUPwrgd,//% Pwrgd of Power Supply
	input               iPsuPwrFlt,//% Pwrgd fault of Power Supply

	input               iMainVRPwrgd,//% Pwrgd of Main Vr's
	input               iMainPwrFlt,//% Pwrgd fault of Main Vr's

	input 				iSocketRemoved,//% Go to Fault State when Socked is removed

	input               PWRGD_P3V3_AUX,//% Pwrgd from P3V3 Aux VR
	input               PWRGD_SYS_PWROK,//% Pwrgd SYS generate by PLD
	input               PWRGD_CPUPWRGD,//% Pwrgd CPU from PCH

	input               FM_BMC_ONCTL_N,//% ONCTL  coming from BMC

	input               FM_SLPS4_N,//% SLP4# from PCH
	input               FM_SLPS3_N,//% SLP3# from PCH
	input               RST_PLTRST_N,//% PLTRST# from PCH
	input               RST_RSMRST_PCH_N,//% RSMRST# from PLD
	input               RST_SRST_BMC_N,//% SRST RST# from BMC

inout FM_ADR_TRIGGER_N,

	output reg          oMemPwrEn,//% Memory VR's enable (For module Mem_Seq).
	output reg          oCpuPwrEn,//% Cpu VR's enable (For module Cpu_Seq).
	output reg          oPsuPwrEn,//% Psu enable (For module PSU_Seq).
	output reg          oMainVRPwrEn,//% Main VR enable (For module MainVR_Seq).

	output              oGoOutFltSt,//% Go out fault state.
	output              oTimeOut,//% Time-out if the Turn ON/OFF take more of 2s the system will force shutdown

	output              oFault,//% Fault Condition
	output 		        oPwrEn, //% PWREN

	output reg [3:0]	oDbgMstSt7Seg,//Code according RPAS 3.7.9 Code Guideline
	output     [3:0]    oDbgMstSt     //Normal Code from 0x0 to 0xC (Done state is 0xA)
);


`ifdef FAST_SIM_MODE
localparam SIMULATION_MODE =  1'b1;
`else
localparam SIMULATION_MODE =  1'b0;
`endif


localparam	ST_FAULT	= 4'h0;
localparam	ST_STBY		= 4'h1;
localparam	ST_OFF		= 4'h2;
localparam	ST_S3		= 4'h3;
localparam	ST_PS		= 4'h4;
localparam	ST_MAIN		= 4'h5;
localparam	ST_MEM		= 4'h6;
localparam	ST_CPU		= 4'h7;
localparam	ST_SYSPWROK	= 4'h8;
localparam	ST_CPUPWRGD	= 4'h9;
localparam	ST_RESET	= 4'hA;
localparam	ST_DONE		= 4'hB;
localparam	ST_SHUTDOWN	= 4'hC;
localparam	ST_MAIN_OFF	= 4'hD;
localparam	ST_PS_OFF	= 4'hE;

localparam	LOW =1'b0;
localparam	HIGH=1'b1;

localparam	T_2S_2M		=  SIMULATION_MODE ? 24'd100 : 24'd4000000;
localparam	T_5S_2M		=  SIMULATION_MODE ? 24'd100 : 24'd10000000;
localparam	T_100MS_2M	=  SIMULATION_MODE ? 24'd100 : 24'd200000;
localparam	T_10MS_2M	=  SIMULATION_MODE ? 24'd100 : 24'd100000;
localparam	T_5MS_2M	=  SIMULATION_MODE ? 24'd100 : 24'd10000;

localparam bitH			= 5'd23;

//////////////////////////////////////////////////////////////////////////////////
// Internal Signals
//////////////////////////////////////////////////////////////////////////////////
reg [3:0] rCurrSt, rNextSt, rCurrSt_dly;
reg [bitH:0] setCount;
reg FM_BMC_ONCTL_N_ff;
wire wPwrgd_All_Aux;
wire wRst_Rsmrst_Srst;
wire wPwrEn;
wire wWatchDogEn;
wire wCntRst_n,wCntDone,wCntEn,wTimeOutX;
wire wPsuTimeOut;
wire wCntDoneX, wPsuWatchDogEn, wPsuTimeOutX;

counter2 #( .MAX_COUNT(T_5S_2M) )
		counter

(       .iClk         	(iClk),
		.iRst_n       	(iRst_n),
		.iCntRst_n  	(wCntRst_n),
		.iCntEn      	(wCntRst_n & wCntEn),
		.iLoad        	(!(wCntRst_n & wCntEn)),
		.iSetCnt    	(setCount),
		.oDone        	(wCntDoneX),
		.oCntr         	(             )
);

counter2 #( .MAX_COUNT(T_2S_2M) )
	WatchDogTimer
(       .iClk         	(iClk),
		.iRst_n       	(iRst_n),
		.iCntRst_n  	(wCntRst_n),
		.iCntEn      	(wCntRst_n & wWatchDogEn),
		.iLoad        	(!(wCntRst_n & wWatchDogEn)),
		.iSetCnt    	(T_2S_2M [23 : 0]),  // set the time out time as 2s
		.oDone        	(wTimeOutX),
		.oCntr         	(             )
);

counter2 #( .MAX_COUNT(T_2S_2M) )
	WatchDogTimer_PSU
(       .iClk         	(iClk),
		.iRst_n       	(iRst_n),
		.iCntRst_n  	(wCntRst_n),
		.iCntEn      	(wCntRst_n & wPsuWatchDogEn),
		.iLoad        	(!(wCntRst_n & wPsuWatchDogEn)),
		.iSetCnt    	(T_2S_2M [23 : 0]),  // set the time out time as 2s
		.oDone        	(wPsuTimeOutX),
		.oCntr         	(             )
);

assign oDbgMstSt 		= rCurrSt;
assign wPwrgd_All_Aux	= iBmcPwrgd && iPchPwrgd && PWRGD_P3V3_AUX;
assign wRst_Rsmrst_Srst	= RST_RSMRST_PCH_N && RST_SRST_BMC_N;
assign wPwrEn 			= (!FM_BMC_ONCTL_N     	&&
							iSysOk             	&&
							!oFault) || iForcePwrOn;
assign oPwrEn 			= wPwrEn;
assign oGoOutFltSt  	= ( (rCurrSt == ST_FAULT) && wCntDone  && iEnableGoOutFltSt && (!FM_BMC_ONCTL_N && FM_BMC_ONCTL_N_ff) ) ; //stay at fault status until the next power on command.

assign oFault 			= (iMainPwrFlt		||
							iPsuPwrFlt		||
							iBmcPwrFlt		||
							iPchPwrFlt		||
							iMemPwrFlt		||
							iCpuPwrFlt		||
							iSocketRemoved) &&
							!iForcePwrOn;

assign wWatchDogEn		=	(	(rCurrSt == ST_STBY)	||//watch dog timer, set watch dog timer for PSU and main VRs
								(rCurrSt == ST_PS)		||
								(rCurrSt == ST_MAIN)	||
								(rCurrSt == ST_MEM)		||
								(rCurrSt == ST_SHUTDOWN));
assign wPsuWatchDogEn	= (rCurrSt == ST_PS_OFF);//watch dog timer for PSU shut down, in some systems, the PSU is always on, and need to support the P12V always on system

assign wCntRst_n 		= rCurrSt == rCurrSt_dly;
assign wCntDone 		= wCntRst_n && wCntDoneX;
assign wCntEn      		= (rCurrSt == ST_MEM ) || (rCurrSt == ST_FAULT);
assign wPsuTimeOut 		= wPsuTimeOutX && wCntRst_n && iEnableTimeOut;
assign oTimeOut 		= wTimeOutX && wCntRst_n && iEnableTimeOut;

always @ (*) begin
	case( rCurrSt )
	ST_MEM           :   setCount = T_10MS_2M [bitH:0];
	ST_FAULT         :   setCount = T_5S_2M [bitH : 0];
	default          :   setCount = T_5S_2M [bitH : 0];
	endcase
end

always @ ( posedge iClk, negedge iRst_n) begin
	if (!iRst_n) begin
		rCurrSt		<= ST_STBY;
		rCurrSt_dly	<= ST_STBY;// the dly is used to compare to the next state to reset the counter.
	end
	else begin
		case(rCurrSt)
			ST_FAULT: begin
				rCurrSt <=
				(oGoOutFltSt)	?	ST_STBY	://0  //F
									ST_FAULT;
			end
			ST_STBY: begin
				rCurrSt <=
				(oFault || oTimeOut)								?	ST_FAULT: 	//1 //A
				(wPwrgd_All_Aux && wRst_Rsmrst_Srst || iForcePwrOn)	?	ST_OFF	:
																		ST_STBY	;
			end
			ST_OFF: begin
				rCurrSt <=
				(oFault)	?	ST_FAULT: 	//2 //9
				(wPwrEn)	?	ST_PS	:
								ST_OFF	;
			end
			ST_S3: begin
				rCurrSt <=
				(oFault)							?	ST_FAULT: 	//3 //8
				(!FM_SLPS4_N && !iMemPwrgd )		?	ST_OFF	:
				(wPwrEn)							?	ST_PS	:
														ST_S3	;
			end
			ST_PS: begin
					rCurrSt <=
					(iPsuPwrFlt)					?	ST_PS_OFF	:
					(iPSUPwrgd)						?	ST_MAIN		:
														ST_PS		;   		//4 //7
			end
			ST_MAIN: begin
					rCurrSt <=
					(!wPwrEn || oTimeOut)			?	ST_MAIN_OFF	://5 //6
					(iMainVRPwrgd)					?	ST_MEM		:
														ST_MAIN		;
			end
			ST_MEM: begin
					rCurrSt <=
					(!wPwrEn || oTimeOut)			?	ST_SHUTDOWN	://6 	//5
					(iMemPwrgd && wCntDone)			?	ST_CPU		:// wCntDone is used as a watchdog timer when a power fault is encountered.
														ST_MEM		;//With rst signal this will be held on 1, until a power fault
			end				  							//occurs and clears this up to wait for the 5s delay
			ST_CPU: begin
					rCurrSt <=
					(!wPwrEn || oTimeOut)			?	ST_SHUTDOWN	: 	//7    //4
					(iCpuPwrgd)						?	ST_SYSPWROK	:
														ST_CPU		;
			end
			ST_SYSPWROK: begin
				rCurrSt <=
				(!wPwrEn)							?	ST_SHUTDOWN	:	//8 //3
				(PWRGD_SYS_PWROK || iForcePwrOn)	?	ST_CPUPWRGD	:
														ST_SYSPWROK	;
			end
			ST_CPUPWRGD: begin
				rCurrSt <=
				(!wPwrEn)							?	ST_SHUTDOWN	://9 //2
				(PWRGD_CPUPWRGD || iForcePwrOn )	?	ST_RESET	:
														ST_CPUPWRGD	;
			end
			ST_RESET: begin
				rCurrSt <=
				(!wPwrEn)							?	ST_SHUTDOWN	:	//A //1
				(RST_PLTRST_N || iForcePwrOn)		?	ST_DONE		:
														ST_RESET	;
			end
			ST_DONE: begin
				rCurrSt <=
				(!wPwrEn )							?	ST_SHUTDOWN	://B  //0
				(!RST_PLTRST_N)						?	ST_RESET	:
														ST_DONE		;
			end
			ST_SHUTDOWN: begin
				rCurrSt <=
				(~iCpuPwrgd || oTimeOut)			?	ST_MAIN_OFF	:	//C    //B
														ST_SHUTDOWN	;
			end
			ST_MAIN_OFF : begin
				rCurrSt <=
				(~iMainVRPwrgd || oTimeOut)			?	ST_PS_OFF	:  	//D // C
														ST_MAIN_OFF	;
			end
			ST_PS_OFF: begin
				rCurrSt <=
				(!FM_SLPS3_N)						?	ST_S3		://E     //D
				(oFault)							?	ST_FAULT	:
				(wPsuTimeOut)						?	ST_S3		:
														ST_PS_OFF	;
			end
			default: begin
				rCurrSt <= ST_STBY;
			end
		endcase
		rCurrSt_dly  <= rCurrSt;
		end
	end

	always @(rCurrSt, FM_SLPS4_N)
		begin
		case(rCurrSt)
			ST_FAULT:
				begin
				oPsuPwrEn		= LOW;
				oMainVRPwrEn	= LOW;
				oMemPwrEn		= LOW;
				oCpuPwrEn		= LOW;
				end
			ST_STBY:
				begin
				oPsuPwrEn		= LOW;
				oMainVRPwrEn	= LOW;
				oMemPwrEn		= LOW;
				oCpuPwrEn		= LOW;
				end
			ST_OFF:
				begin
				oPsuPwrEn		= LOW;
				oMainVRPwrEn	= LOW;
				oMemPwrEn		= LOW;
				oCpuPwrEn		= LOW;
				end
			ST_S3:
				begin
				oPsuPwrEn		= LOW;
				oMainVRPwrEn	= LOW;
				oMemPwrEn		= FM_SLPS4_N;
				oCpuPwrEn		= LOW;
				end
			ST_PS:
				begin
				oPsuPwrEn		= HIGH;
				oMainVRPwrEn	= LOW;
				oMemPwrEn		= FM_SLPS4_N;
				oCpuPwrEn		= LOW;
				end
			ST_MAIN:
				begin
				oPsuPwrEn		= HIGH;
				oMainVRPwrEn	= HIGH;
				oMemPwrEn		= FM_SLPS4_N;
				oCpuPwrEn		= LOW;
				end
			ST_MEM:
				begin
				oPsuPwrEn		= HIGH;
				oMainVRPwrEn	= HIGH;
				oMemPwrEn		= HIGH;
				oCpuPwrEn		= LOW;
				end
			ST_CPU:
				begin
				oPsuPwrEn		= HIGH;
				oMainVRPwrEn	= HIGH;
				oMemPwrEn		= HIGH;
				oCpuPwrEn		= HIGH;
				end
			ST_SYSPWROK:
				begin
				oPsuPwrEn		= HIGH;
				oMainVRPwrEn	= HIGH;
				oMemPwrEn		= HIGH;
				oCpuPwrEn		= HIGH;
				end
			ST_CPUPWRGD:
				begin
				oPsuPwrEn		= HIGH;
				oMainVRPwrEn	= HIGH;
				oMemPwrEn		= HIGH;
				oCpuPwrEn		= HIGH;
				end
			ST_RESET:
				begin
				oPsuPwrEn		= HIGH;
				oMainVRPwrEn	= HIGH;
				oMemPwrEn		= HIGH;
				oCpuPwrEn		= HIGH;
				end
			ST_DONE:
				begin
				oPsuPwrEn		= HIGH;
				oMainVRPwrEn	= HIGH;
				oMemPwrEn		= HIGH;
				oCpuPwrEn		= HIGH;
				end
			ST_SHUTDOWN:
				begin
				oPsuPwrEn		= HIGH;
				oMainVRPwrEn	= HIGH;
				oMemPwrEn		= HIGH;
				oCpuPwrEn		= LOW;
				end
			ST_MAIN_OFF : begin
				oPsuPwrEn		= HIGH;
				oMainVRPwrEn	= LOW;
				oMemPwrEn		= HIGH;
				oCpuPwrEn		= LOW;
			end
			ST_PS_OFF: begin
				oPsuPwrEn		= LOW;
				oMainVRPwrEn	= LOW;
				oMemPwrEn		= HIGH;
				oCpuPwrEn		= LOW;
			end
			default: begin
				oPsuPwrEn		= LOW;
				oMainVRPwrEn	= LOW;
				oMemPwrEn		= LOW;
				oCpuPwrEn		= LOW;
			end
		endcase
		end
//////////////////////////////////////////////////////////////////////
// Local functions
//////////////////////////////////////////////////////////////////////

// This converter is used to show the PAS states in the 7-segment

	always @ (*)
		begin
			case (rCurrSt)
				ST_FAULT: 			oDbgMstSt7Seg = 4'hF;
				ST_STBY: 			oDbgMstSt7Seg = 4'hA;
				ST_OFF: 			oDbgMstSt7Seg = 4'h9;
				ST_S3:  			oDbgMstSt7Seg = 4'h8;
				ST_PS:  			oDbgMstSt7Seg = 4'h7;
				ST_MAIN:            oDbgMstSt7Seg = 4'h6;
				ST_MEM: 	        oDbgMstSt7Seg = 4'h5;
				ST_CPU:     		oDbgMstSt7Seg = 4'h4;
				ST_SYSPWROK: 		oDbgMstSt7Seg = 4'h3;
				ST_CPUPWRGD: 		oDbgMstSt7Seg = 4'h2;
				ST_RESET: 			oDbgMstSt7Seg = 4'h1;
				ST_DONE: 			oDbgMstSt7Seg = 4'h0;
				ST_SHUTDOWN: 		oDbgMstSt7Seg = 4'hB;
				ST_MAIN_OFF:        oDbgMstSt7Seg = 4'hC;
				ST_PS_OFF: 			oDbgMstSt7Seg = 4'hD;
				default: 			oDbgMstSt7Seg = 4'hE;
			endcase
		end

endmodule
