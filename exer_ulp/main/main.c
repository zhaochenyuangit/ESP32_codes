#include <stdio.h>
#include "stdlib.h"
#include <string.h>

#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_system.h"
#include "esp_sleep.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "driver/rtc_io.h"
#include "esp32/ulp.h"
#include "soc/rtc_io_reg.h"
#include "soc/soc_ulp.h"
#include "soc/rtc_i2c_reg.h"
#include "ulp_main.h"//because assemby file has the name "main.S"

static const char *TAG = "whs";
extern const uint8_t ulp_main_bin_start[] asm("_binary_ulp_main_bin_start");
extern const uint8_t ulp_main_bin_end[] asm("_binary_ulp_main_bin_end");
static void init_ulp_program(uint32_t usec)
{
    // initialize ulp variables
    printf("initing...\n");

    ulp_set_wakeup_period(0, usec);

    // You MUST load the binary before setting shared variables!
    esp_err_t err = ulp_load_binary(0, ulp_main_bin_start, (ulp_main_bin_end - ulp_main_bin_start) / sizeof(uint32_t));
    ESP_ERROR_CHECK(err);

    // Setup RTC I2C controller, selection either 0 or 1, see below for exact description and wiring (you can mix and match and comment out unused gpios)
    SET_PERI_REG_BITS(RTC_IO_SAR_I2C_IO_REG, RTC_IO_SAR_I2C_SDA_SEL, 0, RTC_IO_SAR_I2C_SDA_SEL_S);
    SET_PERI_REG_BITS(RTC_IO_SAR_I2C_IO_REG, RTC_IO_SAR_I2C_SCL_SEL, 0, RTC_IO_SAR_I2C_SCL_SEL_S);

    // SDA/SCL selection 0 is for
    // TOUCH0 = GPIO4 = SCL
    // TOUCH1 = GPIO0 = SDA
    rtc_gpio_init(GPIO_NUM_0);
    rtc_gpio_init(GPIO_NUM_4);
    rtc_gpio_init(GPIO_NUM_26);
    rtc_gpio_set_direction(GPIO_NUM_26, RTC_GPIO_MODE_OUTPUT_ONLY);
    rtc_gpio_pulldown_en(GPIO_NUM_26);
    rtc_gpio_pullup_dis(GPIO_NUM_26);
    rtc_gpio_hold_en(GPIO_NUM_26);
    rtc_gpio_set_level(GPIO_NUM_26,1);
    // SDA/SCL selection 1 is for
    // TOUCH2 = GPIO2 = SCL
    // TOUCH3 = GPIO15 = SDA
    //rtc_gpio_init(GPIO_NUM_2);
    //rtc_gpio_init(GPIO_NUM_15);

    // We will be reading and writing on these gpio
    //rtc_gpio_set_direction(GPIO_NUM_15, RTC_GPIO_MODE_INPUT_OUTPUT);
    rtc_gpio_set_direction(GPIO_NUM_4, RTC_GPIO_MODE_INPUT_OUTPUT);
    //rtc_gpio_set_direction(GPIO_NUM_2, RTC_GPIO_MODE_INPUT_OUTPUT);
    rtc_gpio_set_direction(GPIO_NUM_0, RTC_GPIO_MODE_INPUT_OUTPUT);

    // Look up your device for these standard values needed to do i2c:
    WRITE_PERI_REG(RTC_I2C_SCL_LOW_PERIOD_REG, 40); // SCL low/high period = 40, which result driving SCL with 100kHz.
    WRITE_PERI_REG(RTC_I2C_SCL_HIGH_PERIOD_REG, 40);
    WRITE_PERI_REG(RTC_I2C_SDA_DUTY_REG, 16);                                   // SDA duty (delay) cycles from falling edge of SCL when SDA changes.
    WRITE_PERI_REG(RTC_I2C_SCL_START_PERIOD_REG, 30);                           // Number of cycles to wait after START condition.
    WRITE_PERI_REG(RTC_I2C_SCL_STOP_PERIOD_REG, 44);                            // Number of cycles to wait after STOP condition.
    WRITE_PERI_REG(RTC_I2C_TIMEOUT_REG, 10000);                                 // Number of cycles before timeout.
    SET_PERI_REG_BITS(RTC_I2C_CTRL_REG, RTC_I2C_MS_MODE, 1, RTC_I2C_MS_MODE_S); // Set i2c mode to master for the ulp controller

   
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);

    // Any other ulp variables can be assigned here

    printf("end of init\n");
    err = ulp_run(&ulp_entry - RTC_SLOW_MEM);
    ESP_ERROR_CHECK(err);
}
void app_main(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // suppress boot message
    esp_deep_sleep_disable_rom_logging();
    ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version());

    switch (esp_sleep_get_wakeup_cause())
    {
    case ESP_SLEEP_WAKEUP_UNDEFINED:
        printf("Not ULP wakeup, initializing ULP\n");
        init_ulp_program(20000);
        break;
    case ESP_SLEEP_WAKEUP_TIMER:
        printf("Timer wake up\n");
        break;
    case ESP_SLEEP_WAKEUP_ULP:
        printf("waked up by ULP\n");
        printf("ULP variable: %d\n",ulp_dummy);
        break;
    default:
        printf("unhandled event\n");
        break;
    }
    printf("address?: %d\n",ulp_test);
    ESP_ERROR_CHECK(esp_sleep_enable_ulp_wakeup());
    //ESP_ERROR_CHECK(esp_sleep_enable_timer_wakeup(10 * 1000000));
    printf("Entering deep sleep\n\n");
    esp_deep_sleep_start();
}