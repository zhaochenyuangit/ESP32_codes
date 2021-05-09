#include "cv.h"
#include <stdio.h>
#include "stdlib.h"
#include <string.h>
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
    f->weight = NULL;
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
    int *weight = malloc(sizeof(int) * l);
    if (weight == NULL)
    {
        free(kernel);
        free(f);
        return NULL;
    }
    double sigma2 = sigma * sigma;
    int double_to_int_factor = 4 * radius;
    for (int x = -radius; x <= radius; x++)
    {
        int ref = x + radius;
        double value = exp(-0.5 * (x * x) / sigma2);
        value *= double_to_int_factor;
        kernel[ref] = round(value);
    }
    for (int i = 0; i <= radius; i++)
    {
        int weight_sum = 0;
        for (int x = 0; x <= radius + i; x++)
        {
            weight_sum += kernel[x];
        }
        weight[i] = weight_sum;
        weight[2 * radius - i] = weight_sum;
    }
    f->side = l;
    f->kernel = kernel;
    f->weight = weight;
    return f;
}

struct Filter *avg_kern1d(int side){
    if(side%2==0){
        side = side-1;
        printf("update avg kernel side to %d\n",side);
    }
    struct Filter *f =malloc(sizeof(struct Filter));
    if(f==NULL){
        return NULL;
    }
    int *vec = malloc(sizeof(int)*side);
    if(vec ==NULL){
        free(f);
        return NULL;
    }
    int *weight = malloc(sizeof(int)*side);
    if(weight ==NULL){
        free(f);
        free(vec);
        return NULL;
    }
    int radius = (side-1)/2;
    for(int i=0;i<=radius;i++){
        vec[i] =1;
        vec[2*radius-i] = 1;
        weight[i] = radius+i+1;
        weight[2*radius-i] = radius+i+1;
    }
    f->side = side;
    f->kernel = vec;
    f->weight = weight;
    return f;
}

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

void convolution_x(short *image, short *output, int image_width, int image_height, struct Filter *f1d)
{
    int radius = (f1d->side - 1) / 2;
    for (int row = 0; row < image_height; row++)
    {
        for (int col = 0; col < image_width; col++)
        {
            int index  = row * image_width + col;
            unsigned int result = 0;
            for (int x = -radius; x <= radius; x++)
            {
                bool out_of_boundary = ((col + x) < 0) || ((col + x) >= image_width);
                if (out_of_boundary)
                    continue;
                result += image[index + x] * f1d->kernel[x + radius];
            }
            if (col < radius)
            {
                result /= f1d->weight[col];
            }
            else if (col > (image_width - radius - 1))
            {
                result /= f1d->weight[image_width - col - 1];
            }
            else
            {
                result /= f1d->weight[radius];
            }

            output[index] = result;
        }
    }
}

void convolution_y(short *image, short *output, int image_width, int image_height, struct Filter *f1d)
{
    int radius = (f1d->side - 1) / 2;
    for (int row = 0; row < image_height; row++)
    {
        for (int col = 0; col < image_width; col++)
        {
            int index = row * image_width + col;
            unsigned int result = 0;
            for (int x = -radius; x <= radius; x++)
            {
                bool out_of_boundary = ((row + x) < 0) || ((row + x) >= image_height);
                if (out_of_boundary)
                    continue;
                result += image[index + x * image_width] * f1d->kernel[x + radius];
            }
            if (row < radius)
            {
                result /= f1d->weight[row];
            }
            else if (row > (image_height - radius - 1))
            {
                result /= f1d->weight[image_height - row - 1];
            }
            else
            {
                result /= f1d->weight[radius];
            }

            output[index] = result;
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

void summed_area_table(short *image, unsigned int *output, int image_width, int image_height)
{
    output[0] = image[0];
    for(int col = 1;col<image_width;col++){
        output[col] = image[col] + output[col-1]; 
    }
    for(int row=1;row<image_height;row++){
        output[row*image_width] = image[row*image_width] + output[(row-1)*image_width];
    }
    for(int row =1;row<image_height;row++){
        for(int col=1;col<image_width;col++){
            int index = row*image_width + col;
            int left = index -1;
            int up = index-image_width;
            int upleft = up -1;
            output[index] = image[index] + output[up] + output[left] - output[upleft]; 
        }
    }
}

void average_of_area(unsigned int *sum_table, short *output, int image_width,int image_height,int side)
{
    if(side%2==0){
        side = side-1;
        printf("avg from sum table, update side to %d\n",side);
    }
    int radius = (side-1)/2;
    for(int row=0;row<image_height;row++){
        int north = (row - radius-1)<0?-1:(row-radius-1);
        int south = (row + radius)>=image_height? (image_height-1): (row+radius);
        for(int col=0;col<image_width;col++){
            int west = (col - radius-1)<0?-1:(col-radius-1);
            int east = (col + radius)>=image_width? (image_width-1):(col+radius);

            unsigned int nw = ((north<0)||(west<0))?0:sum_table[north*image_width + west];
            unsigned int ne = (north<0)?0:sum_table[north*image_width + east];
            unsigned int sw = (west<0)?0:sum_table[south*image_width + west];
            unsigned int se = sum_table[south*image_width + east];
            unsigned int avg = se - sw - ne + nw;
            avg /= (south-north) * (east-west);
            output[row*image_width+col] = avg;
        }
    }
}

void binary_extract_holes(uint8_t *mask, uint8_t *outmask, int width, int height)
{
    int im_addborder_length = (width+2)*(height+2);
    uint8_t holder[im_addborder_length];
    for(int i =0;i<im_addborder_length;i++){
        holder[i]=0;
    }
    for (int row =0;row<height;row++){
        for (int col=0;col<width;col++){
            int index_in_holder = (row+1)*(width+2) + (col+1);
            holder[index_in_holder] = mask[row*width + col];
        }
    }
    {
    uint8_t bg_search_list[im_addborder_length];
    bg_search_list[0] = 0; //set flood seed to upper left corner in padded mask
    int bg_counted = 0;
    int bg_tocount = 1;
    while(bg_counted<bg_tocount){
        int row = bg_search_list[bg_counted] / (width+2);
        int col = bg_search_list[bg_counted] % (width+2);
        if((row-1)>=0){
            int index_up = (row-1)*(width+2) +col;
            if(holder[index_up]==0){
                holder[index_up] = 1;
                bg_search_list[bg_tocount] = index_up;
                bg_tocount++;
            }
        } 
        if((row+1)<(height+2)){
            int index_down = (row+1)*(width+2) +col;
            if(holder[index_down]==0){
                holder[index_down]=1;
                bg_search_list[bg_tocount] = index_down;
                bg_tocount++;
            }
        }
        if((col-1)>=0){
            int index_left = row*(width+2) + (col -1);
            if(holder[index_left]==0){
                holder[index_left]=1;
                bg_search_list[bg_tocount] = index_left;
                bg_tocount++;
            }
        }
        if((col+1)<(width+2)){
            int index_right = row*(width+2) +(col+1);
            if(holder[index_right]==0){
                holder[index_right]=1;
                bg_search_list[bg_tocount] = index_right;
                bg_tocount++;
            }
        }
        bg_counted++;
    }
    }
    for(int row=0;row<height;row++){
        for(int col=0;col<width;col++){
            int index = row*width + col;
            int index_in_holder = (row+1)*(width+2) + (col+1);
            outmask[index] = (holder[index_in_holder] ^ (uint8_t)1)|(mask[index]);
        }
    }

}