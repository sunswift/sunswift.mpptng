/*! ------------------------------------------------------------------------- 
   Photovoltaics tracking code.
    
   File name: pv_track.c
   Author: David Snowdon 
   Date: 4th June, 2005
    Copyright (C) David Snowdon, 2009. 
   ----------------------------------------------------------------------- */ 

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
#include <signal.h>

#include <scandal/leds.h>
#include <scandal/utils.h>
#include <scandal/devices.h>
#include <scandal/message.h>

#include <arch/adc.h>

#include <project/mpptng.h>
#include <project/hardware.h>
#include <project/control.h>
#include <project/pv_track.h>

/* Different pieces of data to be sent */ 
#define NO_DATA          0
#define IVSWEEP_DATA     1

volatile uint32_t pv_counter; 
volatile int      pv_algorithm;
volatile int      senddata_flag; 

sc_time_t         last_debug_data; 
sc_time_t         last_pvtrack_data; 

int32_t vin_raw;  
int32_t iin_raw;  

/* Prototypes */ 
static inline void pv_track(); 

static inline void pvtrack_openloop_start(void); 
static inline void pvtrack_openloop(void); 

static inline void pvtrack_pando_start(void);
static inline void pvtrack_pando(void); 

static inline void pvtrack_ivsweep_start(void);
static inline void pvtrack_ivsweep(void);

static inline void pvtrack_manual_start(void);
static inline void pvtrack_manual(void);


/* Algorithm data storage */ 
volatile union {
  /* Data for the openloop algorithm */
  struct {
    int mode; /* Whether we are waiting for a sample or converting */ 
    int previous; /* Stores the previous measurement so we can work out the gradient */ 
  } openloop; 

  /* Data for the P & O algorithm */ 
  struct {
    int direction; 
    uint32_t lastpower; 
    int mode;
  } pando;

  /* Data for the Incremental conductance algorithm */ 
  struct {
    
  } inccond;

  /* Data for the IV sweep algorithm */ 
  struct { 
    int last_algorithm; 
    int32_t vin, iin; 
    int phase; 
  } ivsweep; 
} pvdata; 
 


/* Interrupt handler associated with internal RTC */
/* Timer B overflow interrupt */
interrupt (TIMERB0_VECTOR) timerb0(void) {
  pv_track(); 
}

void pv_track_init(void){
  /* Clear counter, input divider /1, ACLK */
  TBCTL = /*TBIE |*/ TBCLR | ID_DIV1 | TBSSEL_ACLK;

  /* Enable Capture/Compare interrupt */
  TBCCTL0 = CCIE;
  TBCCR0 = 32768 / PV_HZ; /* Count 1/PV_HZ sec at ACLK=32768Hz */
  
  /* Start timer in up to CCR0 mode */
  TBCTL |= MC_UPTO_CCR0;

  /* Initialise variables */ 
  pv_counter = 0; 
  
  /* Initialise with default algorithm */ 
  pv_track_switchto(config.algorithm); 

  /* Initialise the send data flags */ 
  senddata_flag = NO_DATA; 
  last_debug_data = sc_get_timer(); 
  last_pvtrack_data = sc_get_timer(); 
}



void pv_track_switchto(int algorithm){
  switch(algorithm){
  case MPPTNG_PANDO:
    pvtrack_pando_start(); 
    break; 

  case MPPTNG_INCCOND:
    break; 

  case MPPTNG_IVSWEEP:
    pvtrack_ivsweep_start(); 
    break; 
    
  case MPPTNG_MANUAL:
    pvtrack_manual_start(); 
    break; 
    
  case MPPTNG_OPENLOOP:
  default:
    pvtrack_openloop_start(); 
    break; 
  }

  pv_algorithm = algorithm; 
}

static inline void pv_track(void){
  vin_raw = adc_acc_read_zero_divide(MEAS_VIN1);
  iin_raw = adc_acc_read_zero_divide(MEAS_IIN1);

  switch(pv_algorithm){
  case MPPTNG_PANDO:
    pvtrack_pando(); 
    break; 

  case MPPTNG_INCCOND:
    break; 

  case MPPTNG_IVSWEEP:
    pvtrack_ivsweep(); 
    break; 

  case MPPTNG_MANUAL:
    pvtrack_manual(); 
    break; 

  case MPPTNG_OPENLOOP:
  default:
    pvtrack_openloop(); 
    break;

  }
}

void pv_track_send_telemetry(void){
  scandal_send_scaled_channel(TELEM_LOW, UNSWMPPTNG_IN_VOLTAGE, vin_raw);
  scandal_send_scaled_channel(TELEM_LOW, UNSWMPPTNG_IN_CURRENT, iin_raw);  
}

void pv_track_send_data(void){
  sc_time_t thetime = sc_get_timer();
  if(thetime > last_pvtrack_data + TELEMETRY_UPDATE_PERIOD){
    last_pvtrack_data = thetime; 
    scandal_send_channel(TELEM_LOW, UNSWMPPTNG_PANDO_POWER, \
			 pvdata.pando.lastpower);
  }

  //#define DEBUG
#ifdef DEBUG
  if(sc_get_timer() > last_debug_data + TELEMETRY_UPDATE_PERIOD){
    scandal_send_channel(TELEM_LOW, 141, control_get_target()); 
    scandal_send_channel(TELEM_LOW, 142, config.out_pid_const.Kp); 
    scandal_send_channel(TELEM_LOW, 143, config.out_pid_const.Ki); 
    scandal_send_channel(TELEM_LOW, 144, config.out_pid_const.Kd);
    scandal_send_channel(TELEM_LOW, 148, ADC12MEM3);
    scandal_send_channel(TELEM_LOW, 145, output); 
    scandal_send_channel(TELEM_LOW, 146, control_error); 
    scandal_send_channel(TELEM_LOW, 147, pv_algorithm); 
    last_debug_data = sc_get_timer(); 
  }
#endif

  if(senddata_flag == IVSWEEP_DATA){
    scandal_send_channel(TELEM_HIGH, 
			 UNSWMPPTNG_SWEEP_IN_VOLTAGE, 
			 pvdata.ivsweep.vin); 

    scandal_send_channel(TELEM_HIGH, 
			 UNSWMPPTNG_SWEEP_IN_CURRENT, 
			 pvdata.ivsweep.iin); 
    senddata_flag = 0; 
  }
}

