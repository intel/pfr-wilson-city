
//////////////////////////////////////////////////////////////////////////////////
/*!

    \brief    <b>%Mux128BitsTo8 </b>
    \file     Mux128BitsTo8.v
    \details    
                 <b>Description:</b> \n

                The module works as 128 bit to 8 bits, the selector will sent the output group by 8 bits:\n

                Example:\n
                
                iSel | Bits on OvSignal
                0|ivSignals[7:0]
                1|ivSignals[15:8]
                2|ivSignals[23:16]
                3|ivSignals[31:24]
                4|ivSignals[39:32]
                5|ivSignals[47:40]
                6|ivSignals[55:48]
                7|ivSignals[63:56]
                8|ivSignals[71:64]
                9|ivSignals[79:72]
                A|ivSignals[87:80]
                B|ivSignals[95:88]
                C|ivSignals[103:96]
                D|ivSignals[111:104]
                E|ivSignals[119:112]
                F|ivSignals[127:120]
                
               only combinational to get the real time signal (witout Clock sampling).\n

                
                <b>Block Diagram:</b>
    \verbatim
        +-----------------------------------------+
 -----> | iSel [3:0]  .       .  ovSignals [7:0]  |----->
 -----> | ivSignals [127:0]   .            .      |
        +-----------------------------------------+
                           Mux128BitsTo8
    \endverbatim   


    \brief  <b>Last modified</b> 
            $Date:   March 2, 2017 $
            $Author:  David.bolanos@intel.com $         
            Project         : Mehlow  
            Group           : BD
    \version    
             20170302 \b  David.bolanos@intel.com - File creation\n
               
    \copyright Intel Proprietary -- Copyright \htmlonly <script>document.write(new Date().getFullYear())</script> \endhtmlonly  Intel -- All rights reserved
*/
//////////////////////////////////////////////////////////////////////////////////



module Mux128BitsTo8 //The selector can select by groups of 8 
(
	input  [3:0]    		  iSel, //%Selector this will select what group you need measure.
	input  [127:0]            ivSignals,
	output reg [7:0]          ovSignals	 
);



//////////////////////////////////////////////////////////////////////////////////
// Combinational logic
//////////////////////////////////////////////////////////////////////////////////
//%Combinational logic<br>
always @*
begin
    // Combinational assigments
    case ( iSel ) 
                4'h0:       ovSignals    =   ivSignals[7:0];
                4'h1:       ovSignals    =   ivSignals[15:8];
                4'h2:       ovSignals    =   ivSignals[23:16];
                4'h3:       ovSignals    =   ivSignals[31:24];
                4'h4:       ovSignals    =   ivSignals[39:32];
                4'h5:       ovSignals    =   ivSignals[47:40];
                4'h6:       ovSignals    =   ivSignals[55:48];
                4'h7:       ovSignals    =   ivSignals[63:56];
                4'h8:       ovSignals    =   ivSignals[71:64];
                4'h9:       ovSignals    =   ivSignals[79:72];
                4'hA:       ovSignals    =   ivSignals[87:80];
                4'hB:       ovSignals    =   ivSignals[95:88];
                4'hC:       ovSignals    =   ivSignals[103:96];
                4'hD:       ovSignals    =   ivSignals[111:104];
                4'hE:       ovSignals    =   ivSignals[119:112];
                4'hF:       ovSignals    =   ivSignals[127:120];
                default:    ovSignals    =   ivSignals[7:0];             
            endcase
end
endmodule