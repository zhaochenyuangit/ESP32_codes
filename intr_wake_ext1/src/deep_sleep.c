#include "deep_sleep.h"
#include "esp_pm.h"
#include "ssd1306.h"

static const char *TAG = "deep sleep";
static RTC_DATA_ATTR struct timeval sleep_enter;
static TimerHandle_t sleep_timer = NULL;

static void go_sleep(void *_)
{   
    ssd1306_displayOff();
    esp_pm_dump_locks(stdout);

    printf("start wake up timer\n");
    esp_sleep_enable_timer_wakeup(WAKE_UP_INTERVAL_S * S_TO_US_FACTOR);

    gettimeofday(&sleep_enter, NULL);
    ESP_LOGI(TAG, "go to deep sleep\n");
    vTaskDelay(10);
    esp_deep_sleep_start();
}

void print_slept_time(void)
{
    struct timeval now;
    gettimeofday(&now, NULL);
    int slept_time = (now.tv_sec - sleep_enter.tv_sec) * 1000 + (now.tv_usec - sleep_enter.tv_usec) / 1000;
    ESP_LOGI(TAG, "slept %d ms", slept_time);
}

void create_sleep_timer(void)
{

    sleep_timer = xTimerCreate("sleep_timer", pdMS_TO_TICKS(SLEEP_TIMER_MS), false, NULL, go_sleep);
    if (sleep_timer == NULL)
    {
        ESP_LOGE(TAG, "create sleep timer failed");
    }
    xTimerStart(sleep_timer, 0);
}

void restart_sleep_timer(void)
{
    if (sleep_timer == NULL)
    {
        ESP_LOGW(TAG, "sleep tiemr is not created, igore");
    }
    else
    {
        xTimerStart(sleep_timer, 0);
    }
}