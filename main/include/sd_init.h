#ifndef HEADER_SD_INIT_H
#define HEADER_SD_INIT_H

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <dirent.h>
#include "esp_vfs_fat.h"
#include "driver/sdmmc_host.h"
#include "driver/sdspi_host.h"
#include "sdmmc_cmd.h"

#include <sys/stat.h>
#include <sys/unistd.h>
#include <errno.h>
#include "esp_system.h"
#include "esp_log.h"

#include <sys/param.h>
#include <dirent.h>
#include <stddef.h>
#include "esp_event.h"
#include <stdarg.h>
#include "esp_system.h"

#define MOUNT_POINT "/sdcard"

#ifdef __cplusplus
extern "C"
{
#endif

    extern sdmmc_card_t *card;
    esp_err_t sd_card_init(void);

#ifdef __cplusplus
}
#endif

#endif