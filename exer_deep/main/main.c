#include <stdio.h>
#include "stdlib.h"
#include <string.h>
#include "time.h"     //for timeval
#include <sys/time.h> //for POSIX function gettimeofday()
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"
#include "esp_sleep.h"
#include "esp_log.h"
#include "driver/rtc_io.h"

#include "sdkconfig.h"

const static char *TAG = "deepsleep";
static RTC_DATA_ATTR struct timeval sleep_enter;
const uint64_t wakeup_pinmask = GPIO_SEL_2 | GPIO_SEL_4;

void app_main(void)
{
    struct timeval now;
    gettimeofday(&now, NULL);
    int slept_time = (now.tv_sec - sleep_enter.tv_sec) * 1000 + (now.tv_usec - sleep_enter.tv_usec) / 1000;
    ESP_LOGI(TAG, "slept %d ms", slept_time);
    switch (esp_sleep_get_wakeup_cause())
    {
    case ESP_SLEEP_WAKEUP_EXT1:
        ESP_LOGI(TAG, "wake up by ext1");
        break;
    default:
        ESP_LOGI(TAG, "unhandled");
    }
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_INPUT_OUTPUT,
        .pin_bit_mask = GPIO_SEL_2 | GPIO_SEL_4,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE,
    };
    gpio_config(&io_conf);
    while(gpio_get_level(2)){
        printf("wating...\n");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    printf("gpio2state:%d\n", gpio_get_level(GPIO_NUM_2));

    vTaskDelay(pdMS_TO_TICKS(1000));
    esp_sleep_enable_ext1_wakeup(wakeup_pinmask, ESP_EXT1_WAKEUP_ANY_HIGH);
#if CONFIG_IDF_TARGET_ESP32
    // Isolate GPIO12 pin from external circuits. This is needed for modules
    // which have an external pull-up resistor on GPIO12 (such as ESP32-WROVER)
    // to minimize current consumption.
    rtc_gpio_isolate(GPIO_NUM_12);
#endif
    gettimeofday(&sleep_enter, NULL);
    ESP_LOGI(TAG,"goto deep sleep");
    esp_deep_sleep_start();
}