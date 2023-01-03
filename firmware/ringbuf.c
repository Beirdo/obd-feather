/* 
 * File:   ringbuf.c
 * Author: gjhurlbu
 *
 * Created on January 2, 2023, 7:59 PM
 */

#include <stdlib.h>
#include "ringbuf.h"

ringbuf_t ringbuffers[2];

ringbuf_t *ringbuf_create(char index, char *notify) {
    if (index > 2) {
        return NULL;
    }
    
    ringbuf_t *rb = &ringbuffers[2];
    rb->head = 0;
    rb->tail = 0;    
    rb->notify = notify;
    if (notify) {
        *notify = 0;
    }
    
    return rb;
}

int ringbuf_write(ringbuf_t *rb, unsigned char *buf, int count) {
    if (!rb) {
        return 0;
    }
    
    int i;
    int stuffed_size = 2;
    
    for (i = 0; i < count; i++) {
        unsigned char ch = buf[i];
        
        stuffed_size++;
        if (ch == CH_SOF || ch == CH_ESC) {
            stuffed_size++;
        }
    }
    
    if (stuffed_size > ringbuf_free(rb)) {
        return 0;
    }
    
    unsigned short int tail = rb->tail;
    
    rb->buf[tail++] = CH_SOF;
    tail %= RINGBUF_SIZE;

    for (i = 0; i < count; i++) {
        unsigned char ch = buf[i];
        if (ch == CH_SOF || ch == CH_ESC) {
            rb->buf[tail++] = CH_ESC;
            tail %= RINGBUF_SIZE;
            rb->buf[tail++] = ch ^ 0x80;
        } else {
            rb->buf[tail++] = ch;
        }
        tail %= RINGBUF_SIZE;
    }
    
    rb->buf[tail++] = CH_SOF;
    tail %= RINGBUF_SIZE;

    rb->tail = tail;
    if (rb->notify) {
        (*rb->notify)++;
    }
    return count;
}

int ringbuf_read(ringbuf_t *rb, unsigned char *buf, int count) {
    if (!rb) {
        return 0;
    }
    
    unsigned short int head = rb->head;
    unsigned short int tail = rb->tail;
    int i;

    /* search for SOF */
    for (head = rb->head, tail = rb->tail; 
            head != tail && rb->buf[head] != CH_SOF;
            head = (head + 1) % RINGBUF_SIZE);
    
    if (head == tail) {
        rb->head = rb->tail;
        return 0;
    }
    
    /* Now eat all SOF */
    for (; head != tail && rb->buf[head] == CH_SOF;
            head = (head + 1) % RINGBUF_SIZE);

    if (head == tail) {
        rb->head = rb->tail;
        return 0;
    }
    
    /* Un-byte-stuff up to the next SOF, up to count output bytes */
    for (i = 0; i < count && rb->buf[head] != CH_SOF && head != tail; 
            head = (head + 1) % RINGBUF_SIZE, i++) {
        unsigned char ch = rb->buf[head];
        
        if (ch == CH_ESC) {
            head = (head + 1) % RINGBUF_SIZE;
            ch = rb->buf[head] ^ 0x80;
        }
        buf[i] = ch;
    }
    
    rb->head = head;
    char success = (head != tail && rb->buf[head] != CH_SOF);
    
    if (success && rb->notify) {
        (*rb->notify)--;
    }
    
    return success;
}

short int ringbuf_avail(ringbuf_t *rb) {
    if (!rb) {
        return 0;
    }
    
    return (rb->tail - rb->head + RINGBUF_SIZE) % RINGBUF_SIZE;
}

short int ringbuf_free(ringbuf_t *rb) {
    if (!rb) {
        return 0;
    }
    
    return (rb->head - rb->tail - 1 + RINGBUF_SIZE) % RINGBUF_SIZE; 
}
