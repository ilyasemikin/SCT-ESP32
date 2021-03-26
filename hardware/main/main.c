#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include <esp_adc_cal.h>
#include <esp_log.h>
#include <esp_system.h>
#include <esp_netif.h>
#include <esp_http_server.h>
#include <esp_spi_flash.h>
#include <esp_spiffs.h>
#include <esp_wifi.h>
#include <nvs_flash.h>
#include <driver/adc.h>
#include <driver/dac.h>
#include <freertos/event_groups.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <cJSON.h>

#define DAC_CHNL DAC_CHANNEL_1
#define ADC_CHNL ADC_CHANNEL_7

#define WIFI_SSID "SCT"
#define WIFI_PASSWORD "12345678"

#define WIFI_STATION_SSID ""
#define WIFI_STATION_PASSWORD ""

#define DEFAULT_VREF 1100

static httpd_handle_t server = NULL;
static esp_adc_cal_characteristics_t *adc_chars;

static EventGroupHandle_t sta_wifi_event_group;

void wifi_station_init(void);
void wifi_softap_init(void);

void spiffs_init(void);

void webserver_start(void);

char *read_whole_file(char *path);

struct metering {
    double ampere;
    double volt;
};

struct metering get_metering(uint8_t dac_value);
cJSON *get_metering_json_object(struct metering metering);

esp_err_t get_page_handler(httpd_req_t *req);
esp_err_t get_metering_handler(httpd_req_t *req);

const static httpd_uri_t main_page = {
    .uri = "/",
    .method = HTTP_GET,
    .handler = get_page_handler,
    .user_ctx = "/spiffs/index.html"
};

const static httpd_uri_t graph_js_page = {
    .uri = "/graph.js",
    .method = HTTP_GET,
    .handler = get_page_handler,
    .user_ctx = "/spiffs/graph.js"
};

const static httpd_uri_t points_js_page = {
    .uri = "/points.js",
    .method = HTTP_GET,
    .handler = get_page_handler,
    .user_ctx = "/spiffs/points.js"
};

const static httpd_uri_t style_css_page = {
    .uri = "/style.css",
    .method = HTTP_GET,
    .handler = get_page_handler,
    .user_ctx = "/spiffs/style.css"
};

const static httpd_uri_t metering_get = {
    .uri = "/metering",
    .method = HTTP_GET,
    .handler = get_metering_handler,
    .user_ctx = NULL
};

void app_main(void) {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    wifi_softap_init();

    webserver_start();

    spiffs_init();

    ESP_ERROR_CHECK(dac_output_enable(DAC_CHNL));
    ESP_ERROR_CHECK(adc1_config_width(ADC_WIDTH_BIT_12));
    ESP_ERROR_CHECK(adc1_config_channel_atten(ADC_CHNL, ADC_ATTEN_DB_0));

    adc_chars = calloc(1, sizeof(esp_adc_cal_characteristics_t));

    esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_0, ADC_WIDTH_BIT_12, DEFAULT_VREF, adc_chars);
}

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        xEventGroupSetBits(sta_wifi_event_group, WIFI_FAIL_BIT);
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        // ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        xEventGroupSetBits(sta_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

void wifi_station_init(void) {
    sta_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t init_cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&init_cfg));

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
            .ssid = WIFI_STATION_SSID,
            .password = WIFI_STATION_PASSWORD,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,

            .pmf_cfg = {
                .capable = true,
                .required = false
            }
        }
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &cfg));
    ESP_ERROR_CHECK(esp_wifi_start());

    EventBits_t bits = xEventGroupWaitBits(sta_wifi_event_group,
        WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
        pdFALSE,
        pdFALSE,
        portMAX_DELAY);

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI("station", "connected to ap SSID: %s", WIFI_STATION_SSID);
    }
    else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI("station", "Failed to connect to SSID: %s", WIFI_STATION_SSID);
    }
    else {
        ESP_LOGE("station", "Unexpected event");
    }

    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, instance_got_ip));
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id));
    vEventGroupDelete(sta_wifi_event_group);
}

void spiffs_init(void) {
    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = NULL,
        .max_files = 5,
        .format_if_mount_failed = false
    };

    ESP_ERROR_CHECK(esp_vfs_spiffs_register(&conf));
}

void wifi_softap_init(void) {
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t init_cfg = WIFI_INIT_CONFIG_DEFAULT();

    ESP_ERROR_CHECK(esp_wifi_init(&init_cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));

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

    ESP_ERROR_CHECK(esp_wifi_start());
}

void webserver_start(void) {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_uri_handlers = 10;

    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_register_uri_handler(server, &metering_get);
        httpd_register_uri_handler(server, &main_page);
        httpd_register_uri_handler(server, &graph_js_page);
        httpd_register_uri_handler(server, &points_js_page);
        httpd_register_uri_handler(server, &style_css_page);
    }
}

char *read_whole_file(char *path) {
    FILE *file = fopen(path, "r");
    long length;

    fseek(file, 0, SEEK_END);
    length = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *str = malloc(length + 1);
    fread(str, 1, length, file);
    str[length] = '\0';

    fclose(file);

    return str;
}

struct metering get_metering(uint8_t dac_value) {
    struct metering result;

    dac_output_voltage(DAC_CHNL, dac_value);

    vTaskDelay(10);

    int raw = adc1_get_raw(ADC_CHNL);
    uint32_t adc_value = esp_adc_cal_raw_to_voltage(raw, adc_chars);
    double out_volt = 3.3 * ((double)adc_value / 4096);
    const double om = 41500;
    double ampere = out_volt / om;

    printf("adc: %d; volt: %lf; A: %lf\n ", adc_value, out_volt, ampere);

    result.ampere = ampere;
    result.volt = 3.3 * ((double)dac_value / 255);

    return result;
}

cJSON *get_metering_json_object(struct metering metering) {
    cJSON *object = cJSON_CreateObject();

    cJSON *ampere = cJSON_CreateNumber(metering.ampere);
    cJSON *volt = cJSON_CreateNumber(metering.volt);

    cJSON_AddItemToObject(object, "volt", volt);
    cJSON_AddItemToObject(object, "ampere", ampere);

    return object;
}

esp_err_t get_page_handler(httpd_req_t *req) {
    char *post_str = read_whole_file(req->user_ctx);

    httpd_resp_send(req, post_str, strlen(post_str));

    free(post_str);

    return ESP_OK;
}

esp_err_t get_metering_handler(httpd_req_t *req) {
    cJSON *array = cJSON_CreateArray();

    for (uint16_t i = 1; i < 250; i += 10) {
        cJSON *object = get_metering_json_object(get_metering(i));
        cJSON_AddItemToArray(array, object);
    }

    char *post_str = cJSON_Print(array);
    cJSON_Delete(array);

    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_send(req, post_str, strlen(post_str));

    free(post_str);

    return ESP_OK;
}