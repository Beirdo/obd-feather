#ifndef __CANBUS_H_
#define __CANBUS_H_

#ifdef __cplusplus

#include <zephyr.h>
#include <kernel.h>
#include <device.h>
#include <drivers/can.h>

#include "modes.h"
#include "obd2.h"

#define CAN_TX_THREAD_STACK_SIZE 512
#define CAN_TX_THREAD_PRIORITY 2
#define CAN_RX_THREAD_STACK_SIZE 512
#define CAN_RX_THREAD_PRIORITY 2
#define CAN_STATE_POLL_THREAD_STACK_SIZE 512
#define CAN_STATE_POLL_THREAD_PRIORITY 2
#define CAN_SLEEP_TIME K_MSEC(250)

void canbus_state_change_work_handler(struct k_work *work);
void canbus_state_change_isr(enum can_state state, struct can_bus_err_cnt err_cnt);
void canbus_rx_thread(void *arg1, void *arg2, void *arg3);
void canbus_tx_thread(void *arg1, void *arg2, void *arg3);
void canbus_poll_state_thread(void *arg1, void *arg2, void *arg3);

class CANBusPort : public OBDPort {
    public:
        CANBusPort() : OBDPort(), _filter_id(-1) {};
        void begin(void);
        void setMode(operation_mode_t mode);
        bool send(obd_packet_t *packet);

        friend void canbus_state_change_work_handler(struct k_work *work);
        friend void canbus_state_change_isr(enum can_state state, struct can_bus_err_cnt err_cnt);
        friend void canbus_rx_thread(void *arg1, void *arg2, void *arg3);
        friend void canbus_tx_thread(void *arg1, void *arg2, void *arg3);
        friend void canbus_poll_state_thread(void *arg1, void *arg2, void *arg3);
        friend void canbus_state_change_work_handler(struct k_work *work);
        friend void canbus_state_change_isr(enum can_state state, struct can_bus_err_cnt err_cnt);

    protected:
        const struct device *_dev;
 
        struct k_thread _rx_thread_data;
        struct k_thread _tx_thread_data;
        struct k_thread _poll_state_thread_data;
        struct k_work _state_change_work;
        enum can_state _current_state;
        struct can_bus_err_cnt _current_err_cnt;

        int _filter_id;

    	k_tid_t _rx_tid;
    	k_tid_t _tx_tid;
        k_tid_t _poll_state_tid;

        void rx_thread(void);
        void tx_thread(void);
        void poll_state_thread(void);
        void state_change_work_handler(struct k_work *work);
        void state_change_isr(enum can_state state, struct can_bus_err_cnt err_cnt);

        static const char *state_to_str(enum can_state state);
};

extern CANBusPort canbus;

extern "C" {
#endif

void canbus_init(void);

#ifdef __cplusplus
}
#endif

#endif