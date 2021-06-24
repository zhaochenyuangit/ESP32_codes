#include "stdio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "edge_intr_sleep.h"
#include "commands_corner_cases.h"

static void my_task(void *_);
void (*gpio_task)() = my_task;

/*task to receive edge intrrupt messages*/
static void my_task(void *_)
{
    uint32_t io_num;
    while (1)
    {
        if (xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY))
        {
            //wait until reading is stable
            vTaskDelay(pdMS_TO_TICKS(DEBOUNCE_TIMER_MS / 2));
            //write your code here
            printf("gpio interrupt on pin %d, val %d\n", io_num, gpio_get_level(io_num));
        }
    }
}

void app_main(void)
{
    
    wakeup_setup();
    
    simulation_setup();
    enter();
    peekIntoandLeaveG11();
}