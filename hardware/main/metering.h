#ifndef SCT_METERING_H_
#define SCT_METERING_H_

#include <stdint.h>

#include <cJSON.h>

struct metering {
    double ampere;
    double volt;
};

struct metering get_metering(uint8_t dac_value);
cJSON *get_metering_json_object(struct metering metering);

#endif
