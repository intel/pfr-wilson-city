//////////////////////////////////////////////////////////////////////////////////
/*!
    \brief      <b>32 inputs Event Logger</b>\n
    \details    Generates the control signals to write in a memory the event log\n 
                up to 32 signals with an Input Mask vector to select which ones do\n
                you want to keep track of\n
    \file       EventLogger32Bits.v
    \author     amr/carlos.l.bernal@intel.com
    \date       Aug 25, 2012
    \brief      $RCSfile: EventLogger32Bits.v.rca $
                $Date: $
                $Author:  $
                $Revision:  $
                $Aliases: $
                $Log: EventLogger32Bits.v.rca $
                
                <b>Project:</b> FPGA coding\n
                <b>Group:</b> PVE-VHG\n
                <b>Testbench:</b> EventLoggerTB.v\n
                <b>Resources:</b>   <ol>
                                        <li>Spartan3AN 
                                            - 222 Slices
                                    </ol>
                <b>Instances:</b>   <ol>
                                        <li>Counter
                                    </ol>                                    
                <b>References:</b>  <ol>
                                        <li>NeonCity
                                    </ol>
                <b>Block Diagram:</b>
    \verbatim
        +----------------------------------------+
 -----> |> iClk     .       .     ovAddress[9:0] |=====>
 -----> |  iRst     .       .       ovData[31:0] |=====>
 -----> |  iSample  .       .       .       .oWE |----->
 =====> |  ivInputs[31:0]   .       .       .    |
        +----------------------------------------+
                      Event Logger
    \endverbatim
        <b>Flow Diagram:</b>
        \image html EventLoggerFlowChart.jpg
        <b>Block Diagram:</b>
        \image html EventLoggerDiagram.jpg

    \version     
                20120625 \b clbernal - File creation\n
                20130207 \b edgarara - Added Comments, Documentation format and
                                    State Diagram\n    
    \copyright Intel Proprietary -- Copyright 2015 Intel -- All rights reserved
    
*/
//////////////////////////////////////////////////////////////////////////////////
`timescale 1ns / 1ps

//////////////////////////////////////////////////////////////////////////////////
//  Event Logger
//////////////////////////////////////////////////////////////////////////////////
module EventLogger//% 32 inputs Event Logger<br>
(
                    //% Clock Input<br>
    input           iClk,
                    //% Asynchronous Reset Input<br>
    input           iRst, 
                    //% Sample Enable Input Signal<br>
    input           iSample,
                    //% Events Input<br>
    input   [31:0]  ivInputs, 
    //Memory Interface
                    //% Event Memory Address<br>
    output  [9:0]   ovAddress, 
                    //% Event Data Output<br>
    output  [31:0]  ovData,
                    //% Memory Writes Enable Output<br>
    output          oWE
    
);

//////////////////////////////////////////////////////////////////////////////////
// Includes
//////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////
// Defines
//////////////////////////////////////////////////////////////////////////////////

            
localparam  SAMPLE_STATE        =   3'd1;//%1: Sample State<br>            
localparam  SET_EVENT_STATE     =   3'd2;//%2: Set Event State<br>           
localparam  WRITE_EVENT_STATE0  =   3'd3;//%3: Write Event State0<br>           
localparam  SET_TICKS_STATE     =   3'd4;//%4: Set Ticks State<br>          
localparam  WRITE_TICKS_STATE0  =   3'd5;//%5: Write Ticks State0<br>          
localparam  WRITE_TICKS_STATE1  =   4'd6;//%6: Write Ticks State1<br>          
localparam  IDLE_STATE          =   3'd0;//%0: Idle State<br>         

//////////////////////////////////////////////////////////////////////////////////
// Internal Signals
//////////////////////////////////////////////////////////////////////////////////
            //%Last Inputs Registers Data Input<br>
reg [31:0]  rvLastInputs_d;
            //%Last Inputs Registers Output<br>
reg [31:0]  rvLastInputs_q;
            //%State Register Data Input<br>
reg [2:0]   rvState_d;
            //%State Register Output<br>
reg [2:0]   rvState_q;
            //%Sample Enable Register Data Input<br>
reg         rSample_d;
            //%Sample Enable Register Output<br>
reg         rSample_q;
            //%Memory Address Register Data Input<br>
reg [9:0]   rvAddress_d;
            //%Memory Address Register Output<br>
reg [9:0]   rvAddress_q;
            //%Data Log Register Data Input<br>
reg [31:0]  rvData_d;
            //%Data Log Register Output<br>
reg [31:0]  rvData_q;
            //%Memory Write Enable Register Data Input<br>
reg         rWE_d;
            //%Memory Write Enable Register Output<br>
reg         rWE_q;
            //%Number of Ticks (Counter Output)<br>
wire    [31:0]  wvTicks;


`ifdef USE_DEBUG
    (* KEEP = "TRUE" *) reg [6:0]   rvAction_d;
    (* KEEP = "TRUE" *) reg [6:0]   rvAction_q;
