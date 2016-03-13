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

#include <msp430x14x.h>
#include <signal.h>
#include <string.h>

#include <scandal/leds.h>
#include <scandal/engine.h>
#include <scandal/utils.h>
#include <scandal/message.h>

#include <arch/adc.h>

#include <project/control.h>
#include <project/config.h>
#include <project/hardware.h>
#include <project/mpptng.h>
#include <project/fpga.h>
#include <project/mpptng_error.h>

#define OUTPUT_TO_PWM(x) (((int32_t)x) >> 14)
#define PWM_TO_OUTPUT(x) (((int32_t)x) << 14)
#define INTEGRAL_DIVIDER_BITS 1
#define DIFFERENTIAL_DIVIDER_BITS 10

#define OUT_MAX PWM_TO_OUTPUT(PWM_MAX)
#define OUT_MIN PWM_TO_OUTPUT(PWM_MIN)

volatile int32_t target; /* target voltage mV */
volatile int32_t output; /* current pwm output */
volatile int32_t control_error; /* current pwn output */

volatile int     active_loop; /* Tracking on input or output */ 

volatile pid_data_t in_pid_data;
volatile pid_data_t out_pid_data;

uint16_t min_vin_adc  = 0;    /* will be updated from the config */ 
uint16_t max_vout_adc = 4095; /* will be updated from the config*/ 

/* Internal prototypes */ 
static inline uint16_t read_adc_value(u08 channel);


/*---------------------------------------------------------------
 ADC Code
 -- 
 The ADC code in the MPPTNG is fairly integrated with the control 
 loop to keep the latency between the measurement of the input
 voltage or input current as low as possible. 
 ----------------------------------------------------------------*/

static volatile uint16_t			samples[ADC_NUM_CHANNELS]; 

volatile uint32_t acc_value[ADC_NUM_CHANNELS]; 
volatile uint16_t acc_num[ADC_NUM_CHANNELS]; 


void init_adc(void) {
	/* Turn on 2.5V reference, enable ADC */
	/* Sample hold timer setting ?, Mulitple sample/conversion */
	ADC12CTL0 = ADC12ON | SHT0_9 | SHT1_9 | REFON | REF2_5V | MSC;  
	
	/* Repeated, sequence mode; Use sampling timer, SMCLK */
	ADC12CTL1 = SHP | CONSEQ_3 | ADC12SSEL_3 | CSTARTADD_0; 
	
	/* 
	 * Monitor the five channels. 
	 * These have a very deliberate ordering
	 * The latency between the control variables (Vin and Iin) and 
	 * the actual interrupt should be minimal. 
	 */
	
	ADC12MCTL0 = SREF_1 | INCH_TAMBIENT;
	ADC12MCTL1 = SREF_1 | INCH_THEATSINK;
	ADC12MCTL2 = SREF_1 | INCH_15V;
	ADC12MCTL3 = SREF_1 | INCH_VOUT;
	ADC12MCTL4 = SREF_1 | INCH_IIN1; 
	ADC12MCTL5 = SREF_1 | EOS | INCH_VIN1; 
	
	/* Enable interrupt for ADC12MCTL5 */
	ADC12IE = (1 << 5);
	
	/* Zero out the sample array */ 
        /* Clear sample arrays */
        memset((uint16_t*)samples, 0, sizeof(samples[0])
	       * ADC_NUM_CHANNELS);

	/* Zero out the accumulator readings */ 
        memset((uint32_t*)acc_value, 0, sizeof(acc_value[0])
	       * ADC_NUM_CHANNELS);

        memset((uint16_t*)acc_num, 0, sizeof(acc_num[0])
	       * ADC_NUM_CHANNELS);


	/* Enable conversions */
	ADC12CTL0 |= ENC | ADC12SC;
}

/* Digital filtering scheme */ 
/* Note that this does not implement higher-order filters */ 
#define DIGITAL_FILTER(sample, new_sample){\
sample += (new_sample);		    \
sample >>= 1;			    \
}

static inline uint16_t 
read_adc_value(u08 channel){
	return samples[channel];
}

