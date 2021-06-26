#include <stdio.h>
#include "stdlib.h"
#include <string.h>
#include "time.h"     //for timeval
#include <sys/time.h> //for POSIX function gettimeofday()
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/timers.h"

#include "esp_sleep.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "driver/rtc_io.h"

#include "sdkconfig.h"

#define DEBOUNCE_TIMER_MS 20
#define GPIO_PIN_1 GPIO_NUM_25
#define GPIO_PIN_2 GPIO_NUM_4

typedef struct gpio_evt_msg
{
    int64_t timestamp;
    int32_t level;
    int32_t pin;
}gpio_evt_msg;

extern QueueHandle_t gpio_evt_queue;

/** 
 * setup two pins as wake up source. Debouncing is already implemented.
 */
void wakeup_setup(void);