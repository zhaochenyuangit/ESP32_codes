#include "stdio.h"
#include "stdint.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "esp_pm.h"
#include "esp_sleep.h"
#include "esp_log.h"

#include "network_common.h"
#include "edge_intr_debounce.h"
#include "commands_corner_cases.h"
#include "sntpController.h"
#include "sdkconfig.h"

#define SLEEP_TIMER_MS 5000
#define DEEP_SLEEP
//#define LIGHT_SLEEP

static const char *TAG = "main";
static esp_mqtt_client_handle_t client = NULL;
static RTC_DATA_ATTR struct timeval sleep_enter;
static TimerHandle_t sleep_timer = NULL;

static void my_task(void *_);
void (*gpio_task)() = my_task;

/*task to receive edge intrrupt messages*/
static void my_task(void *_)
{
    uint32_t io_num;
    while (1)
    {
        char buf[20];
        if (xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY))
        {
            xTimerStart(sleep_timer, 0);
            struct timeval now;
            gettimeofday(&now, NULL);
            printf("now: %ld\n", now.tv_sec);
            //wait until reading is stable
            vTaskDelay(pdMS_TO_TICKS(DEBOUNCE_TIMER_MS / 2));
            //write your code here
            int level = gpio_get_level(io_num);
            printf("gpio interrupt on pin %d, val %d\n", io_num, level);
            sprintf(buf, "pin %d: val %d", io_num, level);
            mqtt_send(client, "test", buf);
        }
    }
}

/*action when goes into sleep mode*/
static void go_sleep(void *_)
{
    esp_sleep_enable_ext1_wakeup(wakeup_pinmask, ESP_EXT1_WAKEUP_ANY_HIGH);
    gettimeofday(&sleep_enter, NULL);
    esp_pm_dump_locks(stdout);
    esp_mqtt_client_disconnect(client);
    esp_wifi_disconnect();
#ifdef LIGHT_SLEEP
    ESP_LOGI(TAG, "enter light sleep\n");
    esp_light_sleep_start();
    esp_err_t ret;
    ret = esp_wifi_connect();
    printf("%s\n", esp_err_to_name(ret));
    vTaskDelay(10);
    ret = esp_mqtt_client_reconnect(client);
    printf("%s\n", esp_err_to_name(ret));
#elif defined(DEEP_SLEEP)
    ESP_LOGI(TAG, "enter deep sleep\n");
    esp_deep_sleep_start();
#else
    printf("no sleep\n");
#endif
}

void pm_config()
{
#if CONFIG_PM_ENABLE
    // Configure dynamic frequency scaling:
    // maximum and minimum frequencies are set in sdkconfig,
    // automatic light sleep is enabled if tickless idle support is enabled.
#if CONFIG_IDF_TARGET_ESP32
    esp_pm_config_esp32_t pm_config = {
#elif CONFIG_IDF_TARGET_ESP32S2
    esp_pm_config_esp32s2_t pm_config = {
#endif
        .max_freq_mhz = 160,
        .min_freq_mhz = 80,
#if CONFIG_FREERTOS_USE_TICKLESS_IDLE
        .light_sleep_enable = false //true
#endif
    };
    ESP_ERROR_CHECK(esp_pm_configure(&pm_config));
#endif // CONFIG_PM_ENABLE
}

void app_main(void)
{
    struct timeval now;
    gettimeofday(&now, NULL);
    int slept_time = (now.tv_sec - sleep_enter.tv_sec) * 1000 + (now.tv_usec - sleep_enter.tv_usec) / 1000;
    ESP_LOGI(TAG, "slept %d ms", slept_time);

    //power save mode
    pm_config();

    
    if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_TIMER)
    {
        start_wifi();
        initializeSntp();
        obtainTime();
        start_mqtt(&client);
    }
    

    //pin
    wakeup_setup();
    simulation_setup();
    //sleep
    sleep_timer = xTimerCreate("sleep_timer", pdMS_TO_TICKS(SLEEP_TIMER_MS), false, NULL, go_sleep);
    if (sleep_timer == NULL)
    {
        ESP_LOGE(TAG, "create sleep timer failed");
    }
    xTimerStart(sleep_timer, 0);

    //enter();
    //peekIntoandLeaveG11();
}