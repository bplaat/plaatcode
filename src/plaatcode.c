#include "win32.h"
#include "browser.h"
#include "editor.h"
#include "resources.h"

wchar_t *window_class_name = L"plaatcode";

#ifdef WIN64
    wchar_t *window_title = L"PlaatCode (64-bit)";
#else
    wchar_t *window_title = L"PlaatCode (32-bit)";
#endif

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 768
#define WINDOW_STYLE WS_OVERLAPPEDWINDOW

typedef struct {
    int32_t width;
    int32_t height;

    uint32_t background_color;
    uint32_t titlebar_background_color;
    uint32_t titlebar_text_color;
    uint32_t statusbar_background_color;
    uint32_t statusbar_text_color;

    HFONT font;
    wchar_t *font_name;
    int32_t font_size;

    int32_t titlebar_height;
    int32_t titlebar_button_width;
    int32_t statusbar_height;
    int32_t statusbar_padding;

    HWND browser;
    HWND editor;
} WindowData;

void UpdateWindowTitle(HWND hwnd) {
    WindowData *window = GetWindowLongPtrW(hwnd, GWLP_USERDATA);
    EditorData *editor = GetWindowLongPtrW(window->editor, GWLP_USERDATA);
    wchar_t title[512];
    if (editor->path == NULL) {
        wcscpy(title, L"Unnamed file");
    } else {
        wcscpy(title, editor->path);
    }
    wcscat(title, L" - ");
    wcscat(title, window_title);
    SendMessageW(hwnd, WM_SETTEXT, NULL, title);

    InvalidateRect(hwnd, NULL, false);
}

