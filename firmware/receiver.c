/* 
 * File:   receiver.c
 * Author: gjhurlbu
 *
 * Created on January 2, 2023, 4:05 PM
 */

#include <string.h>
#include <proc/pic16f15225.h>

#include <main.h>
#include <receiver.h>
#include <ringbuf.h>
#include <i2c_slave.h>

char rx_count;
unsigned char rx_buffer[MAX_RX_BUF];
char rx_bit_count;
unsigned char rx_shift_reg;
char rx_in_frame;

char rx_last_level;
char rx_timer_active;
char rx_protocol_error;

ringbuf_t *rx_ringbuf;
char rx_buffers;

unsigned short int rx_last_timer;
unsigned short int rx_slop;

char check_timing(unsigned short int duration, unsigned short int *timings, 
        char index, unsigned short int fudge);

void receiver_init(void) {
    memset(rx_buffer, 0, MAX_RX_BUF);
    rx_count = 0;
   
    rx_bit_count = 0;
    rx_shift_reg = 0x00;
    rx_in_frame  = 0;
    
    rx_timer_active = 0;
    rx_slop = 0;
    rx_last_level = 0;
    rx_protocol_error = 0;
    
    rx_ringbuf = ringbuf_create(MAX_RINGBUF_SIZE, &rx_buffers);
   
    /* Capture CCP1 on both edges */
    CCP1CON = 0x00;     /* disable first */
    CCP1CAP = 0x00;     /* Use the pin as setup by PPS */
    CCP1CON = 0x83;     /* Set to both edge capture, enable */

    /* Setup CCP1 interrupt */
    INTCONbits.GIE = 0;
    INTCONbits.PEIE = 1;
    PIE1bits.CCP1IE = 0;
    INTCONbits.GIE = 1;
}

char check_timing(unsigned short int duration, unsigned short int *timings, 
        char index, unsigned short int fudge) {
    unsigned short int timing = timings[index];
    if ((duration >= timing - fudge) && (duration <= timing + fudge)) {
        return 1;
    }
    return 0;
}

void ccp1_isr(void) {
    unsigned short int duration = 0;
    unsigned short int now;
    char this_bit = 0;
    char sae_pwm = get_sae_pwm(!rx_timer_active);
    unsigned short int *timings = &j1850_timings[sae_pwm];
    unsigned short int fudge = timings[INDEX_FUDGE];
    char sof = 0;
    char found = 0;

    PIR1bits.CCP1IF = 1;
    
    if (rx_timer_active) {
        now = 0;
        while(now != CCPR1) {
            now = CCPR1;
        }

        if (now >= rx_last_timer) {
            duration = now - rx_last_timer; 
        } else {
            duration = now + 1 + (0xFFFF - rx_last_timer);
        }
        
        if (duration <= TIMING_NOISE_THRESH) {
            rx_slop += duration;
            duration = 0;
        } else {
            duration += rx_slop;
            rx_slop = 0;
        }
        
        if (duration) {
            if (check_timing(duration, timings, INDEX_SOF, fudge)) {
                /* looking for SOF */
                sof = 1;
                rx_in_frame = 1;
                rx_timer_active = 1;
            } else if (!rx_in_frame) {
                /* drop this crap until we have SOF */
            } else if (rx_last_level) {
                /* active timings */
                if (check_timing(duration, timings, INDEX_ACT_0, fudge)) {
                    this_bit = 0;
                    found = 1;
                } else if (check_timing(duration, timings, INDEX_ACT_1, fudge)) {
                    this_bit = 1;
                    found = 1;
                }
            } else {
                /* inactive timings */
                if (check_timing(duration, timings, INDEX_INACT_0, fudge)) {
                    this_bit = 0;
                    found = 1;
                } else if (check_timing(duration, timings, INDEX_INACT_1, fudge)) {
                    this_bit = 1;
                    found = 1;
                }
            }
        }
    }
    
    if (found) {
        rx_shift_reg >>= 1;
        rx_bit_count++;
        rx_shift_reg |= (this_bit << 7);
        
        if (rx_bit_count == 8) {
            rx_bit_count = 0;
            rx_buffer[rx_count++] = rx_shift_reg;
            rx_shift_reg = 0x00;
        }
        
        if (rx_count == MAX_RX_BUF) {
            ringbuf_write(rx_ringbuf, rx_buffer, rx_count);
            rx_count = 0;
            rx_in_frame = 0;
            i2c_rx_kick();
        }
    } 
    
    if (rx_in_frame && !found) {
        rx_protocol_error |= 1;
    }
    
    if (duration || !rx_timer_active) {
        rx_last_level = !(!PORTCbits.RC5);
        rx_timer_active = 1;
    }
}
