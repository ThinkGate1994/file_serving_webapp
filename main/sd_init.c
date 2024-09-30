#include "sd_init.h"

//sd card external
// #define SD_PIN_NUM_MISO  20
// #define SD_PIN_NUM_MOSI  14
// #define SD_PIN_NUM_CLK   19
// #define SD_PIN_NUM_CS    9

//nand flash
#define SD_PIN_NUM_MISO  38
#define SD_PIN_NUM_MOSI  45
#define SD_PIN_NUM_CLK   21
#define SD_PIN_NUM_CS    42

static const char* SDTAG = "SD_SETUP";   
sdmmc_card_t *card;

esp_err_t sd_card_init(void)
{
    ESP_LOGI(SDTAG, "Using SPI peripheral for SD card");
    esp_err_t ret;
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024};

    ESP_LOGI(SDTAG, "Initializing SD card");
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = SD_PIN_NUM_MOSI,
        .miso_io_num = SD_PIN_NUM_MISO,
        .sclk_io_num = SD_PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 6000, //4100 // 4000 // maximum transfer size (in bytes) 
    };

    ret = spi_bus_initialize(host.slot, &bus_cfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK)
    {
        ESP_LOGE(SDTAG, "Failed to initialize bus.");
        return ret;
    }

    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = SD_PIN_NUM_CS;
    slot_config.host_id = host.slot;

    ESP_LOGI(SDTAG, "Mounting filesystem");
    const char mount_point[] = MOUNT_POINT;
    ret = esp_vfs_fat_sdspi_mount(mount_point, &host, &slot_config, &mount_config, &card);
    if (ret != ESP_OK)
    {
        if (ret == ESP_FAIL)
        {
            ESP_LOGE(SDTAG, "Failed to mount filesystem. "
                            "If you want the card to be formatted, set the EXAMPLE_FORMAT_IF_MOUNT_FAILED menuconfig option.");
        }
        else
        {
            ESP_LOGE(SDTAG, "Failed to initialize the card (%s). "
                            "Make sure SD card lines have pull-up resistors in place.",
                     esp_err_to_name(ret));
        }

        return ret;
    }

    ESP_LOGI(SDTAG, "Filesystem mounted");

    /* Card has been initialized, print its properties */
    sdmmc_card_print_info(stdout, card);

    return ret;
}

