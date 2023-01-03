/* 
 * File:   ringbuf.h
 * Author: gjhurlbu
 *
 * Created on January 2, 2023, 7:59 PM
 */

#ifndef RINGBUF_H
#define	RINGBUF_H

#ifdef	__cplusplus
extern "C" {
#endif

#define MAX_RINGBUF_SIZE    128
    
#define CH_SOF      (unsigned char)0x7E
#define CH_ESC      (unsigned char)0x7D

    typedef struct {
        short int buf_size;
        unsigned char *buf;
        short int head;         /* read point */
        short int tail;         /* write point */
        char *notify;
    } ringbuf_t;
    
    ringbuf_t *ringbuf_create(short int buf_size, char *notify);
    int ringbuf_write(ringbuf_t *rb, unsigned char *buf, int count);
    int ringbuf_read(ringbuf_t *rb, unsigned char *buf, int count);
    short int ringbuf_avail(ringbuf_t *rb);
    short int ringbuf_free(ringbuf_t *rb);


#ifdef	__cplusplus
}
#endif

#endif	/* RINGBUF_H */

