/********************************************
* signal.v   								*
* 											*
* Requisites								*
*	reg11.v									*
*	gt11.v									*
*	lt11.v									*
*	mux1.v									*
*											*
* Requisite to								*
*	board.v									*
*											*
*											*
*											*
* Andreas Gotterba							*
* C0ntact:									*
* a_gotterba <at> yahoo.com		 			*
*********************************************
Manages the turning on and off of an individual signal.  Stores the point in the cycle
where a signal should turn on and off (which is currently absolute, not relative to
other signals- need adders!), and detects when that condition is met.  
Also detects when set is after reset (in terms of the counter), and switches the logic to
deal with it.  This is currently done with a comparator, and I would love to do it without 
one (so expensive for such a small function).  Ideas?

Provides a low pass filter on the output (which adds some delay (but it's the same for every
signal)) to eliinate glitches

When adders are implemented, the storage of registers should move to board.v.  The adders should
be placed after the registers, so that the value is updated as soon as the new data is written
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

module signal (
	Gate,					// output signal
	clk,					// 125MHz clock
	setpoint[10:0],			// Set point to be loaded
	resetpoint[10:0],		// Reset point to be loaded
	enable,					// security line.  The ouptut is set low if it drops low
	load,					// signal to load the set and reset point into the registers
	counter[10:0]			// ring counter
	);	

	output	Gate;

	input	clk;
	input	[10:0] setpoint;
	input	[10:0] resetpoint;
	input	enable;
	input	load;
	input	[10:0] counter;

	wire	[10:0] Sreg;	// data from the set register
	wire	[10:0] Rreg;	// data from the reset register

	wire	set;			// signal that set condition met
	wire	reset;			// signal that reset condition met

	wire	sgtr;			// signal as to whether Sreg is larger than Rreg
	wire	gton;			// signal to turn on output, used when Sreg > Rreg
	wire	lton;			// signal to turn on output used when Sreg < Rreg

	wire	Gatedirty;		// The unfiltered gate signal
	wire	Gateraw;		// The gate signal before being ANDed with enable

	// Store the data
	reg11  setreg (.D(setpoint[10:0]), .CLK(clk), .ENA(load), .Q(Sreg[10:0])); 
	reg11  retreg (.D(resetpoint[10:0]), .CLK(clk), .ENA(load), .Q(Rreg[10:0])); 

	// compare the stored values to the counter
	gt11 setcomp (.dataa(counter[10:0]), .datab(Sreg[10:0]), .agb(set));
	lt11 retcomp (.dataa(counter[10:0]), .datab(Rreg[10:0]), .alb(reset));

	//Determine which register is larger, and output the signal from the relevant logic
	gt11 srcomp (.dataa(Sreg[10:0]), .datab(Rreg[10:0]), .agb(sgtr));
	and checkl (lton, set, reset);
	or checkg (gton, set, reset);
	mux1 mux1 (.data0(lton), .data1(gton), .sel(sgtr), .result(Gatedirty));

	//low pass filter to clean the gate signal
	lpf cleangate (.clk(clk), .in(Gatedirty), .out(Gateraw));

	//AND the output with the enable signal
	and sds (Gate, Gateraw, enable);
endmodule
