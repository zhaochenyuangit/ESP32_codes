#include "edge_intr_debounce.h"

const static char *TAG = "debounce";

const uint64_t wakeup_pinmask = BIT(GPIO_PIN_1) | BIT(GPIO_PIN_2);
QueueHandle_t gpio_evt_queue = NULL;
static TimerHandle_t debounce_timer1 = NULL;
static TimerHandle_t debounce_timer2 = NULL;

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
}

static void isr_reenable_cb(TimerHandle_t xTimer)
{
    uint32_t gpio_num = (uint32_t)pvTimerGetTimerID(xTimer);
    gpio_intr_enable(gpio_num);
}

void wakeup_setup(void)
{
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
    gpio_isr_handler_add(GPIO_PIN_2, gpio_isr_handler, (void *)GPIO_PIN_2);

    gpio_evt_queue = xQueueCreate(3, sizeof(uint32_t));
    if (gpio_evt_queue == NULL)
    {
        ESP_LOGE(TAG, "create queue failed");
    }

    xTaskCreatePinnedToCore(gpio_task, "poll gpio", 2048, NULL, 1, NULL, 0);

    debounce_timer1 = xTimerCreate("gpio_reenable1", pdMS_TO_TICKS(DEBOUNCE_TIMER_MS), false, (void *)GPIO_PIN_1, isr_reenable_cb);
    debounce_timer2 = xTimerCreate("gpio_reenable2", pdMS_TO_TICKS(DEBOUNCE_TIMER_MS), false, (void *)GPIO_PIN_2, isr_reenable_cb);
    if ((debounce_timer1 == NULL) || (debounce_timer2 == NULL))
    {
        ESP_LOGE(TAG, "create debounce timer failed");
    }
    xTimerStop(debounce_timer1, 0);
    xTimerStop(debounce_timer2, 0);
}
