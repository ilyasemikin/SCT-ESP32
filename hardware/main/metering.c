#include "metering.h"

#include <esp_adc_cal.h>
#include <driver/adc.h>
#include <driver/dac.h>
#include <driver/gpio.h>
#include <driver/ledc.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "config.h"

static esp_adc_cal_characteristics_t *adc_chars_channel;

// static function declarations

static void raise_first_channel(uint8_t dac_value);
static void raise_second_channel(uint8_t dac_value);
static void raise_third_channel(uint8_t dac_value);

static void reset_channels();

void pull_down(uint8_t gpio);

static uint32_t adc_get_value(adc_channel_t channel, uint32_t repeats);

static struct metering raw_to_metering(uint8_t dac_value, uint32_t adc_value, double oms);

// interface implementation

void init_metering_system() {
    ESP_ERROR_CHECK(adc1_config_width(ADC_WIDTH_BIT_12));

    ESP_ERROR_CHECK(adc1_config_channel_atten(ADC_CHANNEL_4, ADC_ATTEN_DB_11));
    ESP_ERROR_CHECK(adc1_config_channel_atten(ADC_CHANNEL_6, ADC_ATTEN_DB_11));
    ESP_ERROR_CHECK(adc1_config_channel_atten(ADC_CHANNEL_7, ADC_ATTEN_DB_11));

    adc_chars_channel = calloc(1, sizeof(esp_adc_cal_characteristics_t));
    esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, DEFAULT_VREF, adc_chars_channel);

    ledc_timer_config_t ledc_timer = {
        .duty_resolution = LEDC_TIMER_8_BIT,
        .freq_hz = 5000,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = LEDC_TIMER_0,
        .clk_cfg = LEDC_AUTO_CLK
    };

    ledc_timer_config(&ledc_timer);
}

struct metering get_metering(uint8_t raise_ch, uint8_t input_ch, uint8_t dac_value) {
    switch (raise_ch) {
    case 1:
        raise_first_channel(dac_value);
        break;
    case 2:
        raise_second_channel(dac_value);
        break;
    case 3:
        raise_third_channel(dac_value);
        break;
    default:
        break;
    }

    vTaskDelay(1);

    uint32_t adc_value = 0;
    switch (input_ch) {
    case 1:
        pull_down(CHANNEL_1_GND);
        vTaskDelay(1);
        adc_value = adc_get_value(ADC_CHANNEL_7, 1);
        break;
    case 2:
        pull_down(CHANNEL_2_GND);
        vTaskDelay(1);
        adc_value = adc_get_value(ADC_CHANNEL_6, 1);
        break;
    case 3:
        pull_down(CHANNEL_3_GND);
        vTaskDelay(1);
        adc_value = adc_get_value(ADC_CHANNEL_4, 1000);
    default:
        break;
    }

    reset_channels();

    return raw_to_metering(dac_value, adc_value, DEFAULT_RESISTOR_OM);
}

cJSON *get_metering_json_object(struct metering metering) {
    cJSON *object = cJSON_CreateObject();

    cJSON *ampere = cJSON_CreateNumber(metering.ampere);
    cJSON *volt = cJSON_CreateNumber(metering.volt);

    cJSON_AddItemToObject(object, "volt", volt);
    cJSON_AddItemToObject(object, "ampere", ampere);

    return object;
}

int32_t get_dac_raw(double volt) {
    return (int32_t)(255 * volt / 3.3);
}

// static function implementation

void raise_first_channel(uint8_t dac_value) {
    ESP_ERROR_CHECK(dac_output_disable(DAC_CHANNEL_1));
    ESP_ERROR_CHECK(dac_output_enable(DAC_CHANNEL_2));
    ESP_ERROR_CHECK(ledc_stop(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0));
    ESP_ERROR_CHECK(gpio_reset_pin(4));

    dac_output_voltage(DAC_CHANNEL_2, dac_value);
}

void raise_second_channel(uint8_t dac_value) {
    ESP_ERROR_CHECK(dac_output_enable(DAC_CHANNEL_1));
    ESP_ERROR_CHECK(dac_output_disable(DAC_CHANNEL_2));
    ESP_ERROR_CHECK(ledc_stop(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0));
    ESP_ERROR_CHECK(gpio_reset_pin(4));

    dac_output_voltage(DAC_CHANNEL_1, dac_value);
}

void raise_third_channel(uint8_t dac_value) {
    ESP_ERROR_CHECK(dac_output_disable(DAC_CHANNEL_1));
    ESP_ERROR_CHECK(dac_output_disable(DAC_CHANNEL_2));

    ledc_channel_config_t ledc_channel_cfg = {
        .channel = LEDC_CHANNEL_0,
        .duty = 0,
        .gpio_num = 4,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .hpoint = 0,
        .timer_sel = LEDC_TIMER_0
    };

    ledc_channel_config(&ledc_channel_cfg);

    ledc_set_duty(ledc_channel_cfg.speed_mode, ledc_channel_cfg.channel, dac_value);
    ledc_update_duty(ledc_channel_cfg.speed_mode, ledc_channel_cfg.channel);
}

static void reset_channels() {
    gpio_reset_pin(CHANNEL_1_GND);
    gpio_reset_pin(CHANNEL_2_GND);
    gpio_reset_pin(CHANNEL_3_GND);

    raise_first_channel(0);
    raise_second_channel(0);
    raise_third_channel(0);
}

void pull_down(uint8_t gpio) {
    gpio_reset_pin(CHANNEL_1_GND);
    gpio_reset_pin(CHANNEL_2_GND);
    gpio_reset_pin(CHANNEL_3_GND);

    vTaskDelay(1);

    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1ULL << gpio);
    io_conf.pull_down_en = 1;
    io_conf.pull_up_en = 0;

    gpio_config(&io_conf);
}

uint32_t adc_get_value(adc_channel_t channel, uint32_t repeats) {
    uint64_t sum = 0;

    for (uint32_t i = 0; i < repeats; i++) {
        sum += adc1_get_raw(channel);
    }

    return esp_adc_cal_raw_to_voltage(sum / repeats, adc_chars_channel);
}

struct metering raw_to_metering(uint8_t dac_value, uint32_t adc_value, double oms) {
    struct metering result;

    double input_volt = 3.3 * ((double)dac_value / 255);
    double out_volt = (double)adc_value / 1000;

    result.metering_volt = out_volt;
    result.ampere = out_volt / oms;
    result.volt = input_volt;

    return result;
}