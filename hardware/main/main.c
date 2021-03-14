#include <stdio.h>

#include <esp_system.h>
#include <esp_spi_flash.h>
#include <driver/dac.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#define DAC_CHNL DAC_CHANNEL_1

void app_main(void) {
    esp_err_t err;

    err = dac_output_enable(DAC_CHNL);
    if (err != ESP_OK) {
        printf("dac_output_enable() error: %d\n", err);
    }

    while (true) {
        for (int i = 0; i <= 10; i++) {
            err = dac_output_voltage(DAC_CHNL, i * 20);
            if (err != ESP_OK) {
                printf("dac_output_voltage() error: %d\n", err);
            }

            printf("Value: %d\n", i * 20);
            vTaskDelay(200);
        }
    }

    esp_restart();
}