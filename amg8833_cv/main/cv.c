#include "grideye_api_common.h"
#include "freertos/FreeRTOS.h"
#include "math.h"

struct Filter
{
    int *kernel;
    int side;
    int weight;
};

struct Filter *gaussian_kernel_2d(double sigma)
{
    struct Filter *f = malloc(sizeof(struct Filter));
    if (f == NULL)
    {
        return NULL;
    }
    int l = round((3 * sigma) * 2);
    if (l % 2 == 0)
    {
        l = l - 1;
    }
    int radius = (l - 1) / 2;
    int center = radius * l + radius;
    int *kernel = malloc(sizeof(int) * (l * l));
    if (kernel == NULL)
    {
        free(f);
        return NULL;
    }
    for (int y = -radius; y <= radius; y++)
    {
        for (int x = -radius; x <= radius; x++)
        {
            int ref = y * l + x + center;
            double value = exp(-0.5 * (x * x + y * y) / (sigma * sigma));
            value *= 256;
            kernel[ref] = round(value);
        }
    }
    int sum = 0;
    for (int i = 0; i < l * l; i++)
    {
        sum += kernel[i];
    }
    f->side = l;
    f->kernel = kernel;
    f->weight = sum;
    return f;
}

struct Filter *gkern_1d(double sigma)
{
    struct Filter *f = malloc(sizeof(struct Filter));
    if (f == NULL)
    {
        return NULL;
    }
    int l = round(2 * 3 * sigma);
    if (l % 2 == 0)
    {
        l = l - 1;
    }
    int radius = (l - 1) / 2;
    int *kernel = malloc(sizeof(int) * l);
    if (kernel == NULL)
    {
        free(f);
        return NULL;
    }
    double sigma2 = sigma * sigma;
    for (int x = -radius; x <= radius; x++)
    {
        int ref = x + radius;
        double value = exp(-0.5 * (x * x) / sigma2);
        value *= 4*radius;
        kernel[ref] = round(value);
    }

    f->side = l;
    f->kernel = kernel;
    f->weight = 1;
}

