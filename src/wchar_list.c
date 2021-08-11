#include "wchar_list.h"

WcharList *WcharList_New(int32_t capacity) {
    WcharList *list = malloc(sizeof(WcharList));
    WcharList_Init(list, capacity);
    return list;
}

void WcharList_Init(WcharList *list, int32_t capacity) {
    list->capacity = capacity;
    list->size = 0;
    list->items = malloc(sizeof(wchar_t) * capacity);
}

void WcharList_Free(WcharList *list) {
    free(list->items);
    free(list);
}

void WcharList_Add(WcharList *list, wchar_t item) {
    if (list->size == list->capacity) {
        list->capacity *= 2;
        list->items = realloc(list->items, sizeof(wchar_t) * list->capacity);
    }
    list->items[list->size++] = item;
}

void WcharList_Insert(WcharList *list, int32_t index, wchar_t item) {
    for (int32_t i = list->size - 1; i >= index; i--) {
        list->items[i + 1] = list->items[i];
    }
    list->size++;
    list->items[index] = item;
}

void WcharList_Remove(WcharList *list, int32_t index) {
    for (int32_t i = index; i < list->size; i++) {
        list->items[i] = list->items[i + 1];
    }
    list->size--;
}
