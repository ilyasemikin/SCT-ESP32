#include "metering.h"

#include <esp_adc_cal.h>
#include <driver/adc.h>
#include <driver/dac.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "config.h"

extern esp_adc_cal_characteristics_t *adc_chars;

struct metering get_metering(uint8_t dac_value) {
    struct metering result;

    dac_output_voltage(DAC_CHNL, dac_value);

    vTaskDelay(1);

    int raw = adc1_get_raw(ADC_CHNL);
    uint32_t adc_value = esp_adc_cal_raw_to_voltage(raw, adc_chars);
    double out_volt = 3.3 * ((double)adc_value / 4096);
    const double om = DEFAULT_RESISTOR_OM;
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
