#ifndef __HEAP_H
#define __HEAP_H

struct heap_s
{
    void* top;
    size_t n_elements;
    size_t element_size;
    int (*element_cmp)(const void* e1, const void* e2);
};
typedef struct heap_s heap_t;

heap_t* heap_create(size_t element_size, int (*element_cmp)(const void* e1, const void* e2)); 
void heap_add(heap_t* heap, void* element);
void* heap_top(heap_t* heap);
void heap_remove_top(heap_t* heap);
void heap_destroy(heap_t* heap);
int heap_size(heap_t* heap);

#endif
