#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include <esp_adc_cal.h>
#include <esp_log.h>
#include <esp_system.h>
#include <esp_netif.h>
#include <esp_spi_flash.h>
#include <nvs_flash.h>
#include <driver/adc.h>
#include <driver/dac.h>
#include <driver/ledc.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "config.h"
#include "filesystem.h"
#include "list.h"
#include "metering.h"
#include "webserver.h"
#include "wifi.h"

void app_main(void) {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    init_metering_system();

    spiffs_init(MAX_FILES_AMOUNT);

    wifi_softap_init();
    webserver_start();
}
