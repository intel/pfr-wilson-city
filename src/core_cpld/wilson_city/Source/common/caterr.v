//*****************************************************************************
//* INTEL CONFIDENTIAL
//*
//* Copyright(c) Intel Corporation (2015)
//****************************************************************************
//
// DATE:      4/8/2015
// ENGINEER:  Liu, Bruce Z   
// EMAIL:     bruce.z.liu@intel.com           
// FILE:      caterr.v
// BLOCK:     
// REVISION:  
//	0.1   -- Copied from Grantley caterr delay implementation.
//	0.2   -- Rename some signals, reused from Wolf Pass
//
// DESCRIPTION:	
// caterr delay Function Module. 
//
//   
// ASSUMPTIONS:
//   
//****************************************************************************

module caterr (
	input iClk_50M,
	input iCpuPwrgdDly,
	input RST_PLTRST_N, 
	input FM_CPU_CATERR_PLD_LVT3_N,   //this signal can't be buffered by 2MHz CLK
	
	output FM_CPU_CATERR_DLY_LVT3_N	  



);
parameter STATE_IDLE=4'h1, STATE_DELAY_INPRG=4'h2, STATE_PULSE_INPRG=4'h4, STATE_WAIT_IDLE=4'h8;
//STATE_IDLE: Idle Status. Ready to accept another CATERR Delay Request
//STATE_DELAY_INPRG: Delay In Progress.
//STATE_PULSE_INPRG: In Progress of Generating the Pulse.
//STATE_WAIT_IDLE: Wait CATERR Back to High

//State 
//
//
parameter T_500US_50M = 32'd25000;
parameter T_160NS_50M = 32'd8;
reg [3:0] rCurrSt;


reg rCaterr_n_ff1, rCaterr_n_ff2, rCaterr_n_ff3;

wire wCaterrLowPulse;





wire wDlyComplete_500us;
wire wDlyComplete_160ns;

reg rCaterrDly_n;


assign FM_CPU_CATERR_DLY_LVT3_N = iCpuPwrgdDly ? rCaterrDly_n : 1'b1; //CPUPWRGD is used to avoid glictches at system shut down or power on phase

//>>-------------------------------------------------------------------------------
//GET rCaterr_n_ff2 and rCaterr_n_ff3 and Generate wCaterrLowPulse
always @ (negedge iCpuPwrgdDly or posedge iClk_50M)
	begin
		if(iCpuPwrgdDly == 1'b0)	//PLTRST_N Triggered
			begin
				rCaterr_n_ff1 <= 1'b1;
				rCaterr_n_ff2 <= 1'b1;
				rCaterr_n_ff3 <= 1'b1;
			end
		else
			begin
				rCaterr_n_ff3 <= rCaterr_n_ff2;
				rCaterr_n_ff2 <= rCaterr_n_ff1;
				rCaterr_n_ff1 <= FM_CPU_CATERR_PLD_LVT3_N;
			end
	end
	
assign wCaterrLowPulse = !rCaterr_n_ff2 && rCaterr_n_ff3;	
//<<--------------------------------------------------------------------------------

//>>--------------------------------------------------------------------------------

//<<----------------------------------------------------------------------------------

//>>---------------------------------------------------------------------------------
//State Machine and Generate CATERR_DLY_N
always @ (negedge iCpuPwrgdDly or posedge iClk_50M)
	begin
		if (iCpuPwrgdDly == 1'b0)
			begin
				rCurrSt <= STATE_IDLE;
				rCaterrDly_n <= 1'b1;
			end
		else
        begin
					case(rCurrSt) 
						STATE_IDLE:
							begin
								if (wCaterrLowPulse == 1'b1)
									begin
										rCurrSt <= STATE_DELAY_INPRG; //Switch to STATE_DELAY_INPRG
									end
								rCaterrDly_n <= 1'b1;
							end
						STATE_DELAY_INPRG:
							begin
								if (wDlyComplete_500us == 1'b1)
									begin
										rCurrSt <= STATE_PULSE_INPRG; //Switch to STATE_PULSE_INPRG
									end
								rCaterrDly_n <= 1'b1;
							end
						STATE_PULSE_INPRG:
							begin
								if (wDlyComplete_160ns == 1'b1)
									begin
										rCurrSt <= STATE_WAIT_IDLE; //Switch to STATE_WAIT_IDLE
									end
								rCaterrDly_n <= 1'b0;
							end	
						STATE_WAIT_IDLE:
							begin
								//if (FM_CPU_CATERR_PLD_LVT3_N == 1'b1) 
								  if (rCaterr_n_ff3 == 1'b1)	//Use Buffered one to avoid metastability issue
									begin
										rCurrSt <=  STATE_IDLE; //Switch to STATE_IDLE
										rCaterrDly_n <= 1'b1;
									end
								else
									rCaterrDly_n <= 1'b0;
							end	
                        default: begin
                            rCurrSt <=  STATE_IDLE;
                            rCaterrDly_n <= 1'b1;
                        end

					endcase
				end
	end
//<<--------------------------------------------------------------------------------

//>>--------------------------------------------------------------------------------
//Delay Counter Module
//
//
//
//
   genCntr #(  .MAX_COUNT(T_500US_50M)  ) caterr_dly_500us   
    (
        .oCntDone       (wDlyComplete_500us),           // This is high when MAX_COUNT is reached   
        .iClk           (iClk_50M ), 
        .iRst_n         (RST_PLTRST_N),		               
        .iCntEn         (rCurrSt ==  STATE_DELAY_INPRG  ),	                  
        .iCntRst_n      ((rCurrSt ==  STATE_DELAY_INPRG) ),//
		.oCntr          ( /*empty*/   )
    ); 

   genCntr #(  .MAX_COUNT(T_160NS_50M)  ) caterr_dly_160ns   
    (
        .oCntDone       (wDlyComplete_160ns),           // This is high when MAX_COUNT is reached   
        .iClk           (iClk_50M ), 
        .iRst_n         (RST_PLTRST_N),		               
        .iCntEn         (rCurrSt ==  STATE_PULSE_INPRG  ),	                  
        .iCntRst_n      ((rCurrSt ==  STATE_PULSE_INPRG) ),//
		.oCntr          ( /*empty*/   )
    ); 

 
	
endmodule