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

#include <io.h>
#include <signal.h>
#include <iomacros.h>

#include <scandal/timer.h>
#include <scandal/leds.h>
#include <scandal/can.h>
#include <scandal/engine.h>
#include <scandal/spi.h>
#include <scandal/devices.h>
#include <scandal/utils.h>
#include <scandal/message.h>
#include <scandal/error.h>
#include <scandal/eeprom.h>
#include <scandal/adc.h>

#include <arch/adc.h>

#include <project/spi_devices.h>
#include <project/other_spi.h>
#include <project/fpga.h>
#include <project/mpptng.h>
#include <project/control.h>
#include <project/pv_track.h>
#include <project/mpptng_error.h>
#include <project/config.h>
#include <project/hardware.h>

/* Switch to turn on debug information (via CAN) */ 
#define DEBUG           1

/* Switch to enable or disable the watchdog timer */ 
#define USE_WATCHDOG    1

/* Defines */ 
#define WDTCTL_INIT     WDTPW|WDTHOLD

/* Global variables */ 
volatile int tracker_status = 0;  
volatile mpptng_config_t    config; 

#if USE_WATCHDOG
static inline void 
init_watchdog(void){
    /* Set to use SMCLK 7.372800 MHz as the clock, 
        Divide by 512,
        Which gives an overflow of about 16s */ 
  WDTCTL = WDT_ARST_1000;
}

/* Reset the timer to 0 so that we don't get reset */ 
static inline void 
kick_watchdog(void){
  WDTCTL = WDT_ARST_1000 | WDTCNTCL;
}
#endif


void init_ports(void){
  P1OUT = 0x00;
  P1SEL = 0x00;
  P1DIR = 0x00;
  P1IES = CAN_INT;
  P1IE  = 0x00; // CAN_INT;  /* CAN Interrupt disabled by default */

  P2OUT = FPGA_RESET | FPGA_ENABLE; 
  P2SEL = 0x00;
  P2DIR = FS_RESET | FPGA_RESET | FPGA_ENABLE;
  P2IES = 0x00;
  P2IE  = 0x00;

  P3OUT = 0x00;
  P3SEL = SIMO0 | SOMI0 | UCLK0;
  P3DIR = SIMO0 | UCLK0 | FPGA_CS | RED_LED_BIT | YELLOW_LED_BIT;

  P4OUT = 0x00;
  P4SEL = 0x00;
  P4DIR = 0x00;

  P5OUT = CAN_CS;
  P5SEL = SIMO1 | SOMI1 | UCLK1;
  P5DIR = CAN_CS | SIMO1 | UCLK1;

  P6SEL = MEAS15_PIN | IMEAS_IN1_PIN | 
	  UMEAS_OUT1_PIN | UMEAS_IN1_PIN | THEATSINK_PIN;  
}

void init_clock(void){
  volatile unsigned int i;

  /* XTAL = LF crystal, ACLK = LFXT1/1, DCO Rset = 4, XT2 = ON */
  BCSCTL1 = 0x04;
  
  /* Clear OSCOFF flag - start oscillator */
  _BIC_SR( OSCOFF );
  do{
    /* Clear OSCFault flag */
    IFG1 &= ~OFIFG; 
    /* Wait for flag to set */
    for( i = 255; i > 0; i-- )
      ;
  } while(( IFG1 & OFIFG ) != 0);
  
  /* Set MCLK to XT2CLK and SMCLK to XT2CLK */
  BCSCTL2 = 0x88; 
}

/*--------------------------------------------------
  Interrupt handling for CAN stuff 
  --------------------------------------------------*/
void enable_can_interrupt(){
  /*  P1IE = CAN_INT; */
  /* CAN interrupt disabled for now, since it seems to 
     cause problems with the control loops. I suspect
     this is because we are accessing the MCP2510 from 
     both inside and outside an interrupt context, 
     which probably leads to badness */ 
}

void disable_can_interrupt(){
  P1IE = 0x00;
}

/* Interrupt handler associated with internal RTC */
/* Timer A overflow interrupt */
interrupt (PORT1_VECTOR) port1int(void) {
  can_interrupt();
  P1IFG = 0x00;
}


