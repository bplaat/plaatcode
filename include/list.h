#ifndef LIST_H
#define LIST_H

#include "win32.h"

typedef struct {
    void **items;
    int32_t capacity;
    int32_t size;
} List;

List *List_New(int32_t capacity);

void List_Init(List *list, int32_t capacity);

void List_Free(List *list, void (*free_function)(void *item));

void List_Add(List *list, void *item);

void List_Insert(List *list, int32_t index, void *item);

void List_Remove(List *list, int32_t index);

#endif
