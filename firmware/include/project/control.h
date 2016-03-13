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

/* Run the control loop */ 

#ifndef __CONTROL__
#define __CONTROL__

void control_init();
void control_start(void);
void control_set_voltage(int32_t mvolts);	
void control_set_raw(int16_t raw);
int32_t control_get_target(void);
int control_is_saturated(void);

volatile void set_max_vout_adc(uint16_t new_vout_adc);
volatile void set_min_vin_adc(uint16_t new_vin_adc);
volatile void update_control_maxmin(void);

void adc_acc_read_and_zero(int i, uint32_t* value, uint16_t* num);
uint32_t adc_acc_read_zero_divide(int i);

typedef struct pid_data_t {
	/* Variables */
  int32_t uk_1; /* Previous PI output */
  int32_t ek_1; /* Previous error */ 
  int32_t integral; /* Accumulated integral component */ 
} pid_data_t;

typedef struct pid_const_t {
        int32_t Kp; 
        int32_t Ki; 
        int32_t Kd; 
} pid_const_t; 

/* Active loop constants */ 
#define INPUT_LOOP      0
#define OUTPUT_LOOP     1

extern volatile int     active_loop; 
extern volatile int32_t output; 
extern volatile int32_t control_error; /* Get rid of this! */ 

#endif
