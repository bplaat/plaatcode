#include <windows.h>
#include <math.h>

#include "editor.h"

#ifndef WM_MOUSEHWHEEL
#define WM_MOUSEHWHEEL 0x020E
#endif

#ifndef GET_X_LPARAM
#define GET_X_LPARAM(lParam) ((int)(short)LOWORD(lParam))
#endif

#ifndef GET_Y_LPARAM
#define GET_Y_LPARAM(lParam) ((int)(short)HIWORD(lParam))
#endif

char *file_read(char *path) {
    HANDLE file = CreateFile(path, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    int file_size = GetFileSize(file, 0);
    char *file_buffer = malloc(file_size + 1);
    DWORD numRead;
    ReadFile(file, file_buffer, file_size, &numRead, 0);
    CloseHandle(file);
    file_buffer[file_size] = '\0';
    return file_buffer;
}

void calculate_scroll(HWND hwnd, EditorData *data) {
    // Calculate line numbers size
    int number_size = log10(data->lines_count + 1) + 1;

    char test_buffer[sizeof(data->line_numbers_format_string)];
    for (int i = 0; i < number_size; i++) {
        test_buffer[i] = '0';
    }
    test_buffer[number_size] = '\0';

    HDC hdc = GetDC(hwnd);
    SelectObject(hdc, data->font);
    SIZE line_metrics;
    GetTextExtentPoint(hdc, test_buffer, number_size + 1, &line_metrics);
    data->line_numbers_width = data->padding + line_metrics.cx + data->padding;

    wsprintf(data->line_numbers_format_string, "%%%dd", number_size);

    // Calculate horizontal scroll viewport
    data->hscroll_viewport = data->width - data->line_numbers_width - data->scrollbar_size;

    // Calculate vertical scroll viewport
    data->vscroll_viewport = data->height;

    // Calculate horizontal scroll size
    char *largest_line = data->lines[0];
    int largest_line_size = lstrlen(data->lines[0]);

    for (int i = 0; i < data->lines_count; i++) {
        int new_line_size = lstrlen(data->lines[i]);
        if (new_line_size > largest_line_size) {
            largest_line = data->lines[i];
            largest_line_size = new_line_size;
        }
    }

    GetTextExtentPoint(hdc, largest_line, largest_line_size, &line_metrics);
    data->hscroll_size = data->padding + line_metrics.cx + data->padding;

    // Calculate vertical scroll size
    data->vscroll_size = data->padding + data->font_size * data->lines_count +
        data->padding + (data->vscroll_viewport - data->font_size - data->padding);

    // Calculate horizontal scroll offset
    if (data->hscroll_offset + data->hscroll_viewport > data->hscroll_size) {
        data->hscroll_offset = data->hscroll_size - data->hscroll_viewport;

        if (data->hscroll_offset < 0) {
            data->hscroll_offset = 0;
        }
    }
}

LRESULT CALLBACK EditorWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    EditorData *data = (EditorData *)GetWindowLongPtr(hwnd, GWLP_USERDATA);

    if (msg == WM_CREATE) {
        data = malloc(sizeof(EditorData));
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)data);

        RECT editor_rect;
        GetClientRect(hwnd, &editor_rect);
        data->width = editor_rect.right;
        data->height = editor_rect.bottom;

        data->font = GetStockObject(ANSI_FIXED_FONT);
        HDC hdc = GetDC(hwnd);
        SelectObject(hdc, data->font);
        TEXTMETRIC tm;
        GetTextMetrics(hdc, &tm);
        data->font_size = tm.tmHeight;

        data->scrollbar_size = 16;
        data->padding = 8;

        data->lines = malloc(sizeof(char *));
        data->lines[0] = "";
        data->lines_count = 1;

        data->hscroll_down = FALSE;
        data->hscroll_offset = 0;
        data->hscroll_size = 0;

        data->vscroll_down = FALSE;
        data->vscroll_offset = 0;
        data->vscroll_size = 0;
        return 0;
    }

    if (msg == WM_EDITOR_OPEN_FILE) {
        char *file_buffer = file_read((char *)wParam);

        // Count lines
        data->lines_count = 0;
        char *c = file_buffer;
        for (;;) {
            if (*c == '\n') {
                data->lines_count++;
            }

            if (*c == '\r' && *(c + 1) == '\n') {
                data->lines_count++;
                c++;
            }

            if (*c == '\0') {
                data->lines_count++;
                break;
            }

            c++;
        }

        // Read lines
        data->lines = malloc(data->lines_count * sizeof(char *));

        char *current = file_buffer;

        for (int i = 0; i < data->lines_count; i++) {
            // Get line length
            char *end = current;
            while (!(*end == '\0' || *end == '\n' || (*end == '\r' && *(end + 1) == '\n'))) {
                end++;
            }

            char *line = malloc(end - current + 1);
            data->lines[i] = line;

            char *text = current;
            while (text != end) {
                *line++ = *text++;
            }
            *line = '\0';

            if (*end == '\n') {
                end++;
            }

            if (*end == '\r' && *(end + 1) == '\n') {
                end += 2;
            }

            current = end;
        }

        free(file_buffer);

        data->hscroll_offset = 0;
        data->vscroll_offset = 0;

        calculate_scroll(hwnd, data);

        InvalidateRect(hwnd, NULL, FALSE);

        return 0;
    }

    if (msg == WM_SIZE) {
        data->width = LOWORD(lParam);
        data->height = HIWORD(lParam);

        calculate_scroll(hwnd, data);
    }

    if (msg == WM_LBUTTONDOWN) {
        int xPos = GET_X_LPARAM(lParam);
        int yPos = GET_Y_LPARAM(lParam);

        if (
            xPos >= data->line_numbers_width && xPos < data->width - data->scrollbar_size &&
            yPos >= data->vscroll_viewport - data->scrollbar_size
        ) {
            data->hscroll_down = TRUE;
            SetCapture(hwnd);

            data->hscroll_offset = (xPos - (data->hscroll_viewport * data->hscroll_viewport / data->hscroll_size / 2)) * data->hscroll_size / data->hscroll_viewport;

            if (data->hscroll_offset < 0) {
                data->hscroll_offset = 0;
            }

            if (data->hscroll_offset + data->hscroll_viewport > data->hscroll_size) {
                data->hscroll_offset = data->hscroll_size - data->hscroll_viewport;
            }

            InvalidateRect(hwnd, NULL, FALSE);
            return 0;
        }

        if (xPos >= data->width - data->scrollbar_size) {
            data->vscroll_down = TRUE;
            SetCapture(hwnd);

            data->vscroll_offset = (yPos - (data->vscroll_viewport * data->vscroll_viewport / data->vscroll_size / 2)) * data->vscroll_size / data->vscroll_viewport;

            if (data->vscroll_offset < 0) {
                data->vscroll_offset = 0;
            }

            if (data->vscroll_offset + data->vscroll_viewport > data->vscroll_size) {
                data->vscroll_offset = data->vscroll_size - data->vscroll_viewport;
            }

            InvalidateRect(hwnd, NULL, FALSE);
            return 0;
        }

        return 0;
    }

    if (msg == WM_MOUSEMOVE) {
        int xPos = GET_X_LPARAM(lParam);
        int yPos = GET_Y_LPARAM(lParam);

        if (data->hscroll_down && xPos >= data->line_numbers_width && xPos < data->width - data->scrollbar_size) {
            data->hscroll_offset = (xPos - (data->hscroll_viewport * data->hscroll_viewport / data->hscroll_size / 2)) * data->hscroll_size / data->hscroll_viewport;

            if (data->hscroll_offset < 0) {
                data->hscroll_offset = 0;
            }

            if (data->hscroll_offset + data->hscroll_viewport > data->hscroll_size) {
                data->hscroll_offset = data->hscroll_size - data->hscroll_viewport;
            }

            InvalidateRect(hwnd, NULL, FALSE);
        }

        if (data->vscroll_down && yPos >= 0 && yPos < data->vscroll_viewport) {
            data->vscroll_offset = (yPos - (data->vscroll_viewport * data->vscroll_viewport / data->vscroll_size / 2)) * data->vscroll_size / data->vscroll_viewport;

            if (data->vscroll_offset < 0) {
                data->vscroll_offset = 0;
            }

            if (data->vscroll_offset + data->vscroll_viewport > data->vscroll_size) {
                data->vscroll_offset = data->vscroll_size - data->vscroll_viewport;
            }

            InvalidateRect(hwnd, NULL, FALSE);
        }

        return 0;
    }

    if (msg == WM_LBUTTONUP) {
        if (data->hscroll_down) {
            data->hscroll_down = FALSE;
            ReleaseCapture();
        }

        if (data->vscroll_down) {
            data->vscroll_down = FALSE;
            ReleaseCapture();
        }

        return 0;
    }

    if (msg == WM_MOUSEWHEEL) {
        data->vscroll_offset -= GET_WHEEL_DELTA_WPARAM(wParam);

        if (data->vscroll_offset < 0) {
            data->vscroll_offset = 0;
        }

        if (data->vscroll_offset + data->vscroll_viewport > data->vscroll_size) {
            data->vscroll_offset = data->vscroll_size - data->vscroll_viewport;
        }

        InvalidateRect(hwnd, NULL, FALSE);
        return 0;
    }

    if (msg == WM_MOUSEHWHEEL) {
        data->hscroll_offset += GET_WHEEL_DELTA_WPARAM(wParam);

        if (data->hscroll_offset < 0) {
            data->hscroll_offset = 0;
        }

        if (data->hscroll_offset + data->hscroll_viewport > data->hscroll_size) {
            data->hscroll_offset = data->hscroll_size - data->hscroll_viewport;
        }

        InvalidateRect(hwnd, NULL, FALSE);
        return 0;
    }

    if (msg == WM_SETFONT) {
        data->font = (HFONT)wParam;
        HDC hdc = GetDC(hwnd);
        SelectObject(hdc, data->font);
        TEXTMETRIC tm;
        GetTextMetrics(hdc, &tm);
        data->font_size = tm.tmHeight;
        return 0;
    }

    if (msg == WM_GETFONT) {
        return (LRESULT)data->font;
    }

    if (msg == WM_ERASEBKGND) {
        return TRUE;
    }

    if (msg == WM_PAINT) {
        PAINTSTRUCT ps;
        HDC hdc;
        if (wParam == 0) {
            hdc = BeginPaint(hwnd, &ps);
        } else {
            hdc = (HDC)wParam;
        }

        // Create double buffer
        HDC hdcMem = CreateCompatibleDC(hdc);
        HBITMAP hbmMem = CreateCompatibleBitmap(hdc, data->width, data->height);
        HANDLE hOld = SelectObject(hdcMem, hbmMem);

        // Draw background
        RECT editor_rect = { 0, 0, data->width, data->height };
        SetBkColor(hdcMem, data->window_background_color);
        ExtTextOut(hdcMem, 0, 0, ETO_OPAQUE, &editor_rect, "", 0, NULL);

        // Draw lines
        SelectObject(hdcMem, data->font);
        SetBkMode(hdcMem, TRANSPARENT);
        SetTextColor(hdcMem, data->code_color);
        for (int i = 0; i < data->lines_count; i++) {
            int line_size = lstrlen(data->lines[i]);
            int x = data->line_numbers_width + data->padding - data->hscroll_offset;
            int y = data->padding + data->font_size * i - data->vscroll_offset;
            SIZE line_metrics;
            GetTextExtentPoint(hdcMem, data->lines[i], line_size, &line_metrics);
            if (
                x + line_metrics.cx >= data->line_numbers_width && x < data->width &&
                y + data->font_size >= 0 && y < data->height
            ) {
                ExtTextOut(hdcMem, x, y, 0, NULL, data->lines[i], line_size, NULL);
            }
        }

        // Draw line numbers rect
        RECT line_numbers_rect = { 0, 0, data->line_numbers_width, data->height };
        SetBkColor(hdcMem, data->window_background_color);
        ExtTextOut(hdcMem, 0, 0, ETO_OPAQUE, &line_numbers_rect, "", 0, NULL);

        // Draw line numbers
        SetTextColor(hdcMem, data->line_numbers_color);
        for (int i = 1; i <= data->lines_count; i++) {
            int y = data->padding + data->font_size * (i - 1) - data->vscroll_offset;
            if (y + data->font_size >= 0 && y <= data->height) {
                char string_buffer[16];
                wsprintf(string_buffer, data->line_numbers_format_string, i);
                ExtTextOut(hdcMem, data->padding, y, 0, NULL, string_buffer, strlen(string_buffer), NULL);
            }
        }

        // Draw horizontal scrollbar
        if (data->hscroll_size > data->width) {
            RECT hscrollbar_rect = { data->line_numbers_width, data->height - data->scrollbar_size, data->width - data->scrollbar_size, data->height };
            SetBkColor(hdcMem, data->scrollbar_background_color);
            ExtTextOut(hdcMem, 0, 0, ETO_OPAQUE, &hscrollbar_rect, "", 0, NULL);

            int x = data->hscroll_viewport * data->hscroll_offset / data->hscroll_size;
            RECT hscrollbar_thumb_rect = { data->line_numbers_width + x, data->height - data->scrollbar_size, data->line_numbers_width + x + data->hscroll_viewport * data->hscroll_viewport / data->hscroll_size, data->height };
            SetBkColor(hdcMem, data->scrollbar_thumb_background_color);
            ExtTextOut(hdcMem, 0, 0, ETO_OPAQUE, &hscrollbar_thumb_rect, "", 0, NULL);
        }

        // Draw vertical scrollbar
        RECT vscrollbar_rect = { data->width - data->scrollbar_size, 0, data->width, data->vscroll_viewport };
        SetBkColor(hdcMem, data->scrollbar_background_color);
        ExtTextOut(hdcMem, 0, 0, ETO_OPAQUE, &vscrollbar_rect, "", 0, NULL);

        int y = data->vscroll_viewport * data->vscroll_offset / data->vscroll_size;
        RECT vscrollbar_thumb_rect = { data->width - data->scrollbar_size, y, data->width, y +data->vscroll_viewport * data->vscroll_viewport / data->vscroll_size };
        SetBkColor(hdcMem, data->scrollbar_thumb_background_color);
        ExtTextOut(hdcMem, 0, 0, ETO_OPAQUE, &vscrollbar_thumb_rect, "", 0, NULL);

        // Blit double buffer
        BitBlt(hdc, 0, 0, data->width, data->height, hdcMem, 0, 0, SRCCOPY);
        SelectObject(hdcMem, hOld);
        DeleteObject(hbmMem);
        DeleteDC (hdcMem);

        if (wParam == 0) {
            EndPaint(hwnd, &ps);
        }
        return 0;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

void InitEditorControl(void) {
    WNDCLASSEX wc = { 0 };
    wc.cbSize = sizeof(wc);
    wc.lpszClassName = "plaatcode_editor";
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpfnWndProc = EditorWndProc;
    wc.hCursor = LoadCursor (NULL, IDC_ARROW);
    wc.hbrBackground = NULL;
    RegisterClassEx(&wc);
}
