#include "edge_intr_sleep.h"

const static char *TAG = "sleep mode";
static RTC_DATA_ATTR struct timeval sleep_enter;
const uint64_t wakeup_pinmask = BIT(GPIO_PIN_1) | BIT(GPIO_PIN_2);
QueueHandle_t gpio_evt_queue = NULL;
static TimerHandle_t debounce_timer1 = NULL;
static TimerHandle_t debounce_timer2 = NULL;
static TimerHandle_t sleep_timer = NULL;

static void IRAM_ATTR gpio_isr_handler(void *arg)
{
    uint32_t gpio_num = (uint32_t)arg;
    gpio_intr_disable(gpio_num);
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
    switch (gpio_num)
    {
    case GPIO_PIN_1:
        xTimerStart(debounce_timer1, 0);
        break;
    case GPIO_PIN_2:
        xTimerStart(debounce_timer2, 0);
        break;
    default:
        break;
    }
    xTimerStart(sleep_timer, 0);
}

static void isr_reenable_cb(TimerHandle_t xTimer)
{
    uint32_t gpio_num = (uint32_t)pvTimerGetTimerID(xTimer);
    gpio_intr_enable(gpio_num);
}

static void go_sleep(void *_)
{
    esp_sleep_enable_ext1_wakeup(wakeup_pinmask, ESP_EXT1_WAKEUP_ANY_HIGH);
    gettimeofday(&sleep_enter, NULL);
    ESP_LOGI(TAG, "go to sleep\n");
    vTaskDelay(10);
#ifdef LIGHT_SLEEP
    esp_light_sleep_start();
#elif defined(DEEP_SLEEP)
    esp_deep_sleep_start();
#else
    printf("no sleep\n");
#endif
}

void wakeup_setup(void)
{
    struct timeval now;
    gettimeofday(&now, NULL);
    int slept_time = (now.tv_sec - sleep_enter.tv_sec) * 1000 + (now.tv_usec - sleep_enter.tv_usec) / 1000;
    ESP_LOGI(TAG, "slept %d ms", slept_time);

    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_ANYEDGE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = wakeup_pinmask,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE,
    };
    gpio_config(&io_conf);
#if CONFIG_IDF_TARGET_ESP32
    // Isolate GPIO12 pin from external circuits. This is needed for modules
    // which have an external pull-up resistor on GPIO12 (such as ESP32-WROVER)
    // to minimize current consumption.
    rtc_gpio_isolate(GPIO_NUM_12);
#endif
     //install gpio isr service
    gpio_install_isr_service(0);
    //hook isr handler for specific gpio pin
    gpio_isr_handler_add(GPIO_PIN_1, gpio_isr_handler, (void *)GPIO_PIN_1);
    //hook isr handler for specific gpio pin
    gpio_isr_handler_add(GPIO_PIN_2, gpio_isr_handler, (void *)GPIO_PIN_2);
    
    gpio_evt_queue = xQueueCreate(3, sizeof(uint32_t));
    if (gpio_evt_queue == NULL)
    {
        printf("create queue failed\n");
    }

    xTaskCreatePinnedToCore(gpio_task, "poll gpio", 2048, NULL, 1, NULL, 0);

    if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_EXT1)
    {
        ESP_LOGI(TAG, "wake up by ext1");
        if (gpio_get_level(GPIO_PIN_1))
        {
            gpio_num_t pin = GPIO_PIN_1;
            xQueueSend(gpio_evt_queue, &pin, 0);
        }
        else if (gpio_get_level(GPIO_PIN_2))
        {
            gpio_num_t pin = GPIO_PIN_2;
            xQueueSend(gpio_evt_queue, &pin, 0);
        }
    }
    debounce_timer1 = xTimerCreate("gpio_reenable1", pdMS_TO_TICKS(DEBOUNCE_TIMER_MS), false, (void *)GPIO_PIN_1, isr_reenable_cb);
    debounce_timer2 = xTimerCreate("gpio_reenable2", pdMS_TO_TICKS(DEBOUNCE_TIMER_MS), false, (void *)GPIO_PIN_2, isr_reenable_cb);
    sleep_timer = xTimerCreate("sleep_timer", pdMS_TO_TICKS(SLEEP_TIMER_MS), false, NULL, go_sleep);
    if ((debounce_timer1 == NULL) || (debounce_timer2 == NULL) || (sleep_timer == NULL))
    {
        printf("create timer failed\n");
    }
    xTimerStop(debounce_timer1, 0);
    xTimerStop(debounce_timer2, 0);
    xTimerStart(sleep_timer, 0);
}
