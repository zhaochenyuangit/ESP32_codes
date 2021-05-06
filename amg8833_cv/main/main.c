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

static const char *TAG = "debug";
esp_mqtt_client_handle_t client;
static xQueueHandle q71 = NULL;

enum
{
    len_image = 15 * 15
};
const int w = 15, h = 15;
short image71[len_image];
short image_holder1[len_image];
short image_holder2[len_image];
short thres_holder[len_image];

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

void debug_mqtt(void *_)
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
    struct timeval tik;
    struct timeval tok;
    int eclipsed_time_ms;

    struct Filter *g1 = gaussian_kernel(1);
    struct Filter *g3 = gaussian_kernel(3);
    struct Filter *g5 = gaussian_kernel(5);
    if ((g3 == NULL) || (g5 == NULL))
    {
        ESP_LOGW(TAG, "alloc failed");
    }

    gettimeofday(&tik, NULL); /*performance evaluation*/
    interpolation71x71(dummy_event, image71);
    gettimeofday(&tok, NULL); /*performance evaluation*/
    eclipsed_time_ms = (tok.tv_sec - tik.tv_sec) * 1000 + (tok.tv_usec - tik.tv_usec) / 1000;
    printf("eclipsed time: %d ms\n", eclipsed_time_ms);

    gettimeofday(&tik, NULL); /*performance evaluation*/
    discrete_convolution_2d(image71, image_holder1, w, h, g3, 1);
    gettimeofday(&tok, NULL); /*performance evaluation*/
    eclipsed_time_ms = (tok.tv_sec - tik.tv_sec) * 1000 + (tok.tv_usec - tik.tv_usec) / 1000;
    printf("eclipsed time: %d ms\n", eclipsed_time_ms);

    gettimeofday(&tik, NULL); /*performance evaluation*/
    thresholding(image71, image_holder2, len_image, image_holder1, 0, 0);
    gettimeofday(&tok, NULL); /*performance evaluation*/
    eclipsed_time_ms = (tok.tv_sec - tik.tv_sec) * 1000 + (tok.tv_usec - tik.tv_usec) / 1000;
    printf("eclipsed time: %d ms\n", eclipsed_time_ms);

    gettimeofday(&tik, NULL); /*performance evaluation*/
    int max = max_of_array(image_holder2, len_image);
    int std = std_of_array(image_holder2, len_image);
    short th = (max - 3 * std) > 0 ? (max - 3 * std) : 0;
    gettimeofday(&tok, NULL); /*performance evaluation*/
    eclipsed_time_ms = (tok.tv_sec - tik.tv_sec) * 1000 + (tok.tv_usec - tik.tv_usec) / 1000;
    printf("eclipsed time: %d ms\n", eclipsed_time_ms);

    //thresholding(image_holder2, image_holder1, len_image, &th, 1, 0);
    gettimeofday(&tik, NULL); /*performance evaluation*/
    pooling_2d(image_holder1, image_holder2, w, h, g1, max_of_array, 1);
    gettimeofday(&tok, NULL); /*performance evaluation*/
    eclipsed_time_ms = (tok.tv_sec - tik.tv_sec) * 1000 + (tok.tv_usec - tik.tv_usec) / 1000;
    printf("eclipsed time: %d ms\n", eclipsed_time_ms);

    //pooling_2d(image_holder2, image_holder1, w, h, &mean10, min_of_array, 1);
    //discrete_convolution_2d(image_holder1, image_holder2, w, h, g3, 1);
    //thresholding(image_holder2, image_holder1, len_image, &th, 1, 1);
    //pooling_2d(image_holder1, image_holder2, w, h, &mean10, min_of_array, 1);
    //pooling_2d(image_holder2, image_holder1, w, h, &mean10, max_of_array, 1);
    //pooling_2d(sample_out, dummy_out, w, h, &mean10, max_of_array, 1);

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
//struct Filter *g1 = gaussian_kernel(1);
//struct Filter *g3 = gaussian_kernel(3);
//struct Filter *g12 = gaussian_kernel(12);
/*if ((g3 == NULL) || (g12 == NULL))
    {
        ESP_LOGW(TAG, "alloc failed");
    }*/

/*
    thresholding(image71, image_holder2, len_image, image_holder1, 0, 0);
    int max = max_of_array(image_holder2, len_image);
    int std = std_of_array(image_holder2, len_image);
    short th = (max - 3 * std) > 0 ? (max - 3 * std) : 0;
    thresholding(image_holder2, image_holder1, len_image, &th, 1, 0);
    pooling_2d(image_holder1, image_holder2, 71, 71, &mean10, max_of_array, 1);
    pooling_2d(image_holder2, image_holder1, 71, 71, &mean10, min_of_array, 1);
    discrete_convolution_2d(image_holder1, image_holder2, 71, 71, g3, 1);
    thresholding(image_holder2, image_holder1, len_image, &th, 1, 1);
    pooling_2d(image_holder1, image_holder2, 71, 71, &mean10, min_of_array, 1);
    pooling_2d(image_holder2, image_holder1, 71, 71, &mean10, max_of_array, 1);

    discrete_convolution_2d(image_holder1, image_holder2, 71, 71, g3, 10);
    */
/*if (xQueueSend(q71, &sample_out, 20) == pdFALSE)
    {
        ESP_LOGW(TAG, "queue sending failed");
    }*/