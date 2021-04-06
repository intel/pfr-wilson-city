//*****************************************************************************
//* INTEL CONFIDENTIAL
//*
//* Copyright(c) Intel Corporation (2015)
//****************************************************************************
//
// DATE:      23/3/2015
// ENGINEER:  Liu, Bruce Z 
// EMAIL:     bruce.z.liu@intel.com           
// FILE:      counter.v
// BLOCK:     
// REVISION:  
//	0.1   initial version, leveraged from Grantley common core
//
// DESCRIPTION:	time delay module. 
// This module describes a binary counter that counts, when its iCntEn input is high, from zero
//    to a a max value in steps of one unit per each rise edge of the iClk signal. The max value is
//    specified in the "iSetCnt" input; once the max value is reached by the counter a "oDone"
//    flag is set to acknowledge this condition. 
    
//    The module has a syncronous reset input that clears the counter, but not the max value which
//    is set to all 1s from reset. The module also has a iLoad signal to update the max value from
//    the iSetCnt input. The module will ignore the iLoad signal if iCntEn is high.
    
//    Note the module parameter will define the length of the internal register, in such a way that
//    it imposes a restriction on _if.iSetCnt which can not be higher than the MAX_COUNT parameter.
 
//   
// ASSUMPTIONS:
//   
//**************************************************************************** 



 module counter2
 #(                                                     // ================= Parameters ==============
       parameter MAX_COUNT = 100                          // maximum number of clock cycles to count;
 )                                                      // iSetCnt should be smaller than this one.   
 (
                                                        // ================= Inputs =================
       input    iClk,                               // clock signal
       input    iRst_n,                             // reset

       input    iCntRst_n,                        // synchronous reset
       input    iCntEn ,                           // counter iCntEn 
       input    iLoad,                              // iLoad MAX_COUNT internal register	  
       input    [logb2(MAX_COUNT) : 0] iSetCnt,    //	      	 

                                                        // ================= Outputs =================   
       output   oDone,                              // It is high when max count has been reached       
       output  reg [logb2(MAX_COUNT) : 0] oCntr          // Current value of the counter
                                                        // unsigned MAX_COUNT-bits variable
 );

   //+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
   // Returns the floor of the base 2 log of the "size" number, we use the return 
   // value as the MSB bit in vector size definitions. e.g.; we need 4 bits for the
   // number 13, we need a vector with an index from 3 to 0. 
   //
   // flogb2(from 8 to 15) = 3 
   // flogb2(from 7 to 4 ) = 2
   // flogb2(from 3 to 2 ) = 1   
   //+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
     function automatic integer logb2 ( input integer  size );
         integer size_buf;
         begin
         size_buf =size;
         for( logb2 = -1; size_buf > 0; logb2 = logb2 + 1 )  size_buf = size_buf >> 1;
         end   
     endfunction
   //+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+



   //-----//-----//-----//-----//-----//-----//-----//-----//-----//-----//-----//-----//	 
   // We often need latches simply to store bits of information; 
   // save the value the counter need to reach to assert and output flag. 
   //-----//-----//-----//-----//-----//-----//-----//-----//-----//-----//-----//-----//	 
     reg [logb2(MAX_COUNT) : 0] rMaxCnt;
    
//   always_latch begin : max_value
     always @( posedge iClk or negedge iRst_n ) begin 
			 if (     !iRst_n          )   rMaxCnt   <= {{logb2(MAX_COUNT){1'b1}}, 1'b1};            // fills all bits of MAX_COUNT_oCntr with 1 
                else if( iLoad && !iCntEn )   rMaxCnt   <= iSetCnt; 
             else                          rMaxCnt   <= rMaxCnt; 
     end      
	  

   //-----//-----//-----//-----//-----//-----//-----//-----//-----//-----//-----//-----//	 
   // Increments the conter count in each clock cycle if the iCntEn signal is asserted  	  
   //-----//-----//-----//-----//-----//-----//-----//-----//-----//-----//-----//-----//	 
     always @( posedge iClk or negedge iRst_n ) begin 
 
            if (       !iRst_n      )              oCntr   <= 0; 
            else if(   !iCntRst_n )                oCntr   <= 0; 
            else 
            begin
						
                  if (     oCntr == rMaxCnt )      oCntr   <= oCntr;
                  else if( iCntEn              )   oCntr   <= oCntr + 1'b1;
                  else                             oCntr   <= oCntr;
            end		   
      end      


   //-----//-----//-----//-----//-----//-----//-----//-----//-----//-----//-----//-----//	 
   // Assert done signal once the max count value is reached
   //-----//-----//-----//-----//-----//-----//-----//-----//-----//-----//-----//-----//	 
     assign oDone  =  (oCntr == rMaxCnt)  ?  1'b1 : 1'b0;
	  

   //-----//-----//-----//-----//-----//-----//-----//-----//-----//-----//-----//-----//	

 endmodule
 