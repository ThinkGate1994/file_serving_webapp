#include "wifi.h"
#include "esp_mac.h"

/* FreeRTOS event group to signal when we are connected*/
static const char *TAG_STA = "STA MODE";
static const char *TAG_AP = "AP MODE";
static const char *TAG_WIFI = "WIFI MODE";

static EventGroupHandle_t wifi_event_group;
wifi_sta_list_t clients;
esp_netif_t *esp_netif;
wifi_config_t wifi_config;
wifi_ap_record_t ap0;
esp_netif_ip_info_t info;
uint apinfo = 0;
char GW[20];

bool WIFI_CONFIG_INIT = false;
bool WIFI_STA_CONNECTED = false;
int clients_num = 0;
int clients_rssi = 0;

static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    wifi_event_ap_staconnected_t *event_AP_connect = (wifi_event_ap_staconnected_t *)event_data;
    wifi_event_ap_stadisconnected_t *event_AP_disconnect = (wifi_event_ap_stadisconnected_t *)event_data;

    if (event_base == WIFI_EVENT)
    {
        switch (event_id)
        {
        case WIFI_EVENT_STA_START:
            esp_wifi_connect();
            ESP_LOGI(TAG_STA, "Wifi STA start event");
            break;
        case WIFI_EVENT_STA_CONNECTED:
            WIFI_STA_CONNECTED = true;
            ESP_LOGI(TAG_STA, "Station connected to AP");
            break;
        case WIFI_EVENT_STA_DISCONNECTED:
            WIFI_STA_CONNECTED = false;
            esp_wifi_connect();
            ESP_LOGI(TAG_STA, "Disconnected from Router retry connection");
            break;
        case WIFI_EVENT_AP_STACONNECTED:
            ESP_LOGI(TAG_AP, "station " MACSTR " join, AID=%d", MAC2STR(event_AP_connect->mac), event_AP_connect->aid);
            if (esp_wifi_ap_get_sta_list(&clients) == ESP_OK)
            {
                clients_num = clients.num;
                clients_rssi = clients.sta[0].rssi;
                ESP_LOGI(TAG_AP, "station id: %d, RSSI: %d", clients_num, clients_rssi);
            }
            break;
        case WIFI_EVENT_AP_STADISCONNECTED:
            ESP_LOGI(TAG_AP, "station " MACSTR " leave, AID=%d", MAC2STR(event_AP_disconnect->mac), event_AP_disconnect->aid);
            if (esp_wifi_ap_get_sta_list(&clients) == ESP_OK)
            {
                clients_num = clients.num;
            }
            break;
        default:
            break;
        }
    }

    if (event_base == IP_EVENT)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        switch (event_id)
        {
        case IP_EVENT_STA_GOT_IP:
            ESP_LOGI(TAG_STA, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
            xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
            ESP_ERROR_CHECK(esp_netif_get_ip_info(esp_netif, &info));
            ip4addr_ntoa_r((ip4_addr_t *)&info.gw, GW, sizeof(GW));
            ESP_LOGI(TAG_STA, "GW: %s \n", GW);
            esp_wifi_sta_get_ap_info(&ap0);
            apinfo = ap0.rssi;
            break;

        default:
            break;
        }
    }
}

int check_wifi_mode()
{
    wifi_mode_t mode;
    esp_err_t ret = esp_wifi_get_mode(&mode);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG_WIFI, "Failed to get WiFi mode");
    }

    //   switch (mode)
    //   {
    // 	case WIFI_MODE_STA:
    // 		ESP_LOGI(ACTAG, "WiFi mode: Station (STA) mode");
    // 		break;
    // 	case WIFI_MODE_AP:
    // 		ESP_LOGI(ACTAG, "WiFi mode: Access Point (AP) mode");
    // 		break;
    // 	case WIFI_MODE_APSTA:
    // 		ESP_LOGI(ACTAG, "WiFi mode: Both STA and AP mode");
    // 		break;
    // 	default:
    // 		ESP_LOGI(ACTAG, "Unknown WiFi mode");
    // 		break;
    //   }

    return mode;
}

