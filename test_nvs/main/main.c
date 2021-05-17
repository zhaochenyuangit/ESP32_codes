#include <stdio.h>
#include "stdlib.h"
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"
#include "sdkconfig.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "esp_sleep.h"

static RTC_NOINIT_ATTR int count;
//static uint32_t count =0;

void app_main(void)
{
    
    esp_reset_reason_t reason = esp_reset_reason();
    switch (reason)
    {
    case ESP_RST_POWERON:
        printf("first time\n");
        count = 0;
        break;
    case ESP_RST_SW:
        printf("software reset\n");
        break;
    default:
        printf("unhandled event %d\n",reason);
        break;
    }
    count++;
    printf("%d\n", count);
    vTaskDelay(3 * 1000 / portTICK_PERIOD_MS);
    esp_restart();
}
const char *namespace = "testcount";
const char *count_key = "count";
ESP_ERROR_CHECK(nvs_flash_init());
nvs_handle_t handle;
ESP_ERROR_CHECK(nvs_open(namespace, NVS_READWRITE, &handle));
/* Read count from NVS*/
esp_err_t err = nvs_get_u32(handle, count_key, &count);
assert(err == ESP_OK || err == ESP_ERR_NVS_NOT_FOUND);
printf("Read count from NVS: %5d\n", count);
count++;
printf("change count to: %d\n", count);
/* Save the new pulse count to NVS */
ESP_ERROR_CHECK(nvs_set_u32(handle, count_key, count));
ESP_ERROR_CHECK(nvs_commit(handle));
nvs_close(handle);