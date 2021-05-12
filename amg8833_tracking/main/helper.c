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

void blob_detection(short *raw, uint8_t *mask)
{
    short *im = malloc(sizeof(short)*IM_LEN);
    interpolation71x71(raw,im);
    short *avg = average_filter(im,IM_W,IM_H,35);
    short *ROI = malloc(sizeof(short)*IM_LEN);
    grayscale_thresholding(im,ROI,IM_LEN,avg,0);
    free(avg);
    int max = max_of_array(ROI,IM_LEN);
    int std = std_of_array(ROI,IM_LEN);
    short th = max - 2*std;
    binary_thresholding(ROI,mask,IM_LEN,&th,1);
    free(ROI);
    binary_fill_holes(mask,IM_W,IM_H);
    free(im);
    /*
    for(int i =0;i<IM_LEN;i++)
    {
        im[i] = im[i]*mask[i];
    }
    int min = min_of_array(im,IM_LEN);
    short *im2 = average_filter(im,IM_W,IM_H,9);
    free(im);
    binary_thresholding(im2,mask,IM_LEN,&min,1);
    free(im2);
    */
}