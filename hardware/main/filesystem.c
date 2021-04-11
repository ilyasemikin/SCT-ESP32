#include "filesystem.h"

#include <stdlib.h>
#include <string.h>

#include <dirent.h>

#include <esp_log.h>
#include <esp_spiffs.h>

void spiffs_init(size_t max_files) {
    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = NULL,
        .max_files = max_files,
        .format_if_mount_failed = false
    };

    ESP_ERROR_CHECK(esp_vfs_spiffs_register(&conf));
}

struct list *spiffs_list_files(void) {
    const char *dirpath = "/spiffs/";
    DIR *dir = opendir(dirpath);
    if (dir == NULL) {
        return NULL;
    }

    size_t head_len = strlen(dirpath);

    struct dirent *ent;
    struct list *list = list_create();

    char *path;

    size_t name_len;
    size_t path_len;

    while ((ent = readdir(dir)) != NULL) {
        name_len = strlen(ent->d_name);
        path_len = head_len +  name_len + 1;
        path = (char *)malloc(sizeof(char) * path_len);

        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wstringop-truncation"

        strncpy(path, dirpath, head_len);
        strncpy(path + head_len, ent->d_name, name_len);

        #pragma GCC diagnostic pop
        path[path_len - 1] = '\0';

        list_add(list, (void *)path);
    }

    closedir(dir);

    return list;
}