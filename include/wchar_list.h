#ifndef WcharList_H
#define WcharList_H

#include "win32.h"

typedef struct {
    wchar_t *items;
    int32_t capacity;
    int32_t size;
} WcharList;

WcharList *WcharList_New(int32_t capacity);

void WcharList_Init(WcharList *list, int32_t capacity);

void WcharList_Free(WcharList *list);

void WcharList_Add(WcharList *list, wchar_t item);

void WcharList_Insert(WcharList *list, int32_t index, wchar_t item);

void WcharList_Remove(WcharList *list, int32_t index);

#endif
