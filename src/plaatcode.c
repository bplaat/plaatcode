#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>
#include <shellapi.h>

#include "plaatcode.h"
#include "editor.h"

COLORREF window_background_color = RGB(30, 30, 30);
COLORREF menubar_background_color = RGB(50, 50, 51);
COLORREF line_numbers_color = RGB(200, 200, 200);
COLORREF code_color = RGB(255, 255, 255);
COLORREF scrollbar_background_color = RGB(60, 60, 60);
COLORREF scrollbar_thumb_background_color = RGB(120, 120, 120);
COLORREF statusbar_background_color = RGB(0, 122, 204);

char *window_class_name = "plaatcode";
int window_width = 1200;
int window_height = 800;
int menubar_size = 24;
int statusbar_size = 24;

HFONT code_font;
int code_font_size = 20;
HWND editor_hwnd;

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_CREATE) {
        HMENU systemMenu = GetSystemMenu(hwnd, FALSE);
        InsertMenu(systemMenu, 5, MF_BYPOSITION | MF_SEPARATOR, 0, NULL);
        InsertMenu(systemMenu, 6, MF_BYPOSITION, ID_HELP_ABOUT, "About");

        RECT rect;
        GetClientRect(hwnd, &rect);
        int new_width = window_width * 2 - rect.right;
        int new_height = window_height * 2 - rect.bottom;
        SetWindowPos(hwnd, NULL, (GetSystemMetrics(SM_CXSCREEN) - new_width) / 2, (GetSystemMetrics(SM_CYSCREEN) - new_height) / 2, new_width, new_height, SWP_NOZORDER);

        InitEditorControl();

        code_font = CreateFont(code_font_size, 0, 0, 0, FW_NORMAL, 0, 0, 0, 0, 0, 0, CLEARTYPE_QUALITY, 0, "Consolas");

        editor_hwnd = CreateWindowEx(0, "plaatcode_editor", "", WS_VISIBLE | WS_CHILD,
            0, menubar_size, window_width, window_height - menubar_size - statusbar_size, hwnd, NULL, GetModuleHandle(NULL), NULL);
        SendMessage(editor_hwnd, WM_SETFONT, (WPARAM)code_font, TRUE);

        EditorData *data = (EditorData *)GetWindowLongPtr(editor_hwnd, GWLP_USERDATA);
        data->window_background_color = window_background_color;
        data->line_numbers_color = line_numbers_color;
        data->code_color = code_color;
        data->scrollbar_background_color = scrollbar_background_color;
        data->scrollbar_thumb_background_color = scrollbar_thumb_background_color;

        SendMessage(editor_hwnd, WM_EDITOR_OPEN_FILE, (WPARAM)"src/plaatcode.c", 0);

        return 0;
    }

    if (msg == WM_SIZE) {
        window_width = LOWORD(lParam);
        window_height = HIWORD(lParam);
        SetWindowPos(editor_hwnd, 0, 0, menubar_size, window_width, window_height - menubar_size - statusbar_size, SWP_NOZORDER);
        return 0;
    }

    if (msg == WM_GETMINMAXINFO) {
        LPMINMAXINFO lpMMI = (LPMINMAXINFO)lParam;
        lpMMI->ptMinTrackSize.x = 320;
        lpMMI->ptMinTrackSize.y = 240;
        return 0;
    }

    if (msg == WM_ERASEBKGND) {
        return TRUE;
    }

    if (msg == WM_COMMAND || msg == WM_SYSCOMMAND) {
        int id = LOWORD(wParam);

        if (id == ID_FILE_NEW) {

        }

        if (id == ID_FILE_OPEN) {

        }

        if (id == ID_FILE_SAVE) {

        }

        if (id == ID_FILE_SAVE_AS) {

        }

        if (id == ID_FILE_EXIT) {
            DestroyWindow(hwnd);
        }

        if (id == ID_HELP_ABOUT) {
            MessageBox(hwnd, "A simple code editor made by PlaatSoft for Windows\nMade by Bastiaan van der Plaat (https://bastiaan.ml/)", "About PlaatCode", MB_ICONINFORMATION | MB_OK);
        }
    }

    if (msg == WM_PAINT) {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        // Create double buffer
        HDC hdcMem = CreateCompatibleDC(hdc);
        HBITMAP hbmMem = CreateCompatibleBitmap(hdc, window_width, window_height);
        HANDLE hOld = SelectObject(hdcMem, hbmMem);

        // Clear window
        RECT window_rect = { 0, 0, window_width, window_height };
        SetBkColor(hdcMem, window_background_color);
        ExtTextOut(hdcMem, 0, 0, ETO_OPAQUE, &window_rect, "", 0, NULL);

        // Draw menubar
        RECT menubar_rect = { 0, 0, window_width, menubar_size };
        SetBkColor(hdcMem, menubar_background_color);
        ExtTextOut(hdcMem, 0, 0, ETO_OPAQUE, &menubar_rect, "", 0, NULL);

        // Draw statusbar
        RECT statusbar_rect = { 0, window_height - statusbar_size, window_width, window_height };
        SetBkColor(hdcMem, statusbar_background_color);
        ExtTextOut(hdcMem, 0, 0, ETO_OPAQUE, &statusbar_rect, "", 0, NULL);

        // Blit double buffer
        BitBlt(hdc, 0, 0, window_width, window_height, hdcMem, 0, 0, SRCCOPY);
        SelectObject(hdcMem, hOld);
        DeleteObject(hbmMem);
        DeleteDC (hdcMem);

        EndPaint(hwnd, &ps);
        return 0;
    }

    if (msg == WM_DROPFILES) {
        HDROP hDropInfo = (HDROP)wParam;
        char path[MAX_PATH];
        DragQueryFile(hDropInfo, 0, (LPSTR)path, sizeof(path));
        DragFinish(hDropInfo);
        SendMessage(editor_hwnd, WM_EDITOR_OPEN_FILE, (WPARAM)path, 0);
        return 0;
    }

    if (msg == WM_DESTROY) {
        DeleteObject(code_font);
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    (void)hPrevInstance;
    (void)lpCmdLine;

    INITCOMMONCONTROLSEX icc;
    icc.dwSize = sizeof(icc);
    icc.dwICC = ICC_WIN95_CLASSES;
    InitCommonControlsEx(&icc);

    WNDCLASSEX wc = { 0 };
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style =  CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(ID_ICON));
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszMenuName = MAKEINTRESOURCE(ID_MENU);
    wc.lpszClassName = window_class_name;
    wc.hIconSm = (HICON)LoadImage(hInstance, MAKEINTRESOURCE(ID_ICON), IMAGE_ICON, 16, 16, 0);
    RegisterClassEx(&wc);

    HACCEL hAccelerators = LoadAccelerators(hInstance, MAKEINTRESOURCE(ID_ACCELERATOR));

    HWND hwnd = CreateWindowEx(WS_EX_ACCEPTFILES, window_class_name, "PlaatCode",
        WS_OVERLAPPEDWINDOW, 0, 0, window_width, window_height, NULL, NULL, hInstance, NULL);
    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG message;
    while (GetMessage(&message, NULL, 0, 0) > 0) {
        if (!TranslateAccelerator(message.hwnd, hAccelerators, &message)) {
            TranslateMessage(&message);
            DispatchMessage(&message);
        }
    }
    return message.wParam;
}
