#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include "esp_mac.h"
#include "lwip/inet.h"
#include <sys/socket.h>
#include <esp_http_server.h>
#include "esp_log.h"
#include "cJSON.h"

#ifdef __cplusplus
extern "C"
{
#endif

    // Web server control functions
    httpd_handle_t start_webserver(void);
    void stop_webserver();

    // Event handler functions
    void disconnect_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
    void connect_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);

#ifdef __cplusplus
}
#endif

#endif // WEB_SERVER_H