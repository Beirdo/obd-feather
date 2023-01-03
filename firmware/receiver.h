/* 
 * File:   receiver.h
 * Author: gjhurlbu
 *
 * Created on January 2, 2023, 4:05 PM
 */

#ifndef RECEIVER_H
#define	RECEIVER_H

#ifdef	__cplusplus
extern "C" {
#endif

#include "ringbuf.h"
    
#define MAX_RX_BUF      12

    extern char rx_protocol_error;
    extern char rx_buffers;
    extern ringbuf_t *rx_ringbuf;
    
    void receiver_init(void);
    void ccp1_isr(void);


#ifdef	__cplusplus
}
#endif

#endif	/* RECEIVER_H */

