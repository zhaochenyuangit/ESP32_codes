#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp32/rom/uart.h"
#include "sdkconfig.h"
#include "stdlib.h"
#include <string.h>

static TaskHandle_t task1 = NULL;
static TaskHandle_t task2 = NULL;
static const uint8_t str_len = 20;
static char *str_ptr;
static bool ready_flag = 0;

void read_serial(void *pvParameter)
{
    uint8_t ch;
    uint8_t buf[str_len];
    uint8_t idx = 0;
    while (1)
    {
        STATUS s = uart_rx_one_char(&ch);
        if (s == OK)
        {
            //printf("%d,%c\n", ch, ch);
            if (idx < str_len)
            {
                buf[idx] = ch;
                idx++;
            }
            else
            {
                printf("index exceeds max\n");
            }

            if (ch == '\r')
            {
                buf[idx - 1] = '\0';
                if (!ready_flag)
                {
                    ready_flag = 1;
                    printf("allocate memory of %d bytes, and copy message\n", (idx) * sizeof(uint8_t));
                    str_ptr = (char *)malloc((idx) * sizeof(char));
                    memcpy(str_ptr, buf, idx);
                    // reset
                    idx = 0;
                    memset(buf, 0, str_len);
                }
            }
        }
        //printf(uxTaskGetStackHighWaterMark(task1));
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

void print_msg(void *parameter)
{
    while (1)
    {
        if (ready_flag)
        {
            ready_flag = 0;
            printf("the string is %s\n",str_ptr);
            free(str_ptr);
            str_ptr = NULL;
        }
        vTaskDelay(200 / portTICK_PERIOD_MS);
    }
}

void app_main(void)
{
    xTaskCreatePinnedToCore(read_serial, "readAlloc", 4000, NULL, 1, task1, 0);
    xTaskCreatePinnedToCore(print_msg, "printMsg", 2048, NULL, 0, task2, 0);
}