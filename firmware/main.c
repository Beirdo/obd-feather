/* 
 * File:   main.c
 * Author: gjhurlbu
 *
 * Created on January 2, 2023, 2:40 PM
 */

#include <stdio.h>
#include <stdlib.h>

/* PIC16F15225 Configuration Bit Settings
 *
 * 'C' source line config statements
 */

/* CONFIG1 */
#pragma config FEXTOSC = OFF    // External Oscillator Mode Selection bits (Oscillator not enabled)
#pragma config HFINTOSC_32MHZ// Power-up Default Value for COSC bits (HFINTOSC (32 MHz))
#pragma config CLKOUTEN = OFF   // Clock Out Enable bit (CLKOUT function is disabled; I/O function on RA4)
#pragma config VDDAR = HI       // VDD Range Analog Calibration Selection bit (Internal analog systems are calibrated for operation between VDD = 2.3V - 5.5V)

/* CONFIG2 */
#pragma config MCLRE = EXTMCLR  // Master Clear Enable bit (If LVP = 0, MCLR pin is MCLR; If LVP = 1, RA3 pin function is MCLR)
#pragma config PWRTS = PWRT_1   // Power-up Timer Selection bits (PWRT set at 1 ms)
#pragma config WDTE = ON        // WDT Operating Mode bits (WDT enabled regardless of Sleep; SEN bit is ignored)
#pragma config BOREN = ON       // Brown-out Reset Enable bits (Brown-out Reset Enabled, SBOREN bit is ignored)
#pragma config BORV = LO        // Brown-out Reset Voltage Selection bit (Brown-out Reset Voltage (VBOR) set to 1.9V)
#pragma config PPS1WAY = ON     // PPSLOCKED One-Way Set Enable bit (The PPSLOCKED bit can be set once after an unlocking sequence is executed; once PPSLOCKED is set, all future changes to PPS registers are prevented)
#pragma config STVREN = ON      // Stack Overflow/Underflow Reset Enable bit (Stack Overflow or Underflow will cause a reset)

/* CONFIG3 */

/* CONFIG4 */
#pragma config BBSIZE = BB512   // Boot Block Size Selection bits (512 words boot block size)
#pragma config BBEN = OFF       // Boot Block Enable bit (Boot Block is disabled)
#pragma config SAFEN = OFF      // SAF Enable bit (SAF is disabled)
#pragma config WRTAPP = OFF     // Application Block Write Protection bit (Application Block is not write-protected)
#pragma config WRTB = OFF       // Boot Block Write Protection bit (Boot Block is not write-protected)
#pragma config WRTC = OFF       // Configuration Registers Write Protection bit (Configuration Registers are not write-protected)
#pragma config WRTSAF = OFF     // Storage Area Flash (SAF) Write Protection bit (SAF is not write-protected)
#pragma config LVP = ON         // Low Voltage Programming Enable bit (Low Voltage programming enabled. MCLR/Vpp pin function is MCLR. MCLRE Configuration bit is ignored.)

/* CONFIG5 */
#pragma config CP = OFF         // User Program Flash Memory Code Protection bit (User Program Flash Memory code protection is disabled)

/* #pragma config statements should precede project file includes.
 * Use project enums instead of #define for ON and OFF.
 */

#define _XTAL_FREQ 32000000

#include <xc.h>
#include <proc/pic16f15225.h>

#include <main.h>
#include <receiver.h>
#include <transmitter.h>
#include <i2c_slave.h>

void init(void);
void main_loop(void);

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
        TIMING_VPW_SOF,
        TIMING_VPW_INACT_0,
        TIMING_VPW_INACT_1,
        TIMING_VPW_ACT_0,
        TIMING_VPW_ACT_1,
        TIMING_VPW_FUDGE
    }
};

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
    static sae_pwm_read = 0;
    static sae_pwm = 0;
    
    if (!sae_pwm_read || force) {
        sae_pwm = !(!PORTAbits.RA5);
    }
    return sae_pwm;
    
}


void init(void) {
    /* Unlock PPS mapping */
    INTCONbits.GIE = 0; //Suspend interrupts
    PPSLOCK = 0x55; //Required sequence
    PPSLOCK = 0xAA; //Required sequence
    PPSLOCKbits.PPSLOCKED = 0; //Clear PPSLOCKED bit
    INTCONbits.GIE = 1; //Restore interrupts

    /* Setup GPIOs */

    /* RA0 - ICSPDAT
     * RA1 - ICPSCLK
     * RA2 - 
     * RA3 - /MCLR
     * RA4 - 
     * RA5 - SAE_PWM (input)
     */
    
    PORTA  = 0x00;
    LATA   = 0x00;
    ANSELA = 0x00;
    TRISA  = 0x3F;
    
    /* RC0 - SCL
     * RC1 - SDA
     * RC2 -
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
    
    /* Re-lock the PPS mapping */
    INTCONbits.GIE = 0; //Suspend interrupts
    PPSLOCK = 0x55; //Required sequence
    PPSLOCK = 0xAA; //Required sequence
    PPSLOCKbits.PPSLOCKED = 1; //Set PPSLOCKED bit
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
    INTCONbits.PEIE = 0;
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

void interrupt global_isr(void) {
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
}