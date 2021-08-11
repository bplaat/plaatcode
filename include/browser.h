#ifndef BROWSER_H
#define BROWSER_H

#include "win32.h"

#define WM_BROWSER_OPEN_FOLDER (WM_USER + 0x0001)

// typedef struct {
//     uint8_t type;
//     wchar_t *path;
//     wchar_t *name;
// } BrowserFile;

// typedef struct {
//     BrowserFile super;
//     BrowserFile **files;
//     int32_t files_capacity;
//     int32_t files_size;
// } BrowserFolder;

typedef struct {
    int32_t width;
    int32_t height;

    uint32_t text_color;

    HFONT font;
    wchar_t *font_name;
    int32_t font_size;

    wchar_t *path;
    // BrowserFolder *root;
} Browser;

int32_t __stdcall Browser_WndProc(HWND hwnd, uint32_t msg, WPARAM wParam, LPARAM lParam);

void Browser_Register(void);

#endif
