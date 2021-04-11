#ifndef SCT_FILESYSTEM_H_
#define SCT_FILESYSTEM_H_

#include <stdlib.h>
#include <stddef.h>

#include "list.h"

void spiffs_init(size_t max_files);

struct list *spiffs_list_files(void);

#endif
