/* ULP I2C bit bang Example
   adapted from https://github.com/tomtor/ulp-i2c 
*/

#include <stdio.h>
#include <math.h>
#include "esp_sleep.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "soc/rtc_cntl_reg.h"
#include "soc/rtc_io_reg.h"
#include "soc/sens_reg.h"
#include "soc/soc.h"
#include "driver/gpio.h"
#include "driver/rtc_io.h"
#include "esp32/ulp.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"

#include "ulp_main.h"

extern const uint8_t ulp_main_bin_start[] asm("_binary_ulp_main_bin_start");
extern const uint8_t ulp_main_bin_end[] asm("_binary_ulp_main_bin_end");

const gpio_num_t gpio_led = GPIO_NUM_2;
const gpio_num_t gpio_scl = GPIO_NUM_32;
const gpio_num_t gpio_sda = GPIO_NUM_33;
//const gpio_num_t gpio_builtin = GPIO_NUM_22;

static void init_ulp_program()
{
    rtc_gpio_init(gpio_led);
    rtc_gpio_set_direction(gpio_led, RTC_GPIO_MODE_OUTPUT_ONLY);

    rtc_gpio_init(gpio_scl);
    rtc_gpio_set_direction(gpio_scl, RTC_GPIO_MODE_INPUT_OUTPUT);
    //rtc_gpio_pullup_en(gpio_scl);
    //rtc_gpio_pulldown_dis(gpio_scl);
    rtc_gpio_init(gpio_sda);
    rtc_gpio_set_direction(gpio_sda, RTC_GPIO_MODE_INPUT_OUTPUT);
    //rtc_gpio_pullup_en(gpio_sda);
    //rtc_gpio_pulldown_dis(gpio_sda);

    // You MUST load the binary before setting shared variables!
    esp_err_t err = ulp_load_binary(0, ulp_main_bin_start,
                                    (ulp_main_bin_end - ulp_main_bin_start) / sizeof(uint32_t));
    ESP_ERROR_CHECK(err);

    ulp_set_wakeup_period(0,100*1000);
}

void app_main()
{
    esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
    switch (cause)
    {
    case ESP_SLEEP_WAKEUP_UNDEFINED:
        printf("Not ULP wakeup, now initializing ULP\n");
        init_ulp_program();
        break;
    case ESP_SLEEP_WAKEUP_ULP:
        printf("ULP wakeup, printing status\n");
        printf("%d\n", (ulp_temp & 0xffff));
        break;
    case ESP_SLEEP_WAKEUP_TIMER:
        printf("...timer\n");
        printf("%04x\n",ulp_temp);
        break;
    default:
        printf("unhandled event%d\n",cause);
        break;
    }

    printf("Entering deep sleep\n\n");
    //ESP_ERROR_CHECK(esp_sleep_enable_timer_wakeup(10*1000000));
    ESP_ERROR_CHECK(esp_sleep_enable_ulp_wakeup());

    /* Start the program */
    esp_err_t err = ulp_run(&ulp_entry - RTC_SLOW_MEM);
    ESP_ERROR_CHECK(err);

    esp_deep_sleep_start();
}
