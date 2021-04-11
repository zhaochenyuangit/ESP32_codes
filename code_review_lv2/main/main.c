#include <stdio.h>
#include "stdlib.h"
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"
#include "esp32/rom/uart.h"
#include "sdkconfig.h"

#include "grideye_api_lv2_sample.c"
ULONG g_ulFrameNum = 0;
short g_a2shRawTemp[TEMP_FRAME_NUM][SNR_SZ];
short g_ashSnrAveTemp[SNR_SZ];
short g_ashAveTemp[IMG_SZ];
short g_ashBackTemp[IMG_SZ];
short g_ashDiffTemp[IMG_SZ];
UCHAR g_aucDetectImg[IMG_SZ];
UCHAR g_aucOutputImg[OUT_SZ];
USHORT g_ausWork[IMG_SZ];

//static TaskHandle_t task_detect;

void app_main(void)
{
    int printCnt;
    if (i2c_master_init()==ESP_OK)
    {
        printf("i2c master initialized\n");
    }
    bAMG_PUB_SMP_InitializeHumanDetectLv2Sample();
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    while (1)
    {
        printf("\nframe number: %ld\n",g_ulFrameNum);
        if (bAMG_PUB_SMP_ExecuteHumanDetectLv2Sample())
        {
            for (printCnt = 1; printCnt <= IMG_SZ; printCnt++)
            {
                if (printCnt % IMG_SZ_X != 0)
                {
                    printf("%i,",g_aucDetectImg[printCnt - 1]);
                }
                else
                {
                    printf("%i\n", g_aucDetectImg[printCnt - 1]);
                }
            }
        }
        vTaskDelay(90 / portTICK_PERIOD_MS);
    }
    //xTaskCreatePinnedToCore(read_serial, "readAlloc", 4000, NULL, 1, task1, 0);
}