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
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "config.h"
#include "filesystem.h"
#include "list.h"
#include "metering.h"
#include "webserver.h"
#include "wifi.h"

esp_adc_cal_characteristics_t *adc_chars;

void app_main(void) {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    wifi_softap_init();

    spiffs_init(5);

    vTaskDelay(10);

    webserver_start();

    ESP_ERROR_CHECK(dac_output_enable(DAC_CHNL));
    ESP_ERROR_CHECK(adc1_config_width(ADC_WIDTH_BIT_12));
    ESP_ERROR_CHECK(adc1_config_channel_atten(ADC_CHNL, ADC_ATTEN_DB_0));

    adc_chars = calloc(1, sizeof(esp_adc_cal_characteristics_t));

    esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_0, ADC_WIDTH_BIT_12, DEFAULT_VREF, adc_chars);
}
