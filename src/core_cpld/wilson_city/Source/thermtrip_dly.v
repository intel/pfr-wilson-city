//////////////////////////////////////////////////////////////////////////////////
/*!
    \brief    <b>%thermtrip_dly </b>
    \file     thermtrip_dly.v
    \details    <b>Image of the Block:</b>
                \image html thermtrip_dly.png

                 <b>Description:</b> \n
                The delay the thermtrip by 100us to let BMC log the thermal trip event before system shut down.\n\n 
                
                \image html thermtrip_dly_dia.png
                
    \brief  <b>Last modified</b> 
            $Date:   Jan 19, 2018 $
            $Author:  David.bolanos@intel.com $         
            Project         : Wilson City RP  
            Group           : BD
    \version    
             2016609  \b  David.bolanos@intel.com - File creation\n
             20181901 \b  David.bolanos@intel.com - Mofify to adapt to Wilson RP, leverage from Wolf Pass\n 
               
    \copyright Intel Proprietary -- Copyright \htmlonly <script>document.write(new Date().getFullYear())</script> \endhtmlonly Intel -- All rights reserved
*/
//////////////////////////////////////////////////////////////////////////////////

module thermtrip_dly
(
    input  iClk_2M,                   //2MHz Clock Input
    input  iRst_n,
    input  iCpuPwrgdDly,
    input  FM_CPU1_THERMTRIP_LVT3_N,
    input  FM_CPU2_THERMTRIP_LVT3_N,
    input  FM_MEM_THERM_EVENT_CPU1_LVT3_N,
    input  FM_MEM_THERM_EVENT_CPU2_LVT3_N,
    input  FM_CPU2_SKTOCC_LVT3_N,

    output FM_THERMTRIP_DLY
);
    parameter  T_100US_2M  =  32'd200;
    wire wThermtripDly_100us;
    wire wCpuThermtrip_n;

    // Purley changes the memory thermal trip mechanism, and BIOS will set
    // EN_MEMTRIP=0, so CPU thermtrip will not include memory thermtrip, and
    // memory thermtrip is handled by CPLD
    //
    // add mask logic to mask CPU1 thermtrip and CPU1 therm event if CPU1 is
    // not present
    assign wCpuThermtrip_n = FM_CPU1_THERMTRIP_LVT3_N & (FM_CPU2_THERMTRIP_LVT3_N | FM_CPU2_SKTOCC_LVT3_N) & 
                            FM_MEM_THERM_EVENT_CPU1_LVT3_N & ( FM_MEM_THERM_EVENT_CPU2_LVT3_N | FM_CPU2_SKTOCC_LVT3_N )  ;
    assign FM_THERMTRIP_DLY = iCpuPwrgdDly?  (wThermtripDly_100us): 1'b0 ; 
	
    genCntr #( .MAX_COUNT(T_100US_2M)  ) 
    thermtripDelayCounter    
    (
        .oCntDone   (  wThermtripDly_100us ),     // It is high when cnt == MAX_COUNT.   
       
        .iClk         (  iClk_2M     ),  
        .iRst_n       (  iRst_n ),		               
        .iCntEn      (  ~wCpuThermtrip_n),	  
        .iCntRst_n  ( (~wCpuThermtrip_n) | (~wThermtripDly_100us)),
	.oCntr    ( /*empty*/   )
    ); 	 


endmodule
