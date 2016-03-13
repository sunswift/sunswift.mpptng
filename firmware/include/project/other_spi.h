/* Customised SPI functions for communicating with the FPGA on the 
    MPPTNG board. These have been hacked for speed */ 

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


#ifndef __OTHER_SPI__
#define __OTHER_SPI__

#include <io.h>

#include <project/spi_devices.h>

/* Functions -- static inline for speed */
static inline uint8_t 
init_spi0(){
    ME1 |= USPIE0;
  
    U0CTL  = SYNC+MM+CHAR;
    U0TCTL = STC | SSEL1|SSEL0 | CKPL ;/* Not sure about the clock polarity */
    UBR00 = 0x04;
    UBR10 = 0x00;
    UMCTL0 = 0x00;

    DISABLE_FPGA_SPI();
    return(0); 
}

static inline void
spi0_send(uint8_t out_data){
    IFG1 &= ~URXIFG0;

    while((IFG1 & UTXIFG0) == 0)
        ;
  
    TXBUF0 = out_data;
}

static inline uint8_t 
spi0_transfer(uint8_t out_data){ 
    uint8_t	value; 

    IFG1 &= ~URXIFG0;

    while((IFG1 & UTXIFG0) == 0)
        ;
  
    TXBUF0 = out_data;

    while((IFG1 & URXIFG0) == 0)
        ;

    value = RXBUF0;

    return(value); 
} 

#endif