int32_t __stdcall WndProc(HWND hwnd, uint32_t msg, WPARAM wParam, LPARAM lParam) {
    WindowData *window = GetWindowLongPtrW(hwnd, GWLP_USERDATA);

    if (msg == WM_CREATE) {
        // Create window data
        window = malloc(sizeof(WindowData));
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, window);
        window->width = WINDOW_WIDTH;
        window->height = WINDOW_HEIGHT;

        // Theming
        window->background_color = 0x00111111;
        window->titlebar_background_color = 0x002B2521;
        window->titlebar_text_color = 0x00ffffff;
        window->statusbar_background_color = 0x002B2521;
        window->statusbar_text_color = 0x00ffffff;

        window->font_name = L"Segoe UI";
        window->font_size = 18;
        window->font = CreateFontW(window->font_size, 0, 0, 0, FW_NORMAL, false, false, false, ANSI_CHARSET,
            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, window->font_name);

        window->titlebar_height = 32;
        window->titlebar_button_width = 48;
        window->statusbar_height = 24;
        window->statusbar_padding = 8;

        // Add about item to system menu
        HMENU systemMenu = GetSystemMenu(hwnd, false);
        InsertMenuW(systemMenu, 5, MF_BYPOSITION | MF_SEPARATOR, NULL, NULL);
        InsertMenuW(systemMenu, 6, MF_BYPOSITION, (HMENU)ID_HELP_ABOUT, L"About");

        // Create browser control
        window->browser = CreateWindowExW(0, L"plaatcode_browser", NULL, WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hwnd, NULL, NULL, NULL);
        SendMessageW(window->browser, WM_BROWSER_OPEN_FOLDER, L"../../..", NULL);

        // Create editor control
        window->editor = CreateWindowExW(0, L"plaatcode_editor", NULL, WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hwnd, NULL, NULL, NULL);
        SendMessageW(window->editor, WM_EDITOR_OPEN_FILE, L"plaatcode.asm", NULL);
        // SendMessageW(window->editor, WM_EDITOR_OPEN_FILE, L"../../../src/editor.c", NULL);
        UpdateWindowTitle(hwnd);
        return 0;
    }

    if (msg == WM_SIZE) {
        // Save new window size
        window->width = LOWORD(lParam);
        window->height = HIWORD(lParam);

        // Resize browser control
        int32_t browser_width = 240;
        SetWindowPos(window->browser, NULL, 0, 24, browser_width, window->height - 24 - 24, SWP_NOZORDER);

        // Resize editor control
        SetWindowPos(window->editor, NULL, browser_width, 64, window->width - browser_width, window->height - 32 - 32 - 24, SWP_NOZORDER);
        return 0;
    }

    if (msg == WM_COMMAND || msg == WM_SYSCOMMAND) {
        int32_t id = LOWORD(wParam);

        if (id == ID_FILE_NEW) {
            SendMessageW(window->editor, WM_EDITOR_OPEN_FILE, NULL, NULL);
            UpdateWindowTitle(hwnd);
        }

        if (id == ID_FILE_OPEN) {
            OPENFILENAMEW open_file_dialog = { 0 };
            wchar_t path[MAX_PATH];
            open_file_dialog.lStructSize = 152;//sizeof(OPENFILENAMEW);
            open_file_dialog.hwndOwner = hwnd;
            open_file_dialog.lpstrFilter = L"All Files (*.*)\0*.*\0";
            open_file_dialog.lpstrFile = path;
            open_file_dialog.nMaxFile = MAX_PATH;
            open_file_dialog.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
            wprintf(L"%s\n", L"Open dialog");
            if (GetOpenFileNameW(&open_file_dialog)) {
                SendMessageW(window->editor, WM_EDITOR_OPEN_FILE, path, NULL);
                UpdateWindowTitle(hwnd);
            }
        }

        if (id == ID_FILE_SAVE || id == ID_FILE_SAVE_AS) {
            EditorData *editor = GetWindowLongPtrW(window->editor, GWLP_USERDATA);
            if (editor->path == NULL || id == ID_FILE_SAVE_AS) {
                OPENFILENAMEW save_file_dialog = { 0 };
                wchar_t path[MAX_PATH];
                save_file_dialog.lStructSize = 152;//sizeof(OPENFILENAMEW);
                save_file_dialog.hwndOwner = hwnd;
                save_file_dialog.lpstrFilter = L"All Files (*.*)\0*.*\0";
                save_file_dialog.lpstrFile = path;
                save_file_dialog.nMaxFile = MAX_PATH;
                save_file_dialog.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
                wprintf(L"%s\n", L"Save dialog");
                if (GetSaveFileNameW(&save_file_dialog)) {
                    SendMessageW(window->editor, WM_EDITOR_CHANGE_PATH, path, NULL);
                    SendMessageW(window->editor, WM_EDITOR_SAVE_FILE, NULL, NULL);
                    UpdateWindowTitle(hwnd);
                }
            } else {
                SendMessageW(window->editor, WM_EDITOR_SAVE_FILE, NULL, NULL);
            }
        }

        if (id == ID_FILE_EXIT) {
            DestroyWindow(hwnd);
        }

        if (id == ID_HELP_ABOUT) {
            MessageBoxW(hwnd, L"A simple code editor made by PlaatSoft for Windows\n"
                "Made by Bastiaan van der Plaat (https://bastiaan.ml/)",
                L"About PlaatCode", MB_ICONINFORMATION | MB_OK);
        }
    }

    if (msg == WM_DROPFILES) {
        HDROP hDropInfo = (HDROP)wParam;
        wchar_t path[MAX_PATH];
        DragQueryFileW(hDropInfo, 0, (wchar_t *)path, MAX_PATH);
        DragFinish(hDropInfo);
        SendMessageW(window->editor, WM_EDITOR_OPEN_FILE, (WPARAM)path, NULL);
        UpdateWindowTitle(hwnd);
        return 0;
    }

    if (msg == WM_LBUTTONUP) {
        int32_t x = GET_X_LPARAM(lParam);
        int32_t y = GET_Y_LPARAM(lParam);

        // Window decoration buttons
        if (
            y >= 0 &&
            y < window->titlebar_height
        ) {
            if (
                x >= window->width - window->titlebar_button_width * 3 &&
                x < window->width - window->titlebar_button_width  * 2
            ) {
                ShowWindow(hwnd, SW_MINIMIZE);
            }

            if (
                x >= window->width - window->titlebar_button_width  * 2 &&
                x < window->width - window->titlebar_button_width  * 1
            ) {
                if (IsZoomed(hwnd)) {
                    ShowWindow(hwnd, SW_RESTORE);
                } else {
                    ShowWindow(hwnd, SW_MAXIMIZE);
                }
            }

            if (
                x >= window->width - window->titlebar_button_width  * 1 &&
                x < window->width
            ) {
                DestroyWindow(hwnd);
            }
        }
    }

    if (msg == WM_KEYDOWN || msg == WM_CHAR) {
        SendMessageW(window->editor, msg, wParam, lParam);
        return 0;
    }

    if (msg == WM_GETMINMAXINFO) {
        // Set window min size
        MINMAXINFO *minMaxInfo = (MINMAXINFO *)lParam;
        RECT window_rect = { 0, 0, 640, 480 };
        AdjustWindowRectEx(&window_rect, WINDOW_STYLE, true, 0);
        minMaxInfo->ptMinTrackSize.x = window_rect.right - window_rect.left;
        minMaxInfo->ptMinTrackSize.y = window_rect.bottom - window_rect.top;
        return 0;
    }

    if (msg == WM_ERASEBKGND) {
        // Draw no background
        return TRUE;
    }

    if (msg == WM_PAINT) {
        PAINTSTRUCT paint_struct;
        HDC hdc = BeginPaint(hwnd, &paint_struct);

        // Create back buffer
        HDC hdc_buffer = CreateCompatibleDC(hdc);
        HBITMAP bitmap_buffer = CreateCompatibleBitmap(hdc, window->width, window->height);
        SelectObject(hdc_buffer, bitmap_buffer);

        // Draw background color
        HBRUSH brush = CreateSolidBrush(window->background_color);
        RECT rect = { 0, 0, window->width, window->height };
        FillRect(hdc_buffer, &rect, brush);
        DeleteObject(brush);

        // Draw titlebar
        {
            // Draw statusbar
            HBRUSH header_brush = CreateSolidBrush(window->titlebar_background_color);
            RECT header_rect = { 0, 0, window->width, window->titlebar_height };
            FillRect(hdc_buffer, &header_rect, header_brush);
            DeleteObject(header_brush);

            // Draw title
            SelectObject(hdc_buffer, window->font);
            SetBkMode(hdc_buffer, TRANSPARENT);
            SetTextColor(hdc_buffer, window->titlebar_text_color);

            SetTextAlign(hdc_buffer, TA_CENTER);
            wchar_t title_buffer[320];
            SendMessageW(hwnd, WM_GETTEXT, (WPARAM)320, title_buffer);
            TextOutW(hdc_buffer, window->width / 2, (window->titlebar_height - window->font_size) / 2, title_buffer, wcslen(title_buffer));

            // Draw custom window decoration
            int32_t x = window->width - window->titlebar_button_width * 3;
            wchar_t *text = L"\u2581";
            TextOutW(hdc_buffer, x + window->titlebar_button_width / 2, (window->titlebar_height - window->font_size) / 2, text, wcslen(text));
            x += window->titlebar_button_width;

            text = IsZoomed(hwnd) ? L"\u25F3" : L"\u25A1";
            TextOutW(hdc_buffer, x + window->titlebar_button_width / 2, (window->titlebar_height - window->font_size) / 2, text, wcslen(text));
            x += window->titlebar_button_width;

            text = L"\u2A09";
            TextOutW(hdc_buffer, x + window->titlebar_button_width / 2, (window->titlebar_height - window->font_size) / 2, text, wcslen(text));
            x += window->titlebar_button_width;
        }

        // Draw statusbar
        {
            int32_t y = window->height - window->statusbar_height;

            // Draw statusbar
            HBRUSH statusbar_brush = CreateSolidBrush(window->statusbar_background_color);
            RECT statusbar_rect = { 0, y, window->width, window->height };
            FillRect(hdc_buffer, &statusbar_rect, statusbar_brush);
            DeleteObject(statusbar_brush);

            // Draw some text
            SelectObject(hdc_buffer, window->font);
            SetBkMode(hdc_buffer, TRANSPARENT);
            SetTextColor(hdc_buffer, window->statusbar_text_color);

            SetTextAlign(hdc_buffer, TA_LEFT);
            wchar_t *text = L"PlaatCode definitely not a bad vscode rip off 🙂";
            TextOutW(hdc_buffer, window->statusbar_padding, y + (window->statusbar_height - window->font_size) / 2, text, wcslen(text));

            int32_t x = window->width - window->statusbar_padding;
            SetTextAlign(hdc_buffer, TA_RIGHT);

            text = L"UTF-8";
            TextOutW(hdc_buffer, x, y + (window->statusbar_height - window->font_size) / 2, text, wcslen(text));
            x -= 128 - window->statusbar_padding;

            text = L"Windows (CR LF)";
            TextOutW(hdc_buffer, x, y + (window->statusbar_height - window->font_size) / 2, text, wcslen(text));
            x -= 128 - window->statusbar_padding;

            text = L"Spaces: 4";
            TextOutW(hdc_buffer, x, y + (window->statusbar_height - window->font_size) / 2, text, wcslen(text));
            x -= 128 - window->statusbar_padding;

            text = L"Ln: 1, Col: 1";
            TextOutW(hdc_buffer, x, y + (window->statusbar_height - window->font_size) / 2, text, wcslen(text));
            x -= 128 - window->statusbar_padding;
        }

        // Draw and delete back buffer
        BitBlt(hdc, 0, 0, window->width, window->height, hdc_buffer, 0, 0, SRCCOPY);
        DeleteObject(bitmap_buffer);
        DeleteDC(hdc_buffer);

        EndPaint(hwnd, &paint_struct);
        return 0;
    }

    if (msg == WM_DESTROY) {
        // Delete GDI objects
        DeleteObject(window->font);

        // Free window data
        free(window);

        // Close process
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

void _start(void) {
    INITCOMMONCONTROLSEX icc;
    icc.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icc.dwICC = ICC_WIN95_CLASSES;
    InitCommonControlsEx(&icc);

    Browser_Register();
    Editor_Register();

    WNDCLASSEXW wc = {0};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandleW(NULL);
    wc.hIcon = LoadImageW(wc.hInstance, (wchar_t *)ID_ICON, IMAGE_ICON, 0, 0, LR_DEFAULTSIZE | LR_DEFAULTCOLOR | LR_SHARED);
    wc.hCursor = LoadCursorW(NULL, IDC_ARROW);
    wc.lpszClassName = window_class_name;
    wc.lpszMenuName = (HMENU)ID_MENU;
    wc.hIconSm = LoadImageW(wc.hInstance, (wchar_t *)ID_ICON, IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR | LR_SHARED);
    RegisterClassExW(&wc);

    RECT window_rect;
    window_rect.left = (GetSystemMetrics(SM_CXSCREEN) - WINDOW_WIDTH) / 2;
    window_rect.top = (GetSystemMetrics(SM_CYSCREEN) - WINDOW_HEIGHT) / 2;
    window_rect.right = window_rect.left + WINDOW_WIDTH;
    window_rect.bottom = window_rect.top + WINDOW_HEIGHT;
    AdjustWindowRectEx(&window_rect, WINDOW_STYLE, true, 0);

    HWND hwnd = CreateWindowExW(WS_EX_ACCEPTFILES, window_class_name, window_title,
        WINDOW_STYLE, window_rect.left, window_rect.top,
        window_rect.right - window_rect.left, window_rect.bottom - window_rect.top,
        NULL, NULL, wc.hInstance, NULL);
    ShowWindow(hwnd, SW_SHOWDEFAULT);
    UpdateWindow(hwnd);

    HACCEL haccl = LoadAcceleratorsW(wc.hInstance, (wchar_t *)ID_ACCELERATOR);
    MSG message;
    while (GetMessageW(&message, NULL, 0, 0) > 0) {
        if (!TranslateAcceleratorW(hwnd, haccl, &message)) {
            TranslateMessage(&message);
            DispatchMessageW(&message);
        }
    }
    ExitProcess((int32_t)(uintptr_t)message.wParam);
}
