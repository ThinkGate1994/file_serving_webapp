#include "task_scheduler.h"

#define TASK0_STACK_SIZE 1024 * 12
#define TASK0_DELAY (1000 / portTICK_PERIOD_MS)

void main_task(void *pvParameters)
{
  const TickType_t vPeriodicTaskPreiod = TASK0_DELAY;
  TickType_t xLastWakeTime;
  xLastWakeTime = xTaskGetTickCount();

  while (1)
  {
    vTaskDelayUntil(&xLastWakeTime, vPeriodicTaskPreiod);
    main_task_shedular();
  }
  vTaskDelete(NULL);
}

void app_main(void)
{
  app_initialize();

  UBaseType_t base_priority = 10;
  xTaskCreate(main_task, "main_task", TASK0_STACK_SIZE, NULL, base_priority, NULL);
}
