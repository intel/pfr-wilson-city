//////////////////////////////////////////////////////////////////////////////////
/*!
    \brief      <b>IIC Slave</b>\n
    \details    Module\n
    \file       IICSlave.v
    \author     amr/carlos.l.bernal\@intel.com
    \date       Sept 16, 2012
    \brief      $RCSfile: IICSlave.v.rca $
                $Date: Thu Jun 13 14:04:50 2013 $
                $Author: edgarara $
                $Revision: 1.4 $
                $Aliases: Arandas_20130623_1550_0102,BellavistaVRTBCodeBase_20130705_0950,Arandas_20130802_2240_0103,BellavistaVRTB_20130912_1700,PowerupSequenceWorkingMissigPowerDown,Durango-FabA-0x0021,Durango-FabA-0x0022,Durango-FabA-0x0023,Durango-FabA-0x0024,Durango-FabA-0x0025,Durango-FabA-0x0028,Durango-FabA-0x0029,Durango-FabA-0x002A,Durango-FabA-0x002B,Durango-FabA-0x0101,Rev_0x0D,ColimaFabA0x0001,ColimaFabA0x0002,ColimaFabA0x0003,DurangoFabA0x0104,DurangoFabA0x0105,ColimaFabA0x0004,DurangoFabA0x0106,ColimaFabA0x0006,DurangoFabA0x0110,ColimaFabA0x000C,ColimaFabA0x000D,DurangoFabA0x0116,DurangoFabA0x0117,DurangoFabA0x011A,DurangoFabA0x011B,Arandas_20131223_1100_0104,Durango0x0120,Durango0x0123,Durango0x0124,Durango0x0125,Colima0x0011,Durango0x0127,Colima0x0013,Durango0x0128,Durango0x0129,Colima0x0017,Colima0x001A,Colima0x001B,Colima0x002D,Durango0x012D,Durango0x012E,Colima0x001E,Durango0x0133,Durango0x0134,Durango0x0136,Durango0x0137,Durango0x0138,Durango0x013A $
                $Log: IICSlave.v.rca $
                
                 Revision: 1.4 Thu Jun 13 14:04:50 2013 edgarara
                 Project References updated
                
                 Revision: 1.3 Tue Jun 11 13:55:11 2013 edgarara
                 Added comments and documentation form
                <b>Project:</b> FPGA coding\n
                <b>Group:</b> PVE-VHG\n
                <b>Testbench:</b> ModuleTB.v\n
                <b>Resources:</b>   <ol>
                                        <li>Spartan3A
                                            - 75 Slices
                                    </ol>
                <b>Instances:</b>   <ol>
                                    </ol>            
                <b>References:</b>  <ol>
                                        <li>Arandas
                                    </ol>
                <b>Block Diagram:</b>
    \verbatim
        +------------------------------------------+
 -----> |> iClk     .       .       .      onSDAOE | ---->
 -----> |  iRst     .       .       .oAddressMatch | ---->
 -----> |  iSDA     .       .       .     oStartOk | ---->
 -----> |  iSCL     .       .       .      oStopOk | ---->
 =====> |  ivTxData [7:0]   .       .       . oRnW | ---->
 -----> |  iTxAck   .       .       [7:0] ovRxData |=====>
        |   .       .       .       .       oRxAck | ---->
        |   .       .       .       .       .oTxOk | ---->
        |   .       .       .       .       .oRxOk | ---->
        +------------------------------------------+
                           IICSlave
    \endverbatim
        <b>States Diagram:</b>
         \image html IICSlave_Diagram.jpg       
    \version    
                20111225 \b clbernal - File creation\n
                20130604 \b edgarara - Added comments and documentation form\n    
    \copyright Intel Proprietary -- Copyright 2013 Intel -- All rights reserved
*/
//////////////////////////////////////////////////////////////////////////////////

