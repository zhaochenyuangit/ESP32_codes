#include <stdio.h>
#include "stdlib.h"
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "driver/gpio.h"
#include "esp32/rom/uart.h"
#include "sdkconfig.h"

#define LED 4
static TaskHandle_t task1 = NULL;
static TaskHandle_t task2 = NULL;
static xQueueHandle queue1;
static xQueueHandle queue2;

static const uint8_t queue_len = 3;
static const uint8_t str_len = 20;
static char prefix[] = "delay ";

void read_serial(void *pvParameter)
{
    uint8_t ch;
    char buf[str_len];
    uint8_t idx = 0;
    uint16_t msg_num;
    char msgfrom2[str_len];
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
                // do something
                if (strncmp(buf, prefix, strlen(prefix)) == 0)
                {
                    msg_num = atoi(buf + strlen(prefix));
                    if (xQueueSend(queue1, &msg_num, 10) != pdTRUE)
                    {
                        printf("Queue Full!\n");
                    }
                }
                printf("received string is: %s\n", buf);
                // reset
                idx = 0;
                memset(buf, 0, str_len);
            }
        }
        //read anything from queue2 and print
        if (xQueueReceive(queue2,&msgfrom2,0)==pdTRUE){
            printf("message from queue2 is: %s\n",msgfrom2);
        }
        //printf(uxTaskGetStackHighWaterMark(task1));
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

void blink(void *parameter)
{
    gpio_pad_select_gpio(LED);
    gpio_set_direction(LED, GPIO_MODE_OUTPUT);
    uint16_t delay = 1000;
    uint8_t counter = 0;
    while (1)
    {
        if (xQueueReceive(queue1, &delay, 0) == pdTRUE)
        {
            printf("LED Received, set delay to: %d ms\n", delay);
        }
        gpio_set_level(LED, 1);
        vTaskDelay(delay / portTICK_PERIOD_MS);
        gpio_set_level(LED, 0);
        vTaskDelay(delay / portTICK_PERIOD_MS);
        counter++;
        if (counter == 2)
        {
            counter = 0;
            char msg[] = "Blinked";
            if (xQueueSend(queue2, &msg, 10) != pdTRUE)
            {
                printf("Queue2 is Full!\n");
            }
        }
    }
}

void app_main(void)
{
    queue1 = xQueueCreate(queue_len, sizeof(uint16_t));
    queue2 = xQueueCreate(queue_len, sizeof(char[str_len]));
    xTaskCreatePinnedToCore(read_serial, "readAlloc", 4000, NULL, 1, task1, 0);
    xTaskCreatePinnedToCore(blink, "blink", 2048, NULL, 0, task2, 0);
}