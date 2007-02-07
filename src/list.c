#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "list.h"

list_t* list_create()
{
    list_t* l;

    l = malloc(sizeof(list_t));
    if(l == NULL)
    {
        printf("Cannot allocate memory for list structure.\n");
        return NULL;
    }
    memset(l, 0, sizeof(list_t));
    return l;
}

void list_destroy(list_t* l, void (*data_deallocator)(void*))
{
    if(l)
    {
        list_node_t* tmp;
        while(l->head)
        {
            tmp = l->head;
            l->head = tmp->next;

            if(data_deallocator)
                data_deallocator(tmp->data);
            free(tmp);
        }
        free(l);
    }
}

list_node_t* list_append(list_t* l, void* data)
{
    list_node_t* new_item;

    new_item = malloc(sizeof(list_node_t));
    if(new_item == NULL)
    {
        printf("Cannot allocate memory for new list item.\n");
        return NULL;
    }
    new_item->data = data;
    new_item->next = NULL;
    if(l->head == NULL)
        l->head = new_item;
    else
        l->last->next = new_item;
        
    l->last = new_item;
    l->length++;
    return new_item;
}

int list_length(list_t* l)
{
    return l->length;
}

void* list_to_array(list_t* l, size_t item_size)
{
    void* a;
    void* a_ptr;
    list_node_t* i;

    a = malloc(l->length * item_size);
    if(a == NULL)
    {
        printf("Cannot allocate memory for array.\n");
        return NULL;
    }
    a_ptr = a;
    i = l->head;
    while(i)
    {
        memcpy(a_ptr, i->data, item_size);
        i = i->next;
        a_ptr += item_size;
    }
    return a;
}

void* list_iterate(list_t* l, void** status)
{
    if(l)
    {
        *status = l->head;
        return l->head->data;
    }
    else if(status != NULL)
    {
        *status = ((list_node_t*)*status)->next;
        if(*status != NULL)
            return ((list_node_t*)*status)->data; 
        else
            return NULL;
    }
    else
        return NULL;
}
