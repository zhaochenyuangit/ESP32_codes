#include <stdio.h>
#include "stdlib.h"
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"
#include "driver/i2c.h"
#include "esp32/rom/uart.h"
#include "sdkconfig.h"
#include "I2C.h"
#include "grideye_api_lv1.h"

/* Grid-EYE's I2C slave address */
//#define		GRIDEYE_ADR_GND		(0xD0)	/* AD_SELECT pin connect to GND		*/
//#define		GRIDEYE_ADR_VDD		0x69 //(0xD2)	/* AD_SELECT pin connect to VDD		*/
#define GRIDEYE_ADR (0x69)

/* Grid-EYE's register address */
#define GRIDEYE_REG_THS00 (0x0E) /* head address of thermistor  resister	*/
#define GRIDEYE_REG_TMP00 (0x80) /* head address of temperature resister	*/

/* Grid-EYE's register size */
#define GRIDEYE_REGSZ_THS (0x02) /* size of thermistor  resister		*/
#define GRIDEYE_REGSZ_TMP (0x80) /* size of temperature resister		*/

/* Grid-EYE's number of pixels */
#define SNR_SZ (64)

/*data precision*/
#define GRIDEYE_PRECISION_THS .0625
#define GRIDEYE_PRECISION_TMP .25

/*******************************************************************************
	variable value definition
*******************************************************************************/
short g_shThsTemp;          /* thermistor temperature			*/
short g_ashRawTemp[SNR_SZ]; /* temperature of 64 pixels			*/

BOOL bReadTempFromGridEYE(void)
{
    UCHAR aucThsBuf[GRIDEYE_REGSZ_THS];
    UCHAR aucTmpBuf[GRIDEYE_REGSZ_TMP];

    /* Get thermistor register value. */
    if (ESP_FAIL == bAMG_PUB_I2C_Read(GRIDEYE_ADR, GRIDEYE_REG_THS00, GRIDEYE_REGSZ_THS, aucThsBuf))
    {
        return (ESP_FAIL);
    }

    /* Convert thermistor register value. */
    g_shThsTemp = shAMG_PUB_TMP_ConvThermistor(aucThsBuf);

    /* Get temperature register value. */
    if (ESP_FAIL == bAMG_PUB_I2C_Read(GRIDEYE_ADR, GRIDEYE_REG_TMP00, GRIDEYE_REGSZ_TMP, aucTmpBuf))
    {
        return (ESP_FAIL);
    }

    /* Convert temperature register value. */
    vAMG_PUB_TMP_ConvTemperature64(aucTmpBuf, g_ashRawTemp);

    return (ESP_OK);
}
void app_main(void)
{
    i2c_master_init();
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    printf("Starting task loop...\n");
    while (true)
    {
        bReadTempFromGridEYE();
        //printf("thermistor temperature: %.2f", ((float)g_shThsTemp * GRIDEYE_PRECISION_THS));
        printf("[\n");
        for (int i = 1; i <= SNR_SZ; i++)
        {
            printf("%.2f", ((float)g_ashRawTemp[i - 1] * GRIDEYE_PRECISION_TMP));
            if (i != SNR_SZ)
                printf(", ");
            if (i % 8 == 0 && i != SNR_SZ)
                printf("\n");
            if (i % 8 == 0 && i == SNR_SZ)
                printf("\n]");
        }
        printf("\n");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}