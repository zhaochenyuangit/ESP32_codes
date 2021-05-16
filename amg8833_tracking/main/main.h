#include <stdio.h>
#include "stdlib.h"
#include <string.h>
#include <stdint.h>
#include <stddef.h>

#include <sys/time.h>
#include "time.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "sdkconfig.h"

#include "network_common.h"
#include "grideye_api_lv1.h"
#include "cv.h"
#include "sim_uart.h"

#define UART_SIM
#define IM_W 71
#define IM_H 71
#define IM_LEN (IM_W*IM_H)