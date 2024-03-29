/*
 *  main.c - Application main entry point
 */
#include <zephyr.h>
#include <sys/printk.h>

#include "gpio_map.h"
#include "obd2.h"
#include "canbus.h"
#include "kline.h"
#include "j1850.h"
#include "display.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(main, 3);

#define STACKSIZE 256
#define PRIORITY K_IDLE_PRIO

/*---------------------------------------------------------------------------*/
/*                                                                           */
/*---------------------------------------------------------------------------*/
void main_thread(void * id, void * unused1, void * unused2)
{
  LOG_INF("%s", __func__);

  gpio_init();
  obd2_init();
  canbus_init();
  kline_init();
  j1850_init();
  display_init();

  while(1) {
    k_sleep(K_MSEC(100));
  }
}

K_THREAD_DEFINE(main_id, STACKSIZE, main_thread,
                NULL, NULL, NULL, PRIORITY, 0, 0);
