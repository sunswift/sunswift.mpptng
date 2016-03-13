/*! ------------------------------------------------------------------------- 
    \file scandal_config.h
        Scandal Configuration for MPPTNG
	 
	File name: scandal_config.h 
	Author: David Snowdon 
	Date: 1/7/03
    Copyright (C) David Snowdon, 2009. 
    -------------------------------------------------------------------------- */  

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

#ifndef __SCANDAL_CONFIG__
#define __SCANDAL_CONFIG__

#include <scandal/devices.h>

/* Define the type of device being used */
#ifdef THIS_DEVICE_TYPE
#error "Device type multiply defined (redefined to UNSWMPPTNG)"
#endif
#define THIS_DEVICE_TYPE	UNSWMPPTNG

/* Number of channels */
#define NUM_IN_CHANNELS		UNSWMPPTNG_NUM_IN_CHANNELS
#define NUM_OUT_CHANNELS 	UNSWMPPTNG_NUM_OUT_CHANNELS

/* Size of send/receive buffers */
#define CAN_TX_BUFFER_BITS	4
#define CAN_TX_BUFFER_MASK	0x0F
#define CAN_TX_BUFFER_SIZE 	(1<<CAN_TX_BUFFER_BITS)

#define CAN_RX_BUFFER_BITS	4
#define CAN_RX_BUFFER_MASK	0x0F
#define CAN_RX_BUFFER_SIZE	(1<<CAN_RX_BUFFER_BITS)

#endif
