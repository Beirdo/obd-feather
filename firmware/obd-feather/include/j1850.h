#ifndef __J1850_H_
#define __J1850_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr.h>
#include <kernel.h>
#include <drivers/gpio.h>

void j1850_init(void);
void j1850_rx_callback(const struct device *port, struct gpio_callback *cb, gpio_port_pins_t pins);

#ifdef __cplusplus
}

#include "modes.h"
#include "obd2.h"

#define J1850_TX_THREAD_STACK_SIZE 256
#define J1850_TX_THREAD_PRIORITY 2
#define J1850_RX_THREAD_STACK_SIZE 256
#define J1850_RX_THREAD_PRIORITY 2

#define J1850_BUFFER_SIZE 12
#define J1850_BUFFER_COUNT 4

void j1850_rx_thread(void *arg1, void *arg2, void *arg3);
void j1850_rx_bit_thread(void *arg1, void *arg2, void *arg3);
void j1850_tx_thread(void *arg1, void *arg2, void *arg3);

typedef struct {
  int16_t nom;
  int16_t tx_min;
  int16_t tx_max;
  int16_t rx_min;
  int16_t rx_max;
} j1850_timing_t;

typedef enum {
  INDEX_SOF_0,
  INDEX_SOF_1,
  INDEX_EOD,
  INDEX_EOF,
  INDEX_BRK_0,
  INDEX_BRK_1,
  INDEX_IFS,
  INDEX_INACT_0,
  INDEX_INACT_1,
  INDEX_ACT_0,
  INDEX_ACT_1,
  INDEX_ABORT,
  INDEX_COUNT,
} j1850_timing_index_t;

class J1850Port : public OBDPort {
    public:
        J1850Port() : OBDPort(), _initialized(false), _transmitting(false) {};
        void begin(void);
        void setMode(operation_mode_t mode);
        bool send(obd_packet_t *packet);

        friend void j1850_rx_thread(void *arg1, void *arg2, void *arg3);
        friend void j1850_rx_bit_thread(void *arg1, void *arg2, void *arg3);
        friend void j1850_tx_thread(void *arg1, void *arg2, void *arg3);
        friend void j1850_rx_callback(const struct device *port, struct gpio_callback *cb, gpio_port_pins_t pins);

    protected:
        const struct device *_dev;
 
        struct k_thread _rx_thread_data;
        struct k_thread _tx_thread_data;

        struct k_work _rx_bit_work;
        struct k_work _rx_byte_work;

    	k_tid_t _rx_tid;
    	k_tid_t _tx_tid;

        struct k_sem _tx_done_sem;

        bool _initialized;
        bool _transmitting;
        bool _receiving;
        bool _abort;

        uint8_t _rx_shift_reg;
        uint8_t _rx_bit_count;
        uint8_t _rx_buffer[J1850_BUFFER_SIZE];
        uint8_t _rx_buffer_index;
        
        uint32_t _last_edge;
        bool _last_level;

        uint8_t _tx_shift_reg;
        uint8_t _tx_bit_count;
        uint8_t _tx_buffer[J1850_BUFFER_SIZE];
        uint8_t _tx_buffer_len;
        uint8_t _tx_buffer_index;

        j1850_timing_index_t _timing_index;

        static constexpr j1850_timing_t _vpw_timing[12] = {
            {-1, -1, -1, -1, -1},         // SOF_0
            {200, 182, 218, 163, 239},    // SOF_1
            {200, 182, 218, 163, 239},    // EOD
            {280, 261, -1, 239, -1},      // EOF
            {-1, -1, -1, -1, -1},         // BRK_0
            {300, 280, 5000, 238, 32767}, // BRK_1 (actually <= 1.0s for rx_max)
            {300, 280, -1, 280, -1},      // IFS
            {64, 49, 79, 34, 96},         // INACT_0
            {128, 112, 145, 96, 163},     // INACT_1
            {128, 112, 145, 96, 163},     // ACT_0
            {64, 49, 79, 34, 96},         // ACT_1
            {-1, -1, -1, -1, -1},         // ABORT
        };

        static constexpr j1850_timing_t _pwm_timing[12] = {
            {17, 18, 19, 11, 20},     // SOF_0
            {31, 29, 32, 27, 34},     // SOF_1
            {48, 47, 49, 42, 54},     // EOD (tp4)
            {72, 70, -1, 63, -1},     // EOF
            {9, 8, 10, 3, 21},        // BRK_0
            {39, 37, 41, 35, 43},     // BRK_1
            {96, 93, -1, 84, -1},     // IFS
            {8, 6, 8, 4, 10},         // INACT_0
            {16, 14, 16, 12, 18},     // INACT_1
            {16, 14, 16, 12, 18},     // ACT_0
            {8, 6, 8, 4, 10},         // ACT_1
            {-1, -1, -1, -1, -1},     // ABORT
        };

        void rx_thread(void);
        void rx_bit_thread(void);
        void tx_thread(void);

        void rx_callback(void);

        uint8_t checksum(uint8_t *buffer, uint8_t len);

        int write(uint8_t *buffer, uint8_t len);
        void kickTX(void);
        bool isTransmitting(void) { return _transmitting; };
        bool isIdle(void) { return !_transmitting && !_receiving; };

        j1850_timing_index_t get_next_tx_index(bool new_packet = false);

        void processPWM(uint32_t duration, bool level);
        void processVPW(uint32_t duration, bool level);
        static bool isInTiming(const j1850_timing_t *timings, uint8_t index, uint32_t duration);
};

extern J1850Port j1850;

#endif

#endif