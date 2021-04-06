//////////////////////////////////////////////////////////////////////////////////
/*!
    \brief      <b>Parameterizable Delay Module</b>\n
    \details    Sets an output Flag whenever the internal counters reaches a count\n
                fixed by a parameter. The counters can be restarted either\n
                with an asynchronous reset or the input signal iStart.
    \file       Delay.v
    \author     amr/ricardo.ramos.contreras@intel.com
    \date       Oct 31, 2012
    \brief      $RCSfile: Delay.v.rca $
                $Date: Fri Feb  8 16:18:27 2013 $
                $Author: rramosco $
                $Revision: 1.4 $
                $Aliases:  $
                <b>Project:</b> Tazlina Glacier\n
                <b>Group:</b> BD\n
                <b>Testbench:</b> Delay_tb.v\n
                <b>Resources:</b>   <ol>
                                        <li>Spartan3AN
                                    </ol>
                <b>References:</b>  <ol>
                                        <li>
                                    </ol>
                <b>Block Diagram:</b>
    \verbatim
        +-------------------------------------+
 -----> |> iClk     .       .       .   oSignal |----->
 -----> |  iRstN     .       .       .       . |
 -----> |  iSignal  .       .       .       . |
        +-------------------------------------+
                        EdgeDelay
    \endverbatim
----------------------------------------------------------------------------------
    \version
                20190116 \b rramosco - File creation\n
				20200210 \b rramosco - Changed delay to edge delay\n

    \copyright Intel Proprietary -- Copyright 2020 Intel -- All rights reserved
*/
//////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////
//          Delay
//////////////////////////////////////////////////////////////////////////////////
module EdgeDelay #( //delays either a falling or risign edge, regardless of the state value, note this is for edges not logic levels.
	parameter                   COUNT		= 1,	//Number of clock cycles to delay by
	parameter 					RST_VALUE	= 0,	//reset static value
	parameter 					POLARITY	= 0		//0: falling edge, 1: rising edge
)(
    input                       iClk,		//Reference Clock
    input                       iRstN,		//Ascyncronous reset
    input                       iSignal,	//Signal to be delayed
    output                      oSignal		//Delayed signal
);

localparam TOTAL_BITS = clog2(COUNT);//word size big enough to hold count unsigned integer

reg                     rSignal;
reg [(TOTAL_BITS-1):0]  rCount;

//
always @(posedge iClk or negedge iRstN) begin
	if (!iRstN) begin					//Reset
		rSignal	<=	RST_VALUE;			//Done flag
		rCount	<=	{TOTAL_BITS{1'b0}};	//Counter
	end
	else begin
		if(!POLARITY == iSignal) begin		//resets counter on inverse polarity edge
			rSignal	<= !POLARITY;
			rCount	<= {TOTAL_BITS{1'b0}};
		end
		else if (COUNT-1 > rCount) begin	//keeps signal until counter expires
			rSignal	<= rSignal;
			rCount	<= rCount + 1'b1;
		end
		else begin							//counter expired and edge is given
			rSignal	<= POLARITY;
			rCount	<= rCount;
		end
	end
end

assign oSignal = rSignal;//Output
//base 2 logarithm, run in precompile stage
function integer clog2;
input integer value;
begin
value = value-1;
for (clog2=0; value>0; clog2=clog2+1)
value = value>>1;
end
endfunction

endmodule
