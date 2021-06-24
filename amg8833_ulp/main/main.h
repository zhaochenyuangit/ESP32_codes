#include <stdio.h>
#include "stdlib.h"
#include <string.h>
#include <stdint.h>
#include <stddef.h>

// used for speed evaluation
#include <sys/time.h>
#include "time.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "freertos/timers.h"

#include "sdkconfig.h"

#include "network_common.h"
#include "grideye_api_lv1.h"
#include "cv.h"
#include "sim_uart.h"
// ULP coprocessor
#include "nvs.h"
#include "nvs_flash.h"
#include "soc/rtc_cntl_reg.h"
#include "soc/rtc_io_reg.h"
#include "soc/sens_reg.h"
#include "soc/soc.h"
#include "driver/gpio.h"
#include "driver/rtc_io.h"
#include "esp32/ulp.h"

#include "esp_sleep.h"

//#define UART_SIM
#define IM_W 71
#define IM_H 71
#define IM_LEN (IM_W*IM_H)

