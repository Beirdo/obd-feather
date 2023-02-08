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

#define TIMING_NOISE_THRESH 2       /* 1us */

    /* All values in units of 0.5us (500ns) */
    typedef struct {
	short int nom;
	short int tx_min;
	short int tx_max;
	short int rx_min;
	short int rx_max;
    } timing_t;

    enum {
        INDEX_SOF,
	INDEX_EOF,
	INDEX_BRK,
	INDEX_IFS,
        INDEX_INACT_0,
        INDEX_INACT_1,
        INDEX_ACT_0,
        INDEX_ACT_1,
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

