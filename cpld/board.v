/********************************************
* board.v   								*
* 											*
* 											*
* Requisites								*
*	edecode4.v								*
*	signal.v								*
*	counter.v								*
*											*
* Requisite to:								*
*	mining2fpga.v							*
*	NgControlCpld.v							*
*											*
*											*
* Andreas Gotterba							*
* C0ntact:									*
* a_gotterba <at> yahoo.com		 			*
*********************************************
This is the control logic for an individual power board.  
It instantiates 3 signal modules, and decodes the top 2 bits of the data
bus to decide which signal the data is for.  

It also shuts the power board down if there is a fault, if the MSP stops sending the enable signal,
or if the PLL is not locked.  The diode signal is also disabled if the main signal is on
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

module board (
	aux, main, diode, SD, 	// the control signals to the board
	clk,				// the fast (~32MHz) clock
	data[13:0],			// the input data, including signal address
	load,				// enable the board to send data through.  Comes from the decoding of the board addres (data[14:13]) in inblock
	nFS,				// Fault signal
	FS_Reset,			// Reset the fault signal latch
	countglobal//,			//signal to load the reset value for the Counter FIXME: the counter should be integrated with the rest of the board signal; do this along with the overhaul of inblock. Fix: Now does both; can reset clocks locally or globally
//	led1, led2
	);

	output aux;
	output main;
	output diode;
	output SD;
//	output led1; 
//	output led2;
	
	input clk;
	input [13:0] data;
	input load;
	input FS_Reset;
	input nFS;
	input countglobal; 
	
	wire [10:0] counter;
	wire [10:0] period;
	wire [10:0] aux_length, aux_overlap, main_length, dead_time; 
	wire [10:0] aux_on, aux_off, main_on, main_off, diode_on, diode_off; 
		
	wire load_al, load_ao, load_ml, load_dt; // Register load signals
	
	wire nmain;			//inversion of main for diode protection
	wire enable; 
	wire denable;		//separate enable signal for the diode fet
	wire nenable;		//inversion of enable signal, to reset the counter when there is an error
	wire countlocal;	// signal to change counter reset value through the BB00X command (only effects this board, preferable)
	wire countload;		// signal to change counter reset value through the 00XXX command (this will apply to every board controled by the device, for reverse compatability)
	wire signal_load; 	// Load signal for the three signal modules to load the set/reset registers

	wire latch_reset;   // Generated signal for reset-ing the latch. 

	// Generate the load signals for the registers
	edecode5 DECODE (.data(data[13:11]), .enable(load), .eq0(countlocal), 
					 .eq1(load_al), .eq2(load_ao), .eq3(load_ml), .eq4(load_dt));

	or(countload, countlocal, countglobal);		// There are two ways of setting the counter period

	// Registers to store various values
	reg11 	period_reg (.CLK(clk), .ENA(countload), .D(data[10:0]), .Q(period[10:0])); // Counter period
	reg11 	aux_length_reg (.CLK(clk), .ENA(load_al), .D(data[10:0]), .Q(aux_length[10:0])); // Auxilliary pulse length
	reg11 	aux_overlap_reg (.CLK(clk), .ENA(load_ao), .D(data[10:0]), .Q(aux_overlap[10:0])); // Main/Auxilliary overlap
	reg11 	main_length_reg (.CLK(clk), .ENA(load_ml), .D(data[10:0]), .Q(main_length[10:0])); // Main pulse length
	reg11 	dead_time_reg (.CLK(clk), .ENA(load_dt), .D(data[10:0]), .Q(dead_time[10:0])); // Dead time around diode	

	//security logic
	and(latch_reset, nFS, FS_Reset);

	DFFE (.D(1'b1), .CLK(latch_reset), .CLRN(nFS), .PRN(1'b1), .Q(enable)); //structure used before
	not (nenable, enable); //to reset the counter
	not (SD, enable);	
	
	not(nmain, main);
//	and(denable, enable, nmain); //only allow the diode to be on when main is not
	buf(denable, 1'b0); // Disable the diode for testing. 

	//the counter
	counter count(.clk(clk), .counter(counter[10:0]), .period(period[10:0]), .reset(nenable), .enable(enable)); //change reset to any off->on transistion (this coveres nSD, include re enabled by MSP

	// Calculate the on/off times for the signals. 
//	buf(aux_on[10:0], 11'b0);															// aux_on = 0
//	buf(aux_off[10:0], aux_length[10:0]);												// aux_off = aux_length

	sub11(.result(main_on[10:0]), .dataa(aux_length[10:0]), .datab(aux_overlap[10:0])); // main_on = aux_length - aux_overlap
	add11(.result(main_off[10:0]), .dataa(main_on[10:0]), .datab(main_length[10:0])); 			// main_off = main_on + main_length
	
	add11(.result(diode_on[10:0]), .dataa(main_off[10:0]), .datab(dead_time[10:0]));	// diode_on = main_off + dead_time
	sub11(.result(diode_off[10:0]), .dataa(period[10:0]), .datab(dead_time[10:0])); 	// diode_off = period - dead_time

	//Generate the signal load signal by delaying the other load
	DFFE(.D(load), .CLK(clk), .CLRN(1'b1), .PRN(1'b1), .Q(signal_load));

	//The Individual Signals
//	signal AUXSIG (.Gate(aux), .clk(clk), .setpoint(aux_on[10:0]), .resetpoint(aux_off[10:0]), 
//					.enable(enable), .load(signal_load), .counter(counter[10:0]));
	signal AUXSIG (.Gate(aux), .clk(clk), .setpoint(11'b0), .resetpoint(aux_length[10:0]), 
					.enable(enable), .load(signal_load), .counter(counter[10:0]));
	signal MAINSIG (.Gate(main), .clk(clk), .setpoint(main_on[10:0]), .resetpoint(main_off[10:0]),
					.enable(enable), .load(signal_load), .counter(counter[10:0]));
	signal DIODESIG (.Gate(diode), .clk(clk), .setpoint(diode_on[10:0]), .resetpoint(diode_off[10:0]),
					.enable(denable), .load(signal_load), .counter(counter[10:0]));

endmodule