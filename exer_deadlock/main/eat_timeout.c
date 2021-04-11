#include "main.h"

void eat(void *parameters)
{

  int num;
  char buf[50];
  int left = -1;
  int right = -1;

  // Copy parameter and increment semaphore count
  num = *(int *)parameters;
  xSemaphoreGive(bin_sem);
  while (left < 0)
  {
    // Take left chopstick
    for (int i = 0; i < NUM_TASKS; i++)
    {
      if (xSemaphoreTake(chopstick[i], 10) == pdTRUE)
      {
        left = i;
        sprintf(buf, "Philosopher %i took chopstick %i", num, left);
        printf("%s\n", buf);
        break;
      }
    }
    // Add some delay to force deadlock
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    // Take right chopstick
    right = (left + 1) % NUM_TASKS;
    if (xSemaphoreTake(chopstick[right], 1000) == pdTRUE)
    {
      sprintf(buf, "Philosopher %i took chopstick %i", num, right);
      printf("%s\n", buf);
    }
    else
    {
      printf("Philosopher %i no more eat! put back the left chopstick!\n", num);
      xSemaphoreGive(chopstick[left]);
      left = -1;
      right = -1;
    }
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }

  // Do some eating
  sprintf(buf, "Philosopher %i is eating", num);
  printf("%s\n", buf);
  vTaskDelay(10 / portTICK_PERIOD_MS);

  // Put down right chopstick
  xSemaphoreGive(chopstick[right]);
  sprintf(buf, "Philosopher %i returned chopstick %i", num, right);
  printf("%s\n", buf);

  // Put down left chopstick
  xSemaphoreGive(chopstick[left]);
  sprintf(buf, "Philosopher %i returned chopstick %i", num, left);
  printf("%s\n", buf);

  // Notify main task and delete self
  xSemaphoreGive(done_sem);
  vTaskDelete(NULL);
}
