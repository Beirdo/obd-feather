/* 
 * File:   main.h
 * Author: gjhurlbu
 *
 * Created on January 2, 2023, 4:58 PM
 */

#ifndef MAIN_H
#define	MAIN_H

#ifdef	__cplusplus
extern "C" {
#endif


#define SAE_PWM     0x20    /* PA5 */
#define nJ1850_INT  0x08    /* RC3 */
#define J1850_TX    0x10    /* RC4 */
#define J1850_RX    0x20    /* RC5 */
    
#define TIMING_PWM_SOF      96      /* 48us */
#define TIMING_PWM_INACT_0  16      /* 8us */  
#define TIMING_PWM_INACT_1  32      /* 16us */
#define TIMING_PWM_ACT_0    32      /* 16us */
#define TIMING_PWM_ACT_1    16      /* 8us */
#define TIMING_PWM_FUDGE    2       /* +/- 1us */

#define TIMING_VPW_SOF      400     /* 200us */
#define TIMING_VPW_INACT_0  128     /* 64us */
#define TIMING_VPW_INACT_1  256     /* 128us */
#define TIMING_VPW_ACT_0    256     /* 128us */
#define TIMING_VPW_ACT_1    128     /* 64us */
#define TIMING_VPW_FUDGE    8       /* +/- 4us */
    
#define TIMING_NOISE_THRESH 2       /* 1us */


    enum {
        INDEX_SOF,
        INDEX_INACT_0,
        INDEX_INACT_1,
        INDEX_ACT_0,
        INDEX_ACT_1,
        INDEX_FUDGE,
        INDEX_COUNT,
    };
    
    enum {
        TYPE_VPW,
        TYPE_PWM,
        TYPE_COUNT,
    };
    
    extern const unsigned short int j1850_timings[TYPE_COUNT][INDEX_COUNT];
    
    char get_sae_pwm(char force);


#ifdef	__cplusplus
}
#endif

#endif	/* MAIN_H */

