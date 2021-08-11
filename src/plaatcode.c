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

#define WINDOW_WIDTH 1024
#define WINDOW_HEIGHT 768
#define WINDOW_STYLE WS_OVERLAPPEDWINDOW

typedef struct {
    int32_t width;
    int32_t height;
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
}

int32_t __stdcall WndProc(HWND hwnd, uint32_t msg, WPARAM wParam, LPARAM lParam) {
    WindowData *window = GetWindowLongPtrW(hwnd, GWLP_USERDATA);

    if (msg == WM_CREATE) {
        // Create window data
        window = malloc(sizeof(WindowData));
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, window);
        window->width = WINDOW_WIDTH;
        window->height = WINDOW_HEIGHT;

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
        SetWindowPos(window->editor, NULL, browser_width, 24, window->width - browser_width, window->height - 24 - 24, SWP_NOZORDER);
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
        return 0;
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
        HBRUSH brush = CreateSolidBrush(RGB(17, 17, 17));
        RECT rect = { 0, 0, window->width, window->height };
        FillRect(hdc_buffer, &rect, brush);
        DeleteObject(brush);

        // Draw and delete back buffer
        BitBlt(hdc, 0, 0, window->width, window->height, hdc_buffer, 0, 0, SRCCOPY);
        DeleteObject(bitmap_buffer);
        DeleteDC(hdc_buffer);

        EndPaint(hwnd, &paint_struct);
        return 0;
    }

    if (msg == WM_DESTROY) {
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
