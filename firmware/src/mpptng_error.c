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

/* error.c 
 * David Snowdon, 28 March, 2008
 */

#include <io.h>
#include <signal.h>
#include <iomacros.h>

#include <scandal/engine.h>
#include <scandal/leds.h>
#include <scandal/timer.h>
#include <scandal/error.h>

#include <project/hardware.h>
#include <project/fpga.h>
#include <project/mpptng.h>
#include <project/mpptng_error.h>

int last_error = 0; 

void mpptng_do_errors(void){
  if(last_error != NO_ERR){
    scandal_do_user_err(last_error); 
    last_error = NO_ERR; 
  }
}

void mpptng_error(int error){
  last_error = error; 
}

/* This function doesn't return - we sit in here until 
   we're told to do something via CAN */ 

void mpptng_fatal_error(int error){
  int count = 0; 

  /* Disable the tracker */ 
  fpga_enable(FPGA_OFF); 
  tracker_status &= ~STATUS_TRACKING; 

  red_led(0); 

  while(1){
    volatile uint32_t i; 

    handle_scandal(); 
    
    for(i=0; i<100000; i++)
      ; 
    
    count ++; 
    if(count <= (2 * error))
      toggle_red_led(); 
    if(count > (2 * error + 4)){
      scandal_do_user_err(error); 
      count = 0;
    }

  }
}
