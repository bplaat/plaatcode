#include "win32.h"

void *malloc(size_t size) {
    return HeapAlloc(GetProcessHeap(), 0, size);
}

void *realloc(void *ptr, size_t size) {
    return HeapReAlloc(GetProcessHeap(), 0, ptr, size);
}

void free(void *ptr) {
    HeapFree(GetProcessHeap(), 0, ptr);
}

size_t wcslen(wchar_t *string) {
    wchar_t *start = string;
    while (*string++ != '\0');
    return string - start - 1;
}

int32_t wcscmp(wchar_t *wcs1, wchar_t *wcs2) {
    while (*wcs1 && (*wcs1 == *wcs2)) {
        wcs1++;
        wcs2++;
    }
    return *wcs1 - *wcs2;
}

wchar_t *wcscpy(wchar_t *dest, wchar_t *src) {
    wchar_t *start = dest;
    while ((*dest++ = *src++) != '\0');
    return start;
}

wchar_t *wcscat(wchar_t *dest, wchar_t *src) {
    wchar_t *start = dest;
    while (*dest++ != '\0');
    dest--;
    while ((*dest++ = *src++) != '\0');
    *dest = '\0';
    return start;
}

wchar_t *wcsdup(wchar_t *src) {
    wchar_t *dst = malloc((wcslen(src) + 1) * sizeof(wchar_t));
    if (dst == NULL) return NULL;
    wcscpy(dst, src);
    return dst;
}

void wprintf(wchar_t *format, ...) {
    wchar_t string_buffer[256];
    va_list args;
    va_start(args, format);
    wvsprintfW(string_buffer, format, args);
    va_end(args);
    WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE), string_buffer, wcslen(string_buffer), NULL, NULL);
}
