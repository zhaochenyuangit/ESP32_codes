#include "grideye_api_common.h"
#include "assert.h"
#include "math.h"

#define uint8_t unsigned char
#define uint16_t unsigned short
#define bool _Bool

struct Filter
{
    int *kernel;
    int side;
    int *weight;
};

typedef struct Filter Filter;
struct Filter *gaussian_kernel_2d(double sigma);
struct Filter *gkern_1d(double sigma);
struct Filter *avg_kern1d(int side);

typedef int (*pool_function_t)(short[], int);
int max_of_array(short *array, int size);
int min_of_array(short *array, int size);
int avg_of_array(short *array, int size);
int std_of_array(short *array, int size);

void interpolation71x71(short *input8x8, short *output71x71);
void discrete_convolution_2d(short *image, short *output, int image_width, int image_height, struct Filter *filter, int step);
void convolution_x(short *image, short *output, int image_width, int image_height, struct Filter *f1d);
void convolution_y(short *image, short *output, int image_width, int image_height, struct Filter *f1d);
void pooling_2d(short *image, short *output, int image_width, int image_height, struct Filter *mask, pool_function_t fun, int step);
void thresholding(short *image, short *output, int image_size, short *threshold, bool broadcast, bool binary);
void summed_area_table(short *image, unsigned int *output, int image_width, int image_height);
void average_of_area(unsigned int *sum_table, short *output, int image_width,int image_height,int side);

void binary_extract_holes(uint8_t *mask, uint8_t *output, int width, int height);