u16 sample_adc(u08 channel){
	uint16_t sample; 
	
	/* Disable the ADC interrupt */ 
	ADC12IE &= ~(1 << 9);
	
	/* Short pause to make sure its off */ 
	{volatile int i; 
		for(i=15; i>0; i--)
			; 
	}
	
	sample = read_adc_value(channel); 
	
	/* Turn the interrupt back on again */ 
	ADC12IE |= (1<<9); 
	
	/* Return the most recent value of the ADC - Unscaled */
	return(sample); 
}

#define ACCUMULATE_VALUE(i, new_sample){\
  acc_value[i] += new_sample; \
  acc_num[i] ++; \
}

void adc_acc_read_and_zero(int i, uint32_t* value, uint16_t* num){
  /* Disable the ADC interrupt */ 
  ADC12IE &= ~(1 << 9);
  
  /* Short pause to make sure its off */ 
  {volatile int i; 
    for(i=15; i>0; i--)
      ; 
  }
  
  *value = acc_value[i]; 
  *num = acc_num[i]; 

  acc_value[i] = 0;
  acc_num[i] = 0; 

  /* Turn the interrupt back on again */ 
  ADC12IE |= (1<<9); 
}

/* Returns the number of samples */ 
uint32_t adc_acc_read_zero_divide(int i){
  uint32_t value; 
  uint16_t num; 

  adc_acc_read_and_zero(i, &value, &num); 
  
  return value / num;  
}


/*---------------------------------------------------------------
 Control loop code
 -- 
 The control loop code is designed to minimise the delay between 
 the input voltage measurement and the setting of the PWM. 
 ----------------------------------------------------------------*/

int control_is_saturated(void){
  return(in_pid_data.uk_1 >= OUT_MAX);
}

void
pid_init(volatile pid_data_t* pid_data){
	pid_data->uk_1 = 0;
	pid_data->ek_1 = 0; 
	pid_data->integral = 0; 
}

static inline int32_t /* uk */
pid_ctrl (int32_t ek, 
	 volatile pid_data_t* pid_data, 
	 volatile pid_const_t* pid_const) {
  int32_t uk;
  int32_t value; 
  
  uk = ek * pid_const->Kp; /* Proportional */ 
  uk += pid_const->Kd * (ek - pid_data->ek_1); /* Differential */ 
  pid_data->integral += ek * pid_const->Ki;

  /* Rate limit the increase of uk */
  value = uk + pid_data->integral; 
  if(( value - pid_data->uk_1) > (OUT_MAX >> 3)){
    value = pid_data->uk_1 + (OUT_MAX >> 3);
    pid_data->integral = value - uk; 
  }

  /* yyy <= xxx */
  if(value > OUT_MAX){
    value = OUT_MAX;
    pid_data->integral = value - uk; 
  }else if(value < OUT_MIN){
    value = OUT_MIN;
    pid_data->integral = value - uk; 
  }

  pid_data->uk_1 = value;
  pid_data->ek_1 = ek;


  return value;
}

void control_set_voltage(int32_t mvolts){
  int32_t min_vin; 
  
  min_vin = config.min_vin; 
  if(mvolts < min_vin)
    mvolts = config.min_vin;

  /* Convert to ADC reading */
  scandal_get_unscaled_value(UNSWMPPTNG_IN_VOLTAGE, &mvolts);

  ADC_INTERRUPT_DISABLE();
  target = mvolts;
  ADC_INTERRUPT_ENABLE();	
}

void control_set_raw(int16_t raw){
  /* Fix this when you fix all the scaling */ 
  if(raw < min_vin_adc)
    raw = min_vin_adc; 

  ADC_INTERRUPT_DISABLE();
  target = raw; 
  ADC_INTERRUPT_ENABLE(); 
}

int32_t control_get_target(void){
    return target; 
}

void tracker_panic(int error){
  fpga_enable(FPGA_OFF); 
  tracker_status &= ~STATUS_TRACKING; 
  mpptng_error(error); 
}

void control_init(void){
  active_loop = INPUT_LOOP; 
  output = PWM_MIN; 

  /* FIXME: Should scale using the calibrated constants here */ 
  update_control_maxmin(); 
  init_adc(); 
}

void control_start(){
	pid_init(&in_pid_data); 
	pid_init(&out_pid_data); 
	output = PWM_MIN;
	active_loop = INPUT_LOOP; 
	tracker_status |= STATUS_INPUT_LOOP;
	fpga_setpwm(output); 
}

