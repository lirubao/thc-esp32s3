#include "ap_sta_wifi.h"
#include <string.h>

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

#define AP_SSID "ESP32-SoftAP"
#define AP_PASSWORD "12345678"
#define STA_SSID "ESP32-SoftAP"
#define STA_PASSWORD "12345678"

static EventGroupHandle_t s_wifi_event_group;
static const char *TAG_AP = "WiFi SoftAP";
static const char *TAG_STA = "WiFi Sta";
static int s_retry_num = 0;

static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id,
                               void *event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *)event_data;
        logi(TAG_AP, "Station " MACSTR " joined, AID=%d", MAC2STR(event->mac), event->aid);
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *)event_data;
        logi(TAG_AP, "Station " MACSTR " left, AID=%d", MAC2STR(event->mac), event->aid);
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
        logi(TAG_STA, "Station started");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        logi(TAG_STA, "Got IP:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

esp_netif_t *wifi_init_softap() {
    esp_netif_t *ap_netif = esp_netif_create_default_wifi_ap();

    wifi_config_t ap_config = {
        .ap =
            {
                .ssid = AP_SSID,
                .password = AP_PASSWORD,
                .ssid_len = strlen(AP_PASSWORD),
                .channel = 1,
                .authmode = WIFI_AUTH_WPA2_PSK,
                .max_connection = 4,
                .pmf_cfg =
                    {
                        .required = false,
                    },
            },
    };

    if (strlen(AP_PASSWORD) == 0) { ap_config.ap.authmode = WIFI_AUTH_OPEN; }

    check_err(esp_wifi_set_config(WIFI_IF_AP, &ap_config));
    logi(TAG_AP, "wifi_init_softap finished. SSID:%s password:%s", AP_SSID, AP_PASSWORD);

    return ap_netif;
}

esp_netif_t *wifi_init_sta() {
    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    wifi_config_t sta_config = {
        .sta =
            {
                .ssid = STA_SSID,
                .password = STA_PASSWORD,
                .scan_method = WIFI_ALL_CHANNEL_SCAN,
                .failure_retry_cnt = 10,
            },
    };

    check_err(esp_wifi_set_config(WIFI_IF_STA, &sta_config));
    logi(TAG_STA, "wifi_init_sta finished. SSID:%s password:%s", STA_SSID, STA_PASSWORD);
    return sta_netif;
}

void start_wifi() {
    check_err(esp_netif_init());
    check_err(esp_event_loop_create_default());
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        check_err(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    check_err(ret);
    s_wifi_event_group = xEventGroupCreate();
    check_err(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler,
                                                  NULL, NULL));
    check_err(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                                  &wifi_event_handler, NULL, NULL));
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    check_err(esp_wifi_init(&cfg));

    check_err(esp_wifi_set_mode(WIFI_MODE_APSTA));

    logi(TAG_AP, "ESP_WIFI_MODE_AP");
    esp_netif_t *ap_netif = wifi_init_softap();

    logi(TAG_STA, "ESP_WIFI_MODE_STA");
    esp_netif_t *sta_netif = wifi_init_sta();

    check_err(esp_wifi_start());

    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT, pdFALSE, pdFALSE,
                                           portMAX_DELAY);

    if (bits & WIFI_CONNECTED_BIT) {
        logi(TAG_STA, "connected to ap SSID:%s password:%s", STA_SSID, STA_PASSWORD);
    } else if (bits & WIFI_FAIL_BIT) {
        logi(TAG_STA, "Failed to connect to SSID:%s, password:%s", STA_SSID, STA_PASSWORD);
    } else {
        logi(TAG_STA, "UNEXPECTED EVENT");
        return;
    }

    /* Set sta as the default interface */
    esp_netif_set_default_netif(sta_netif);

    /* Enable napt on the AP netif */
    if (esp_netif_napt_enable(ap_netif) != ESP_OK) {
        logi(TAG_STA, "NAPT not enabled on the netif: %p", ap_netif);
    }
}