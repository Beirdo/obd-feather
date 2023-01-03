/* 
 * File:   ringbuf.c
 * Author: gjhurlbu
 *
 * Created on January 2, 2023, 7:59 PM
 */

#include <stdlib.h>

#include <ringbuf.h>

ringbuf_t *ringbuf_create(short int buf_size, char *notify) {
    if (buf_size > MAX_RINGBUF_SIZE) {
        buf_size = MAX_RINGBUF_SIZE;
    }

    if (buf_size <= 0) {
        return NULL;
    }
    
    ringbuf_t *rb = (ringbuf_t *)malloc(sizeof(ringbuf_t));
    if (!rb) {
        return rb;
    }
    
    rb->buf = (unsigned char *)malloc(buf_size);
    if (!rb->buf) {
        free(rb);
        return NULL;
    }
    
    rb->buf_size = buf_size;
    rb->head = 0;
    rb->tail = 0;    
    rb->notify = notify;
    if (notify) {
        *notify = 0;
    }
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
    
    int tail = rb->tail;
    
    rb->buf[tail++] = CH_SOF;
    tail %= rb->buf_size;

    for (i = 0; i < count; i++) {
        unsigned char ch = buf[i];
        if (ch == CH_SOF || ch == CH_ESC) {
            rb->buf[tail++] = CH_ESC;
            tail %= rb->buf_size;
            rb->buf[tail++] = ch ^ 0x80;
        } else {
            rb->buf[tail++] = ch;
        }
        tail %= rb->buf_size;
    }
    
    rb->buf[tail++] = CH_SOF;
    tail %= rb->buf_size;

    rb->tail = tail;
    if (rb->notify) {
        *(rb->notify)++;
    }
    return count;
}

int ringbuf_read(ringbuf_t *rb, unsigned char *buf, int count) {
    if (!rb) {
        return 0;
    }
    
    int head = rb->head;
    int tail = rb->tail;
    int i;

    /* search for SOF */
    for (head = rb->head, tail = rb->tail; 
            head != tail && rb->buf[head] != CH_SOF;
            head = (head + 1) % rb->buf_size);
    
    if (head == tail) {
        rb->head = rb->tail;
        return 0;
    }
    
    /* Now eat all SOF */
    for (; head != tail && rb->buf[head] == CH_SOF;
            head = (head + 1) % rb->buf_size);

    if (head == tail) {
        rb->head = rb->tail;
        return 0;
    }
    
    /* Un-byte-stuff up to the next SOF, up to count output bytes */
    for (i = 0; i < count && rb->buf[head] != CH_SOF && head != tail; 
            head = (head + 1) % rb->buf_size, i++) {
        unsigned char ch = rb->buf[head];
        
        if (ch == CH_ESC) {
            head = (head + 1) % rb->buf_size;
            ch = rb->buf[head] ^ 0x80;
        }
        buf[i] = ch;
    }
    
    rb->head = head;
    char success = (head != tail && rb->buf[head] != CH_SOF);
    
    if (success && rb->notify) {
        *(rb->notify)--;
        if (*(rb->notify) < 0) {
            *(rb->notify) = 0;
        }
    }
    
    return success;
}

short int ringbuf_avail(ringbuf_t *rb) {
    if (!rb) {
        return 0;
    }
    
    return (rb->tail - rb->head + rb->buf_size) % rb->buf_size;
}

short int ringbuf_free(ringbuf_t *rb) {
    if (!rb) {
        return 0;
    }
    
    return (rb->head - rb->tail - 1 + rb->buf_size) % rb->buf_size; 
}
