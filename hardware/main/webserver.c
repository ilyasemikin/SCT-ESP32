#include "webserver.h"

#include <esp_http_server.h>
#include <esp_log.h>

#include <cJSON.h>

#include "config.h"
#include "filesystem.h"
#include "list.h"
#include "metering.h"

static httpd_handle_t server = NULL;

esp_err_t get_file_handler(httpd_req_t *req);
esp_err_t get_metering_handler(httpd_req_t *req);

const static httpd_uri_t metering_get = {
    .uri = "/metering",
    .method = HTTP_GET,
    .handler = get_metering_handler,
    .user_ctx = NULL
};

const static httpd_uri_t main_page_get = {
    .uri = "/",
    .method = HTTP_GET,
    .handler = get_file_handler,
    .user_ctx = "/spiffs/index.html"
};

void webserver_init_spiffs_files(void);

void webserver_start(void) {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_uri_handlers = 10;

    if (httpd_start(&server, &config) == ESP_OK) {
        webserver_init_spiffs_files();

        ESP_ERROR_CHECK(httpd_register_uri_handler(server, &metering_get));
        ESP_ERROR_CHECK(httpd_register_uri_handler(server, &main_page_get));
    }
}

esp_err_t get_file_handler(httpd_req_t *req) {
    FILE *fd = fopen(req->user_ctx, "r");
    if (fd == NULL) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to read existing file");
        return ESP_FAIL;
    }

    char *chunk = (char *)malloc(sizeof(char) * SCRATCH_BUFSIZE);
    size_t chunksize;
    do {
        chunksize = fread(chunk, 1, SCRATCH_BUFSIZE, fd);

        if (chunksize > 0) {
            if (httpd_resp_send_chunk(req, chunk, chunksize) != ESP_OK) {
                fclose(fd);
                httpd_resp_sendstr_chunk(req, NULL);
                httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to send file");
                return ESP_FAIL;
            }
        }
    } while (chunksize != 0);

    fclose(fd);
    httpd_resp_send_chunk(req, NULL, 0);

    return ESP_OK;
}

esp_err_t get_metering_handler(httpd_req_t *req) {
    cJSON *array = cJSON_CreateArray();

    char buf[128];
    esp_err_t err = httpd_req_get_url_query_str(req, buf, sizeof(buf) / sizeof(char));
    if (err == ESP_ERR_HTTPD_RESULT_TRUNC) {
        printf("empty\n");
    }
    else if (err == ESP_OK) {
        printf("%s\n", buf);
    }

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
