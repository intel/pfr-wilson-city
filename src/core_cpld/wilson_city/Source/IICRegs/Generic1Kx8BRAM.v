`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
/*!
    \brief      <b>Generic Dual Port BRAM</b> 
    \details    Generic instance of a Dual Port 1Kx8 RAM\n
    \file       Generic1Kx8BRAM.v
    \author     amr/carlos.l.bernal@intel.com
    \date       Aug 10, 2012
    \brief      $RCSfile: Generic1Kx8BRAM.v.rca $
                $Date:  $
                $Author:  $
                $Revision:  $
                $Aliases: $
                $Log: Generic1Kx8BRAM.v.rca $
                
                <b>Project:</b> Neon City\n
                <b>Group:</b> BD\n
                <b>Testbench:</b>
                <b>References:</b>   <ol>
                                        <li>NeonCity
                                        <li>Arandas
                                        <li>Durango
                                        <li>Bellavista
                                        <li>Summer Valley
                                    </ol>
                <b>Resources:</b>  <ol>
                                        <li>MachXO2
                                        <li>Cyclone IV
                                        <li>Spartan3AN
                                    </ol>
                <b>Block Diagram:</b>
    \verbatim
        +----------------------------------------------+
 -----> |> iClk .       .        .        .        .   |
 =====> | ivAddressA[9:0]        .        ovDataA[7:0] | ====>
 =====> | ivDataA[7:0]  .        .        .        .   |
 -----> | iWEA  .       .        .        .        .   |
        |       .       .        .        .        .   |
 =====> | ivAddressB[9:0]        .        ovDataB[7:0] | ====>
 =====> | ivDataB[7:0]  .        .        .        .   |
 -----> | iWEB  .       .        .        .        .   |
        +----------------------------------------------+
                           Gen1Kx8BRAM
    \endverbatim
    \copyright  Intel Proprietary -- Copyright 2015 Intel -- All rights reserved
*/
//////////////////////////////////////////////////////////////////////////////////
module Gen1Kx8BRAM
(
    //% Clock
    input           iClk,
    //% Port A Address
    input   [9:0]   ivAddressA,
    //% Port A input data
    input   [7:0]   ivDataA,
    //% Port A Write Enable
    input           iWEA,
    //% Port A output data
    output  [7:0]   ovDataA,
    //% Port B Address
    input   [9:0]   ivAddressB,
    //% Port B input data    
    input   [7:0]   ivDataB,
    //% Port B write enable
    input           iWEB,
    //% Port B output data
    output  [7:0]   ovDataB    
);
//////////////////////////////////////////////////////////////////////////////////
// Includes
//////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////
// Defines
//////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////
// Internal Signals
//////////////////////////////////////////////////////////////////////////////////

reg [7:0]   rvvMem      [1023:0];
reg [7:0]   rvDataA;
reg [7:0]   rvDataB;

//////////////////////////////////////////////////////////////////////////////////
// Continous assigments
//////////////////////////////////////////////////////////////////////////////////

assign ovDataA =   rvDataA;
assign ovDataB =   rvDataB;
//////////////////////////////////////////////////////////////////////////////////
// Sequential logic
//////////////////////////////////////////////////////////////////////////////////
//% Sequential logic<br>
always @ (posedge iClk)
begin
    if ( iWEA )
    begin
        rvvMem[ivAddressA] <=  ivDataA;
    end
    rvDataA <=  rvvMem[ivAddressA];
end	 
//////////////////////////////////////////////////////////////////////////////////
// Combinational logic
//////////////////////////////////////////////////////////////////////////////////
//%Combinational logic<br>
always @ (posedge iClk)
begin
    if ( iWEB )
    begin
        rvvMem[ivAddressB]  <=  ivDataB;
    end
    rvDataB <=  rvvMem[ivAddressB];
end	 
//////////////////////////////////////////////////////////////////////////////////
// Instances
//////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////
endmodule
