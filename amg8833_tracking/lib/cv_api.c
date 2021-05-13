#include "cv.h"

static const char *TAG = "CV api";

void image_copy(short *src, short *dst, int size)
{
    for (int i = 0; i < size; i++)
    {
        dst[i] = src[i];
    }
}

void mask_copy(uint8_t *src, uint8_t *dst, int size)
{
    for (int i = 0; i < size; i++)
    {
        dst[i] = src[i];
    }
}

int labeling8(uint8_t *mask, int width, int height)
{
    if ((width > 256) || (height > 256))
    {
        ESP_LOGE(TAG, "ERROR: grideye api support image size up to uint8_t");
        return -1;
    }
    uint8_t mark = 1;
    uint16_t area_min = 10;
    uint16_t *search_list = malloc(sizeof(uint16_t) * (width * height));
    if (search_list == NULL)
    {
        return -1;
    }
    uint8_t num = ucAMG_PUB_ODT_CalcDataLabeling8((uint8_t)width, (uint8_t)height, mark, area_min, mask, search_list);
    free(search_list);
    if (num == 0)
    {
        return num;
    }
    if (num > 0)
    {
        /** 
         * blob number returned from Panasonic's API is add by 1
         * when not zero. Do not understand what is the purpose
         * so just quick fix it
         */
        return num - 1;
    }
    return num;
}

bool average_filter(short *image, int width, int height, int side)
{
    unsigned int *sum_table = malloc(sizeof(int) * (width * height));
    if (sum_table == NULL)
    {
        return 0;
    }
    summed_area_table(image, sum_table, width, height);
    average_of_area(sum_table, image, width, height, side);
    free(sum_table);
    return 1;
}

bool binary_fill_holes(uint8_t *mask, int width, int height)
{
    if (((width + 2) * (height + 2)) > 65536)
    {
        /** back ground search list is used to store pixel coordinates
         * it must be large enought to hold (width+2)*(height+2)
         * here it is a uint16_t list so max index is 65535
        */
        return 0;
    }
    uint16_t *search_list = malloc((width + 2) * (height + 2) * sizeof(uint16_t));
    if (search_list == NULL)
    {
        return 0;
    }
    uint8_t *padded = malloc((width + 2) * (height + 2) * sizeof(uint8_t));
    if (padded == NULL)
    {
        free(search_list);
        return 0;
    }
    uint8_t *holemask = malloc((width * height) * sizeof(uint8_t));
    if (holemask == NULL)
    {
        free(search_list);
        free(padded);
        return 0;
    }
    binary_extract_holes(mask, holemask, width, height, padded, search_list);
    for (int i = 0; i < (width * height); i++)
    {
        mask[i] = (holemask[i] | mask[i]);
    }
    free(holemask);
    free(padded);
    free(search_list);
    return 1;
}

bool discrete_convolution_2d_seperable(short *image, int width, int height, Filter *fx, Filter *fy)
{
    short *holder = malloc(sizeof(short) * (width * height));
    if (holder == NULL)
    {
        return 0;
    }
    convolution_x(image, holder, width, height, fx);
    convolution_y(holder, image, width, height, fy);
    free(holder);
    return 1;
}