`timescale 1ns / 1ps

module IICSlave #
(
    parameter MODULE_ADDRESS = 7'h50
)
(
                    //%Clock Input<br>
    input           iClk,
                    //%Asynchronous Reset Input<br> 
    input           iRst,
                    //%IIC Serial Data<br>
    input           iSDA,
                    //%IIC Serial Clock<br>
    input           iSCL,
                    //%Tx Data Input port<br>
    input   [7:0]   ivTxData,
                    //%Tx Acknowledge Flag<br>
    input           iTxAck,
                    //%IIC SDA Output Enable<br>
    output          onSDAOE,
                    //%Address Match Flag Output<br>
    output          oAddressMatch,
                    //%Start Ok Flag Output<br>
    output          oStartOk,
                    //%Stop Ok Flag Output<br>
    output          oStopOk,
                    
    output          oRnW,
    output  [7:0]   ovRxData,
                    //%Rx Acknowledge Flag<br>
    output          oRxAck,
                    //% Transmition Ok Flag<br>
    output          oTxOk,
                    //% Reception Ok Flag<br>
    output          oRxOk
);

//////////////////////////////////////////////////////////////////////////////////
// Includes
//////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////
// Defines
//////////////////////////////////////////////////////////////////////////////////
                //%Wait Start State<br>
localparam      WAIT_START_STATE         =    4'd1;
                //%Address Capture State<br>
localparam      ADDRESS_CAPTURE_STATE    =    4'd2;
                //%Transmition Data State<br>
localparam      TX_DATA_STATE            =    4'd4;
                //%Reciving Data State<br>
localparam      RX_DATA_STATE            =    4'd8;
                //%Idle State<br>
localparam      IDLE_STATE               =    4'd0;
//////////////////////////////////////////////////////////////////////////////////
// Internal Signals
//////////////////////////////////////////////////////////////////////////////////
                //%Shifter Register DataIn<br>
reg    [7:0]    rvShifter_d;
                //%Shifter Register DataOut<br>
reg    [7:0]    rvShifter_q;
                //%Address Match Register DataIn<br>
reg             rAddressMatch_d;
                //%Address Match Register DataOut<br>
reg             rAddressMatch_q;
                //%Actual State Register DataIn<br>
reg    [3:0]    rvState_d;
                //%Actual State Register DataOut<br>
reg    [3:0]    rvState_q;
                //%Bits Counter Register DataIn<br>
reg    [3:0]    rvBitsCounter_d;
                //%Bits Counter Register DataOut<br>
reg    [3:0]    rvBitsCounter_q;
                //%Serial Clock Line Register DataIn<br>
reg             rSCL_d;    
                //%Serial Clock Line Register DataOut<br>
reg             rSCL_q;     
                //%Serial Data Line Register DataIn<br>
reg             rSDA_d;     
                //%Serial Data Line Register DataOut<br>
reg             rSDA_q;     
                //%Start Ok Flag Register DataIn<br>
reg             rStartOk_d;
                //%Start Ok Flag Register DataOut<br>
reg             rStartOk_q;
                //%Stop Ok Flag Register DataIn<br>
reg             rStopOk_d;
                //%Stop Ok Flag Register DataOut<br>
reg             rStopOk_q;
                //%Read and Write Register DataIn<br>
reg             rRnW_d;
                //%Read and Write Register DataOut<br>
reg             rRnW_q;
                //%Rx Ok Flag Register DataIn<br>
reg             rRxOk_d;
                //%Rx Ok Flag Register DataOut<br>
reg             rRxOk_q;
                //%Tx Ok Flag Register DataIn<br>
reg             rTxOk_d;
                //%Tx Ok Flag Register DataOut<br>
reg             rTxOk_q;
                //%Rx Acknowledge Register DataIn<br>
reg             rRxAck_d;
                //%Rx Acknowledge Register DataOut<br>
reg             rRxAck_q;
                //%Serial Data Line Output Enable Register DataIn<br>
reg             rnSDAOE_d;
                //%Serial Data Line Output Enable Register DataOut<br>
reg             rnSDAOE_q;

`undef USE_DEBUG
`ifdef USE_DEBUG
    (* KEEP = "TRUE" *) reg [6:0]   rvAction_d;
    (* KEEP = "TRUE" *) reg [6:0]   rvAction_q;
    (* KEEP = "TRUE" *) reg         rFallingEdge_d;
    (* KEEP = "TRUE" *) reg         rFallingEdge_q;
    (* KEEP = "TRUE" *) reg         rRisingEdge_d;
    (* KEEP = "TRUE" *) reg         rRisingEdge_q;

`endif
//%
reg            rOutputAddressMatch_d;
reg            rOutputAddressMatch_q;
//////////////////////////////////////////////////////////////////////////////////
// Continous assigments
//////////////////////////////////////////////////////////////////////////////////
assign      oAddressMatch   =    rOutputAddressMatch_q;
assign      oStopOk         =    rStopOk_q;
assign      oStartOk        =    rStartOk_q;
assign      oRnW            =    rRnW_q;
assign      ovRxData        =    rvShifter_q;
assign      oRxAck          =    rRxAck_q;
assign      onSDAOE         =    rnSDAOE_q;
assign      oRxOk           =   rRxOk_q;
assign      oTxOk           =   rTxOk_q;
//////////////////////////////////////////////////////////////////////////////////
// Sequential logic
//////////////////////////////////////////////////////////////////////////////////
always @(posedge iClk or posedge iRst)
begin
    if(iRst)
    begin
        //rSCL_q              <=  iSCL;
        //rSDA_q              <=  iSDA;
        rSCL_q              <=  1'b0;
        rSDA_q              <=  1'b0;
        rvShifter_q         <=  8'hff;
        rvBitsCounter_q     <=  4'd0;
        rvState_q           <=  IDLE_STATE;
        rRxOk_q             <=  1'b0;
        rTxOk_q             <=  1'b0;
        rRxAck_q            <=  1'b1;
        rRnW_q              <=  1'b0;
        rStopOk_q           <=  1'b0;
        rAddressMatch_q     <=  1'b0;
        rStartOk_q          <=  1'b0;
        rnSDAOE_q           <=  1'b1;
        `ifdef USE_DEBUG
            rvAction_q      <=  "-";
            rFallingEdge_q  <=  0;
            rRisingEdge_q   <=  0;
        `endif
        rOutputAddressMatch_q    <=    1'b0;
    end
    else
    begin
        rSCL_q                  <=  rSCL_d;
        rSDA_q                  <=  rSDA_d;
        rvShifter_q             <=  rvShifter_d;
        rvBitsCounter_q         <=  rvBitsCounter_d;
        rvState_q               <=  rvState_d;
        rRxOk_q                 <=  rRxOk_d;
        rTxOk_q                 <=  rTxOk_d;
        rRxAck_q                <=  rRxAck_d;
        rRnW_q                  <=  rRnW_d;
        rStopOk_q               <=  rStopOk_d;
        rAddressMatch_q         <=  rAddressMatch_d;
        rStartOk_q              <=  rStartOk_d;
        rnSDAOE_q               <=  rnSDAOE_d;
        `ifdef USE_DEBUG
            rvAction_q          <=  rvAction_d;
            rRisingEdge_q       <=  rRisingEdge_d;
            rFallingEdge_q      <=  rFallingEdge_d;            
        `endif
        rOutputAddressMatch_q   <=  rOutputAddressMatch_d;
    end
