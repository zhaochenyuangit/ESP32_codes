#include <stdio.h>
#include "stdlib.h"
#include <string.h>
#include <stdint.h>
#include <stddef.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "sdkconfig.h"

#include "network_common.h"
#include "grideye_api_lv1.h"
#include "cv.h"
#include "sim_uart.h"

#define UART_SIM 1
#define IM_W 71
#define IM_H 71
#define IM_LEN (IM_W*IM_H)

static const char *TAG = "debug";
esp_mqtt_client_handle_t client = NULL;
xQueueHandle q_pixels = NULL;
static xQueueHandle q_thms = NULL;
static xQueueHandle q_mask = NULL;
static const uint8_t q_len = 3;

#include "helper.c"

void pub_msg(void *parameter)
{
    short pixel_value[SNR_SZ];
    for (int i =0;i<SNR_SZ;i++){
        pixel_value[i]=0;
    }
    short thms_value;
    UCHAR mask[SNR_SZ];
    const char *topic = "amg8833/pixels";
    const char *topic2 = "amg8833/thermistor";
    const char *topic3 = "amg8833/mask";
    char pixel_msg_buf[400];
    char thms_msg_buf[20];
    char mask_msg_buf[130];
    while (1)
    {
        if (xQueueReceive(q_pixels, &pixel_value, 0) == pdTRUE)
        {
            array8x8_to_string(pixel_value, pixel_msg_buf);
            //print_pixels_to_serial(pixel_value, true);
            mqtt_send(client, topic, pixel_msg_buf);
        }
        if (xQueueReceive(q_thms, &thms_value, 0) == pdTRUE)
        {
            sprintf(thms_msg_buf, "%d", thms_value);
            //printf("thms value: %.2f\n", (float)thms_value / CONVERT_FACTOR);
            mqtt_send(client, topic2, thms_msg_buf);
        }
        if (xQueueReceive(q_mask, &mask, 0) == pdTRUE)
        {
            mask_to_string(mask,mask_msg_buf);
            //printf("%s\n",mask_msg_buf);
            mqtt_send(client, topic3, mask_msg_buf);
        }

        //printf("task pub watermark: %d\n", uxTaskGetStackHighWaterMark(NULL));
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

void read_grideye(void *parameter)
{
    short pixel_value[SNR_SZ];
    short thms_value;
    UCHAR mask[SNR_SZ];
    while (1)
    {
        read_pixels(pixel_value);
        read_thermistor(&thms_value);
        int count = detect_activation(pixel_value, thms_value, mask);
        if (count)
        {
            if (xQueueSend(q_pixels, &pixel_value, 10) != pdTRUE)
            {
                ESP_LOGI(TAG, "queue pixel is full");
            }
            if (xQueueSend(q_thms, &thms_value, 10) != pdTRUE)
            {
                ESP_LOGI(TAG, "queue thms is full");
            }
            if (xQueueSend(q_mask, &mask, 10) != pdTRUE)
            {
                ESP_LOGI(TAG, "queue mask is full");
            }
        }

        //printf("task read watermark: %d\n", uxTaskGetStackHighWaterMark(NULL));
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

void app_main(void)
{
    #ifdef UART_SIM
    printf("main programm start\n");
    uart_init();
    #endif
    #ifndef UART_SIM
    i2c_master_init();
    #endif
    start_wifi();
    start_mqtt(&client);
    q_pixels = xQueueCreate(q_len, sizeof(short[SNR_SZ]));
    q_thms = xQueueCreate(q_len, sizeof(short));
    q_mask = xQueueCreate(q_len, sizeof(UCHAR[SNR_SZ]));
    if (q_pixels == NULL)
    {
        ESP_LOGE(TAG, "create pixels queue failed");
    }
    if (q_thms == NULL)
    {
        ESP_LOGE(TAG, "create thermistor queue failed");
    }
    if (q_mask == NULL)
    {
        ESP_LOGE(TAG, "create mask queue failed");
    }
    #ifndef UART_SIM
    xTaskCreatePinnedToCore(read_grideye, "read_grideye", 3000,
                            NULL, 1, task_read, 1);
    #endif
    #ifdef UART_SIM
    xTaskCreatePinnedToCore(uart_event_task,"uart_event",4000,
                            (void *)q_pixels,5,NULL,1);
    #endif
    xTaskCreatePinnedToCore(pub_msg, "publish", 3000,
                            NULL, 1, NULL, 0);
}