void wifi_AP_mode_init(char *ssid, char *pass, char *ip_addr, char *gtw_addr, char *netmask_addr)
{
    esp_log_level_set("wifi", ESP_LOG_ERROR);
    if (WIFI_CONFIG_INIT == false)
    {
        wifi_event_group = xEventGroupCreate();
        ESP_ERROR_CHECK(esp_netif_init());
        ESP_ERROR_CHECK(esp_event_loop_create_default());
        esp_netif = esp_netif_create_default_wifi_ap();
    }
    else
    {
        ESP_LOGI(TAG_AP, "WIFI is stopping to reinitialise to WIFI AP\r\n");
        ESP_ERROR_CHECK(esp_wifi_stop());
    }

    ESP_ERROR_CHECK(esp_netif_dhcps_stop(esp_netif)); // To stop the DHCP server

    esp_netif_ip_info_t info; // To assign a static IP to the network interface
    memset(&info, 0, sizeof(info));
    inet_pton(AF_INET, ip_addr, &info.ip);           // Custom IP address
    inet_pton(AF_INET, gtw_addr, &info.gw);          // Custom gateway address
    inet_pton(AF_INET, netmask_addr, &info.netmask); // Subnet address

    ESP_ERROR_CHECK(esp_netif_set_ip_info(esp_netif, &info)); // To set adapter with custom settings
    ESP_ERROR_CHECK(esp_netif_dhcps_start(esp_netif));        // To start the DHCP server with the set IP

    if (WIFI_CONFIG_INIT == false)
    {
        wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
        ESP_ERROR_CHECK(esp_wifi_init(&wifi_init_config));

        ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                            ESP_EVENT_ANY_ID,
                                                            &wifi_event_handler,
                                                            NULL,
                                                            NULL));
        ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    }

    wifi_config_t ap_config = {
        // configure the wifi connection and start the interface
        .ap = {
            .max_connection = 1,
            .authmode = WIFI_AUTH_WPA2_PSK,
        },
    };
    strcpy((char *)ap_config.sta.ssid, (const char *)ssid);
    strcpy((char *)ap_config.sta.password, (const char *)pass);

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG_AP, "Starting WIFI AP\n");
    WIFI_CONFIG_INIT = true;
}

void wifi_wait_connected()
{
    xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_BIT, false, true, 12000 / portTICK_PERIOD_MS);
}

void wifi_STA_mode_init(char *ssid, char *password, char *hostname)
{
    if (WIFI_CONFIG_INIT == false)
    {
        esp_log_level_set("wifi", ESP_LOG_ERROR);

        clients_num = -1;
        wifi_event_group = xEventGroupCreate();
        ESP_ERROR_CHECK(esp_netif_init());

        ESP_ERROR_CHECK(esp_event_loop_create_default());
        esp_netif = esp_netif_create_default_wifi_sta();

        wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
        ESP_ERROR_CHECK(esp_wifi_init(&wifi_init_config));

        ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                            ESP_EVENT_ANY_ID,
                                                            &wifi_event_handler,
                                                            NULL,
                                                            NULL));
        ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                            IP_EVENT_STA_GOT_IP,
                                                            &wifi_event_handler,
                                                            NULL,
                                                            NULL));
        ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    }
    else // else already intialized, stop wifi to reconfigure
    {
        ESP_LOGI(TAG_STA, "WIFI is stopping to reinitialise to WIFI STA\r\n");
        ESP_ERROR_CHECK(esp_wifi_stop());

        clients_num = -1;
        esp_netif = esp_netif_create_default_wifi_sta();

        ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                            IP_EVENT_STA_GOT_IP,
                                                            &wifi_event_handler,
                                                            NULL,
                                                            NULL));
    }

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = {0},
            .password = {0},
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };

    strcpy((char *)wifi_config.sta.ssid, (const char *)ssid);
    strcpy((char *)wifi_config.sta.password, (const char *)password);

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    // Set the maximum transmit power
    ESP_ERROR_CHECK(esp_wifi_set_max_tx_power(72));

    // esp_netif_set_hostname(esp_netif, hostname);
    WIFI_CONFIG_INIT = true;

    wifi_wait_connected();
}
