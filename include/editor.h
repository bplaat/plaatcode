#ifndef EDITOR_H
#define EDITOR_H

#include "win32.h"
#include "list.h"

#define WM_EDITOR_OPEN_FILE (WM_USER + 0x0001)
#define WM_EDITOR_CHANGE_PATH (WM_USER + 0x0002)
#define WM_EDITOR_SAVE_FILE (WM_USER + 0x0003)

#define TOKEN_NORMAL 0
#define TOKEN_SPACE 1
#define TOKEN_COMMENT 2
#define TOKEN_KEYWORD 3
#define TOKEN_CONSTANT 4
#define TOKEN_STRING 5

typedef struct {
    bool debug;
    int32_t width;
    int32_t height;

    uint32_t background_color;
    uint32_t scrollbar_background_color;
    uint32_t scrollbar_thumb_background_color;
    uint32_t line_numbers_text_color;
    uint32_t line_numbers_active_text_color;
    uint32_t token_normal_text_color;
    uint32_t token_space_text_color;
    uint32_t token_comment_text_color;
    uint32_t token_keyword_text_color;
    uint32_t token_constant_text_color;
    uint32_t token_string_text_color;

    HFONT font;
    wchar_t *font_name;
    int32_t font_size;

    wchar_t indentation_character;
    int32_t indentation_size;

    wchar_t *path;
    wchar_t *extension;
    List *lines;

    int32_t padding;
    int32_t scrollbar_size;
    int32_t line_numbers_width;
    wchar_t *line_numbers_format;

    int32_t hscroll_viewport;
    int32_t hscroll_size;
    int32_t hscroll_offset;
    bool hscroll_down;

    int32_t vscroll_viewport;
    int32_t vscroll_size;
    int32_t vscroll_offset;
    bool vscroll_down;

    int32_t cursor_x;
    int32_t cursor_y;
} EditorData;

void Editor_UpdateLines(EditorData *editor);

void Editor_UpdateScroll(EditorData *editor);

int32_t __stdcall Editor_WndProc(HWND hwnd, uint32_t msg, WPARAM wParam, LPARAM lParam);

void Editor_Register(void);

#endif
