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

/* Hardware definitions */
#define BIT(x) (1<<x)

/* Port 1 */
#define CAN_INT         BIT(4)
#define FPGA_GPIO1      BIT(6)
#define FPGA_GPIO2      BIT(7)

/* Port 2 */
#define FPGA_GPIO3      BIT(0)
#define FS              BIT(1)
#define FS_RESET        BIT(2)
#define FPGA_RESET      BIT(3)
#define FPGA_ENABLE     BIT(5)

/* Port 3 */
#define RED_LED_PORT    3
#define YELLOW_LED_PORT 3

#define STE0            BIT(0
#define SIMO0           BIT(1)
#define SOMI0           BIT(2)
#define UCLK0           BIT(3)
#define FPGA_CS         BIT(4) /* FPGA Chip Select (labelled "chpsel" on the board) */
#define RED_LED_BIT     BIT(5)
#define YELLOW_LED_BIT  BIT(6)

/* Port 4 */

/* Port 5 */
#define CAN_CS          BIT(0) /* Note: re-defined in scandal_devices.h */
#define SIMO1           BIT(1)
#define SOMI1           BIT(2)
#define UCLK1           BIT(3)

/* Port 6 / ADC */
#define MEAS15_PIN      (BIT(3))
#define UMEAS_OUT1_PIN  (BIT(4))
#define UMEAS_IN1_PIN   (BIT(5))
#define IMEAS_IN1_PIN   (BIT(6))
#define THEATSINK_PIN  (BIT(7))

/* ADC channel definitions */
/* These are a little different to most other MSP code I've written
   since they are optimised for the MPPTNG. 
   NOTE: The numbers below do not correspond to the pins. 
   THis is a bit of a hack, but check out control.c if you want to 
   see why this is so */ 

#define MEAS_VOUT       3
#define MEAS_VIN1       5
#define MEAS_IIN1       4
#define MEAS_15V        2
#define MEAS_THEATSINK  1
#define MEAS_TAMBIENT   0

#define ADC_NUM_CHANNELS 6

/* ADC Channel definitions 
	These define the actual channels which are used for each MEMCTL register */ 
#define INCH_VOUT               INCH_4
#define INCH_VIN1		INCH_5
#define INCH_IIN1		INCH_6
#define INCH_15V		INCH_3
#define INCH_THEATSINK	        INCH_7
#define INCH_TAMBIENT	        INCH_10

/* Useful macros */ 
#define FAULT_SIGNAL()  (P2IN & FS)
