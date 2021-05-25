#ifndef SCT_WIFI_H_
#define SCT_WIFI_H_

struct current_station_info {
    char *ssid;
    char *ip;
};

void wifi_init(void);

void wifi_softap_config(void);
void wifi_station_config(const char *ssid, const char *password);

const struct current_station_info *get_current_station_info();

#endif
