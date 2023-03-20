#ifndef __KLINE_H_
#define __KLINE_H_

#ifdef __cplusplus

#include <zephyr.h>
#include <kernel.h>
#include <device.h>
#include <drivers/uart.h>

#include "modes.h"
#include "obd2.h"

#define KLINE_TX_THREAD_STACK_SIZE 256
#define KLINE_TX_THREAD_PRIORITY 2
#define KLINE_RX_THREAD_STACK_SIZE 256
#define KLINE_RX_THREAD_PRIORITY 2

#define KLINE_BUFFER_SIZE 16
#define KLINE_BUFFER_COUNT 4

void kline_rx_thread(void *arg1, void *arg2, void *arg3);
void kline_tx_thread(void *arg1, void *arg2, void *arg3);
void kline_uart_callback(const struct device *dev, struct uart_event *evt, void *user_data);

class KLinePort : public OBDPort {
    public:
        KLinePort() : OBDPort(), _initialized(false) {};
        void begin(void);
        void setMode(operation_mode_t mode);
        bool send(obd_packet_t *packet);

        friend void kline_rx_thread(void *arg1, void *arg2, void *arg3);
        friend void kline_tx_thread(void *arg1, void *arg2, void *arg3);
        friend void kline_uart_callback(const struct device *dev, struct uart_event *evt, void *user_data);

    protected:
        const struct device *_dev;
 
        struct k_thread _rx_thread_data;
        struct k_thread _tx_thread_data;

    	k_tid_t _rx_tid;
    	k_tid_t _tx_tid;

        bool _initialized;
        struct k_sem _tx_done_sem;
        int _tx_sent;

        uint8_t _rx_buffers[KLINE_BUFFER_COUNT][KLINE_BUFFER_SIZE];
        int _rx_index;
        struct k_sem _rx_rdy_sem;

        void rx_thread(void);
        void tx_thread(void);

        void tx_done_callback(int sent);
        void rx_ready_callback(uint8_t *buf, uint8_t offset, uint8_t len);
		void rx_buf_request_callback(void);
		void rx_buf_released_callback(uint8_t *buf);
		void rx_disabled_callback();
		void rx_stopped_callback(uint8_t *buf, uint8_t offset, uint8_t len, enum uart_rx_stop_reason reason);

        bool configure(uint32_t baud);
        void init(void);
        void init_5baud(void);
        void init_fast(void);

        void enable(void);
        void disable(void);
        uint8_t checksum(uint8_t *buffer, uint8_t len);
};

extern KLinePort kline;

extern "C" {
#endif

void kline_init(void);

#ifdef __cplusplus
}
#endif

#endif