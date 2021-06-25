#include "stdio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "esp_pm.h"

#include "edge_intr_debounce.h"
#include "commands_corner_cases.h"
#include "sntpController.h"
#include "network_common.h"
#include "deep_sleep.h"

static void corner_case_handler(int message);
static void pm_config();
static void gpio_task(void *_);

#ifdef DEEP_SLEEP
struct CountValue
{
    int64_t timestamp_ms;
    int count;
};
enum
{
    COUNT_DATA_N_MAX = 20
};
static RTC_DATA_ATTR int count_data_n = 0;
static RTC_DATA_ATTR struct CountValue count_data[COUNT_DATA_N_MAX];
#endif

static const char *TAG = "main";
static RTC_NOINIT_ATTR int count;

static esp_mqtt_client_handle_t client = NULL;

/*task to receive edge intrrupt messages*/
static void gpio_task(void *_)
{
    gpio_evt_msg msg;
    while (1)
    {
        if (xQueueReceive(gpio_evt_queue, &(msg), portMAX_DELAY))
        {
#ifdef DEEP_SLEEP
            restart_sleep_timer();
#endif
            struct timeval now;
            gettimeofday(&now, NULL);
            int64_t timestamp_now = (now.tv_sec * (int64_t)1000) + (now.tv_usec / 1000);
            printf("now ms: %lld\n", timestamp_now);
            printf("gpio interrupt on pin %d, val %d\n", msg.pin, msg.level);
#ifndef DEEP_SLEEP
            corner_case_handler(1);
            size_t buf_len = 40;
            char buf[buf_len];
            snprintf(buf, buf_len, "timestamp %lld: count %d\n", timestamp_now, count);
            mqtt_send(client, "test", buf);
#else
            corner_case_handler(1);
            if (count_data_n >= COUNT_DATA_N_MAX)
            {
                ESP_LOGW(TAG, "not enough space to store");
                continue;
            }
            count_data[count_data_n].timestamp_ms = timestamp_now;
            count_data[count_data_n].count = count;
            count_data_n += 1;
#endif
        }
    }
}

static void corner_case_handler(int message)
{
    //TODO: intergrate your cornercase handler
    count += message % 2;
}

#ifdef DEEP_SLEEP
void publish_rtc_data(void)
{
    /*use snprintf to avoid buffer overflow, 
    change buffer length if the message becomes longer*/
    size_t buf_len = 40;
    char buf[buf_len];
    for (int i = 0; i < count_data_n; i++)
    {
        snprintf(buf, buf_len, "time: %lld, count: %d", count_data[i].timestamp_ms, count_data[i].count);
        mqtt_send(client, "test", buf);
    }
    count_data_n = 0;
}
#endif

static void pm_config()
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
        .light_sleep_enable = true
#endif
    };
    ESP_ERROR_CHECK(esp_pm_configure(&pm_config));
#endif // CONFIG_PM_ENABLE
}

#ifndef DEEP_SLEEP
void app_main(void)
{
    pm_config();
    if (esp_reset_reason() == ESP_RST_POWERON)
    {
        printf("reset count to zero");
        count = 0;
    }
    start_wifi();
    initializeSntp();
    obtainTime();
    start_mqtt(&client);

    wakeup_setup();
    xTaskCreatePinnedToCore(gpio_task,"gpio task",4000,NULL,1,NULL,1);
    simulation_setup();
}
#else
void app_main(void)
{
    pm_config();
    wakeup_setup();
    xTaskCreatePinnedToCore(gpio_task,"gpio task",4000,NULL,1,NULL,1);
    simulation_setup();
    print_slept_time();
    //pm_config();
    if (esp_reset_reason() == ESP_RST_POWERON)
    {
        printf("reset count to zero");
        count = 0;
        start_wifi();
        initializeSntp();
        obtainTime();
    }

    if ((esp_reset_reason() == ESP_RST_DEEPSLEEP) && (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_TIMER))
    {
        if (count_data_n)
        {
            start_wifi();
            start_mqtt(&client);
            publish_rtc_data();
        }
        else
        {
            printf("nothing to publish\n");
        }
    }
    create_sleep_timer();
}
#endif