end
//////////////////////////////////////////////////////////////////////////////////
// Combinational logic
//////////////////////////////////////////////////////////////////////////////////
always @*
begin
    rSCL_d                  =   iSCL;
    rSDA_d                  =   iSDA;
    rvShifter_d             =   rvShifter_q;
    rvBitsCounter_d         =   rvBitsCounter_q;
    rvState_d               =   rvState_q;
    rAddressMatch_d         =   rAddressMatch_q;
    rStartOk_d              =   rStartOk_q;
    rStopOk_d               =   rStopOk_q;
    rRxOk_d                 =   rRxOk_q;
    rTxOk_d                 =   rTxOk_q;
    rRxAck_d                =   rRxAck_q;
    rRnW_d                  =   rRnW_q;
    rOutputAddressMatch_d   =   rOutputAddressMatch_q;
    rnSDAOE_d               =   rnSDAOE_q;
    `ifdef USE_DEBUG
        rvAction_d              =    rvAction_q;
        rRisingEdge_d           =   0;
        rFallingEdge_d          =   0;
    `endif    
    case(rvState_q)
        WAIT_START_STATE:                       //%Waits for the start bit
        begin
            rRxOk_d                 =   1'b0;
            rTxOk_d                 =   1'b0;
            rvState_d               =   WAIT_START_STATE;
            rAddressMatch_d         =   1'b0;
            rOutputAddressMatch_d   =   1'b0;
            rStartOk_d              =   1'b0;
            rStopOk_d               =   1'b0;
            rvBitsCounter_d         =   1'b0;
            rnSDAOE_d               =   1'b1;
        end
        ADDRESS_CAPTURE_STATE,
        RX_DATA_STATE:        
        begin                                                       //The Start Ok Flag is set Low
           rStartOk_d  =   1'b0;                                    //Added by MM 5/8/2012
                                                                    //Low to high transition?
            if({rSCL_q,iSCL} == 2'b01)
            begin
                `ifdef USE_DEBUG
                    rRisingEdge_d   =   1'b1;
                `endif
                rvBitsCounter_d     =   rvBitsCounter_q + 1'b1;
                if(rvBitsCounter_q < 4'd8)
                begin
                                                                    //Yes, Capture the 7 bits
                    rvShifter_d     =   {rvShifter_q[6:0],iSDA};
                    `ifdef USE_DEBUG
                        rvAction_d      =    "<";
                    `endif                    
                end
                if(rvState_q == ADDRESS_CAPTURE_STATE)              //Address Capture?
                begin                                                               
                    if(rvBitsCounter_q == 4'd7)                 
                    begin
                        if(rvShifter_q[6:0] == MODULE_ADDRESS)  
                        begin
                            rAddressMatch_d =    1'b1;
                            `ifdef USE_DEBUG
                                rvAction_d      =    "V";
                            `endif
                        end
                        else
                        begin
                            rvState_d    =    WAIT_START_STATE;
                        end
                    end
                    if(rvBitsCounter_q == 4'd8) 
                    begin                                
                        rRnW_d  =   rvShifter_q[0];
                        rOutputAddressMatch_d    =    rAddressMatch_q;                        
                        `ifdef USE_DEBUG
                            rvAction_d    =    "O";
                        `endif
                    end                                    
                end
            end
            //High to low transition
            if({rSCL_q,iSCL}    ==    2'b10)
            begin
                `ifdef USE_DEBUG
                    rFallingEdge_d   =   1'b1;
                `endif
                rnSDAOE_d   =   1'b1;
                rRxOk_d     =   1'b0;
                if(rvBitsCounter_q == 4'd8)
                begin
                    //Ack
                    rnSDAOE_d   = (rvState_q == ADDRESS_CAPTURE_STATE) ? ~rAddressMatch_q : ~iTxAck;
                end
                if(rvBitsCounter_q == 4'd9)
                begin
                    rvBitsCounter_d =   4'd0;
                    if(rvState_q == ADDRESS_CAPTURE_STATE)
                    begin
                        rvState_d    =    RX_DATA_STATE;
                        if(rRnW_q)
                        begin
                            rvState_d       =   TX_DATA_STATE;
                            rvShifter_d     =   {ivTxData[6:0],1'b1};
                            rnSDAOE_d       =   ivTxData[7];
                        end
                        `ifdef USE_DEBUG
                            rvAction_d      =   "J";
                        `endif
                    end
                    else
                    begin
                        rRxOk_d     =   1'b1;
                    end
                end
            end
        end

        TX_DATA_STATE:
        begin
            `ifdef USE_DEBUG
                rvAction_d    =    "T";
            `endif
        
            //Low to high transition
            if({rSCL_q,iSCL} == 2'b01)
            begin
                `ifdef USE_DEBUG
                    rRisingEdge_d   =   1'b1;
                `endif
                if(rvBitsCounter_q == 4'd8)
                begin
                    rRxAck_d    =   iSDA;
                    if(iSDA)
                    begin
                        rvState_d   =   WAIT_START_STATE;
                    end
                end
            end
            //High to low transition
            if({rSCL_q,iSCL}    ==    2'b10)
            begin
                `ifdef USE_DEBUG
                    rFallingEdge_d   =   1'b1;
                `endif
                rTxOk_d = 1'b0;
                rvBitsCounter_d =   rvBitsCounter_q + 1'b1;
                if(rvBitsCounter_q == 4'd7)                
                begin
                    rTxOk_d     =   1'b1;   //Added by MM 5/14/2012
                end    
                if(rvBitsCounter_q < 4'd8)
                begin
                    rvShifter_d =   {rvShifter_q[6:0],1'b1};
                    rnSDAOE_d   =   rvShifter_q[7];
                end
                if(rvBitsCounter_q == 4'd8)
                begin
                    rvShifter_d     =   {ivTxData[6:0],1'b1};
                    rnSDAOE_d       =   ivTxData[7];
                    rvBitsCounter_d =   0;
                end
            end
        end

        default:
        begin
            rvState_d    =    WAIT_START_STATE;
        end
    endcase
    //Stop?
    if({rSDA_q,iSDA,rSCL_q,iSCL} == 4'b0111)
    begin
        rvState_d               =   WAIT_START_STATE;
        rStopOk_d               =   1'b1;
        rvShifter_d[7]          =   1'b1;
        rAddressMatch_d         =   1'b0; //Added by MM  4/20/2012
        rOutputAddressMatch_d   =   1'b0;
    end

    //Start?
    if({rSDA_q,iSDA,rSCL_q,iSCL} == 4'b1011)
    begin
        rvState_d               =   ADDRESS_CAPTURE_STATE;
        rStartOk_d              =   1'b1;
        //Added by Mario M  4/20/2012
        rRxOk_d                 =   1'b0;
        rTxOk_d                 =   1'b0;            
        rAddressMatch_d         =   1'b0;
        rOutputAddressMatch_d   =   1'b0;
        rStopOk_d               =   1'b0;
        rvBitsCounter_d         =   1'b0;
        rnSDAOE_d               =   1'b1;
        //////////////////////////////////
          
    end    
end
//////////////////////////////////////////////////////////////////////////////////
// Instances
//////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////
endmodule
