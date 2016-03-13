/********************************************
* counter.v  								*
*											*
* Requisites								*
*	count11.v								*
*	reg11.v									*
*	gt11.v									*
* Requisite to								*
*	board.v									*
*											*
*											*
* 											*
* Andreas Gotterba							*
* C0ntact:									*
* a_gotterba <at> yahoo.com		 			*
*********************************************
This is all the  stuff related to the resetable counter
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

module counter (
	clk,
	counter[10:0], 
	period[10:0],
	reset,
	enable
	);
	input clk;
	input [10:0] period;	//counter's reset point
	input reset;			//exteral reset signal
	input enable;			//external enable signal
	
	output [10:0] counter;	
		
	wire gt;				//dirty signal that the restart condition is met
	wire restart;			//signal to restart counter
	wire clear;				//signal to clear the counter

	//the ring counter
	count11	count11	(.clock(clk), .q(counter[10:0]), .sclr(clear), .cnt_en(enable)); 


	
	//Detects when the counter should reset
	gte11  	detect (.dataa(counter[10:0]), .datab(period[10:0]), .ageb(gt));
	
	//strict low pass filter on the reset signal, to prevent any glitches
	slpf 	cleaner	(.clk(clk), .in(gt), .out(restart));
	
	//clears the counter when it meets the reset condition, or when it gets an exteral reset signal
	or(clear, reset, restart);
	
endmodule
	