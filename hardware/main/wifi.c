#include "wifi.h"

#include <string.h>

#include <esp_log.h>
#include <esp_wifi.h>
#include <freertos/event_groups.h>

#include "config.h"

static EventGroupHandle_t sta_wifi_event_group;
static int s_retry_num = 0;
static struct current_station_info station_info;

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);

void wifi_init(void) {
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t init_cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&init_cfg));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));

    wifi_softap_config();
    ESP_ERROR_CHECK(esp_wifi_start());

    station_info.ssid = NULL;
    station_info.ip = NULL;
}

void wifi_softap_config(void) {
    wifi_config_t cfg = {
        .ap = {
            .ssid = WIFI_SSID,
            .ssid_len = strlen(WIFI_SSID),
            .max_connection = 4,
            .password = WIFI_PASSWORD,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK
        }
    };
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &cfg));
}

void wifi_station_config(const char *ssid, const char *password) {
    sta_wifi_event_group = xEventGroupCreate();

    printf("Station Up\n");

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    wifi_config_t cfg = {
        .sta = {
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,

            .pmf_cfg = {
                .capable = true,
                .required = false
            }
        }
    };

    if (station_info.ssid != NULL) {
        free(station_info.ssid);
        station_info.ssid = NULL;
    }
    if (station_info.ip != NULL) {
        free(station_info.ip);
        station_info.ip = NULL;
    }

    station_info.ssid = strdup(ssid);

    memcpy(cfg.sta.ssid, ssid, (strlen(ssid) + 1) * sizeof(char));
    memcpy(cfg.sta.password, password, (strlen(password) + 1) * sizeof(char));

    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &cfg));
    ESP_ERROR_CHECK(esp_wifi_connect());

    s_retry_num = 0;
    EventBits_t bits = xEventGroupWaitBits(sta_wifi_event_group,
        WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
        pdFALSE,
        pdFALSE,
        portMAX_DELAY);

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI("station", "connected to ap SSID: %s", WIFI_STATION_SSID);
    }
    else if (bits & WIFI_FAIL_BIT) {
        free(station_info.ssid);
        station_info.ssid = NULL;
        ESP_LOGI("station", "Failed to connect to SSID: %s", WIFI_STATION_SSID);
    }
    else {
        free(station_info.ssid);
        station_info.ssid = NULL;
        ESP_LOGE("station", "Unexpected event");
    }

    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, instance_got_ip));
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id));
    vEventGroupDelete(sta_wifi_event_group);
}

void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    static int s_retry_num = 0;

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < WIFI_SOFTAP_CONNECTION_MAX_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
        }
        else {
            xEventGroupSetBits(sta_wifi_event_group, WIFI_FAIL_BIT);
        }
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        char ip[16];
        
        s_retry_num = 0;
        xEventGroupSetBits(sta_wifi_event_group, WIFI_CONNECTED_BIT);
        
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        sprintf(ip, IPSTR, IP2STR(&event->ip_info.ip));
        station_info.ip = strdup(ip);
    }
}

const struct current_station_info *get_current_station_info() {
    return &station_info;
}