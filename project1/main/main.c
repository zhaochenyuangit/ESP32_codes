/*
* this is an exercise project to familiarize myself with concepts in RTOS
* such as semaphore, hardware interrupts, memory management......
*/

#include <stdio.h>
#include "stdlib.h"
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "driver/gpio.h"
#include "driver/timer.h"
#include "driver/adc.h"
#include "esp32/rom/uart.h"
#include "sdkconfig.h"

#define LED 4

static TaskHandle_t task1 = NULL;
static TaskHandle_t task2 = NULL;
static SemaphoreHandle_t sema;

static bool led_state = 0;
static uint8_t str_len = 32;

static volatile int val;
enum
{
    buf_len = 10
};
static int val_buf[buf_len];
static uint8_t val_idx = 0;
static float avg = 0;

void serial_echo(void *parameter)
{
    uint8_t ch;
    char str_buf[str_len];
    uint8_t idx = 0;
    static char aquire[] = "avg";
    while (1)
    {
        STATUS s = uart_rx_one_char(&ch);
        if (s == OK)
        {
            if (idx < str_len)
            {
                str_buf[idx++] = ch;
            }
            else
            {
                printf("str max exceeded!\n");
            }
            if (ch == '\r')
            {
                str_buf[idx - 1] = '\0';
                printf("%s\n", str_buf);
                if (memcmp(str_buf, aquire, strlen(aquire)) == 0)
                {
                    printf("current average value is %f\n", avg);
                }
                // reset
                idx = 0;
                memset(str_buf, 0, str_len);
            }
        }
        vTaskDelay(50 / portTICK_PERIOD_MS);
    }
}

void calc_avg(void *parameter)
{
    while (1)
    {
        xSemaphoreTake(sema, portMAX_DELAY);
        printf("calc_avg watermark %d\n",uxTaskGetStackHighWaterMark(task2));
        printf("serial echo watermark %d\n",uxTaskGetStackHighWaterMark(task1));
        printf("semaphore!\n");
        int sum = 0;
        for (int i = 0; i < buf_len; i++)
        {
            sum += val_buf[i];
        }
        avg = sum / buf_len;
        //vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

bool IRAM_ATTR my_isr(void *group_num)
{
    /*this uint32 to void* convert and vice versa is stupid but necessary
    to avoid annoying warnings*/
    timer_spinlock_take((uint32_t)group_num);
    led_state = !led_state;
    gpio_set_level(LED, led_state);

    val = adc1_get_raw(ADC1_CHANNEL_6);
    // must use ets_printf if want to debug inside an ISR
    ets_printf("read value is %d \n",val);
    val_buf[val_idx++] = val;
    if (val_idx == buf_len)
    {
        val_idx = 0;
        xSemaphoreGive(sema);
    }
    timer_spinlock_give((uint32_t)group_num);
    return 0;
}

void set_hw_timer_template(uint32_t group_num, uint8_t timer_num, uint32_t delay_ms, bool reload, timer_isr_t isr)
{
    /*
* all four esp32 hardware timer are fed with a 80MHz APB Clock
* which can be prescaled by a factor between 2~65535 (uint16)
* counts up to uint64  
    */
    uint16_t prescale = 80;
    uint64_t alarm_value = (TIMER_BASE_CLK / prescale) * (delay_ms / 1000);
    const timer_config_t hw_timer_config = {
        .divider = prescale,
        .counter_dir = TIMER_COUNT_UP,
        .counter_en = TIMER_START,
        .alarm_en = TIMER_ALARM_EN,
        .auto_reload = TIMER_AUTORELOAD_DIS,
        .intr_type = TIMER_INTR_LEVEL};
    timer_init(group_num, timer_num, &hw_timer_config);
    timer_set_counter_value(group_num, timer_num, 0);                                         //initial value is 0
    timer_set_alarm_value(group_num, timer_num, alarm_value);                                 //call ISR when reached alarm value
    timer_isr_callback_add(group_num, timer_num, isr, (void *)group_num, ESP_INTR_FLAG_IRAM); // attach ISR
    timer_start(group_num, timer_num);
}
void app_main(void)
{
    gpio_pad_select_gpio(LED);
    gpio_set_direction(LED, GPIO_MODE_OUTPUT);
    gpio_set_level(LED, led_state);

    sema = xSemaphoreCreateBinary();

    adc1_config_channel_atten(ADC1_CHANNEL_6, ADC_ATTEN_DB_11);

    set_hw_timer_template(0, 0, 2000, 1, my_isr);
    xTaskCreatePinnedToCore(serial_echo, "echo", 4000,
                            NULL, 1, task1, 0);
    xTaskCreatePinnedToCore(calc_avg, "avg", 1600,
                            NULL, 0, task2, 0);
}