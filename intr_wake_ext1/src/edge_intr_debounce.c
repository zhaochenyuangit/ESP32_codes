#include "edge_intr_debounce.h"

const static char *TAG = "debounce";

const uint64_t wakeup_pinmask = BIT(GPIO_PIN_1) | BIT(GPIO_PIN_2);
static QueueHandle_t edge_intr_queue = NULL;
QueueHandle_t gpio_evt_queue = NULL;
static TimerHandle_t debounce_timer1 = NULL;
static TimerHandle_t debounce_timer2 = NULL;

static void IRAM_ATTR gpio_isr_handler(void *arg)
{
    uint32_t gpio_num = (uint32_t)arg;
    gpio_intr_disable(gpio_num);
    xQueueSendFromISR(edge_intr_queue, &gpio_num, NULL);
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

static void edge_intr_task(void *_)
{
    uint32_t io_num;
    while (1)
    {
        if (xQueueReceive(edge_intr_queue, &io_num, portMAX_DELAY))
        {
            //wait until reading is stable
            vTaskDelay(pdMS_TO_TICKS(DEBOUNCE_TIMER_MS));
            //
            int pin_level = gpio_get_level(io_num);
            gpio_evt_msg msg = {
                .pin = io_num,
                .level = pin_level,
            };
            xQueueSend(gpio_evt_queue, &msg, 0);
        }
    }
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

    //enable barrier pins as wake up soruce
    esp_sleep_enable_ext1_wakeup(wakeup_pinmask, ESP_EXT1_WAKEUP_ANY_HIGH);

    edge_intr_queue = xQueueCreate(3, sizeof(int));
    gpio_evt_queue = xQueueCreate(3, sizeof(gpio_evt_msg));

    if ((gpio_evt_queue == NULL) || (edge_intr_queue == NULL))
    {
        ESP_LOGE(TAG, "create queue failed");
    }

    xTaskCreatePinnedToCore(edge_intr_task, "edge intr", 2048, NULL, 10, NULL, 1);

    //only for deepsleep
    if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_EXT1)
    {
        if (gpio_get_level(GPIO_PIN_1))
        {
            gpio_evt_msg msg = {
                .pin = GPIO_PIN_1,
                .level = 1,
            };
            xQueueSend(gpio_evt_queue, &msg, 0);
        }
        else if (gpio_get_level(GPIO_PIN_2))
        {
            gpio_evt_msg msg = {
                .pin = GPIO_PIN_2,
                .level = 1,
            };
            xQueueSend(gpio_evt_queue, &msg, 0);
        }
    }
    debounce_timer1 = xTimerCreate("gpio_reenable1", pdMS_TO_TICKS(DEBOUNCE_TIMER_MS), false, (void *)GPIO_PIN_1, isr_reenable_cb);
    debounce_timer2 = xTimerCreate("gpio_reenable2", pdMS_TO_TICKS(DEBOUNCE_TIMER_MS), false, (void *)GPIO_PIN_2, isr_reenable_cb);
    if ((debounce_timer1 == NULL) || (debounce_timer2 == NULL))
    {
        ESP_LOGE(TAG, "create debounce timer failed");
    }
    xTimerStop(debounce_timer1, 0);
    xTimerStop(debounce_timer2, 0);
}
