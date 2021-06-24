#include "main.h"
#include "ulp_main.h"

extern const uint8_t ulp_main_bin_start[] asm("_binary_ulp_main_bin_start");
extern const uint8_t ulp_main_bin_end[] asm("_binary_ulp_main_bin_end");

const gpio_num_t gpio_led = GPIO_NUM_2;
const gpio_num_t gpio_scl = GPIO_NUM_32;
const gpio_num_t gpio_sda = GPIO_NUM_33;

static const char *TAG = "debug";
esp_mqtt_client_handle_t client = NULL;
xQueueHandle q_pixels = NULL;
xQueueHandle q_thms = NULL;
xQueueHandle q_speed = NULL;
static const uint8_t q_len = 30;
SemaphoreHandle_t sema_raw = NULL;
SemaphoreHandle_t sema_im = NULL;
TimerHandle_t sleep_alarm = NULL;

char pixel_msg_buf[400];
char thms_msg_buf[20];
char mask_msg_buf[26000];

short im[IM_LEN];
uint8_t mask[IM_LEN];
static short holder1[IM_LEN];
static short holder2[IM_LEN];

void blob_detection(short *raw, uint8_t *result);
void image_process(void *_);
void pub_raw(void *_);
void pub_im(void *_);
void pub_thms(void *_);
void read_grideye(void *parameter);
static void go_to_sleep(TimerHandle_t xTimer);
static void init_ulp_program(void);
#include "helper.c"


void app_main(void)
{
    esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
    switch (cause)
    {
    case ESP_SLEEP_WAKEUP_UNDEFINED:
        printf("Not ULP wakeup, now initializing ULP\n");
        init_ulp_program();
        go_to_sleep(NULL);
        break;
    case ESP_SLEEP_WAKEUP_ULP:
        printf("ULP wakeup, printing status\n");
        printf("%d,%d,%d\n", (ulp_thms & 0xffff), (ulp_temp & 0xffff), (ulp_debug & 0xffff));
        break;
    case ESP_SLEEP_WAKEUP_TIMER:
        printf("...timer\n");
        break;
    default:
        printf("unhandled event%d\n", cause);
        break;
    }
    sleep_alarm = xTimerCreate("sleep",6000/portTICK_PERIOD_MS,pdFALSE,0,go_to_sleep);
    xTimerStop(sleep_alarm,0);
#ifdef UART_SIM
    printf("main programm start\n");
    uart_init();
#else
    i2c_master_init();
    vTaskDelay(10);
#endif
    q_pixels = xQueueCreate(q_len, sizeof(short[SNR_SZ]));
    q_thms = xQueueCreate(q_len, sizeof(short));
    q_speed = xQueueCreate(q_len, sizeof(double));
    if (q_pixels == NULL)
    {
        ESP_LOGE(TAG, "create pixels queue failed");
    }
    if (q_thms == NULL)
    {
        ESP_LOGE(TAG, "create thms queue failed");
    }
    if (q_speed == NULL)
    {
        ESP_LOGE(TAG, "create performance queue failed");
    }
    sema_raw = xSemaphoreCreateBinary();
    sema_im = xSemaphoreCreateBinary();
    if (sema_raw == NULL)
    {
        ESP_LOGE(TAG, "create sema raw failed");
    }
    if (sema_im == NULL)
    {
        ESP_LOGE(TAG, "create sema im failed");
    }
#ifdef UART_SIM
    xTaskCreatePinnedToCore(uart_receive_pixels, "uart_event", 4000, (void *)q_pixels, 5, NULL, 1);
#else
    xTaskCreatePinnedToCore(read_grideye, "read_grideye", 3000, NULL, 1, NULL, 1);
    
#endif

    //performance_evaluation(0);
    //start_wifi();
    //start_mqtt(&client);
    //printf("net start time: %f",performance_evaluation(1));
    //vTaskDelay(10);
    //xTaskCreatePinnedToCore(pub_thms, "publish thms", 2000, NULL, 1, NULL, 0);
    //xTaskCreatePinnedToCore(pub_raw, "publish", 2000, NULL, 1, NULL, 0);
    //xTaskCreatePinnedToCore(pub_im, "publish im ", 2000, NULL, 1, NULL, 0);
    xTaskCreatePinnedToCore(image_process, "process", 4000, NULL, 2, NULL, 1);
    xTimerStart(sleep_alarm,0);
    
}

void blob_detection(short *raw, uint8_t *result)
{
    interpolation71x71(raw, im);
    image_copy(im, holder1, IM_LEN);
    average_filter(holder1, IM_W, IM_H, 35);
    grayscale_thresholding(im, holder2, IM_LEN, holder1, 0);
    int max = max_of_array(holder2, IM_LEN);
    int std = std_of_array(holder2, IM_LEN);
    short th = max - 2 * std;
    binary_thresholding(holder2, result, IM_LEN, &th, 1);
    binary_fill_holes(result, IM_W, IM_H);
    for (int i = 0; i < IM_LEN; i++)
    {
        im[i] = im[i] * result[i];
    }
    average_filter(im, IM_W, IM_H, 7);
    binary_thresholding(im, result, IM_LEN, &th, 1);
    int num = labeling8(result, IM_W, IM_H);
}

