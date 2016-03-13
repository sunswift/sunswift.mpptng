/* Copyright (C) David Snowdon, 2009 <scandal@snowdon.id.au> */ 

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

#include <scandal/types.h>
#include <scandal/leds.h>

#include <project/hardware.h>
#include <project/other_spi.h>
#include <project/spi_devices.h>
#include <project/fpga.h>
#include <project/mpptng.h>

void fpga_init(void){
	init_spi0();

	fpga_transfer(1, SIGNAL_RESTART, DEFAULT_RESTART);
	fpga_transfer(1, SIGNAL_AUX_LENGTH, DEFAULT_AUX_LENGTH);
	fpga_transfer(1, SIGNAL_AUX_OVERLAP, DEFAULT_AUX_OVERLAP);
	fpga_transfer(1, SIGNAL_PWM, DEFAULT_PWM);
	fpga_transfer(1, SIGNAL_DEADTIME, DEFAULT_DEADTIME);

	fpga_reset(); 
}
