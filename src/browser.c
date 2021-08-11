#include "browser.h"

int32_t __stdcall Browser_WndProc(HWND hwnd, uint32_t msg, WPARAM wParam, LPARAM lParam) {
    Browser *browser = GetWindowLongPtrW(hwnd, GWLP_USERDATA);

    if (msg == WM_CREATE) {
        // Create browser data
        browser = malloc(sizeof(Browser));
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, browser);

        browser->text_color = 0x00ffffff;

        browser->font_name = L"Segoe UI";
        browser->font_size = 18;
        browser->font = CreateFontW(browser->font_size, 0, 0, 0, FW_NORMAL, false, false, false, ANSI_CHARSET,
            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, browser->font_name);

        browser->path = NULL;
        return 0;
    }

    if (msg == WM_BROWSER_OPEN_FOLDER) {
        if (wParam == NULL) {
            browser->path = NULL;
            return 0;
        }

        // Get full file path
        wchar_t path[MAX_PATH];
        GetFullPathNameW(wParam, MAX_PATH, path, NULL);

        browser->path = wcsdup(path);

        return 0;
    }

    if (msg == WM_SIZE) {
        // Save new browser size
        browser->width = LOWORD(lParam);
        browser->height = HIWORD(lParam);
        return 0;
    }

    if (msg == WM_PAINT) {
        PAINTSTRUCT paint_struct;
        HDC hdc = BeginPaint(hwnd, &paint_struct);

        // Create back buffer
        HDC hdc_buffer = CreateCompatibleDC(hdc);
        HBITMAP bitmap_buffer = CreateCompatibleBitmap(hdc, browser->width, browser->height);
        SelectObject(hdc_buffer, bitmap_buffer);

        // Draw background color
        HBRUSH brush = CreateSolidBrush(0x00342C28);
        RECT rect = { 0, 0, browser->width, browser->height };
        FillRect(hdc_buffer, &rect, brush);
        DeleteObject(brush);

        // Draw folder path
        SelectObject(hdc_buffer, browser->font);
        SetBkMode(hdc_buffer, TRANSPARENT);
        SetTextColor(hdc_buffer, browser->text_color);
        SetTextAlign(hdc_buffer, TA_LEFT);
        TextOutW(hdc_buffer, 0, 0, browser->path, wcslen(browser->path));

        // Draw files
        wchar_t folder_path[MAX_PATH];
        wcscpy(folder_path, browser->path);
        wcscat(folder_path, L"\\*");

        WIN32_FIND_DATAW file_data;
        HANDLE find_handle = FindFirstFileW(folder_path, &file_data);
        if (find_handle != INVALID_HANDLE_VALUE) {
            int32_t y = browser->font_size;
            do {
                if (file_data.cFileName[0] == '.' || (file_data.cFileName[0] == '.' && file_data.cFileName[1] == '.')) {
                    continue;
                }

                if (file_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                    SetTextColor(hdc_buffer, 0x000000ff);
                } else {
                    SetTextColor(hdc_buffer, browser->text_color);
                }
                TextOutW(hdc_buffer, 0, y, file_data.cFileName, wcslen(file_data.cFileName));
                y += browser->font_size;
            } while (FindNextFileW(find_handle, &file_data));
            FindClose(find_handle);
        }

        // Draw and delete back buffer
        BitBlt(hdc, 0, 0, browser->width, browser->height, hdc_buffer, 0, 0, SRCCOPY);
        DeleteObject(bitmap_buffer);
        DeleteDC(hdc_buffer);

        EndPaint(hwnd, &paint_struct);
        return 0;
    }

    if (msg == WM_DESTROY) {
        // Delete GDI objects
        DeleteObject(browser->font);

        // Free browser data
        free(browser);

        return 0;
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

void Browser_Register(void) {
    WNDCLASSEXW wc = { 0 };
    wc.cbSize = sizeof(wc);
    wc.lpszClassName = L"plaatcode_browser";
    wc.hInstance = GetModuleHandleW(NULL);
    wc.lpfnWndProc = Browser_WndProc;
    wc.hCursor = LoadCursorW(NULL, IDC_ARROW);
    RegisterClassExW(&wc);
}
