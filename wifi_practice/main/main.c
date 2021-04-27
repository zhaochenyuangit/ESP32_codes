#include "driver/gpio.h"
#include "sdkconfig.h"

#include <stdio.h>
#include "stdlib.h"
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_wifi.h"
#include "esp_log.h"
#include "nvs_flash.h"

static const char *MYTAG = "my wifi";

static void my_handler(void *handler_arg, esp_event_base_t base,
                       int32_t id, void *data)
{
    if (base == WIFI_EVENT)
    {
        switch (id)
        {
        case WIFI_EVENT_STA_START:
            ESP_LOGI(MYTAG, "first time?");
            esp_wifi_connect();
            break;
        case WIFI_EVENT_STA_CONNECTED:
            ESP_LOGI(MYTAG, "wifi connected!");
            break;
        case WIFI_EVENT_STA_DISCONNECTED:
            ESP_LOGI(MYTAG, "trying to reconnect......");
            esp_wifi_connect();
            break;
        default:
            ESP_LOGI(MYTAG, "??");
        }
    }
}

void how2startwifi(void)
{
    /* 1. lwip initialize*/
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                                        &my_handler, NULL, NULL));

    /*2. wifi config*/
    wifi_config_t wifi_cfg = {
        .sta = {
            .ssid = "CAPS",
            .password = "caps!schulz-wifi",
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
            .pmf_cfg = {
                .capable = true,
                .required = false},
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_cfg));
    ESP_LOGI(MYTAG, "init finished");
    /*3. wifi start*/
    ESP_ERROR_CHECK(esp_wifi_start());
}

void app_main(void)
{
    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    how2startwifi();
}