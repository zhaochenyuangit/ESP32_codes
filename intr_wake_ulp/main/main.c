#include "stdio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_pm.h"

#include "soc/rtc_cntl_reg.h"
#include "soc/rtc_io_reg.h"
#include "soc/sens_reg.h"
#include "soc/soc.h"
#include "driver/rtc_io.h"
#include "driver/rtc_cntl.h"
#include "esp32/ulp.h"
#include "sdkconfig.h"

#include "network_common.h"
#include "edge_intr_debounce.h"
#include "commands_corner_cases.h"
#include "sntpController.h"
#include "deep_sleep.h"

#include "ulp_main.h"
#define RTC_PIN_1 GPIO_NUM_34
#define RTC_PIN_2 GPIO_NUM_35

extern const uint8_t ulp_main_bin_start[] asm("_binary_ulp_main_bin_start");
extern const uint8_t ulp_main_bin_end[] asm("_binary_ulp_main_bin_end");

static void my_task(void *_);
static void corner_case_handler(int message);
static void pm_config();

#ifndef DEEP_SLEEP
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
//static RTC_NOINIT_ATTR int count;

static esp_mqtt_client_handle_t client = NULL;
void (*gpio_task)() = my_task;

/*task to receive edge intrrupt messages*/
static void my_task(void *_)
{
    uint32_t io_num;
    while (1)
    {
        if (xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY))
        {
#ifdef DEEP_SLEEP
            restart_sleep_timer();
#endif
            //wait until reading is stable
            vTaskDelay(pdMS_TO_TICKS(DEBOUNCE_TIMER_MS / 2));
            //write your code here
            int pin_level = gpio_get_level(io_num);
            struct timeval now;
            gettimeofday(&now, NULL);
            int64_t timestamp_now = (now.tv_sec * (int64_t)1000) + (now.tv_usec / 1000);
            printf("now ms: %lld\n", timestamp_now);
            printf("gpio interrupt on pin %d, val %d\n", io_num, pin_level);
#ifndef DEEP_SLEEP
            corner_case_handler(io_num);
            size_t buf_len = 40;
            char buf[buf_len];
            snprintf(buf, buf_len, "timestamp %lld: count %d\n", timestamp_now, count);
            mqtt_send(client, "test", buf);
            //#else
            corner_case_handler(io_num);
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
/*
static void corner_case_handler(int message)
{
    //TODO: intergrate your cornercase handler
    count += message % 2;
}*/

#ifndef DEEP_SLEEP
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

#ifdef DEEP_SLEEP
static void init_ulp_program()
{
    ESP_LOGI(TAG, "init ulp");
    rtc_gpio_init(RTC_PIN_1);
    rtc_gpio_set_direction(RTC_PIN_1, RTC_GPIO_MODE_INPUT_ONLY);
    rtc_gpio_pullup_dis(RTC_PIN_1);
    rtc_gpio_pulldown_dis(RTC_PIN_1);
    rtc_gpio_hold_en(RTC_PIN_1);

    rtc_gpio_init(RTC_PIN_2);
    rtc_gpio_set_direction(RTC_PIN_2, RTC_GPIO_MODE_INPUT_ONLY);
    rtc_gpio_pullup_dis(RTC_PIN_2);
    rtc_gpio_pulldown_dis(RTC_PIN_2);
    rtc_gpio_hold_en(RTC_PIN_2);

    // You MUST load the binary before setting shared variables!
    esp_err_t err = ulp_load_binary(0, ulp_main_bin_start,
                                    (ulp_main_bin_end - ulp_main_bin_start) / sizeof(uint32_t));
    ESP_ERROR_CHECK(err);
    ulp_set_wakeup_period(0, 100 * 1000);
}
#endif

static void go_to_sleep(TimerHandle_t xTimer)
{
    printf("Entering deep sleep\n\n");
    //ESP_ERROR_CHECK(esp_sleep_enable_timer_wakeup(10*1000000));
    ESP_ERROR_CHECK(esp_sleep_enable_ulp_wakeup());

    /* Start the program */
    esp_err_t err = ulp_run(&ulp_entry - RTC_SLOW_MEM);
    ESP_ERROR_CHECK(err);

    esp_deep_sleep_start();
}

void app_main(void)
{
#ifndef DEEP_SLEEP
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
    simulation_setup();

#else
    //pm_config();
    wakeup_setup();
    print_slept_time();
    printf("ulp debug: %d, %4x, %d\n", (ulp_debug & 0xffff), (ulp_debug2 & 0xffff), (ulp_triggered_pin & 0xffff));
    

    if (esp_reset_reason() == ESP_RST_POWERON)
    {
        printf("reset count to zero");
        //count = 0;
        start_wifi();
        initializeSntp();
        obtainTime();
        init_ulp_program();
    }
    /*
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
    }*/

    if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_ULP)
    {
        printf("triggered pin is: %d\n", (ulp_triggered_pin & 0xffff));
    }

    create_sleep_timer();
    rtc_isr_register()
#endif
}
