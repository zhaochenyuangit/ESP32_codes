#include <stdio.h>
#include "stdlib.h"
#include <string.h>
#include <sys/time.h>
#include "time.h"
#include <sys/time.h>
#include "time.h"

#include "grideye_api_common.h"
#include "cv.h"


#define IM_H (71)
#define IM_W (71)
#define IM_LEN (IM_H * IM_W)

short dummy_event[] = {4672, 4544, 4672, 4544, 4672, 4864, 4864, 4992,
                       4608, 4608, 4672, 4672, 4736, 4800, 5056, 5056,
                       4672, 4544, 4608, 4864, 4864, 4928, 4928, 4928,
                       4864, 5440, 5504, 5568, 5632, 5504, 5056, 5248,
                       4864, 6080, 5888, 5952, 5824, 5568, 4800, 4864,
                       4672, 5184, 5632, 5952, 6080, 5248, 4992, 4864,
                       4672, 4544, 4864, 4992, 4928, 4864, 4992, 4800,
                       4736, 4800, 4800, 4800, 4736, 4864, 4736, 4992};

short image_origin[IM_LEN];
short image_holder1[IM_LEN];
short image_holder2[IM_LEN];
short image_holder3[IM_LEN];
unsigned int sum_table[IM_LEN];
short result_2d[IM_LEN];
short result_1d[IM_LEN];
short result_tab[IM_LEN];
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

int main(void)
{
    struct Filter *g1d = gkern_1d(1.5);
    struct Filter *g1d3 = gkern_1d(3);
    struct Filter *g2d = gaussian_kernel_2d(3);
    struct Filter *avg20 = avg_kern1d(20);
    Filter avg9_manual=
{
    .side = 9,
    .kernel = (int[]){1,1,1,1,1,1,1,1,1,
                        1,1,1,1,1,1,1,1,1,
                        1,1,1,1,1,1,1,1,1,
                        1,1,1,1,1,1,1,1,1,
                        1,1,1,1,1,1,1,1,1,
                        1,1,1,1,1,1,1,1,1,
                        1,1,1,1,1,1,1,1,1,
                        1,1,1,1,1,1,1,1,1,
                        1,1,1,1,1,1,1,1,1},
    .weight = NULL,
};
    if ((g1d == NULL)||(g2d==NULL))
    {
        printf("alloc failed\n");
    }
    printf("image size: %dx%d\n",IM_W,IM_H);

    performance_evaluation(0);
    interpolation71x71(dummy_event, image_origin);
    printf("interpolation: %.2f ms\n", performance_evaluation(1));

    performance_evaluation(0);
    convolution_x(image_origin,image_holder1,IM_W,IM_H,g1d3);
    convolution_y(image_holder1,image_holder2,IM_W,IM_H,g1d3);
    printf("1d gauss oneshot: %.2f ms\n", performance_evaluation(1));

    performance_evaluation(0);
    discrete_convolution_2d(image_origin, result_2d, IM_W, IM_H, &avg9_manual, 1);
    printf("2d mean: %.2f ms\n", performance_evaluation(1));

    performance_evaluation(0);
    convolution_x(image_origin,image_holder1,IM_W,IM_H,avg20);
    convolution_y(image_holder1,result_1d,IM_W,IM_H,avg20);
    printf("mean filter: %.2f ms\n", performance_evaluation(1));

    performance_evaluation(0);
    summed_area_table(image_origin,sum_table,IM_W,IM_H);
    average_of_area(sum_table,result_tab,IM_W,IM_H,20);
    printf("sum table: %.2f ms\n",performance_evaluation(1));

    /*
    for(int x = 0;x<IM_W;x++){
        printf("%d,%d,%d\n",result_2d[x*IM_W],result_1d[x*IM_W], result_tab[x*IM_W]);
    }
    //*/
    ///*
    for (int i = 0; i < 5; i++)
    {
        for (int j = 0; j < IM_W; j++)
        {
            int index = i * IM_W + j;
            int diff = result_tab[index] - result_1d[index];
            if (abs(diff) > 1)
            {
                printf("%d,%d,diff is %d\n", i, j, diff);
            }
        }
    }
    //*/
    
    //print_pixels_to_serial_8x8(sample_out, false);
    return 0;
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