#include "main.h"


void eat(void *parameters)
{

  int num;
  char buf[50];

  // Copy parameter and increment semaphore count
  num = *(int *)parameters;
  xSemaphoreGive(bin_sem);
  xSemaphoreTake(arbitrator,portMAX_DELAY);
  // Take left chopstick
  xSemaphoreTake(chopstick[num], portMAX_DELAY);
  sprintf(buf, "Philosopher %i took chopstick %i", num, num);
  printf("%s\n", buf);

  // Add some delay to force deadlock
  vTaskDelay(10 / portTICK_PERIOD_MS);

  // Take right chopstick
  xSemaphoreTake(chopstick[(num + 1) % NUM_TASKS], portMAX_DELAY);
  sprintf(buf, "Philosopher %i took chopstick %i", num, (num + 1) % NUM_TASKS);
  printf("%s\n", buf);

  // Do some eating
  sprintf(buf, "Philosopher %i is eating", num);
  printf("%s\n", buf);
  vTaskDelay(10 / portTICK_PERIOD_MS);

  // Put down right chopstick
  xSemaphoreGive(chopstick[(num + 1) % NUM_TASKS]);
  sprintf(buf, "Philosopher %i returned chopstick %i", num, (num + 1) % NUM_TASKS);
  printf("%s\n", buf);

  // Put down left chopstick
  xSemaphoreGive(chopstick[num]);
  sprintf(buf, "Philosopher %i returned chopstick %i", num, num);
  printf("%s\n", buf);
  xSemaphoreGive(arbitrator);
  // Notify main task and delete self
  xSemaphoreGive(done_sem);
  vTaskDelete(NULL);
}
