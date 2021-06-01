#include "webserver.h"

#include <stdbool.h>

#include <esp_http_server.h>
#include <esp_log.h>

#include <cJSON.h>

#include "config.h"
#include "filesystem.h"
#include "list.h"
#include "metering.h"
#include "wifi.h"

static httpd_handle_t server = NULL;
static double zero_compensation_ampere = 0;

// static functions definition
static void webserver_init_spiffs_files(void);
static esp_err_t set_content_type_from_file(httpd_req_t *req, const char * filename);
// handlers definition
static esp_err_t get_file_handler(httpd_req_t *req);
static esp_err_t get_metering_handler(httpd_req_t *req);
static esp_err_t get_channel_test(httpd_req_t *req);
static esp_err_t get_device_info(httpd_req_t *req);

const static httpd_uri_t metering_get = {
    .uri = "/metering",
    .method = HTTP_GET,
    .handler = get_metering_handler,
    .user_ctx = NULL
};

const static httpd_uri_t channel_test_get = {
    .uri = "/channel_test",
    .method = HTTP_GET,
    .handler = get_channel_test,
    .user_ctx = NULL
};

const static httpd_uri_t device_info_get = {
    .uri = "/device_info",
    .method = HTTP_GET,
    .handler = get_device_info,
    .user_ctx = NULL
};

const static httpd_uri_t main_page_get = {
    .uri = "/",
    .method = HTTP_GET,
    .handler = get_file_handler,
    .user_ctx = "/spiffs/index.html"
};

void webserver_start(void) {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    // TODO: избавиться от константы
    config.max_uri_handlers = 20;

    if (httpd_start(&server, &config) == ESP_OK) {
        webserver_init_spiffs_files();

        ESP_ERROR_CHECK(httpd_register_uri_handler(server, &metering_get));
        ESP_ERROR_CHECK(httpd_register_uri_handler(server, &main_page_get));
        ESP_ERROR_CHECK(httpd_register_uri_handler(server, &channel_test_get));
        ESP_ERROR_CHECK(httpd_register_uri_handler(server, &device_info_get));
    }
}

// static functions implementation

void webserver_init_spiffs_files(void) {
    struct list *list = spiffs_list_files();
    struct list_node *cur = list->head;

    httpd_uri_t uri = {
        .method = HTTP_GET,
        .handler = get_file_handler
    };

    while (cur != NULL) {
        uri.user_ctx = (char *)cur->data;
        uri.uri = (char *)cur->data + 7;

        ESP_ERROR_CHECK(httpd_register_uri_handler(server, &uri));

        cur = cur->next;
    }
    list_free(list);
}

esp_err_t set_content_type_from_file(httpd_req_t *req, const char * filename) {
    const char *ext = get_extension(filename);

    if (ext == NULL) {
        return ESP_FAIL;
    }

    if (!strcmp(ext, ".html")) {
        return httpd_resp_set_type(req, "text/html");
    }
    else if (!strcmp(ext, ".css")) {
        return httpd_resp_set_type(req, "text/css");
    }
    else if (!strcmp(ext, ".js")) {
        return httpd_resp_set_type(req, "application/javascript");
    }

    return httpd_resp_set_type(req, "text/plain");
}

// handlers implementation

esp_err_t get_file_handler(httpd_req_t *req) {
    const char *filename = req->user_ctx;
    FILE *fd = fopen(filename, "r");
    if (fd == NULL) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to read existing file");
        return ESP_FAIL;
    }

    set_content_type_from_file(req, filename);

    char *chunk = (char *)malloc(sizeof(char) * SCRATCH_BUFSIZE);
    size_t chunksize;
    do {
        chunksize = fread(chunk, 1, SCRATCH_BUFSIZE, fd);

        if (chunksize > 0) {
            if (httpd_resp_send_chunk(req, chunk, chunksize) != ESP_OK) {
                fclose(fd);
                free(chunk);
                httpd_resp_sendstr_chunk(req, NULL);
                httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to send file");
                return ESP_FAIL;
            }
        }
    } while (chunksize != 0);

    free(chunk);
    fclose(fd);

    httpd_resp_send_chunk(req, NULL, 0);

    return ESP_OK;
}

