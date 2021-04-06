#ifndef SCT_LIST_H_
#define SCT_LIST_H_

#include <stddef.h>

struct list_node {
    void *data;
    struct list_node *next;
};

struct list {
    size_t length;
    struct list_node *head;
};

struct list_node *list_node_create(void);
struct list *list_create(void);
struct list_node *list_add(struct list *list, void *item);
struct list_node *list_get_last(struct list *list);
void list_free(struct list *list);

#endif
