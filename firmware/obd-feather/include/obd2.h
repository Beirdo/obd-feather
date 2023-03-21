#ifndef __OBD2_H_
#define __OBD2_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr.h>
#include "modes.h"

#ifdef __cplusplus
}

#define OBD2_TX_THREAD_STACK_SIZE 256
#define OBD2_TX_THREAD_PRIORITY 2
#define OBD2_RX_THREAD_STACK_SIZE 256
#define OBD2_RX_THREAD_PRIORITY 2

typedef struct {
    operation_mode_t mode;
    uint32_t id;
    uint8_t count;
    uint8_t service;
    uint8_t pid;
    uint8_t a;
    uint8_t b;
    uint8_t c;
    uint8_t d;
    uint8_t unused;
} obd_packet_t;

class OBDPort {
    public:
        OBDPort() : _mode(MODE_IDLE) {};
        virtual void begin(void) = 0;
        virtual void setMode(operation_mode_t mode) = 0;
        virtual bool send(obd_packet_t *packet) = 0;
    protected:
        operation_mode_t _mode;
};

void obd2_rx_thread(void *arg1, void *arg2, void *arg3);
void obd2_tx_thread(void *arg1, void *arg2, void *arg3);

class OBD2 {
    public:
        OBD2() : _port(0), _mode(MODE_IDLE) { k_mutex_init(&_mutex); };
        void begin(void);
        void setMode(operation_mode_t mode);
        operation_mode_t getMode(void);
        bool send(obd_packet_t *packet); 
        bool receive(obd_packet_t *packet);
        operation_mode_t scan(int delay_ms);

        friend void obd2_rx_thread(void *arg1, void *arg2, void *arg3);
        friend void obd2_tx_thread(void *arg1, void *arg2, void *arg3);

    protected:
        OBDPort *_port;
        operation_mode_t _mode;
        struct k_mutex _mutex;

        struct k_thread _rx_thread_data;
        struct k_thread _tx_thread_data;

    	k_tid_t _rx_tid;
    	k_tid_t _tx_tid;

        void enable(operation_mode_t mode);
        void disable(void);
        void tx_thread(void);
        void rx_thread(void);
};

extern OBD2 obd2;

extern "C" {
#endif

void obd2_init(void);
void set_operation_mode(operation_mode_t mode);
operation_mode_t get_operation_mode(void);
operation_mode_t scan_operation_modes(int delay_ms);

#ifdef __cplusplus
}
#endif

#endif