#include "sdkconfig.h"

#include <stdio.h>
#include "stdlib.h"
#include <string.h>
#include <stdint.h>
#include <stddef.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "network_common.h"
#include "grideye_api_lv1.h"

static const char *TAG = "debug";
esp_mqtt_client_handle_t client = NULL;
static TaskHandle_t task_publish = NULL;

short pixel_value[SNR_SZ];
short thms_value;

void pixel_to_string(short *raw_temp, char *buf)
{
    int index = 0;
    for (int i = 0; i < SNR_SZ; i++)
    {
        index += sprintf(&buf[index], "%d", raw_temp[i]);
        if ((i+1) % SNR_SZ == 0)
        {
            ;
        }
        else
        {
            index += sprintf(&buf[index], ",");
        }
    }
}
void pub(void *parameter)
{
    const char *topic = "amg8833/pixels";
    const char *topic2 = "amg8833/thermistor";
    char pixel_msg_buf[400];
    char thms_msg_buf[20];
    while (1)
    {
        read_pixels(pixel_value);
        pixel_to_string(pixel_value, pixel_msg_buf);
        read_thermistor(&thms_value);
        sprintf(thms_msg_buf,"%d",thms_value);
        //print_pixels_to_serial(pixel_value,true);
        //printf("thms value: %.2f\n",(float)thms_value / CONVERT_FACTOR);
        mqtt_send(client, topic, pixel_msg_buf);
        mqtt_send(client,topic2,thms_msg_buf);
        printf("%d\n",uxTaskGetStackHighWaterMark(NULL));
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

void app_main(void)
{
    i2c_master_init();
    start_wifi();
    start_mqtt(&client);

    xTaskCreatePinnedToCore(pub, "pubpixels", 4000,
                            NULL, 1, task_publish, 0);
}