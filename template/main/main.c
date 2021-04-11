#include <stdio.h>
#include "stdlib.h"
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"
#include "esp32/rom/uart.h"
#include "sdkconfig.h"

static TaskHandle_t task1 = NULL;
static TaskHandle_t task2 = NULL;

void app_main(void)
{
    xTaskCreatePinnedToCore(read_serial, "readAlloc", 4000, NULL, 1, task1, 0);
    xTaskCreatePinnedToCore(print_msg, "printMsg", 2048, NULL, 0, task2, 0);
}