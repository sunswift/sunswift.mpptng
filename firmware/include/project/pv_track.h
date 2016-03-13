/*! ------------------------------------------------------------------------- 
   Photovoltaics tracking code.
    
   File name: pv_track.c
   Author: David Snowdon 
   Date: 4th June, 2005
    Copyright (C) David Snowdon, 2009. 
   ----------------------------------------------------------------------- */ 

/* Constants */ 
#define PV_HZ                    32L

#define PVTRACK_PERIOD_TO_COUNT(x) ((((int32_t)x) * PV_HZ) / 1000L)

/* Open loop algorithm */ 
#define OL_RETRACK_PERIOD        10000    /* Re-tracking period in ms - should be configurable? */     
#define OL_RETRACK_COUNT         ((OL_RETRACK_PERIOD * PV_HZ) / 1000L)

#define OL_SAMPLING_COUNT        0

#define OL_SAMPLING              0
#define OL_CONVERTING            1

/* PandO algorithm */ 
#define PANDO_UPDATE_COUNT       2
#define PANDO_SAMPLE_PERIOD      100
#define PANDO_SAMPLE_COUNT       ((PANDO_SAMPLE_PERIOD * PV_HZ) / 1000L)

#define PANDO_SAMPLING           0
#define PANDO_TRACKING           1


/* IV SWEET algorithm */ 
#define IVSWEEP_PHASE_SETTLE     0
#define IVSWEEP_PHASE_SWEEP      1
#define IVSWEEP_SETTLE_MS        1000

void pv_track_init(void); 
void pv_track_switchto(int algorithm);
void pv_track_send_data(void);
void pv_track_send_telemetry(void);
