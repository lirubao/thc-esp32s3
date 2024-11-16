#ifndef __AP_STA_WIFI_H__
#define __AP_STA_WIFI_H__
#ifdef __cplusplus
extern "C" {
#endif
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_mac.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "utils.h"
#include "esp_netif_net_stack.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "lwip/inet.h"
#include "lwip/netdb.h"
#include "lwip/sockets.h"
#if IP_NAPT
#    include "lwip/lwip_napt.h"
#endif
#include "lwip/err.h"
#include "lwip/sys.h"

static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id,
                               void *event_data);
esp_netif_t *wifi_init_softap();
esp_netif_t *wifi_init_sta();
void start_wifi();
#ifdef __cplusplus
}
#endif
#endif
