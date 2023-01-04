/* 
 * File:   transmitter.h
 * Author: gjhurlbu
 *
 * Created on January 2, 2023, 4:51 PM
 */

#ifndef TRANSMITTER_H
#define	TRANSMITTER_H

#ifdef	__cplusplus
extern "C" {
#endif

#include "ringbuf.h"
    
#define MAX_TX_BUF      12

    
    extern char tx_active;
    extern char tx_space_available;
    extern ringbuf_t *tx_ringbuf;
    
    void transmitter_init(void);
    void ccp2_isr(void);


#ifdef	__cplusplus
}
#endif

#endif	/* TRANSMITTER_H */

