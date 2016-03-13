/* -------------------------------------------------------------------------- 
	SPI Devices 
	 
	File name: spi_devices.h
	Author: David Snowdon 
	Date: 12/04/02 
    Copyright (C) David Snowdon, 2009. 
   ------------------------------------------------------------------------*/ 
  
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


#ifndef __SPIDEVICES__ 
#define __SPIDEVICES__ 

#include <io.h>

#define BIT(x) (1<<x)

#define MCP2510			0              
#define SPI_NUM_DEVICES         1
#define SPI_DEVICE_NONE		SPI_NUM_DEVICES 

#define FPGA                    0
#define SPI0_NUM_DEVICES        1
#define SPI0_DEVICE_NONE        SPI0_NUM_DEVICES


/* MCP2510 */
#define ENABLE_MCP2510()        (P5OUT &= ~BIT(0))
#define DISABLE_MCP2510()       (P5OUT |= BIT(0))

/* FPGA */
#define ENABLE_FPGA_SPI()           (P3OUT &= ~BIT(4))
#define DISABLE_FPGA_SPI()          (P3OUT |= BIT(4))


#endif
