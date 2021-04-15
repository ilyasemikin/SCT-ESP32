#ifndef SCT_METERING_H_
#define SCT_METERING_H_

#include <stdint.h>

#include <cJSON.h>

#define CHANNEL_1_GND 17
#define CHANNEL_2_GND 18
#define CHANNEL_3_GND 19

struct metering {
    double ampere;
    double volt;
};

void init_metering_system();

struct metering get_metering(uint8_t raise_ch, uint8_t input_ch, uint8_t dac_value);
cJSON *get_metering_json_object(struct metering metering);

#endif
