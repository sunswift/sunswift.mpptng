/* ------------------------------------------------------------------------
   Scandal Obligations
   Functions which the application writer must provide.
   
   File name: scandal_obligations.c
   Author: David Snowdon
   Date: 1/9/03
    Copyright (C) David Snowdon, 2009. 
   ------------------------------------------------------------------------- */

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


#include <io.h>

#include <scandal/obligations.h>
#include <scandal/error.h>
#include <scandal/devices.h>
#include <scandal/leds.h>
#include <scandal/engine.h>
#include <scandal/eeprom.h>
#include <scandal/message.h>
#include <scandal/adc.h>

#include <arch/adc.h>

#include <project/hardware.h>
#include <project/mpptng.h>
#include <project/spi_devices.h>
#include <project/other_spi.h>
#include <project/fpga.h>
#include <project/pv_track.h>
#include <project/config.h>
#include <project/mpptng_error.h>

/* Reset the node in a safe manner
	- will be called from handle_scandal */
void scandal_reset_node(void){
  /* Reset the node here */
  /* Write an invalid password to the WDT */
  /* Panic the tracker to disable the conversion */ 
  fpga_enable(FPGA_OFF); 
  WDTCTL = ~WDTPW;
}

void scandal_user_do_first_run(void){
  scandal_set_m(UNSWMPPTNG_IN_VOLTAGE, DEFAULT_VIN_M);
  scandal_set_b(UNSWMPPTNG_IN_VOLTAGE, DEFAULT_VIN_B); 

  scandal_set_m(UNSWMPPTNG_IN_CURRENT, DEFAULT_IIN_M);
  scandal_set_b(UNSWMPPTNG_IN_CURRENT, DEFAULT_IIN_B); 

  scandal_set_m(UNSWMPPTNG_OUT_VOLTAGE, DEFAULT_VOUT_M);
  scandal_set_b(UNSWMPPTNG_OUT_VOLTAGE, DEFAULT_VOUT_B); 

  scandal_set_m(UNSWMPPTNG_15V, DEFAULT_15V_M);
  scandal_set_b(UNSWMPPTNG_15V, DEFAULT_15V_B); 

  scandal_set_m(UNSWMPPTNG_HEATSINK_TEMP, DEFAULT_HEATSINK_TEMP_M);
  scandal_set_b(UNSWMPPTNG_HEATSINK_TEMP, DEFAULT_HEATSINK_TEMP_B); 

  scandal_set_m(UNSWMPPTNG_AMBIENT_TEMP, DEFAULT_AMBIENT_TEMP_M);
  scandal_set_b(UNSWMPPTNG_AMBIENT_TEMP, DEFAULT_AMBIENT_TEMP_B); 

  config.max_vout = DEFAULT_MAX_VOUT; 
  config.min_vin = DEFAULT_MIN_VIN;
  config.algorithm = DEFAULT_ALGORITHM; 
  config.in_pid_const.Kp = DEFAULT_IN_KP; 
  config.in_pid_const.Ki = DEFAULT_IN_KI; 
  config.in_pid_const.Kd = DEFAULT_IN_KD; 
  config.out_pid_const.Kp = DEFAULT_OUT_KP; 
  config.out_pid_const.Ki = DEFAULT_OUT_KI; 
  config.out_pid_const.Kd = DEFAULT_OUT_KD;
 
  config.openloop_ratio = DEFAULT_OPENLOOP_RATIO; 
  config.openloop_retrack_period = 
    PVTRACK_PERIOD_TO_COUNT(DEFAULT_OPENLOOP_RETRACK_PERIOD); 
  config.pando_increment = DEFAULT_PANDO_INCREMENT; 
  config.ivsweep_step_size = DEFAULT_IVSWEEP_STEP_SIZE; 
  config.ivsweep_sample_period = 
    PVTRACK_PERIOD_TO_COUNT(DEFAULT_IVSWEEP_SAMPLE_PERIOD); 

  config_write(); 

  return;
}

u08 scandal_user_do_config(u08 param, s32 value, s32 value2){
  /* Disable the tracker */ 
  fpga_enable(FPGA_OFF); 
  tracker_status &= ~STATUS_TRACKING; 

  switch(param){
  case UNSWMPPTNG_MAX_VOUT:
    if(value <= ABS_MAX_VOUT)
      config.max_vout = value;
    update_control_maxmin();
    break; 
    
  case UNSWMPPTNG_MIN_VIN:
    if(value >= ABS_MIN_VIN)
      config.min_vin = value;
    update_control_maxmin();
    break; 
    
  case UNSWMPPTNG_ALGORITHM:
    config.algorithm = value;
    pv_track_switchto(config.algorithm); 
    break; 

  case UNSWMPPTNG_IN_KP:
    config.in_pid_const.Kp = value; 
    break; 
    
  case UNSWMPPTNG_IN_KI:
    config.in_pid_const.Ki = value; 
    break; 
    
  case UNSWMPPTNG_IN_KD:
    config.in_pid_const.Kd = value; 
    break; 

  case UNSWMPPTNG_OUT_KP:
    config.out_pid_const.Kp = value; 
    break; 
    
  case UNSWMPPTNG_OUT_KI:
    config.out_pid_const.Ki = value; 
    break; 
    
  case UNSWMPPTNG_OUT_KD:
    config.out_pid_const.Kd = value; 
    break; 

  case UNSWMPPTNG_OPENLOOP_RATIO:
    config.openloop_ratio = value; 
    break; 

  case UNSWMPPTNG_OPENLOOP_RETRACK_PERIOD:
    config.openloop_retrack_period = 
      PVTRACK_PERIOD_TO_COUNT(value);
    break; 

  case UNSWMPPTNG_PANDO_INCREMENT:
    config.pando_increment = value; 
    break; 

  case UNSWMPPTNG_IVSWEEP_SAMPLE_PERIOD:
    config.ivsweep_sample_period = 
      PVTRACK_PERIOD_TO_COUNT(value); 
    break; 

  case UNSWMPPTNG_IVSWEEP_STEP_SIZE: 
    config.ivsweep_step_size = value; 
  }
  
  config_write(); 
  
  /* Reset the tracker to get us started again */ 
  scandal_reset_node(); 

  return NO_ERR;
}

u08 scandal_user_handle_command(u08 command, u08* data){
  switch(command){
  case UNSWMPPTNG_COMMAND_IVSWEEP:
    pv_track_switchto(MPPTNG_IVSWEEP);
    break;
  case UNSWMPPTNG_COMMAND_SET_TARGET:
    {
      uint32_t value; 
      value = ((uint32_t)data[0]) << 24 |
	((uint32_t)data[1]) << 16 |
	((uint32_t)data[2]) << 8 |
	((uint32_t)data[3]) << 0 ;
      control_set_voltage(value);
    }
    break;

  case UNSWMPPTNG_COMMAND_SET_AND_TUNE:
    {
      uint32_t value; 
      sc_time_t waittime; 

      value = ((uint32_t)data[0]) << 24 |
	((uint32_t)data[1]) << 16 |
	((uint32_t)data[2]) << 8 |
	((uint32_t)data[3]) << 0 ;
      control_set_voltage(value);

      waittime = sc_get_timer();

      while(sc_get_timer() < waittime + 300){
	scandal_send_channel(TELEM_LOW, 161, sample_adc(MEAS_VIN1));
	scandal_send_channel(TELEM_LOW, 162, output);
      }
    }
    break;
    

  }
  return NO_ERR; 
}

u08 scandal_user_handle_message(can_msg* msg){
  return NO_ERR;
}
