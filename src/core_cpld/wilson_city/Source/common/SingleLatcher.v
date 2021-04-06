
//////////////////////////////////////////////////////////////////////////////////
/*!

	\brief    <b>%SingleLatcher </b>
	\file     SingleLatcher.v
	\details    
				 <b>Description:</b> \n

				the iSignalLatch will be latch based on 2 conditions if iEnableLatch = 1 and when oSignalLatched = EDGELATCH,
				the oSignalLatched can only be reset on 2 conditions:\n

				* iRst_n = 0
				* iEnableLatch =0
				
				the EDGELATCH is used to determinate if the latch will be capture on posedge or negedge:\n

				* EDGELATCH = 0, the latch will capture in transition from 1 to 0.  
				* EDGELATCH = 1, the latch will capture in transition from 0 to 1.  

                
                <b>Block Diagram:</b>
    \verbatim
        +-----------------------------------------+
 -----> |> iClk     .       .      oSignalLatched |----->
 -----> |  iRst_n   .       .       .       .     |
 -----> |  iEnableLatch     .       .       .     |
 =====> |  iSignalLatch     .       .       .     |
        +-----------------------------------------+
                           SingleLatcher
    \endverbatim   


	\brief  <b>Last modified</b> 
			$Date:   March 2, 2017 $
			$Author:  David.bolanos@intel.com $			
			Project			: Mehlow  
			Group			: BD
	\version    
			 20170302 \b  David.bolanos@intel.com - File creation\n
			   
	\copyright Intel Proprietary -- Copyright \htmlonly <script>document.write(new Date().getFullYear())</script> \endhtmlonly  Intel -- All rights reserved
*/
//////////////////////////////////////////////////////////////////////////////////



module SingleLatcher#
(

	parameter        EDGELATCH = 1

)
(
	input		        iClk,//%Clock input 
	input		        iRst_n,//%Reset enable on low
	input 				iEnableLatch,
	input               iSignalLatch,

	output              oSignalLatched

);



//////////////////////////////////////////////////////////////////////////////////
// Internal Signals
//////////////////////////////////////////////////////////////////////////////////

wire wLatchClr;
wire wLatchSet;
wire wSignalLatched;

//////////////////////////////////////////////////////////////////////////////////
// Continuous assignments
//////////////////////////////////////////////////////////////////////////////////

//since the latch is really control by clear condition depending of EDGELATCH
//by this reason is needed invert for the case when the condition of EDGELATCH=1

assign oSignalLatched = EDGELATCH ? ~wSignalLatched : wSignalLatched;

//////////////////////////////////////////////////////////////////////////////////
// Instances
//////////////////////////////////////////////////////////////////////////////////

EdgeDetector #
(
    .EDGE                   ( 1 )
)mLatchSet
(
    .iClk                   ( iClk ),
    .iRst                   ( ~iRst_n ),
    .iSignal                ( iEnableLatch ),              
    .oEdgeDetected          ( wLatchSet )
);

EdgeDetector #
(
    .EDGE                   ( EDGELATCH )
)mLatchClr
(
    .iClk                   ( iClk ),
    .iRst                   ( ~iRst_n ),
    .iSignal                (iSignalLatch), 
    .oEdgeDetected          ( wLatchClr )
);


FF_SR mLatchOut
(
    .iClk                   ( iClk ),
    .iRst                   ( wLatchClr),
    .iSet                   ( wLatchSet || ~iRst_n),
    .oQ                     ( wSignalLatched )
);

endmodule