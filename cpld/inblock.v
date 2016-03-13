/********************************************
* inblock.v   								*
* 											*
* Requisites								*
*	sreg16.v								*
*	edecode4.v								*
*											*
* Requisite to								*
*	mining2fpga.v							*
*											*
* Andreas Gotterba							*
* C0ntact:									*
* a_gotterba <at> yahoo			 			*
*********************************************

This module handles the SPI input from the MSP430.  This design is
a slave.  It takes a chip select (write enable), clock, and data line from the 
master, and when enabled, shifts data in from the data line.  It writes the received
data to the rest of the device when the enable drops low

FIXED!!! :-): As the clock from the MSP only runs when data is being sent, an extra byte must be 'written'
(while keeping chpsel high) for the circuit to realize that transmission is finished.  This should
be replaced with a system that does not need this (a way of doing this is described in Andreas' 
notes, talk to him if he left before implementing it).  This would mean moving the load signal to the
main (256Mhz) clock
*/

/* Copyright (C) Andreas Gotterba, 2009 */ 

/* 
 * This file is part of the UNSWMPPTNG firmware.
 * 
 * The UNSWMPPTNG firmware is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 
 * The UNSWMPPTNG firmware is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 
 * You should have received a copy of the GNU General Public License
 * along with the UNSWMPPTNG firmware.  If not, see <http://www.gnu.org/licenses/>.
 */

module inblock (
	data[15:0], // the incoming data, including the information of where to send it
	countglobal,//signal to all the boards to set there counter reset point to the data; included for reverse compatability
	load1,		// signal for board1 to load data
	load2,		// signal for board2 to load data
	load3,		// signal for board3 to load data
	clk,		// the regular clock
	mspclk,		// the slow clock from the master SPI device
	serialin,	// the data from the master SPI device
	nchpsel		// the chip select (write enable) from the master SPI device (active low)
	);

	output [15:0] data;
	output countglobal;
	output load1;
	output load2;
	output load3;
	
	input clk;
	input mspclk;
	input serialin;
	input nchpsel;
	
	wire chpsel;		// invert the n chip select line  for edge detection (active high)
	wire chpsel1;		// delay the chip select line one clock cycle so I can detect its falling edge
	wire chpsel2;		// delay it another cycle for good measure, I don't know how long it will take for everything to be written
	wire chpseld;		// the or of chpsel1 and chpsel2, so that two cycles are availible to load
	wire loaden;		// load (decode the board address and send the data on its way)
	wire clear;			// clear the shift register
	
	//the input shift register
	sreg16 SHIFT( .clock(mspclk), .enable(chpsel), .shiftin(serialin), .sclr(clear), .q( data[15:0]));

	not (chpsel, nchpsel);	//invert the active low chip select line
	DFF DCS1 (.D(chpsel), .CLK(clk), .Q(chpsel1)); //negative edge detector on chpsel
	DFF DCS2 (.D(chpsel1), .CLK(clk), .Q(chpsel2)); //two clock cycle dely for good measure
	or (chpseld, chpsel1, chpsel2);
	and (loaden, nchpsel, chpseld); // To load data, chpsel must go from low to high. Hold this for two cycles, as the load net is large and may be slow
	and (clear, nchpsel, chpsel2);  //clear only after the two cycles

	edecode4 BOARDSEL(	.data(data[15:14]), .enable(loaden), .eq0(countglobal), 
						.eq1(load1), .eq2(load2), .eq3(load3));
//the load pulse will last for 1 mspclk cycle, which will contain several fast clk cycles	
//if this is a problem, could register it so that its clock is converted
endmodule	