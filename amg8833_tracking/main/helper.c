#include "freertos/FreeRTOS.h"
#include "grideye_api_common.h"
#include <string.h>
void array8x8_to_string(short *raw_temp, char *buf)
{
    int index = 0;
    for (int i = 0; i < SNR_SZ; i++)
    {
        index += sprintf(&buf[index], "%d", raw_temp[i]);
        if ((i + 1) % SNR_SZ == 0)
        {
            ;
        }
        else
        {
            index += sprintf(&buf[index], ",");
        }
    }
}

int detect_activation(short *pixels, short thms, UCHAR *mask)
{
    int count = 0;
    memset(mask, 0, SNR_SZ);
    short low_b = thms - 2.5 * 256;
    for (int i = 0; i < SNR_SZ; i++)
    {
        if (pixels[i] > low_b)
        {
            mask[i] = pdTRUE;
            count++;
        }
    }
    return count;
}

void mask_to_string(UCHAR *mask, char *buf)
{
    int index = 0;
    for (int i =0;i<SNR_SZ;i++){
        index += sprintf(&buf[index],"%d",mask[i]);
        if ((i + 1) % SNR_SZ == 0)
        {
            ;
        }
        else
        {
            index += sprintf(&buf[index], ",");
        }
    }
}

void print_mask(UCHAR *mask)
{
    for (int i = 1; i <= SNR_SZ; i++)
    {
        {
            printf("%d", mask[i - 1]);
        }
        if (i != SNR_SZ)
            printf(", ");
        if (i % 8 == 0 && i != SNR_SZ)
            printf("...\n");
        if (i % 8 == 0 && i == SNR_SZ)
            printf("\n");
    }
    printf("\n");
}


