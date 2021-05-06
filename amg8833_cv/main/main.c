#include <stdio.h>
#include "stdlib.h"
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "driver/gpio.h"
#include "esp32/rom/uart.h"
#include "sdkconfig.h"
#include <sys/time.h>
#include "time.h"
#include "esp_log.h"

#include "network_common.h"
#include "cv.c"
#include "dummy.c"

#define IM_H (71)
#define IM_W (71)
#define IM_LEN (IM_H * IM_W)
static const char *TAG = "debug";
esp_mqtt_client_handle_t client;
static xQueueHandle q71 = NULL;

short image_origin[IM_LEN];
short image_holder1[IM_LEN];
short image_holder2[IM_LEN];
short image_holder3[IM_LEN];
short thres_holder[IM_LEN];

void array_to_string_71(short *raw_temp, char *buf)
{
    int index = 0;
    for (int i = 0; i < (71 * 71); i++)
    {
        index += sprintf(&buf[index], "%d", raw_temp[i]);
        if ((i + 1) % (71 * 71) == 0)
        {
            ;
        }
        else
        {
            index += sprintf(&buf[index], ",");
        }
    }
}
void print_pixels_to_serial_8x8(short *raw_temp, bool print_float)
{
    printf("[\n");
    for (int i = 1; i <= 64; i++)
    {
        if (print_float)
        {
            printf("%.2f", ((float)raw_temp[i - 1] / CONVERT_FACTOR));
        }
        else
        {
            printf("%d", raw_temp[i - 1]);
        }
        if (i != SNR_SZ)
            printf(", ");
        if (i % 8 == 0 && i != SNR_SZ)
            printf("\n");
        if (i % 8 == 0 && i == SNR_SZ)
            printf("\n]");
    }
    printf("\n");
}
void debug_mqtt(void *q71)
{
    short image71[71 * 71];
    char topic[] = "amg8833/image71";
    char buf[25600];
    while (1)
    {
        if (xQueueReceive(q71, &image71, 0) == pdTRUE)
        {
            array_to_string_71(image71, buf);
            mqtt_send(client, topic, buf);
            ESP_LOGI(TAG, "mqtt sent");
        }
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}
void app_main(void)
{
    struct Filter *g31d = gkern_1d(12);
    struct Filter *g32d = gaussian_kernel_2d(12);

    if ((g31d == NULL))
    {
        ESP_LOGW(TAG, "alloc failed");
    }

    performance_evaluation(0);
    interpolation71x71(dummy_event, image_origin);
    printf("interpolation: %d ms\n", performance_evaluation(1));

    performance_evaluation(0);
    discrete_convolution_2d(image_origin, image_holder3, IM_W, IM_H, g32d, 1);
    printf("2d gauss: %d ms\n", performance_evaluation(1));

    performance_evaluation(0);
    convolution_x(image_origin, image_holder1, IM_W, IM_H, g31d);
    convolution_y(image_holder1, image_holder2, IM_W, IM_H, g31d);
    printf("1d gauss: %d ms\n", performance_evaluation(1));

    for (int i = 0; i < IM_H; i++)
    {
        for (int j = 0; j < IM_W; j++)
        {
            int index = i * IM_W + j;
            int diff = image_holder2[index] - image_holder3[index];
            if (abs(diff) > 5)
            {
                printf("%d,%d,diff is %d\n", i, j, diff);
            }
        }
    }

    //print_pixels_to_serial_8x8(sample_out, false);
}

//start_wifi();
//start_mqtt(&client);
/*
    q71 = xQueueCreate(3, sizeof(short[71 * 71]));
    if (q71 == NULL)
    {
        ESP_LOGW(TAG, "create queue failed");
    }*/

//xTaskCreatePinnedToCore(debug_mqtt, "mqtt", 40000, NULL, 5, NULL, 0);