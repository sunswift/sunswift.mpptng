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

#ifndef __FPGA__
#define __FGPA__

#include <project/hardware.h>
#include <scandal/types.h>
#include <project/other_spi.h>
#include <project/spi_devices.h>
#include <project/mpptng.h>

#define FPGA_ON      1
#define FPGA_OFF     0

/* Signal definitions to be used in
   fpga_set_register(X, signal, X); */
#define SIGNAL_RESTART       0
#define SIGNAL_AUX_LENGTH    1
#define SIGNAL_AUX_OVERLAP   2
#define SIGNAL_PWM           3
#define SIGNAL_DEADTIME      4

/* Prototypes */ 
void fpga_init(void);
static inline void fpga_transfer(u08 board, u08 signal, u16 value);

/* Static functions you should use */ 
static inline void 
fpga_setpwm(uint16_t value){
    if(value > PWM_MAX)
        value = PWM_MAX;
    else if(value < PWM_MIN)
        value = PWM_MIN; 

    fpga_transfer(1, SIGNAL_PWM, value); 
}

static inline void 
fpga_enable(u08 on){
	if(on == FPGA_ON)
		P2OUT |= FPGA_ENABLE;
	else
        P2OUT &= ~FPGA_ENABLE; 
}

/* FPGA_RESET is active low */ 
static inline void 
fpga_reset(void){
	P2OUT &= ~FPGA_RESET; 
	P2OUT |= FPGA_RESET; 
}

static inline void 
fs_reset(void){
	P2OUT |= FS_RESET; 
	P2OUT &= ~FS_RESET; 
}

static inline uint8_t 
fpga_nFS(void){
	return P2IN & FS; 
}


/* -------------------------------------------------- */ 
/* Static inline functions you shouldn't use directly, 
   but are here for speed */ 
/* -------------------------------------------------- */ 

static inline void 
fpga_transfer(u08 board, u08 signal, u16 value){
    u16 transfer = 0; 

	transfer |= ((u16)board & 0x03) << 14;
	transfer |= ((u16)signal & 0x07) << 11; 
	transfer |= (value & 0x07FF) << 0; 

    ENABLE_FPGA_SPI(); 
    spi0_transfer((transfer >> 8) & 0xFF); 
	spi0_transfer(transfer & 0xFF); 
	DISABLE_FPGA_SPI(); 

	spi0_transfer(0x00); 
}

#endif