void image_process(void *_)
{
    short pixel_value[SNR_SZ];
    char performance_msg_buf[10];
    while (1)
    {
        if (xQueueReceive(q_pixels, &pixel_value, portMAX_DELAY) == pdTRUE)
        {
            performance_evaluation(0);
            array8x8_to_string(pixel_value, pixel_msg_buf);
            xSemaphoreGive(sema_raw);

            blob_detection(pixel_value, mask);
            //array71x71_to_string(holder1,mask_msg_buf);

            //mask71x71_to_string(mask, mask_msg_buf);
            //xSemaphoreGive(sema_im);

            sprintf(performance_msg_buf, "%.2f", performance_evaluation(1));
            printf("%s\n",performance_msg_buf);
            //mqtt_send(client, "amg8833/speed", performance_msg_buf);
        }
        //vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

void pub_raw(void *_)
{
    const char *topic = "amg8833/pixels";
    while (1)
    {
        if (xSemaphoreTake(sema_raw, portMAX_DELAY) == pdTRUE)
        {
            mqtt_send(client, topic, pixel_msg_buf);
        }
        //printf("task pub watermark: %d\n", uxTaskGetStackHighWaterMark(NULL));
        //vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

void pub_im(void *_)
{
    const char *topic = "amg8833/image71";
    while (1)
    {
        if (xSemaphoreTake(sema_im, portMAX_DELAY) == pdTRUE)
        {
            mqtt_send(client, topic, mask_msg_buf);
        }
        //printf("task pub watermark: %d\n", uxTaskGetStackHighWaterMark(NULL));
        //vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

#ifndef UART_SIM
void read_grideye(void *parameter)
{
    short pixel_value[SNR_SZ];
    short thms_value;
    while (1)
    {
        read_pixels(pixel_value);
        read_thermistor(&thms_value);
        int count = detect_activation(pixel_value, thms_value, mask);
        if (count)
        {
            if (xQueueSend(q_pixels, &pixel_value, 10) != pdTRUE)
            {
                ESP_LOGI(TAG, "queue pixel is full");
            }
            if (xQueueSend(q_thms, &thms_value, 10) != pdTRUE)
            {
                ESP_LOGI(TAG, "queue thms is full");
            }
            printf("waiting: %d\n",uxQueueMessagesWaiting(q_pixels));
            //print_pixels_to_serial(pixel_value,false);
            xTimerStart(sleep_alarm,0);
        }

        //printf("task read watermark: %d\n", uxTaskGetStackHighWaterMark(NULL));
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

void pub_thms(void *_)
{
    const char *topic2 = "amg8833/thermistor";
    short thms;
    char thms_msg_buf[10];
    while (1)
    {
        if (xQueueReceive(q_thms, &thms, portMAX_DELAY) == pdTRUE)
        {
            sprintf(thms_msg_buf, "%hd", thms);
            mqtt_send(client, topic2, thms_msg_buf);
        }
        //vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}
#endif
static void go_to_sleep(TimerHandle_t xTimer)
{
    printf("Entering deep sleep\n\n");
    //ESP_ERROR_CHECK(esp_sleep_enable_timer_wakeup(10*1000000));
    ESP_ERROR_CHECK(esp_sleep_enable_ulp_wakeup());

    /* Start the program */
    esp_err_t err = ulp_run(&ulp_entry - RTC_SLOW_MEM);
    ESP_ERROR_CHECK(err);

    esp_deep_sleep_start();
}

static void init_ulp_program()
{
    rtc_gpio_init(gpio_led);
    rtc_gpio_set_direction(gpio_led, RTC_GPIO_MODE_OUTPUT_ONLY);
    rtc_gpio_hold_en(gpio_led);
    rtc_gpio_init(gpio_scl);
    rtc_gpio_set_direction(gpio_scl, RTC_GPIO_MODE_INPUT_OUTPUT);
    rtc_gpio_pullup_en(gpio_scl);
    rtc_gpio_pulldown_dis(gpio_scl);
    rtc_gpio_init(gpio_sda);
    rtc_gpio_set_direction(gpio_sda, RTC_GPIO_MODE_INPUT_OUTPUT);
    rtc_gpio_pullup_en(gpio_sda);
    rtc_gpio_pulldown_dis(gpio_sda);

    // You MUST load the binary before setting shared variables!
    esp_err_t err = ulp_load_binary(0, ulp_main_bin_start,
                                    (ulp_main_bin_end - ulp_main_bin_start) / sizeof(uint32_t));
    ESP_ERROR_CHECK(err);

    ulp_set_wakeup_period(0, 100 * 1000);
}