/* 
 * File:   i2c_slave.c
 * Author: gjhurlbu
 *
 * Created on January 2, 2023, 4:53 PM
 */

#include <xc.h>
#include <proc/pic16f15225.h>

#include "i2c_slave.h"
#include "interrupt.h"
#include "ringbuf.h"
#include "receiver.h"
#include "transmitter.h"

char i2c_error;
char i2c_state;
unsigned char i2c_reg_addr;
char i2c_read;

unsigned char i2c_tx_buf[MAX_TX_BUF];
char i2c_tx_count;
char i2c_tx_tail;

unsigned char i2c_rx_buf[MAX_RX_BUF];
char i2c_rx_count;
char i2c_rx_head;

const unsigned char *registers[I2C_REG_COUNT];

void i2c_slave_init(void) {
    i2c_state = I2C_STATE_IDLE;
    i2c_rx_count = 0;
    i2c_rx_head = 0;
    i2c_tx_count = 0;
    i2c_tx_tail = 0;
    i2c_error = 0;
    
    /* Initialize the register map */
    registers[I2C_REG_INT_ENABLES] = &irq_enable;
    registers[I2C_REG_INT_FLAGS]   = &irq_flags;
    registers[I2C_REG_DATA_FIFO]   = NULL;
    
    /* Setup the I2C Subsystem */
    SSP1ADD = I2C_SLAVE_ADDRESS;
    SSP1MSK = 0xFE;
    SSP1CON2 = 0x01;    /* Enable clock stretching */
    SSP1CON3 = 0x04;    /* Enable collision detection */
    SSP1CON1 = 0x3E;    /* Set to I2C Slave, enable */
    
    /* Enable I2C interrupt */
    INTCONbits.GIE = 0;
    INTCONbits.PEIE = 1;
    PIE1bits.SSP1IE = 1;
    INTCONbits.GIE = 1;
}    

void ssp1_isr(void) {
    unsigned char ch;
    char abort = 0;
    unsigned char *reg;
    static unsigned char dummy;
    
    dummy = 0xFF;
    PIR1bits.SSP1IF = 1;
    
    if (SSP1CON1bits.WCOL) {
        SSP1CON1bits.WCOL = 1;
        i2c_state = I2C_STATE_IDLE;
        i2c_error = 1;
        abort = 1;
    }
    
    if (SSP1CON1bits.SSPOV) {
        SSP1CON1bits.SSPOV = 1;
        i2c_state = I2C_STATE_IDLE;
        i2c_error = 1;
        abort = 1;
    }
    
    if (abort) {
        return;
    }
    
    switch (i2c_state) {
        case I2C_STATE_IDLE:
            ch = SSP1BUF;
            if (ch & 0xFE != I2C_SLAVE_ADDRESS) {
                return;
            }
            i2c_read = ch & 0x01;
            i2c_state = I2C_STATE_ADDR;
            break;
            
        case I2C_STATE_ADDR:
            ch = SSP1BUF;
            i2c_reg_addr = ch;
            i2c_state = I2C_STATE_REG_ADDR;
            break;
            
        case I2C_STATE_REG_ADDR:
        case I2C_STATE_DATA:
            if (i2c_reg_addr >= I2C_REG_COUNT) {
                reg = &dummy;
            } else {
                reg = &registers[i2c_reg_addr];
            }
     
            if (i2c_read) {
                if (i2c_reg_addr == I2C_REG_DATA_FIFO) {
                    if (i2c_rx_head == i2c_rx_count && i2c_state == I2C_STATE_REG_ADDR) {
                        i2c_rx_kick();
                    }
                    
                    if (i2c_rx_head == i2c_rx_count) {
                        reg = &dummy;
                    }

                    reg = &i2c_rx_buf[i2c_rx_head++];
                }

                if (!reg) {
                    reg = &dummy;
                }
                
                SSP1BUF = *reg;
            } else {
                if (i2c_reg_addr == I2C_REG_DATA_FIFO) {
                    reg = &i2c_tx_buf[i2c_tx_tail++];
                }
                
                ch = SSP1BUF;
                
                if (i2c_reg_addr == I2C_REG_INT_FLAGS) {
                    ch &= irq_clear_on_write;
                    ch = (*reg & ~ch);
                }
                
                if (!reg) {
                    reg = &dummy;
                }
                
                *reg = ch;
                
                if (i2c_tx_tail == MAX_TX_BUF) {
                    ringbuf_write(tx_ringbuf, i2c_tx_buf, MAX_TX_BUF);
                    i2c_tx_tail = 0;
                }
            }
            
            if (i2c_reg_addr == I2C_REG_DATA_FIFO) {
                i2c_state = I2C_STATE_DATA;
            } else {
                i2c_state = I2C_STATE_IDLE;
            }
            break;
            
        default:
            i2c_state = I2C_STATE_IDLE;
            break;
    }
}


void i2c_rx_kick(void) {
    if (i2c_rx_head != i2c_rx_count) {
        return;
    }
    
    if (i2c_state == I2C_STATE_DATA) {
        return;
    }
    
    i2c_rx_count = (char)ringbuf_read(rx_ringbuf, i2c_rx_buf, MAX_RX_BUF);
    i2c_rx_head = 0;
}