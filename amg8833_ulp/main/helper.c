#include "main.h"

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
    for (int i = 0; i < SNR_SZ; i++)
    {
        if (pixels[i] > thms-2*256)
        {
            mask[i] = pdTRUE;
            count++;
        }
    }
    return count;
}


void array71x71_to_string(short *im, char *buf)
{
    int index = 0;
    for (int i =0;i<IM_LEN;i++){
        index += sprintf(&buf[index],"%d",im[i]);
        if ((i + 1) % IM_LEN == 0)
        {
            ;
        }
        else
        {
            index += sprintf(&buf[index], ",");
        }
    }
}

void mask71x71_to_string(UCHAR *mask, char *buf)
{
    int index = 0;
    for (int i =0;i<IM_LEN;i++){
        index += sprintf(&buf[index],"%d",mask[i]);
        if ((i + 1) % IM_LEN == 0)
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

/*0 is start, 1 is end*/
double performance_evaluation(int start_or_end)
{
    static struct timeval tik, tok;
    if (start_or_end)
    {
        gettimeofday(&tok, NULL);
        double eclipsed_time_ms = (tok.tv_sec - tik.tv_sec) * 1000 + (tok.tv_usec - tik.tv_usec) / 1000.0;
        return eclipsed_time_ms;
    }
    else
    {
        gettimeofday(&tik, NULL);
        return -1;
    }
}
