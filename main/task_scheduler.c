#include "task_scheduler.h"
#include "wifi.h"
#include "web_server.h"
#include "sd_init.h"

bool wifi_ap_mode_init = false;
bool webserver_init = false;
bool webserver_stopped = false;

void main_task_shedular(void)
{

    if (!wifi_ap_mode_init)
    {
        char esp_uuid[32];
        uint8_t mac_id[8] = {0};

        // retreive esp UUID
        esp_efuse_mac_get_default(mac_id);
        sprintf(esp_uuid, "Aimagin-%02x%02x%02x%02x%02x%02x", mac_id[0], mac_id[1], mac_id[2], mac_id[3], mac_id[4], mac_id[5]);
        esp_uuid[31] = '\0';
        ESP_LOGI("System", "-UID:%s\n\n", esp_uuid);

        wifi_AP_mode_init(esp_uuid, "123456789", "192.168.4.10", "192.168.4.10", "255.255.255.0");
        wifi_ap_mode_init = true;
    }

    if (clients_num > 0)
    {
        if (!webserver_init)
        {
            webserver_stopped = false;
            start_webserver();
            webserver_init = true;
        }
    }
}

void app_initialize(void)
{
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    sd_card_init();
}