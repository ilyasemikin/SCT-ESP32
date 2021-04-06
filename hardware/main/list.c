#include "list.h"

#include <malloc.h>

struct list_node *list_node_create(void) {
    struct list_node *node = (struct list_node *)malloc(sizeof(struct list_node));

    node->next = NULL;
    node->data = NULL;

    return node;
}

struct list *list_create(void) {
    struct list *list = (struct list *)malloc(sizeof(struct list));

    list->head = NULL;
    list->length = 0;

    return list;
}

struct list_node *list_add(struct list *list, void *item) {
    struct list_node **node_ptr;
    if (list->length == 0) {
        node_ptr = &list->head;
    }
    else {
        node_ptr = &list_get_last(list)->next;
    }

    *node_ptr = list_node_create();
    (*node_ptr)->data = item;

    list->length++;

    return *node_ptr;
}

struct list_node *list_get_last(struct list *list) {
    struct list_node *cur = list->head;
    if (cur == NULL) {
        return NULL;
    }

    while (cur->next != NULL) {
        cur = cur->next;
    }

    return cur;
}

void list_free(struct list *list) {
    if (list->length != 0) {
        struct list_node *cur = list->head;
        while (cur != NULL) {
            struct line_node *tmp = (struct line_node *)cur;
            cur = cur->next;
            free(tmp);
        }

        list->length = 0;
    }

    free(list);
}
