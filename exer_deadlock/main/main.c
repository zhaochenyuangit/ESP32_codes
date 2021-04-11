// The only task: eating
#include "eat_hierarchy.c"
//#include "eat_arbitrator.c"
void app_main(void)
{
    char task_name[30];

    // Wait a moment to start (so we don't miss Serial output)
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    printf("\n---FreeRTOS Dining Philosophers Challenge---\n");

    // Create kernel objects before starting tasks
    bin_sem = xSemaphoreCreateBinary();
    done_sem = xSemaphoreCreateCounting(NUM_TASKS, 0);
    for (int i = 0; i < NUM_TASKS; i++)
    {
        chopstick[i] = xSemaphoreCreateMutex();
    }
    arbitrator = xSemaphoreCreateMutex();

    // Have the philosphers start eating
    for (int i = 0; i < NUM_TASKS; i++)
    {
        sprintf(task_name, "Philosopher %i", i);
        xTaskCreatePinnedToCore(eat,
                                task_name,
                                TASK_STACK_SIZE,
                                (void *)&i,
                                1,
                                NULL,
                                0);
        xSemaphoreTake(bin_sem, portMAX_DELAY);
    }

    // Wait until all the philosophers are done
    for (int i = 0; i < NUM_TASKS; i++)
    {
        xSemaphoreTake(done_sem, portMAX_DELAY);
    }
    // Say that we made it through without deadlock
    printf("Done! No deadlock occurred!\n");
}