struct Filter mean10 = {
    .side = 10,
    .kernel = (int[100]){1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                         1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                         1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                         1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                         1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                         1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                         1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                         1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                         1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                         1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    .weight = 100,
};

struct Filter g1_manual = {
    .side = 5,
    .kernel = (int[]){1, 4, 7, 4, 1,
                      4, 16, 26, 16, 4,
                      7, 26, 41, 26, 7,
                      4, 16, 26, 16, 4,
                      1, 4, 7, 4, 1},
    .weight = 273,
};
typedef int (*pool_function_t)(short[], int);

int max_of_array(short *array, int size)
{
    int loc = 0;
    for (int index = 1; index < size; index++)
    {
        if (array[index] > array[loc])
        {
            loc = index;
        }
    }
    return array[loc];
}

int min_of_array(short *array, int size)
{
    int loc = 0;
    for (int index = 1; index < size; index++)
    {
        if (array[index] < array[loc])
        {
            loc = index;
        }
    }
    return array[loc];
}

int avg_of_array(short *array, int size)
{
    int sum = 0;
    int count = 0;
    for (int index = 0; index < size; index++)
    {
        if (array[index] == 0)
            continue;
        sum += array[index];
        count++;
    }
    return sum / count;
}

int std_of_array(short *array, int size)
{
    int avg = avg_of_array(array, size);
    int var = 0;
    int count = 0;
    for (int index = 0; index < size; index++)
    {
        if (array[index] == 0)
            continue;
        // do not use pow in loop, too slow
        var += (array[index] - avg) * (array[index] - avg);
        count++;
    }
    var /= count;
    return (int)sqrt(var);
}

void interpolation71x71(short *input8x8, short *output71x71)
{
    /*insert 9 pixels between each pixel*/
    const uint8_t w = SNR_SZ_X;
    const uint8_t h = SNR_SZ_Y;

    uint16_t index;
    uint8_t col = 0;
    uint8_t row = 0;
    for (row = 0; row < h; row++)
    {
        for (col = 0; col < w; col++)
        {
            index = 10 * row * 71 + 10 * col;
            output71x71[index] = input8x8[row * w + col];
        }
        for (col = 0; col < w - 1; col++)
        {
            short left = input8x8[row * w + col];
            short right = input8x8[row * w + col + 1];
            short diff = right - left;
            for (int newcol = 1; newcol <= 9; newcol++)
            {
                index = 10 * row * 71 + 10 * col + newcol;
                output71x71[index] = left + (diff * newcol / 10);
            }
        }
    }
    for (row = 0; row < h - 1; row++)
    {
        for (col = 0; col < 71; col++)
        {
            short up = output71x71[10 * row * 71 + col];
            short down = output71x71[10 * (row + 1) * 71 + col];
            short diff = down - up;
            for (int newrow = 1; newrow <= 9; newrow++)
            {
                index = (10 * row + newrow) * 71 + col;
                output71x71[index] = up + (diff * newrow / 10);
            }
        }
    }
}

void discrete_convolution_2d(short *image, short *output, int image_width, int image_height, struct Filter *filter, int step)
{
    assert(((image_width - 1) / step) % 1 == 0);
    assert(((image_height - 1) / step) % 1 == 0);

    int output_index = 0;
    int radius = (filter->side - 1) / 2;
    int center = radius * filter->side + radius;
    for (int row = 0; row < image_height; row += step)
    {
        for (int col = 0; col < image_width; col += step)
        {
            int index = row * image_width + col;
            unsigned int intermediate_sum = 0;
            int weight = 0;
            for (int ref = 0; ref < (filter->side * filter->side); ref++)
            {
                /*if (filter.kernel[ref] == 0)
                {
                    continue;
                }*/
                int row_shift = (ref - center) / filter->side;
                int col_shift = (ref - center) % filter->side;
                if (col_shift < -radius)
                {
                    row_shift = row_shift - 1;
                    col_shift = col_shift + filter->side;
                }
                else if (col_shift > radius)
                {
                    row_shift = row_shift + 1;
                    col_shift = col_shift - filter->side;
                }
                bool out_of_bondary;
                out_of_bondary = ((row + row_shift) < 0) ||
                                 ((row + row_shift) >= image_height) ||
                                 ((col + col_shift) < 0) ||
                                 ((col + col_shift) >= image_width);
                if (out_of_bondary)
                {
                    continue;
                }
                int index_shift = row_shift * image_width + col_shift;
                intermediate_sum += image[index + index_shift] * filter->kernel[ref];
                weight += filter->kernel[ref];
            }
            intermediate_sum /= weight;
            output[output_index] = intermediate_sum;
            output_index++;
        }
    }
}

void pooling_2d(short *image, short *output, int image_width, int image_height, struct Filter *mask, pool_function_t fun, int step)
{
    assert(((image_width - 1) / step) % 1 == 0);
    assert(((image_height - 1) / step) % 1 == 0);

    int output_index = 0;
    int radius = (mask->side - 1) / 2;
    int center = radius * mask->side + radius;
    for (int row = 0; row < image_height; row += step)
    {
        for (int col = 0; col < image_width; col += step)
        {
            int index = row * image_width + col;
            short array[mask->side * mask->side];
            int count = 0;
            for (int ref = 0; ref < (mask->side * mask->side); ref++)
            {
                if (mask->kernel[ref] == 0)
                {
                    continue;
                }
                int row_shift = (ref - center) / mask->side;
                int col_shift = (ref - center) % mask->side;
                if (col_shift < -radius)
                {
                    row_shift = row_shift - 1;
                    col_shift = col_shift + mask->side;
                }
                else if (col_shift > radius)
                {
                    row_shift = row_shift + 1;
                    col_shift = col_shift - mask->side;
                }
                bool out_of_bondary;
                out_of_bondary = ((row + row_shift) < 0) ||
                                 ((row + row_shift) >= image_height) ||
                                 ((col + col_shift) < 0) ||
                                 ((col + col_shift) >= image_width);
                if (out_of_bondary)
                {
                    continue;
                }
                int index_shift = row_shift * image_width + col_shift;
                array[count] = image[index + index_shift];
                count++;
            }
            output[output_index] = (short)fun(array, count);
            output_index++;
        }
    }
}

void thresholding(short *image, short *output, int image_size, short *threshold, bool broadcast, bool binary)
{
    for (int index = 0; index < image_size; index++)
    {
        output[index] = 0;
    }
    if (broadcast)
    {
        short th = threshold[0];
        for (int index = 0; index < image_size; index++)
        {
            if (image[index] < th)
                continue;
            output[index] = image[index];
        }
    }
    else
    {
        for (int index = 0; index < image_size; index++)
        {
            if (image[index] < threshold[index])
                continue;
            output[index] = image[index];
        }
    }
    if (binary)
    {
        for (int index = 0; index < image_size; index++)
        {
            output[index] = (output[index] != 0);
        }
    }
}