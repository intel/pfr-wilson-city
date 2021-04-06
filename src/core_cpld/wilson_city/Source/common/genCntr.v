/* ==========================================================================================================
   = Project     :  Purley Commercial PLD
   = Title       :  Generic counter
   = File        :  genCntr.v
   ==========================================================================================================
   = Author      :  Bruce Liu;Roxana
   = Departure   :  
   = E-mail      : bruce.z.liu@intel.com
   ==========================================================================================================
   = Description :  This module implements a generic counter, user just need to provide the number of clock
                    cycles to count and the vector length of all internal variables will be computed automati-
                    cally by the module itself with the help of the logb2 function. 
   
                    When the module is out of reset and the counter_en input is asserted, the internal counter 
                    starts to increment, for each clock cycle, from zero to MAX_COUNT. When this last value is 
                    reached, the module assert a done output signal to indicate the event, the counter will not 
                    restart to zero until its reset input is asserted.
                   
   = Constraints :  
   
   = Notes       :  
   
   ==========================================================================================================
   = Revisions   :  
   1.0 September 22, 2009;  rev. 1.0;  first version by Marco Jacobo 
   1.1 March 23, 2013, Rev 1.1: Put counter_reg as internal
   1.2 March 19, 2015, modify the module as per the new PLD design rules
   
   ========================================================================================================== */


module genCntr
#(
   //================= Parameters ==============
     parameter MAX_COUNT = 1000                       //number of clock cycles to count
)
(  //================= Outputs ================= 
     output wire  oCntDone,                    //It is high when max count has been reached 

   //================= Inputs =================
     input  wire  iClk,                          //Clock signal
     input  wire  iCntEn,	                   //Counter enable 
     input  wire  iRst_n,                        //reset
     input  wire  iCntRst_n,                   //synchronous reset
	  
     output reg   [logb2(MAX_COUNT) : 0]  oCntr	
);

   //+----+----+----+----+----+----+----+----+----+
   //Local functions
   //+----+----+----+----+----+----+----+----+----+	
   
	 function integer logb2 ( input integer size );
	    integer size_buf;
     begin
          size_buf = size;
			for(logb2=-1; size_buf>0; logb2=logb2+1) size_buf = size_buf >> 1;
     end
     endfunction

	  	  
   //-----//-----//-----//-----//-----//-----//-----//-----//-----//-----//-----//-----//	 
     always @ ( posedge iClk or negedge iRst_n ) 
     begin
 
           if ( !iRst_n )               oCntr  <= 0; 
           else if( !iCntRst_n )       oCntr   <= 0; 
           else 
           begin
						
                if ( oCntr == MAX_COUNT )   
                begin 
				    oCntr   <= oCntr;

			    end
			    else if(iCntEn)  
                begin             
				   oCntr   <= oCntr + 1'b1;
                end
			    else  
                begin                          
				   oCntr   <= oCntr; 
    			end
           end		   
     end      
	  
   //-----//-----//-----//-----//-----//-----//-----//-----//-----//-----//-----//-----//	
    assign oCntDone = ( oCntr == MAX_COUNT ) ? 1'b1 : 1'b0;
	  

	 
endmodule
