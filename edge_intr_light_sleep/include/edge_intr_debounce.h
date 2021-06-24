#include <stdio.h>
#include "stdlib.h"
#include <string.h>
#include "time.h"     //for timeval
#include <sys/time.h> //for POSIX function gettimeofday()
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/timers.h"

#include "driver/gpio.h"
#include "esp_sleep.h"
#include "esp_log.h"
#include "driver/rtc_io.h"

#include "sdkconfig.h"

#define GPIO_PIN_1 GPIO_NUM_25
#define GPIO_PIN_2 GPIO_NUM_4
#define wakeup_pinmask  (BIT(GPIO_PIN_1) | BIT(GPIO_PIN_2))
#define DEBOUNCE_TIMER_MS 20


/*the queue handler as well as the function together 
 * serve as interface to outside*/
extern QueueHandle_t gpio_evt_queue;
extern void (*gpio_task)();

/** 
 * setup twp pins as wake up source, MCU will sleep if no action happens
 * within a certain amount of time. Debouncing is already implemented.
 */
void wakeup_setup(void);