esp_err_t get_metering_handler(httpd_req_t *req) {
    uint8_t channel_in = 2;
    uint8_t channel_out = 1;
    char *ptr;
    double from = 0;
    double to = 3;
    double step = 0.1;
    char param[128];
    if (httpd_req_get_url_query_str(req, param, sizeof(param) / sizeof(char)) == ESP_OK) {
        char buf[16];
        const size_t buf_len = sizeof(buf) / sizeof(char);
        if (httpd_query_key_value(param, "in", buf, buf_len) == ESP_OK) {
            channel_in = atoi(buf);
        }

        if (httpd_query_key_value(param, "raise", buf, buf_len) == ESP_OK) {
            channel_out = atoi(buf);
        }

        if (httpd_query_key_value(param, "from", buf, buf_len) == ESP_OK) {
            from = strtod(buf, &ptr);
        }

        if (httpd_query_key_value(param, "to", buf, buf_len) == ESP_OK) {
            to = strtod(buf, &ptr);
        }

        if (httpd_query_key_value(param, "step", buf, buf_len) == ESP_OK) {
            step = strtod(buf, &ptr);
        }
    }
    else {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    int16_t dac_from = get_dac_raw(from);
    int16_t dac_to = get_dac_raw(to);
    int16_t dac_step = get_dac_raw(step);

    printf("%d %d\n", dac_from, dac_to);

    cJSON *array = cJSON_CreateArray();
    struct metering metering;
    for (int16_t value = dac_from; value < dac_to; value += dac_step) {
        if (value >= 0) {
            metering = get_metering(channel_out, channel_in, (uint8_t)value);
            metering.ampere = metering.ampere - zero_compensation_ampere;
        }
        else {
            metering = get_metering(channel_in, channel_out, (uint8_t)-value);
            metering.ampere = metering.ampere - zero_compensation_ampere;

            metering.volt = -metering.volt;
            metering.ampere = -metering.ampere;
        }

        cJSON *object = get_metering_json_object(metering);
        cJSON_AddItemToArray(array, object);
    }

    char *post_str = cJSON_Print(array);
    cJSON_Delete(array);

    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_send(req, post_str, strlen(post_str));

    free(post_str);

    return ESP_OK;
}

esp_err_t get_channel_test(httpd_req_t *req) {
    char buf[16];
    const size_t buf_len = sizeof(buf) / sizeof(char);
    char param[128];

    bool correct = true;
    uint8_t channel_raise = 1;
    uint8_t channel_metering = 1;
    uint8_t value = 255;

    if (httpd_req_get_url_query_str(req, param, sizeof(param) / sizeof(char)) == ESP_OK) {
        if (httpd_query_key_value(param, "raise", buf, buf_len) == ESP_OK) {
            channel_raise = (uint8_t)atoi(buf);
        }
        if (httpd_query_key_value(param, "in", buf, buf_len) == ESP_OK) {
            channel_metering = (uint8_t)atoi(buf);
        }
        if (httpd_query_key_value(param, "value", buf, buf_len) == ESP_OK) {
            value = (uint8_t)atoi(buf);
        }
    }

    if (!correct) {
        return ESP_FAIL;
    }

    struct metering metering = get_metering(channel_raise, channel_metering, value);

    cJSON *object = cJSON_CreateObject();

    cJSON *ampere = cJSON_CreateNumber(metering.ampere);
    cJSON *volt = cJSON_CreateNumber(metering.volt);
    cJSON *met_volt = cJSON_CreateNumber(metering.metering_volt);
    cJSON *raise_ch = cJSON_CreateNumber(channel_raise);
    cJSON *input_ch = cJSON_CreateNumber(channel_metering);

    if (channel_raise == channel_metering && value == 0) {
        cJSON *z_compens = cJSON_CreateBool(true);
        zero_compensation_ampere = metering.ampere;
        metering.ampere = 0;
        cJSON_AddItemToObject(object, "zero_compensation", z_compens);
    }

    cJSON_AddItemToObject(object, "channel_raise", raise_ch);
    cJSON_AddItemToObject(object, "channel_metering", input_ch);
    cJSON_AddItemToObject(object, "volt", volt);
    cJSON_AddItemToObject(object, "metering_volt", met_volt);
    cJSON_AddItemToObject(object, "ampere", ampere);

    char *post_str = cJSON_Print(object);
    cJSON_Delete(object);

    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_send(req, post_str, strlen(post_str));

    free(post_str);

    return ESP_OK;
}

esp_err_t get_device_info(httpd_req_t *req) {
    const struct current_station_info *station_info = get_current_station_info();

    cJSON *object = cJSON_CreateObject();

    cJSON *ssid = cJSON_CreateString(station_info->ssid);
    cJSON *ip = cJSON_CreateString(station_info->ip);
    cJSON *resistor = cJSON_CreateNumber(DEFAULT_RESISTOR_OM);
    cJSON *z_compens = cJSON_CreateNumber(zero_compensation_ampere);

    cJSON_AddItemToObject(object, "ssid", ssid);
    cJSON_AddItemToObject(object, "ip", ip);
    cJSON_AddItemToObject(object, "resistor", resistor);
    cJSON_AddItemToObject(object, "zero_compensation", z_compens);

    char *post_str = cJSON_Print(object);
    cJSON_Delete(object);

    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_send(req, post_str, strlen(post_str));

    free(post_str);
    
    return ESP_OK;
}