#include "cv.h"

static const char *TAG = "CV api";
int labeling8(uint8_t *mask, int width, int height)
{
    if((width>=256)||(height>=256)){
        ESP_LOGE(TAG,"ERROR: grideye api support image size up to uint8_t");
    }
    uint8_t mark = 1;
    uint16_t area_min = 10;
    uint16_t *search_list = malloc(sizeof(uint16_t)*(width*height)); 
    uint8_t num = ucAMG_PUB_ODT_CalcDataLabeling8((uint8_t)width,(uint8_t)height,mark,area_min,mask,search_list);
    free(search_list);
    return num;
}

short *average_filter(short *image,int width, int height, int side)
{
    short *result = malloc(sizeof(short)*(width*height));
    unsigned int *sum_table = malloc(sizeof(int)*(width*height));
    summed_area_table(image,sum_table,width,height);
    average_of_area(sum_table,result,width,height,side);
    free(sum_table);
    return result;
}

void binary_fill_holes(uint8_t *mask,int width, int height)
{
    uint8_t *holemask = malloc((width*height)*sizeof(uint8_t));
    binary_extract_holes(mask,holemask,width,height);
    for(int i =0;i<(width*height);i++)
    {
        mask[i] = (holemask[i] | mask[i]);
    }
    free(holemask);
}

short *discrete_convolution_2d_seperable(short *image,int width,int height,Filter *fx, Filter *fy)
{
    short *holder = malloc(sizeof(short)*(width*height));
    short *result = malloc(sizeof(short)*(width*height));
    convolution_x(image,holder,width,height,fx);
    convolution_y(holder,result,width,height,fy);
    free(holder);
    return result;
}

