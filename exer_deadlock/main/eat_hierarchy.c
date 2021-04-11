#include "main.h"

void eat(void *parameters)
{

  int num;
  char buf[50];
  // Copy parameter and increment semaphore count
  num = *(int *)parameters;
  xSemaphoreGive(bin_sem);
  int left = num;
  int right = (left + 1) % NUM_TASKS;
  if (left > right)
  {
    left = right;
    right = num;
  }
  // take left chopstick
  xSemaphoreTake(chopstick[left], portMAX_DELAY);
  sprintf(buf, "Philosopher %i took chopstick %i", num, left);
  printf("%s\n", buf);

  // Add some delay to force deadlock
  vTaskDelay(100 / portTICK_PERIOD_MS);

  // Take right chopstick
  xSemaphoreTake(chopstick[right], portMAX_DELAY);
  sprintf(buf, "Philosopher %i took chopstick %i", num, right);
  printf("%s\n", buf);

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
