#ifndef __MODES_H_
#define __MODES_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <kernel.h>

typedef enum {
    MODE_IDLE = 0,
    MODE_HS_CAN,
    MODE_MS_CAN,
    MODE_SW_CAN,
    MODE_J1850_PWM,
    MODE_J1850_VPW,
    MODE_ISO9141_5BAUD_INIT,
    MODE_ISO14230_5BAUD_INIT,
    MODE_ISO14230_FAST_INIT,
    MAX_MODE,
} operation_mode_t;

#define MODE_IS_CAN(x)      ((x) == MODE_HS_CAN || (x) == MODE_MS_CAN || (x) == MODE_SW_CAN)
#define MODE_IS_KLINE(x)    ((x) == MODE_ISO9141_5BAUD_INIT || (x) == MODE_ISO14230_5BAUD_INIT || (x) == MODE_ISO14230_FAST_INIT)
#define MODE_IS_ISO9141(x)  ((x) == MODE_ISO9141_5BAUD_INIT)

#define MODE_IS_J1850(x)    ((x) == MODE_J1850_PWM || (x) == MODE_J1850_VPW)

extern struct k_mutex mode_mutex;

void set_operation_mode(operation_mode_t mode);
operation_mode_t get_operation_mode(void);
operation_mode_t scan_operation_modes(int ms_delay);

#ifdef __cplusplus
}
#endif

#endif