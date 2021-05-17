#include <stdio.h>
#include "stdlib.h"
#include <string.h>

#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "time.h"
#include <sys/time.h>

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
#include "soc/sens_reg.h"

#include "ulp_main.h"

static const char *TAG = "whs";
static RTC_DATA_ATTR struct timeval then;
extern const uint8_t ulp_main_bin_start[] asm("_binary_ulp_main_bin_start");
extern const uint8_t ulp_main_bin_end[] asm("_binary_ulp_main_bin_end");
static void init_ulp_program()
{
    esp_err_t err = ulp_load_binary(0, ulp_main_bin_start, (ulp_main_bin_end - ulp_main_bin_start) / sizeof(uint32_t));
    ESP_ERROR_CHECK(err);
    //    GPIO 26 - RTC 7
    gpio_num_t gpio_p1 = GPIO_NUM_26;
    rtc_gpio_init(gpio_p1);
    rtc_gpio_set_direction(gpio_p1, RTC_GPIO_MODE_INPUT_ONLY);
    rtc_gpio_pulldown_en(gpio_p1);
    rtc_gpio_pullup_dis(gpio_p1);
    rtc_gpio_hold_en(gpio_p1);
    //rtc_gpio_isolate(GPIO_NUM_26);

    SET_PERI_REG_BITS(SENS_SAR_SLAVE_ADDR1_REG, SENS_I2C_SLAVE_ADDR0, 0x69, SENS_I2C_SLAVE_ADDR0_S);  // Set I2C device address.
    SET_PERI_REG_BITS(SENS_SAR_SLAVE_ADDR1_REG, SENS_I2C_SLAVE_ADDR1, 0x69, SENS_I2C_SLAVE_ADDR1_S);  // Set I2C device address.
    gpio_num_t sda_pin = GPIO_NUM_0;
    gpio_num_t scl_pin = GPIO_NUM_4;
    //sda
    rtc_gpio_init(sda_pin);
    rtc_gpio_set_level(sda_pin, 0);
	rtc_gpio_pulldown_dis(sda_pin);
    rtc_gpio_pullup_en(sda_pin);
    rtc_gpio_set_direction(sda_pin, RTC_GPIO_MODE_INPUT_OUTPUT);
    REG_SET_FIELD(RTC_IO_SAR_I2C_IO_REG, RTC_IO_SAR_I2C_SDA_SEL, 0);
    //scl
    rtc_gpio_init(scl_pin);
    rtc_gpio_set_level(scl_pin, 0);
	rtc_gpio_pulldown_dis(scl_pin);
    rtc_gpio_pullup_en(scl_pin);
    rtc_gpio_set_direction(scl_pin, RTC_GPIO_MODE_INPUT_OUTPUT);
    REG_SET_FIELD(RTC_IO_SAR_I2C_IO_REG, RTC_IO_SAR_I2C_SCL_SEL, 0);

    SET_PERI_REG_BITS(rtc_io_desc[rtc_io_number_get(scl_pin)].reg, RTC_IO_TOUCH_PAD1_FUN_SEL_V, 0x3, rtc_io_desc[rtc_io_number_get(scl_pin)].func);
    SET_PERI_REG_BITS(rtc_io_desc[rtc_io_number_get(sda_pin)].reg, RTC_IO_TOUCH_PAD1_FUN_SEL_V, 0x3, rtc_io_desc[rtc_io_number_get(sda_pin)].func);
    
    // Look up your device for these standard values needed to do i2c:
    WRITE_PERI_REG(RTC_I2C_SCL_LOW_PERIOD_REG, 40); // SCL low/high period = 40, which result driving SCL with 100kHz.
    WRITE_PERI_REG(RTC_I2C_SCL_HIGH_PERIOD_REG, 40);  
    WRITE_PERI_REG(RTC_I2C_SDA_DUTY_REG, 16);     // SDA duty (delay) cycles from falling edge of SCL when SDA changes.
    WRITE_PERI_REG(RTC_I2C_SCL_START_PERIOD_REG, 30); // Number of cycles to wait after START condition.
    WRITE_PERI_REG(RTC_I2C_SCL_STOP_PERIOD_REG, 44);  // Number of cycles to wait after STOP condition.
    WRITE_PERI_REG(RTC_I2C_TIMEOUT_REG, 200);     // Number of cycles before timeout.
    REG_SET_FIELD(RTC_I2C_CTRL_REG, RTC_I2C_MS_MODE, 1);// set mode to master
    

    ulp_set_wakeup_period(0, 20 * 1000); // 20 ms
    esp_deep_sleep_disable_rom_logging();
    printf("run ulp!\n");
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

    struct timeval now;
    gettimeofday(&now, NULL);
    int slept_time = (now.tv_sec - then.tv_sec) * 1000 + (now.tv_usec - then.tv_usec) / 1000;
    printf("slept time:%d\n",slept_time);
    printf("dummy value: %d\n", ulp_dummy);
    // suppress boot message
    esp_deep_sleep_disable_rom_logging();

    ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version());

    esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
    printf("cause is %d\n", cause);
    if (cause != ESP_SLEEP_WAKEUP_ULP)
    {
        printf("Not ULP wakeup, initializing ULP\n");
        init_ulp_program();
    }

    printf("%d : %d \n", ulp_p1_status & UINT16_MAX, ulp_event_counter & UINT16_MAX);
    gettimeofday(&then,NULL);
    printf("Entering deep sleep\n\n");
    ESP_ERROR_CHECK(esp_sleep_enable_ulp_wakeup());
    esp_deep_sleep_start();
}
