#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp32/rom/uart.h"
#include "sdkconfig.h"
#include "stdlib.h"
#include <string.h>

#define LED 19

static TaskHandle_t task1 = NULL;
static TaskHandle_t task2 = NULL;
volatile short freq = 1000;

void blink_led(void *parameter)
{
    while (1)
    {
        gpio_set_level(LED, 0);
        vTaskDelay(freq / portTICK_PERIOD_MS);
        printf("LED state");
        gpio_set_level(LED, 1);
        vTaskDelay(freq / portTICK_PERIOD_MS);
    }
}

void read_serial(void *parameter)
{
    uint8_t c;
    char c_buffer[20];
    uint8_t idx = 0;
    while (1)
    {
        STATUS s = uart_rx_one_char(&c);
        if (s == OK)
        {
            printf("%d,%c\n", c, c);
            if (c == '\r') //end of line symbol of ESP32 is \r
            {
                freq = atoi(c_buffer);
                printf("set LED blink freq to: %d\n", freq);
                idx = 0;
                memset(c_buffer, 0, 20);
            }
            else
            {
                c_buffer[idx] = c;
                idx++;
            }
        }
        vTaskDelay(200 / portTICK_PERIOD_MS);
    }
}

void app_main(void)
{
    gpio_pad_select_gpio(LED);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(LED, GPIO_MODE_OUTPUT);
    xTaskCreate(blink_led,
                "task1",
                1024,
                NULL,
                0,
                task1);
    xTaskCreate(read_serial,
                "task2",
                2048,
                NULL,
                1,
                task2);
}
