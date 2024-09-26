#ifndef TASK_SCHEDULAR_H
#define TASK_SCHEDULAR_H

#include "nvs_flash.h"
#include <stdio.h>
#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <sdkconfig.h>
#include "esp_log.h"

#ifdef __cplusplus
extern "C"
{
#endif

    void main_task_shedular(void);
    void app_initialize(void);

#ifdef __cplusplus
}
#endif

#endif