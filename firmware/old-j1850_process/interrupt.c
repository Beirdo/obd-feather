/* 
 * File:   interrupt.c
 * Author: gjhurlbu
 *
 * Created on January 2, 2023, 9:31 PM
 */

#include <xc.h>
#include <proc/pic16f15225.h>

#include "interrupt.h"


unsigned char irq_enable;
unsigned char irq_flags;
unsigned char irq_clear_on_write;

void interrupt_init(void) {
    irq_enable = 0x00;
    irq_flags  = 0x00;
    irq_clear_on_write = (1 << IRQ_RX_PROTOCOL_ERROR) | (1 << IRQ_I2C_ERROR);
    interrupt_assert();
}

void interrupt_update(char index, char value) {
    unsigned char bitmask = (unsigned char)(1 << index);
    
    if (value) {
        irq_flags |= bitmask;
    } else {
        irq_flags &= ~bitmask;
    }
    interrupt_assert();
}

void interrupt_assert(void) {
    unsigned char irq_masked = irq_enable & irq_flags;
    
    if (irq_masked) {
        PORTCbits.RC3 = 0;
    } else {
        PORTCbits.RC3 = 1;
    }
}

char interrupt_active(char index) {
    unsigned char bitmask = (unsigned char)(1 << index);
    unsigned char irq_masked = irq_enable & irq_flags;
    
    return !(!(irq_masked & bitmask));
}