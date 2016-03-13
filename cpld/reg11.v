/********************************************
* reg11.v   								*
*											*
* Requisites								*
*	<none>									*
*											*
* Requisite to								*
*	signal.v								*
*	signalslow.v							*
*	minigtimingCy.v							*
*											*
*											*
* 											*
* Andreas Gotterba							*
* C0ntact:									*
* a_gotterba <at> yahoo.com		 			*
*********************************************
This is just a 11 bit wide register.  Altera doesn't provide a macrofunction for 
registers; I'll bet there's a way to instaniate an array of modules, but I don't 
know what it is.  This works just as well, it is just as efficient as a schematic
register.
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

module reg11 (
	D[10:0],		// input data
	CLK,		// the clock
	ENA,		// enable the loading of new data
	Q[10:0]		// output data
	);
	
	input [10:0] D;
	input CLK;
	input ENA;
	output [10:0] Q;
	
	//A whole buch of flip-flops
	DFFE FF0 (.CLK(CLK), .ENA(ENA), .D(D[0]), .Q(Q[0]));
	DFFE FF1 (.CLK(CLK), .ENA(ENA), .D(D[1]), .Q(Q[1]));
	DFFE FF2 (.CLK(CLK), .ENA(ENA), .D(D[2]), .Q(Q[2]));
	DFFE FF3 (.CLK(CLK), .ENA(ENA), .D(D[3]), .Q(Q[3]));
	DFFE FF4 (.CLK(CLK), .ENA(ENA), .D(D[4]), .Q(Q[4]));
	DFFE FF5 (.CLK(CLK), .ENA(ENA), .D(D[5]), .Q(Q[5]));
	DFFE FF6 (.CLK(CLK), .ENA(ENA), .D(D[6]), .Q(Q[6]));
	DFFE FF7 (.CLK(CLK), .ENA(ENA), .D(D[7]), .Q(Q[7]));
	DFFE FF8 (.CLK(CLK), .ENA(ENA), .D(D[8]), .Q(Q[8]));
	DFFE FF9 (.CLK(CLK), .ENA(ENA), .D(D[9]), .Q(Q[9]));
	DFFE FF10 (.CLK(CLK), .ENA(ENA), .D(D[10]), .Q(Q[10]));

endmodule