/* -------------------------------
 Interrupt handlers 
 ------------------------------- */
/* ADC Interrupt -- Run the control loop */ 

#define ADC12MEM_VOUT			ADC12MEM3
#define ADC12MEM_VIN1			ADC12MEM5
#define ADC12MEM_IIN1			ADC12MEM4
#define ADC12MEM_15V			ADC12MEM2
#define ADC12MEM_THEATSINK		ADC12MEM1
#define ADC12MEM_TAMBIENT		ADC12MEM0

/* FIXME -- should be set to the maximum output voltage, converted to the equivalent ADC reading
	     -- should be updated whenever the config is updated */

volatile void set_max_vout_adc(uint16_t new_vout_adc){
  ADC_INTERRUPT_DISABLE();
  max_vout_adc = new_vout_adc; 
  ADC_INTERRUPT_ENABLE(); 
}

volatile void set_min_vin_adc(uint16_t new_vin_adc){
  ADC_INTERRUPT_DISABLE();
  min_vin_adc = new_vin_adc; 
  ADC_INTERRUPT_ENABLE(); 
}

volatile void update_control_maxmin(void){
  int32_t value; 
  
  value = config.max_vout; 
  scandal_get_unscaled_value(UNSWMPPTNG_OUT_VOLTAGE, &value); 
  set_max_vout_adc(value); 

  value = config.min_vin; 
  scandal_get_unscaled_value(UNSWMPPTNG_IN_VOLTAGE, &value); 
  set_min_vin_adc(value); 
}

interrupt (ADC_VECTOR) ADC12ISR(void) {
  uint32_t uk = OUT_MIN, in_uk = OUT_MIN, out_uk=OUT_MIN;  
  int16_t vout = ADC12MEM_VOUT; 
  int16_t vin  = ADC12MEM_VIN1;

	if(vout > ADC_ABS_MAX_VOUT){
		tracker_panic(UNSWMPPTNG_ERROR_OUTPUT_OVER_VOLTAGE); 
	}else if(vin < ADC_ABS_MIN_VIN){
		tracker_panic(UNSWMPPTNG_ERROR_INPUT_UNDER_VOLTAGE);
	}else if((tracker_status & STATUS_TRACKING) == 0){
	        fpga_setpwm(PWM_MIN); 
	}else{
		/* If we have a fault signal from the FPGA, panic */
		if(fpga_nFS() == 0){
			tracker_panic(UNSWMPPTNG_ERROR_FPGA_SHUTDOWN); 
			return; 
		}

		/* Run the output control loop */ 
		out_uk = pid_ctrl(vout - (int16_t)max_vout_adc, &out_pid_data, &config.out_pid_const);
		
		in_uk = pid_ctrl(vin-target, &in_pid_data, &config.in_pid_const);

		if(out_uk < in_uk)
		  uk = out_uk; 
		else
		  uk = in_uk; 
		
		fpga_setpwm(OUTPUT_TO_PWM(uk));
		output = OUTPUT_TO_PWM(uk); 
		control_error = out_uk; /*vout - (int16_t)max_vout_adc;*/ 

		if(out_uk > in_uk){
		  tracker_status |= STATUS_INPUT_LOOP; 
		  tracker_status &= ~STATUS_OUTPUT_LOOP; 
		}else{
		  tracker_status |= STATUS_OUTPUT_LOOP; 
		  tracker_status &= ~STATUS_INPUT_LOOP; 
		}
	}

	/* We do any extra gumph for the rest of the system here */ 
	DIGITAL_FILTER(samples[0], ADC12MEM0);
	DIGITAL_FILTER(samples[1], ADC12MEM1);
	DIGITAL_FILTER(samples[2], ADC12MEM2);
	DIGITAL_FILTER(samples[3], ADC12MEM3);
	DIGITAL_FILTER(samples[4], ADC12MEM4);
	DIGITAL_FILTER(samples[5], ADC12MEM0);

	ACCUMULATE_VALUE(0, ADC12MEM0)
	ACCUMULATE_VALUE(1, ADC12MEM1)
	ACCUMULATE_VALUE(2, ADC12MEM2)
	ACCUMULATE_VALUE(3, ADC12MEM3)
	ACCUMULATE_VALUE(4, ADC12MEM4)
	ACCUMULATE_VALUE(5, ADC12MEM5)
}
