#ifndef __LIST_H
#define __LIST_H

#include <sys/types.h>

struct list_node_s
{
    void* data;
    struct list_node_s* next;
};
typedef struct list_node_s list_node_t;
    
struct list_s
{
    int length;
    list_node_t* head;
    list_node_t* last;
};
typedef struct list_s list_t;

list_t* list_create();
void list_destroy(list_t* l, void (*data_deallocator)(void*));
list_node_t* list_append(list_t* l, void* data);
int list_length(list_t* l);
void* list_to_array(list_t* l, size_t item_size);
void* list_iterate(list_t* l, void** status);

#endif
