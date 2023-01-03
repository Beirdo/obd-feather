/* 
 * File:   interrupt.h
 * Author: gjhurlbu
 *
 * Created on January 2, 2023, 9:31 PM
 */

#ifndef INTERRUPT_H
#define	INTERRUPT_H

#ifdef	__cplusplus
extern "C" {
#endif

    enum {
        IRQ_RX_PROTOCOL_ERROR,
        IRQ_RX_BUFFERS_AVAILABLE,
        IRQ_TX_ACTIVE,
        IRQ_TX_SPACE_AVAILABLE,
        IRQ_I2C_ERROR,
    };
    
    extern unsigned char irq_enable;
    extern unsigned char irq_flags;
    extern unsigned char irq_clear_on_write;

    void interrupt_init(void);
    void interrupt_assert(void);
    void interrupt_update(char index, char value);
    char interrupt_active(char index);


#ifdef	__cplusplus
}
#endif

#endif	/* INTERRUPT_H */

