`timescale 1ps/1ps
//////////////////////////////////////////////////////////////////////////////////
/*!
    \brief      <b>1Kx32 to 4Kx8 Dual Port Memory</b>\n
    \details    \n
    \file       Mem1Kx32_4Kx8.v
    \author     amr/carlos.l.bernal@intel.com
    \date       Mar 9, 2015
    \brief      $RCSfile: Mem1Kx32_4Kx8.v.rca $
                $Date: $
                $Author:  $
                $Revision:  $
                $Aliases: $
                $Log: Mem1Kx32_4Kx8.v.rca $
                
                <b>Project:</b> FPGA coding\n
                <b>Group:</b> PVE-VHG\n
                <b>Testbench:</b> Mem1Kx32_4Kx8TB.v\n
                <b>Resources:</b>   <ol>
                                        <li>Spartan3AN 
                                        <li>MachXO2
                                        <l1>Cyclone IV
                                    </ol>
                <b>Instances:</b>   <ol>
                                        <li>Gen1Kx8BRAM
                                    </ol>                                    
                <b>Block Diagram:</b>
    \copyright Intel Proprietary -- Copyright 2015 Intel -- All rights reserved    
*/
//////////////////////////////////////////////////////////////////////////////////

module Mem1Kx32_4Kx8
(
    //% Clock
    input           iClk,
    //% PortA Address ( 1K )
    input   [9:0]   ivAddressA,
    //% PortB Address ( 4K )
    input   [11:0]  ivAddressB,
    //% Write enable
    input           iWE,
    //% PortA input data ( 32 bits)
    input   [31:0]  ivData,
    //% PortB output data ( 8 bits )
    output  [7:0]   ovData
);
wire    [7:0]   wvDO0;
wire    [7:0]   wvDO1;
wire    [7:0]   wvDO2;
wire    [7:0]   wvDO3;
reg     [7:0]   rvData;

assign  ovData  =   rvData;
//
//
//
always @(posedge iClk )
begin
    case ( ivAddressB[1:0] )
    2'd0:       rvData  <=  wvDO0;
    2'd1:       rvData  <=  wvDO1;
    2'd2:       rvData  <=  wvDO2;
    default:    rvData  <=  wvDO3;
    endcase
end
//
//
//
Gen1Kx8BRAM mBlok0
(
    .iClk           ( iClk ),
    .ivAddressA     ( ivAddressA ),
    .ivDataA        ( ivData[7:0] ),
    .iWEA           ( iWE ),
    .ovDataA        (),
    .ivAddressB     ( ivAddressB[11:2] ),
    .ivDataB        ( 8'b0 ),
    .iWEB           ( 1'b0 ),
    .ovDataB        ( wvDO0 )
);
//
//
//
Gen1Kx8BRAM mBlok1
(
    .iClk           ( iClk ),
    .ivAddressA     ( ivAddressA ),
    .ivDataA        ( ivData[15:8] ),
    .iWEA           ( iWE ),
    .ovDataA        (),
    .ivAddressB     ( ivAddressB[11:2] ),
    .ivDataB        ( 8'b0 ),
    .iWEB           ( 1'b0 ),
    .ovDataB        ( wvDO1 )
);
//
//
//
Gen1Kx8BRAM mBlok2
(
    .iClk           ( iClk ),
    .ivAddressA     ( ivAddressA ),
    .ivDataA        ( ivData[23:16] ),
    .iWEA           ( iWE ),
    .ovDataA        (),
    .ivAddressB     ( ivAddressB[11:2] ),
    .ivDataB        ( 8'b0 ),
    .iWEB           ( 1'b0 ),
    .ovDataB        ( wvDO2 )
);
//
//
//
Gen1Kx8BRAM mBlok3
(
    .iClk           ( iClk ),
    .ivAddressA     ( ivAddressA ),
    .ivDataA        ( ivData[31:24] ),
    .iWEA           ( iWE ),
    .ovDataA        (),
    .ivAddressB     ( ivAddressB[11:2] ),
    .ivDataB        ( 8'b0 ),
    .iWEB           ( 1'b0 ),
    .ovDataB        ( wvDO3 )
);
//
//
//


endmodule
	