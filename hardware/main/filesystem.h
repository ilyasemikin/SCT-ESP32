#ifndef SCT_FILESYSTEM_H_
#define SCT_FILESYSTEM_H_

#include <stddef.h>

#include "list.h"

void spiffs_init(size_t max_files);

struct list *spiffs_list_files(void);

char *read_whole_file(char *path);

#endif
