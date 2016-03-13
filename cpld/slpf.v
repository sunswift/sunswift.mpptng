/********************************************
* slpf.v   									*
* 											*
* Requisites								*
*											*
* Requisite to								*
*	counter.v								*
*											*
*											*
*											*
* Copyright (C) 2004, Andreas Gotterba							*
* C0ntact:									*
* a_gotterba <at> yahoo.com		 			*
*********************************************
A strict low pass filter; often necessary as 1 cycle glitches are common
this version requires the signal on for 3 consecutive clock cycles
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

module slpf (
	clk,
	in,
	out
	);	

	output	out;

	input	clk;
	input	in;

	// Three buffers (as shift register) for the low pass filter
	wire	ubuff1;
	wire	ubuff2;
	wire	ubuff3;
	// Signals from the ANDs of the buffers
	wire	ubuff12;		
	wire	ubuff13;
	wire	ubuff23;

	//The low pass filter: if any of the bits are low, the output is low
	DFF (.D(in), .CLK(clk), .Q(ubuff1)); 
	DFF (.D(ubuff1), .CLK(clk), .Q(ubuff2)); 
	DFF (.D(ubuff2), .CLK(clk), .Q(ubuff3));

	and prod (out, ubuff1, ubuff2, ubuff3);		

endmodule