/* Main function */
int main(void) {
  sc_time_t     my_timer;  
  int32_t value; 

  dint();

#if USE_WATCHDOG
    init_watchdog(); 
#else
    WDTCTL = WDTCTL_INIT;               //Init watchdog timer
#endif
    
  init_ports();
  init_clock();
  sc_init_timer();

  scandal_init(); 

  config_read(); 

  {volatile int i; 
    for(i=0; i<100; i++)
      ;
  }

  /* Below here, we assume we have a config */ 

  /* Send out the error that we've reset -- it's not fatal obviously, 
     but we want to know when it happens, and it really is an error, 
     since something that's solar powered should be fairly constantly 
     powered */ 
  scandal_do_user_err(UNSWMPPTNG_ERROR_WATCHDOG_RESET); 
    
    /* Make sure our variables are set up properly */ 
    tracker_status = 0;     
    
  /* Initialise FPGA (or, our case, CPLD) stuff */ 
  fpga_init(); 

  /* Starts the ADC and control loop interrupt */
  control_init(); 

  /* Initialise the PV tracking mechanism */ 
  pv_track_init(); 

  eint();

  my_timer = sc_get_timer(); 

  while (1) {
    sc_time_t timeval; 

    timeval = sc_get_timer();

    handle_scandal(); 
    
    /* pv_track sends data when it feels like it */ 
    pv_track_send_data(); 

    /* Periodically send out the values recorded by the ADC */ 
    if(timeval >= my_timer + TELEMETRY_UPDATE_PERIOD){
        my_timer = timeval;
        toggle_yellow_led();
#if USE_WATCHDOG
	kick_watchdog(); 
#endif
        
        mpptng_do_errors(); 

        pv_track_send_telemetry(); 
        /* We send the Input current and voltage from 
            within the pvtrack module */ 

        /*  scandal_send_scaled_channel(TELEM_LOW, UNSWMPPTNG_IN_VOLTAGE, 
                        sample_adc(MEAS_VIN1));
            scandal_send_scaled_channel(TELEM_LOW, UNSWMPPTNG_IN_CURRENT, 
                        sample_adc(MEAS_IIN1));*/ 

        scandal_send_scaled_channel(TELEM_LOW, UNSWMPPTNG_OUT_VOLTAGE, 
                    sample_adc(MEAS_VOUT));
        scandal_send_scaled_channel(TELEM_LOW, UNSWMPPTNG_HEATSINK_TEMP, 
                    sample_adc(MEAS_THEATSINK));
        scandal_send_scaled_channel(TELEM_LOW, UNSWMPPTNG_15V, 
                    sample_adc(MEAS_15V));
        scandal_send_channel(TELEM_LOW, UNSWMPPTNG_STATUS, 
                    tracker_status); 

        /* Pre-scale for the temperature */ 
        {
            int32_t degC = sample_adc(MEAS_TAMBIENT); 
            degC = (((degC - 1615)*704*1000)/4095);
            scandal_send_scaled_channel(TELEM_LOW, UNSWMPPTNG_AMBIENT_TEMP, 
                                        degC);
        }

#if DEBUG >= 1
        scandal_send_channel(TELEM_LOW, 134, output);	
        scandal_send_channel(TELEM_LOW, 136, fpga_nFS()); 
#endif
    } 

    /*  If we're not tracking, 
        check to see that our start-up criteria are satisfied, and then
        initialise the control loops and restart tracking */ 
    if((tracker_status & STATUS_TRACKING) == 0){
        /* Check the input voltage */
        value = sample_adc(MEAS_VIN1); 
        scandal_get_scaled_value(UNSWMPPTNG_IN_VOLTAGE, &value);
        if(value < config.min_vin)
            continue; 

        /* Check the output voltage */
        value = sample_adc(MEAS_VOUT);
        scandal_get_scaled_value(UNSWMPPTNG_OUT_VOLTAGE, &value); 
        if(value > config.max_vout)
            continue; 

        tracker_status |= STATUS_TRACKING; 

        /* Initialise the tracking algorithm */ 
        //      pv_track_init(); 

        /* Reset the FPGA */ 	 
        fs_reset(); 

        /* Initialise the control loop */ 
        control_start(); 

        /* Enable the FPGA */ 
        fpga_enable(FPGA_ON); 
    }
  }
}


