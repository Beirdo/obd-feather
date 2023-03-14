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
    MODE_K_LINE,
    MAX_MODE,
} operation_mode_t;

#define MODE_IS_CAN(x)  ((x) == MODE_HS_CAN || (x) == MODE_MS_CAN || (x) == MODE_SW_CAN)

extern struct k_mutex mode_mutex;

void set_operation_mode(operation_mode_t mode);
operation_mode_t get_operation_mode(void);
operation_mode_t scan_operation_modes(int ms_delay);

#ifdef __cplusplus
}
#endif

#endif