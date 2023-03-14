#ifndef __OBD2_H_
#define __OBD2_H_

#ifdef __cplusplus

#include <zephyr.h>
#include "modes.h"

class OBDPort {
    public:
        OBDPort() : _mode(MODE_IDLE) {};
        virtual void begin(void) = 0;
        virtual void setMode(operation_mode_t mode) = 0;
        virtual bool send(uint32_t id, uint16_t pid, uint8_t *data, uint8_t len) = 0; 
        virtual bool receive(uint32_t *id, uint16_t *pid, uint8_t *data, uint8_t *len) = 0;
    protected:
        operation_mode_t _mode;
};

class OBD2 {
    public:
        OBD2() : _port(0), _mode(MODE_IDLE) { k_mutex_init(&_mutex); };
        void begin(void);
        void setMode(operation_mode_t mode);
        operation_mode_t getMode(void);
        bool send(uint32_t id, uint16_t pid, uint8_t *data, uint8_t len); 
        bool receive(uint32_t *id, uint16_t *pid, uint8_t *data, uint8_t *len);
        operation_mode_t scan(int delay_ms);
    protected:
        OBDPort *_port;
        operation_mode_t _mode;
        struct k_mutex _mutex;

        void enable(operation_mode_t mode);
        void disable(void);
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