/* 
 * File:   transmitter.c
 * Author: gjhurlbu
 *
 * Created on January 2, 2023, 4:51 PM
 */

#include <xc.h>
#include <proc/pic16f15225.h>

#include "main.h"
#include "transmitter.h"
#include "ringbuf.h"


#define TX_INTERFRAME   -10     /* 5us */

char tx_count;
char tx_read_byte;
unsigned char tx_buffer[MAX_TX_BUF];
char tx_bit_count;
unsigned char tx_shift_reg;
char tx_active;

char tx_space_available;

ringbuf_t *tx_ringbuf;

void transmitter_update_flags(void);
unsigned short int transmitter_next_trigger(const unsigned short int *timings,
        int index);

void transmitter_init(void) {
    tx_active = 0;
    tx_ringbuf = ringbuf_create(1, NULL);
    
    transmitter_update_flags();

    /* Compare CCP2 to create output signal */
    CCP2CON = 0x00;     /* disable until transmission starts */

    /* Setup CCP2 interrupt */
    INTCONbits.GIE = 0;
    INTCONbits.PEIE = 1;
    PIE2bits.CCP2IE = 1;
    INTCONbits.GIE = 1;
}

void transmitter_update_flags(void) {
    tx_space_available = (ringbuf_avail(tx_ringbuf) >= MAX_TX_BUF);
};

unsigned short int transmitter_next_trigger(const unsigned short int *timings,
        int index) {
    unsigned short int now = (unsigned short int)(TMR1L | (TMR1H << 8));

    if (index < 0) {
        return (unsigned short int)( (unsigned int)((int)now - index) & 0xFFFF);
    }
    return (unsigned short int)((now + timings[index]) & 0xFFFF);
}

void transmitter_kick(void) {
    if (tx_active) {
        return;
    }

    CCP2CON = 0x00;     /* To switch modes, must be disabled first */

    char sae_pwm = get_sae_pwm(1);
    const unsigned short int *timings = j1850_timings[sae_pwm];
    
    /* The transmitter is being kicked with a new buffer to send */
    tx_count = (char)ringbuf_read(tx_ringbuf, tx_buffer, MAX_TX_BUF);
    tx_read_byte = 0;
    transmitter_update_flags();
    
    if (!tx_count) {
        return;
    }
   
    tx_active = 1;
    
    CCPR2 = (unsigned short int)transmitter_next_trigger(timings, TX_INTERFRAME);
    CCP2CON = 0x89;     /* Trigger low for pre-SOF */    
}

void ccp2_isr(void) {
    unsigned short int next_trigger;
    char this_bit = 0;
    char current_level;
    char sae_pwm = get_sae_pwm(0);
    const unsigned short int *timings = j1850_timings[sae_pwm];
    char index;
    
    PIR2bits.CCP2IF = 1;
    
    index = tx_active - 1;
    
    if (!index) {
        next_trigger = transmitter_next_trigger(timings, INDEX_SOF);
    } else {
        current_level = !(!PORTCbits.RC4);
        if (tx_bit_count == 0) {
            if (tx_read_byte == tx_count) {
                tx_active = 0;
                transmitter_kick();
                return;
            }
            tx_shift_reg = tx_buffer[tx_read_byte++];
            tx_bit_count = 8;
        }
        this_bit = tx_shift_reg & 0x01;
        tx_shift_reg >>= 1;
        tx_bit_count--;
        
        index = INDEX_INACT_0 + 2 * current_level + this_bit;
        next_trigger = transmitter_next_trigger(timings, index);
    }
    
    CCP2CON = 0x00;     /* must disable to change mode */
    CCPR2 = next_trigger;
    CCP2CON = 0x88 | current_level; /* drive the opposite way after pulse */
        
    tx_active++;
}

