#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "heap.h"

#define ELEMENT(h, i) ((h)->top + ((h)->element_size * (i)))
#define PARENT_INDEX(i) (((i) - 1) / 2)
#define LEFT_INDEX(i) (((i) * 2 + 1))
#define RIGHT_INDEX(i) (((i) * 2 + 2))

void heap_bubble_up(heap_t* heap, int index)
{
    int i = index;

    void* e = ELEMENT(heap, i);
    void* p = ELEMENT(heap, PARENT_INDEX(i));
    void* tmp = malloc(heap->element_size);
    if(tmp == NULL)
    {
        printf("Cannot allocate memory for temporary element.\n");
        return;
    }
    while(e != heap->top && heap->element_cmp(e, p) == 1)
    {
        memcpy(tmp, p, heap->element_size);
        memcpy(p, e, heap->element_size);
        memcpy(e, tmp, heap->element_size);
        i = PARENT_INDEX(i);
        e = ELEMENT(heap, i);
        p = ELEMENT(heap, PARENT_INDEX(i));
    }
    free(tmp);
}

void heap_bubble_down(heap_t* heap, int index)
{
    int i = index;
    int done = 0;
    int ri;
    int li;
    void* e;
    void* r;
    void* l;
    
    void* tmp = malloc(heap->element_size);
    if(tmp == NULL)
    {
        printf("Cannot allocate memory for temporary element.\n");
        return;
    }
    while(!done && i < heap->n_elements)
    {
        li = LEFT_INDEX(i);
        ri = RIGHT_INDEX(i);
        e = ELEMENT(heap, i);
        l = ELEMENT(heap, li);
        r = ELEMENT(heap, ri);
        
        if(ri < heap->n_elements && li < heap->n_elements)
        {
            /* If the right child is the larger of the two and larger than
             * the parent, then swap parent and right child. */
            if(heap->element_cmp(r, l) == 1 && heap->element_cmp(r, e) == 1)
            {
                memcpy(tmp, r, heap->element_size);
                memcpy(r, e, heap->element_size);
                memcpy(e, tmp, heap->element_size);
                i = ri;
            }
            else if(heap->element_cmp(l, e) == 1)
            {
                memcpy(tmp, l, heap->element_size);
                memcpy(l, e, heap->element_size);
                memcpy(e, tmp, heap->element_size);
                i = li;
            }
            else
                done = 1;
        }
        else if(ri < heap->n_elements && heap->element_cmp(r, e) == 1)
        {
            memcpy(tmp, r, heap->element_size);
            memcpy(r, e, heap->element_size);
            memcpy(e, tmp, heap->element_size);
            i = ri;
        }
        else if(li < heap->n_elements && heap->element_cmp(l, e) == 1)
        {
            memcpy(tmp, l, heap->element_size);
            memcpy(l, e, heap->element_size);
            memcpy(e, tmp, heap->element_size);
            i = li;
        }
        else
            done = 1;
    }
    free(tmp);
}

heap_t* heap_create(size_t element_size, int (*element_cmp)(const void* e1, const void* e2))
{
    heap_t* h;

    h = malloc(sizeof(heap_t));
    if(h == NULL)
    {
        printf("Cannot allocate heap data structure.\n");
        return NULL;
    }
    memset(h, 0, sizeof(heap_t));
    h->n_elements = 0;
    h->element_size = element_size;
    h->element_cmp = element_cmp;
    return h;
}

void* heap_top(heap_t* heap)
{
    return heap->top;
}

void heap_add(heap_t* heap, void* element)
{
    void* tmp;
    if(heap->top != NULL)
    {
        tmp = realloc(heap->top, heap->element_size * (heap->n_elements + 1));
        if(tmp == NULL)
        {
            printf("Cannot resize heap.\n");
            return;
        }
        heap->top = tmp;
        memcpy(heap->top + heap->element_size * heap->n_elements, element, heap->element_size);
        heap->n_elements++;
        heap_bubble_up(heap, heap->n_elements - 1);
    }
    else
    {
        heap->top = malloc(heap->element_size);
        if(heap->top == NULL)
        {
            printf("Cannot allocate heap storage.\n");
            return;
        }
        memcpy(heap->top, element, heap->element_size);
        heap->n_elements++;
    }
}

void heap_remove_top(heap_t* heap)
{
    if(heap->n_elements == 1)
    {
        free(heap->top);
        heap->top = NULL;
        heap->n_elements--;
    }
    else
    {
        memcpy(heap->top, ELEMENT(heap, heap->n_elements - 1), heap->element_size);
        heap->n_elements--;
        heap->top = realloc(heap->top, heap->element_size * heap->n_elements);
        heap_bubble_down(heap, 0);
    }
}

int heap_size(heap_t* heap)
{
    return heap->n_elements;
}

void heap_destroy(heap_t* heap)
{
    if(heap)
    {
        if(heap->top)
            free(heap->top);
        free(heap);
    }
}
