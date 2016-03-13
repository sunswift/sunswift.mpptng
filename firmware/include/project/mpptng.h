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

#ifndef __MPPTNG__
#define __MPPTNG__

#include <project/control.h>

/* For use as absolute max/min in the control loop */ 
#define VOUT_BOTTOM_RESISTANCE (1.0 / ( (1.0 / 47000.0) + (1.0 / (680000.0 + 10000.0)))) 
#define REF_VOLTAGE      (2.5)
#define ADC_BITS         (12)
#define ADC_READING(x)   ((((float)x) / REF_VOLTAGE) * ((float)(1<<ADC_BITS))) 
#define VOUT_TO_ADC(x)   (uint16_t)ADC_READING( ((float)x) *		\
						 VOUT_BOTTOM_RESISTANCE / \
						(VOUT_BOTTOM_RESISTANCE + 3000000.0))
#define VIN_TO_ADC(x)    (uint16_t)(ADC_READING((float)x * 47.0 / (47.0 + 3000.0)))
#define FIFTEEN_TO_ADC(x) (uint16_t)(x) /* FIXME! */ 
#define TEMP_TO_ADC(x)    (uint16_t)(x) /* FIXME! */ 


/* Tracker status */ 
#define STATUS_TRACKING         BIT(0)
#define STATUS_INPUT_LOOP       BIT(1)   /* Input loop active -- control.c */ 
#define STATUS_OUTPUT_LOOP      BIT(2)   /* Output loop active -- control.c */ 

/* Tracking algorithms */ 
#define MPPTNG_OPENLOOP        0
#define MPPTNG_PANDO           1
#define MPPTNG_INCCOND         2
#define MPPTNG_IVSWEEP         3
#define MPPTNG_MANUAL          4
#define MPPTNG_NUM_ALGORITHMS  5

/* Absolute limits of the tracker */ 
#define ABS_MAX_VOUT     170000
#define ABS_MIN_VIN      26000
#define ABS_MAX_VIN      150000
#define ABS_MIN_15V      12000
#define ABS_MAX_HS_TEMP  100000
#define ABS_MAX_VOUT_OVERSHOOT 1000

#define DEFAULT_MAX_VOUT 154000
#define DEFAULT_MIN_VIN  30000
#define DEFAULT_ALGORITHM MPPTNG_PANDO
#define DEFAULT_IN_KP    7000
#define DEFAULT_IN_KI    400 
#define DEFAULT_IN_KD    0
#define DEFAULT_OUT_KP   -10000
#define DEFAULT_OUT_KI   -30
#define DEFAULT_OUT_KD   0

/* Tracking algorithm stuff */ 
#define DEFAULT_OPENLOOP_RATIO  800
#define DEFAULT_OPENLOOP_RETRACK_PERIOD 10000
#define DEFAULT_PANDO_INCREMENT VIN_TO_ADC(0.5)
#define DEFAULT_IVSWEEP_STEP_SIZE 300
#define DEFAULT_IVSWEEP_SAMPLE_PERIOD 30
 
/* Frequency constants */ 
#define CONTROL_FS       1160L
#define twoFs            (2 * 1160)

/* Other constants */ 
#define TELEMETRY_UPDATE_PERIOD  800 /* Note: must be less than 1s, 
					because the watchdog timer must
					be kicked at least every second */ 

/* Default settings */ 
#define DEFAULT_PWM          0
#define DEFAULT_AUX_LENGTH   55
#define DEFAULT_AUX_OVERLAP  8
#define DEFAULT_DEADTIME     25
#define DEFAULT_RESTART      1300

/* PWM constants */ 
#define PWM_MAX              ((DEFAULT_RESTART - DEFAULT_AUX_LENGTH) * 8 / 10)
#define PWM_MIN              0

/* Default scaling factors - should be re-calibrated */ 

#define DEFAULT_VIN_M                  40522
#define DEFAULT_VIN_B                  -370615

#define DEFAULT_IIN_M                  2580
#define DEFAULT_IIN_B                  -8000

#define DEFAULT_VOUT_M                 43376  
#define DEFAULT_VOUT_B                 -1401352

#define DEFAULT_15V_M                  5208
#define DEFAULT_15V_B                  -526772

#define DEFAULT_HEATSINK_TEMP_M        425354         /* FIXME: VERY rough and doesn't work */ 
#define DEFAULT_HEATSINK_TEMP_B        -45371077

#define DEFAULT_AMBIENT_TEMP_M         1024
#define DEFAULT_AMBIENT_TEMP_B         0

/* Other constants */ 
#define OUTPUT_LOOP_CHANGEOVER_HYSTERESIS    2000     /* 2V */ 

/* Rough limits defined in terms of the default scaling values */ 
#define ADC_ABS_MAX_VOUT			VOUT_TO_ADC(ABS_MAX_VOUT / 1000.0)
#define ADC_ABS_MIN_VIN				VIN_TO_ADC(ABS_MIN_VIN / 1000.0)
#define ADC_ABS_MAX_VIN				VIN_TO_ADC(ABS_MAX_VIN / 1000.0)
#define ADC_ABS_MIN_15V				2000 /* 12.0V  -- NOT CORRECT, NEEDS UPDATE */
#define ADC_ABS_MAX_HS_TEMP			3000 /* 100 Degrees -- NOT CORRECT, NEEDS UPDATE */
#define ADC_OUTPUT_LOOP_CHANGEOVER_HYSTERESIS   100     /* 2V -- NOT CORRECT, NEEDS UPDATE */ 


typedef struct mpptng_config{
  int32_t max_vout; 
  int32_t min_vin; 
  uint8_t algorithm;

  /* Control loop parameters */ 
  pid_const_t   in_pid_const; 
  pid_const_t   out_pid_const; 

  /* PV tracking parameters */ 
  uint16_t openloop_ratio; 
  uint16_t pando_increment; 
  uint16_t openloop_retrack_period; 
  uint16_t ivsweep_sample_period; 
  uint16_t ivsweep_step_size; 
  
  /* Checksums */ 
  uint8_t magic; 
  uint8_t checksum; 
  uint8_t checkxor; 
}mpptng_config_t; 

extern volatile mpptng_config_t config; 
extern volatile int tracker_status; 

#endif
