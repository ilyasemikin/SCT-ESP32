#include <stdio.h>
#include <string.h>

#include <esp_adc_cal.h>
#include <esp_system.h>
#include <esp_netif.h>
#include <esp_http_server.h>
#include <esp_spi_flash.h>
#include <esp_wifi.h>
#include <nvs_flash.h>
#include <driver/adc.h>
#include <driver/dac.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#define DAC_CHNL DAC_CHANNEL_1
#define ADC_CHNL ADC_CHANNEL_7

#define WIFI_SSID "SCT"
#define WIFI_PASSWORD "12345678"

#define DEFAULT_VREF 1100

static httpd_handle_t server = NULL;
static esp_adc_cal_characteristics_t *adc_chars;
static uint8_t dac_value = 0;

void wifi_softap_init(void);
void webserver_start(void);

esp_err_t get_main_page_handler(httpd_req_t *req);
esp_err_t get_dac_value_handler(httpd_req_t *req);
esp_err_t get_adc_value_handler(httpd_req_t *req);

const static httpd_uri_t main_page = {
    .uri = "/",
    .method = HTTP_GET,
    .handler = get_main_page_handler,
    .user_ctx = NULL
};

const static httpd_uri_t dac_value_get = {
    .uri = "/dac_value",
    .method = HTTP_GET,
    .handler = get_dac_value_handler,
    .user_ctx = NULL
};

const static httpd_uri_t dac_value_head = {
    .uri = "/dac_value",
    .method = HTTP_HEAD,
    .handler = get_dac_value_handler,
    .user_ctx = NULL
};

const static httpd_uri_t adc_value_get = {
    .uri = "/adc_value",
    .method = HTTP_GET,
    .handler = get_adc_value_handler,
    .user_ctx = NULL
};

const static httpd_uri_t adc_value_head = {
    .uri = "/adc_value",
    .method = HTTP_HEAD,
    .handler = get_adc_value_handler,
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

    ESP_ERROR_CHECK(dac_output_enable(DAC_CHNL));
    ESP_ERROR_CHECK(adc1_config_width(ADC_WIDTH_BIT_12));
    ESP_ERROR_CHECK(adc1_config_channel_atten(ADC_CHNL, ADC_ATTEN_DB_0));

    adc_chars = calloc(1, sizeof(esp_adc_cal_characteristics_t));

    esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_0, ADC_WIDTH_BIT_12, DEFAULT_VREF, adc_chars);

    while (true) {
        dac_output_voltage(DAC_CHNL, dac_value);
        if (dac_value == 200) {
            dac_value = 0;
        }
        else {
            dac_value += 20;
        }
        vTaskDelay(250);
    }
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
    
    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_register_uri_handler(server, &dac_value_get);
        httpd_register_uri_handler(server, &adc_value_get);
        httpd_register_uri_handler(server, &dac_value_head);
        httpd_register_uri_handler(server, &adc_value_head);
        httpd_register_uri_handler(server, &main_page);
    }
}

esp_err_t get_main_page_handler(httpd_req_t *req) {
    const char *post_str = "<html>\n\
    <head>\n\
        <title>Sample page</title>\n\
    </head>\n\
    <body>\n\
        <i>DAC value:</i>\n\
        <p id='dac_value'></p>\n\
        <i>ADC value:</i>\n\
        <p id='adc_value'></p>\n\
        <input value='Update' onclick='updateValue(\"dac_value\");updateValue(\"adc_value\");', type='button'>\n\
    </body>\n\
</html>\n\
\n\
<script type='text/javascript'>\n\
    function updateValue(request) {\n\
        fetch('./' + request).then(response => response.text())\n\
                                   .then(x => document.getElementById(request).innerText = x);\n\
    }\n\
\n\
    setInterval(() => { updateValue('dac_value'); updateValue('adc_value')}, 1000);\n\
</script>";

    httpd_resp_send(req, post_str, strlen(post_str));

    return ESP_OK;
}

esp_err_t get_dac_value_handler(httpd_req_t *req) {
    char post_str[10];

    itoa(dac_value, post_str, 10);

    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_send(req, post_str, strlen(post_str));

    return ESP_OK;
}

esp_err_t get_adc_value_handler(httpd_req_t *req) {
    char post_str[32];

    int raw = adc1_get_raw(ADC_CHNL);
    uint32_t voltage = esp_adc_cal_raw_to_voltage(raw, adc_chars);
    itoa(voltage, post_str, 10);

    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_send(req, post_str, strlen(post_str));

    return ESP_OK;
}