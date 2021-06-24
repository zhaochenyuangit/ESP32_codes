#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/gpio.h"

#define ledPin GPIO_NUM_2
#define triggerPinOut GPIO_NUM_18
#define triggerPinIn GPIO_NUM_19

void simulation_setup();
void enter();
void leave();
void peekIntoandLeaveG11();