`endif
//////////////////////////////////////////////////////////////////////////////////
// Continous assigments
//////////////////////////////////////////////////////////////////////////////////
assign  oWE         =   rWE_q;
assign  ovAddress   =   rvAddress_q;
assign  ovData      =   rvData_q;
//////////////////////////////////////////////////////////////////////////////////
// Sequential logic
//////////////////////////////////////////////////////////////////////////////////
always @(posedge iClk or posedge iRst)
begin
    if(iRst)
    begin
        rvLastInputs_q      <=      32'b0;
        rvState_q           <=      IDLE_STATE;
        rSample_q           <=      1'b0;
        rvAddress_q         <=      10'b0;
        rvData_q            <=      32'b0;
        rWE_q               <=      1'b0;
    end 
    else    
    begin   
        rvState_q           <=      rvState_d;
        rvLastInputs_q      <=      rvLastInputs_d;
        rSample_q           <=      rSample_d;
        rvAddress_q         <=      rvAddress_d;
        rvData_q            <=      rvData_d;
        rWE_q               <=      rWE_d;
    end
end
//////////////////////////////////////////////////////////////////////////////////
// Combinational logic
//////////////////////////////////////////////////////////////////////////////////
always @*
begin
    rSample_d       =   iSample;
    rvLastInputs_d  =   rvLastInputs_q;
    rvAddress_d     =   rvAddress_q;
    rvState_d       =   rvState_q;
    rWE_d           =   1'b0;
    rvData_d        =   rvData_q;
    case (rvState_q)
    SAMPLE_STATE:
    begin
        rvState_d       =   IDLE_STATE;
        rvLastInputs_d  =   ivInputs;
        if(( ivInputs != rvLastInputs_q )
            && ( rvAddress_q != 10'd1023 ))
        begin
            rvState_d   =   SET_EVENT_STATE;
            rvData_d    =   ivInputs;
        end
    end
    SET_EVENT_STATE:
    begin
        rWE_d           =   1'b1;
        rvState_d       =   WRITE_EVENT_STATE0;  
    end
    WRITE_EVENT_STATE0:
    begin
        rWE_d           =   1'b1;
        rvState_d       =   SET_TICKS_STATE;  
    end
    SET_TICKS_STATE:
    begin
        rvState_d       =   WRITE_TICKS_STATE0;
        rvAddress_d     =   rvAddress_q + 1'b1;
        rvData_d        =   wvTicks;
    end
    WRITE_TICKS_STATE0:
    begin
        rvState_d       =   WRITE_TICKS_STATE1;
        rWE_d           =   1'b1;
    end
    WRITE_TICKS_STATE1:
    begin
        rWE_d           =   1'b1;
        rvState_d       =   IDLE_STATE;
        rvAddress_d     =   rvAddress_q + 1'b1;
    end
    default:
    begin
        rvState_d       =   IDLE_STATE;
        if({rSample_q,iSample} == 2'b01)
        begin
            rvState_d   =   SAMPLE_STATE;
        end
    end
    endcase
end

//////////////////////////////////////////////////////////////////////////////////
// Instances
//////////////////////////////////////////////////////////////////////////////////
/*
    \fn     mSampleCounter
    \brief  Event logger time base counter
*/
Counter #
(
    .TOTAL_BITS( 32 )
) mSampleCounter   //% Event logger time base
(
    .iClk       (iClk),
    .iRst       (iRst),
    .iCE        (iSample),
    .ovCnt      (wvTicks)
);

//////////////////////////////////////////////////////////////////////////////////
endmodule
