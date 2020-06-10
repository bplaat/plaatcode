#ifndef EDITOR_H
#define EDITOR_H

#include <windows.h>

#define WM_EDITOR_OPEN_FILE (WM_USER + 0x0001)

typedef struct EditorData {
    int width;
    int height;

    COLORREF window_background_color;
    COLORREF line_numbers_color;
    COLORREF code_color;
    COLORREF scrollbar_background_color;
    COLORREF scrollbar_thumb_background_color;

    HFONT font;
    int font_size;

    int scrollbar_size;
    int padding;

    char **lines;
    int lines_count;

    BOOL hscroll_down;
    int hscroll_viewport;
    int hscroll_offset;
    int hscroll_size;

    BOOL vscroll_down;
    int vscroll_viewport;
    int vscroll_offset;
    int vscroll_size;

    int line_numbers_width;
    char line_numbers_format_string[10];
} EditorData;

char *file_read(char *path);

void calculate_scroll(HWND hwnd, EditorData *data);

LRESULT CALLBACK EditorWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

void InitEditorControl(void);

#endif
