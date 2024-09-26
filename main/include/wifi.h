#ifndef WIFI_H
#define WIFI_H

#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include <string.h>
#include "lwip/inet.h"
#include <sys/socket.h>

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1
#define ESP_MAXIMUM_RETRY 5
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA_WPA2_PSK

#ifdef __cplusplus
extern "C"
{
#endif

    int check_wifi_mode();
    void wifi_STA_mode_init(char *ssid, char *password, char * hostname);
    void wifi_AP_mode_init(char *ssid, char *pass, char *ip_addr, char *gtw_addr, char *netmask_addr);
    extern int clients_num;
    extern bool WIFI_STA_CONNECTED;

#ifdef __cplusplus
}
#endif

#endif