/**
 * ESP32 Priority Inversion Demo
 * 
 * Demonstrate priority inversion.
 * 
 * Date: February 12, 2021
 * Author: Shawn Hymel
 * License: 0BSD
 */

// You'll likely need this on vanilla FreeRTOS
#include "sdkconfig.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"

// Settings
TickType_t cs_wait = 250;   // Time spent in critical section (ms)
TickType_t med_wait = 5000; // Time medium task spends working (ms)

// Globals
static const char *TAG = "debug";
static SemaphoreHandle_t lock;

//*****************************************************************************
// Tasks

// Task L (low priority)
void doTaskL(void *parameters)
{

    TickType_t timestamp;

    // Do forever
    while (1)
    {

        // Take lock
        ESP_LOGI(TAG, "Task L trying to take lock...");
        timestamp = xTaskGetTickCount() * portTICK_PERIOD_MS;
        xSemaphoreTake(lock, portMAX_DELAY);

        // Say how long we spend waiting for a lock
        ESP_LOGI(TAG, "Task L got lock. Spent %d ms waiting for lock. Doing some work...",
                 (xTaskGetTickCount() * portTICK_PERIOD_MS) - timestamp);

        // Hog the processor for a while doing nothing
        timestamp = xTaskGetTickCount() * portTICK_PERIOD_MS;
        while ((xTaskGetTickCount() * portTICK_PERIOD_MS) - timestamp < cs_wait)
            ;

        // Release lock
        ESP_LOGI(TAG, "Task L releasing lock.");
        xSemaphoreGive(lock);

        // Go to sleep
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
}

// Task M (medium priority)
void doTaskM(void *parameters)
{

    TickType_t timestamp;

    // Do forever
    while (1)
    {

        // Hog the processor for a while doing nothing
        ESP_LOGI(TAG, "Task M doing some work...");
        timestamp = xTaskGetTickCount() * portTICK_PERIOD_MS;
        while ((xTaskGetTickCount() * portTICK_PERIOD_MS) - timestamp < med_wait)
            ;

        // Go to sleep
        ESP_LOGI(TAG, "Task M done!");
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
}

// Task H (high priority)
void doTaskH(void *parameters)
{

    TickType_t timestamp;

    // Do forever
    while (1)
    {

        // Take lock
        ESP_LOGI(TAG, "Task H trying to take lock...");
        timestamp = xTaskGetTickCount() * portTICK_PERIOD_MS;
        xSemaphoreTake(lock, portMAX_DELAY);

        // Say how long we spend waiting for a lock
        ESP_LOGI(TAG, "Task H got lock. Spent %d ms waiting for lock. Doing some work...",
                 (xTaskGetTickCount() * portTICK_PERIOD_MS) - timestamp);

        // Hog the processor for a while doing nothing
        timestamp = xTaskGetTickCount() * portTICK_PERIOD_MS;
        while ((xTaskGetTickCount() * portTICK_PERIOD_MS) - timestamp < cs_wait)
            ;

        // Release lock
        ESP_LOGI(TAG, "Task H releasing lock.");
        xSemaphoreGive(lock);

        // Go to sleep
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

//*****************************************************************************
// Main (runs as its own task with priority 1 on core 1)

void setup(void)
{
    // Wait a moment to start (so we don't miss Serial output)
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    ESP_LOGI(TAG, "---FreeRTOS Priority Inversion Demo---");

    // Create semaphores and mutexes before starting tasks
    lock = xSemaphoreCreateMutex();
    //xSemaphoreGive(lock); // Make sure binary semaphore starts at 1

    // The order of starting the tasks matters to force priority inversion

    // Start Task L (low priority)
    xTaskCreatePinnedToCore(doTaskL,
                            "Task L",
                            2000,
                            NULL,
                            1,
                            NULL,
                            0);

    // Introduce a delay to force priority inversion
    vTaskDelay(100 / portTICK_PERIOD_MS);

    // Start Task H (high priority)
    xTaskCreatePinnedToCore(doTaskH,
                            "Task H",
                            2000,
                            NULL,
                            3,
                            NULL,
                            0);

    // Start Task M (medium priority)
    xTaskCreatePinnedToCore(doTaskM,
                            "Task M",
                            2000,
                            NULL,
                            2,
                            NULL,
                            0);
}

void app_main(void)
{
    setup();
}