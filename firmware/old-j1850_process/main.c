/* 
 * File:   main.c
 * Author: gjhurlbu
 *
 * Created on January 2, 2023, 2:40 PM
 */

#include <stdio.h>
#include <stdlib.h>

#define _XTAL_FREQ 32000000

#include <xc.h>
#include <proc/pic16f15225.h>

#include "main.h"
#include "receiver.h"
#include "transmitter.h"
#include "i2c_slave.h"
#include "interrupt.h"

void init(void);
void main_loop(void);
void global_isr(void);

const unsigned short int j1850_timings[TYPE_COUNT][INDEX_COUNT] = {
    {
        TIMING_PWM_SOF,
        TIMING_PWM_INACT_0,
        TIMING_PWM_INACT_1,
        TIMING_PWM_ACT_0,
        TIMING_PWM_ACT_1,
        TIMING_PWM_FUDGE
    },
    {
        {400, 364, 436, 326, 478},
        {560, 522, -1, 478, -1},
        {600, 560, 10000, 476, 32768},
        {600, 560, -1, 560, -1},
        {128, 98, 158, 68, 192},
        {256, 224, 290, 192, 326},
        {256, 224, 290, 192, 326},
        {128, 98, 158, 68, 192},
    }
};

static char sae_pwm_read = 0;
static char sae_pwm = 0;


/*
 * 
 */
int main(int argc, char** argv) {

    init();
    while(1) {
        CLRWDT();
        get_sae_pwm(1);
        main_loop();
    }
    return (EXIT_SUCCESS);
}

char get_sae_pwm(char force) {
    if (!sae_pwm_read || force) {
        sae_pwm = !(!PORTAbits.RA5);
    }
    return sae_pwm;
    
}


void init(void) {
    /* Disable all Interrupts */
    INTCON  = 0;     //Suspend interrupts
    PIE1    = 0x00;
    PIE2    = 0x00;
    
    /* Unlock PPS mapping */
    INTCONbits.GIE = 0; //Suspend interrupts
    PPSLOCK = 0x55; //Required sequence
    PPSLOCK = 0xAA; //Required sequence
    PPSLOCKbits.PPSLOCKED = 0; //Clear PPSLOCKED bit
    INTCONbits.GIE = 1; //Restore interrupts

    /* Setup GPIOs */

    /* RA0 - ICSPDAT
     * RA1 - ICPSCLK
     * RA2 - PIC_RX
     * RA3 - /MCLR
     * RA4 - /PIC_BL_ENTRY (input)
     * RA5 - SAE_PWM (input)
     */
    
    PORTA  = 0x00;
    LATC   = 0x00;
    ANSELA = 0x00;
    ODCONA = 0x10;
    TRISA  = 0x3F;
    
    /* RC0 - SCL
     * RC1 - SDA
     * RC2 - PIC_TX
     * RC3 - /J1850_INT (output)
     * RC4 - J1850_TX (output)
     * RC5 - J1850_RX (input)
     */

    PORTC  = 0x08;
    LATC   = 0x00;
    ANSELC = 0x00;
    ODCONC = 0x08;
    TRISC  = 0x27;
    
    /* PPS mapping for I2C */
    SSP1CLKPPS = 0x10;      /* RC0 */
    RC0PPS     = 0x07;      /* SCL */
    SSP1DATPPS = 0x11;      /* RC1 */
    RC1PPS     = 0x08;      /* SDA */
    
    /* PPS mapping for CCP1 */
    CCP1PPS    = 0x15;      /* RC5 */
    RC5PPS     = 0x01;      /* CCP1 */
    
    /* PPS mapping for CCP2 */
    CCP2PPS    = 0x14;      /* RC4 */
    RC4PPS     = 0x02;      /* CCP2 */
    
    /* PPS mapping for SAE_PWM */
    RA5PPS     = 0x00;      /* GPIO */
    
    /* PPS mapping for /PIC_BL_ENTRY */
    RA4PPS     = 0x00;      /* GPIO */
    
    /* Re-lock the PPS mapping */
    INTCONbits.GIE = 0; //Suspend interrupts
    PPSLOCK = 0x55; //Required sequence
    PPSLOCK = 0xAA; //Required sequence
    PPSLOCKbits.PPSLOCKED = 1; //Set PPSLOCKED bit
    INTCONbits.GIE = 1; //Restore interrupts

    /* Setup interrupt on the BR_ENABLE pin */
    INTCONbits.GIE = 0; //Suspend interrupts
    PIE0bits.IOCIE = 0;
    IOCANbits.IOCAN4 = 1;
    IOCAPbits.IOCAP4 = 0;
    IOCAFbits.IOCAF4 = 1;
    PIE0bits.IOCIE = 1;
    INTCONbits.PEIE = 1;
    INTCONbits.GIE = 1; //Restore interrupts
    
    /* Setup TMR1 to use a 500nS tick */
    T1CON   = 0x00;  /* disable first */
    CCP1CON = 0x00;  /* disable first */
    CCP2CON = 0x00;  /* disable first */
    T1GCON  = 0x00;  /* disable gating, this timer will free run */
    T1CLK   = 0x01;  /* clock the prescaler with Fosc/4 = 8MHz */
    T1CON   = 0x23;  /* prescale 4:1 (2MHz), enable 16 bit access, enable */

    /* Disable Timer1 overflow interrupt */
    INTCONbits.GIE  = 0;
    PIE1bits.TMR1IE = 0;
    INTCONbits.GIE  = 1;

    /* Initialize the receiver */
    receiver_init();
    
    /* Initialize the transmitter */
    transmitter_init();
    
    /* Initialize the I2C slave */
    i2c_slave_init();
    
    /* Initialize the IRQ output */
    interrupt_init();
}

void main_loop(void) {
    
}

void __interrupt() global_isr(void) {
    /* IOCAP4 interrupt */
    if (INTCONbits.PEIE && PIE0bits.IOCIE && IOCANbits.IOCAN4) {
        /* Reset */
        RESET();
    }
    
    /* CCP1 interrupt */
    if (INTCONbits.PEIE && PIE1bits.CCP1IE && PIR1bits.CCP1IF) {
        ccp1_isr();
    }
    
    interrupt_update(IRQ_RX_PROTOCOL_ERROR, rx_protocol_error);
    interrupt_update(IRQ_RX_BUFFERS_AVAILABLE, rx_buffers);

    /* CCP2 interrupt */
    if (INTCONbits.PEIE && PIE2bits.CCP2IE && PIR2bits.CCP2IF) {
        ccp2_isr();
    }
    
    interrupt_update(IRQ_TX_SPACE_AVAILABLE, tx_space_available);
    interrupt_update(IRQ_TX_ACTIVE, tx_active);

    /* SSP1 interrupt */
    if (INTCONbits.PEIE && PIE1bits.SSP1IE && PIR1bits.SSP1IF) {
        ssp1_isr();
    }
    
    interrupt_update(IRQ_I2C_ERROR, i2c_error);
}
