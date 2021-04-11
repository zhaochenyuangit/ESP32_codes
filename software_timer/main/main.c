#include <stdio.h>
#include "stdlib.h"
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"

#include "driver/gpio.h"
#include "esp32/rom/uart.h"
#include "sdkconfig.h"

#define LED 4
static TaskHandle_t task1 = NULL;
static xTimerHandle oneshot = NULL;

void mycallback(TimerHandle_t xTimer)
{
    if (pvTimerGetTimerID(xTimer) == 0)
    {
        printf("this is me\n");
        gpio_set_level(LED, 0);
    }
    else
    {
        printf("this is not me!\n");
    }
}

void read_serial(void *parameter)
{
    uint8_t ch;
    UBaseType_t uxHighWaterMark;
    while (1)
    {
        STATUS s = uart_rx_one_char(&ch);
        if (s == OK)
        {
            printf("%c",ch);
            fflush(stdout);
            gpio_set_level(LED, 1);
            xTimerStart(oneshot, portMAX_DELAY);
        }
        uxHighWaterMark = uxTaskGetStackHighWaterMark(NULL);
        printf("water mark is %d\n",uxHighWaterMark);
        vTaskDelay(100/portTICK_PERIOD_MS);
    }
}
void app_main(void)
{
    gpio_pad_select_gpio(LED);
    gpio_set_direction(LED, GPIO_MODE_OUTPUT);
    gpio_set_level(LED, 0);
    // create timer
    oneshot = xTimerCreate("one_shot",
                           4000 / portTICK_PERIOD_MS,
                           pdFALSE,
                           0,
                           mycallback);

    if (oneshot == NULL)
    {
        printf("create timer failed\n");
    }
    // create task
    xTaskCreatePinnedToCore(read_serial, "read_serial",
                             1600, NULL, 1, task1, 0);
}