static inline void pvtrack_openloop_start(void){
  pvdata.openloop.mode = OL_SAMPLING;  /* Start out by taking a sample */ 
  pv_counter = 0;                      /* Start counter again */ 
  control_set_voltage(ABS_MAX_VIN);    /* Set the control loop to the absolute maximum input V */ 
}

static inline void pvtrack_openloop(void){
  switch(pvdata.openloop.mode){
  case OL_SAMPLING:
    if(pv_counter++ >= OL_SAMPLING_COUNT){
      scandal_get_scaled_value(UNSWMPPTNG_IN_VOLTAGE, &vin_raw);               /* Scale the voltage to the real values */ 
      control_set_voltage(((int32_t)config.openloop_ratio * vin_raw) / 1000);  /* Set the output voltage */ 

      pvdata.openloop.mode = OL_CONVERTING;                                    /* Get ready for the next sample */ 

      pv_counter = 0; 
    }
    break; 

  case OL_CONVERTING:
  default: 
    if((pv_counter++) > config.openloop_retrack_period){
      control_set_voltage(ABS_MAX_VIN); 
      pvdata.openloop.mode = OL_SAMPLING; 
      pv_counter = 0; 
    }
    toggle_red_led(); 
    break; 
  }
}




static inline void pvtrack_pando_start(void){
  pvdata.pando.mode = PANDO_SAMPLING;  /* Start out by taking a sample */ 
  pv_counter = 0;                      /* Start counter again */ 
  pvdata.pando.direction = config.pando_increment;          /* Start positively. :-). */ 
  pvdata.pando.lastpower = 0;          /* This will get update on our first trip through the loop */ 

  control_set_voltage(ABS_MAX_VIN);    /* Set the control loop to the absolute maximum input V */ 
}

static inline void pvtrack_pando(void){
  uint64_t power_raw;

  switch(pvdata.pando.mode){
  case PANDO_SAMPLING:
    if((pv_counter++) >= PANDO_SAMPLE_COUNT){
      scandal_get_scaled_value(UNSWMPPTNG_IN_VOLTAGE, &vin_raw);               /* Scale the voltage to the real values */ 
      control_set_voltage(((int32_t)config.openloop_ratio * vin_raw) / 1000);  /* Set the output voltage */ 

      pvdata.pando.mode = PANDO_TRACKING;                                    /* Get ready for the next sample */ 

      pv_counter = 0; 
    }
    break; 

  case PANDO_TRACKING:
    if((pv_counter++) >= PANDO_UPDATE_COUNT){
      power_raw = (uint64_t)vin_raw * (uint64_t)iin_raw; 
      
      /* If we got less power than last time, switch directions */ 
      if(power_raw < pvdata.pando.lastpower)
	pvdata.pando.direction = -pvdata.pando.direction; 
      
      /* Take an offset from the present value */ 
      control_set_raw(vin_raw + pvdata.pando.direction);
      
      pvdata.pando.lastpower = power_raw; 
      
      toggle_red_led(); 
      pv_counter = 0;
    } 
    break; 
  }
}

static inline void pvtrack_inccond(void){
  
}

static inline void pvtrack_ivsweep_start(void){
  /* Don't start a new IV sweep mid-sweep */ 
  if(pv_algorithm != MPPTNG_IVSWEEP){
    /* set the phase to settling */ 
    pvdata.ivsweep.phase = IVSWEEP_PHASE_SETTLE; 

    /* Record what the previous algorithm was */ 
    pvdata.ivsweep.last_algorithm = pv_algorithm; 
    control_set_voltage(ABS_MAX_VIN);
    pv_counter = 0; 
  }
}

static inline void pvtrack_ivsweep(void){
  switch(pvdata.ivsweep.phase){
  case IVSWEEP_PHASE_SETTLE:
    if(pv_counter > PVTRACK_PERIOD_TO_COUNT(IVSWEEP_SETTLE_MS)){
      pvdata.ivsweep.phase = IVSWEEP_PHASE_SWEEP; 
      pv_counter = 0; 
    }else
      pv_counter++;
    break; 
    
  case IVSWEEP_PHASE_SWEEP:
    if((pv_counter++) >= config.ivsweep_sample_period){
      uint32_t vin = vin_raw, iin=iin_raw; 

      scandal_get_scaled_value(UNSWMPPTNG_IN_VOLTAGE, &vin); 
      scandal_get_scaled_value(UNSWMPPTNG_IN_CURRENT, &iin); 
      
      pvdata.ivsweep.vin = vin; 
      pvdata.ivsweep.iin = iin; 
      senddata_flag = IVSWEEP_DATA; 
      
      vin -= config.ivsweep_step_size; 
      /* Once we hit the lowest voltage, switch
	 back to the original algorithm */ 
      if((vin < config.min_vin) || (control_is_saturated()))
	pv_track_switchto(pvdata.ivsweep.last_algorithm); 
      
      control_set_voltage(vin); 
      
      pv_counter = 0; 
    }
    break;
  }
}

static inline void pvtrack_manual_start(void){
    control_set_voltage(ABS_MAX_VIN);
}

static inline void pvtrack_manual(void){
}
