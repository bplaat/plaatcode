#include "editor.h"
#include "wchar_list.h"
#include "lexers.h"

// TODO
Lexer lexers[] = {
    { L"s", Lexer_Assembly },
    { L"asm", Lexer_Assembly },
    { L"c", Lexer_C },
    { L"h", Lexer_C }
};

// Editor control functions
void Editor_UpdateLines(EditorData *editor) {
    // Generate line number format string
    int32_t line_number_size = 1;
    int32_t c = editor->lines->size;
    while (c > 10) {
        c /= 10;
        line_number_size++;
    }
    if (line_number_size < 3) line_number_size = 3;
    if (editor->debug) {
        wsprintfW(editor->line_numbers_format, L"%%0%dd %%03d/%%03d", line_number_size);
    } else {
        wsprintfW(editor->line_numbers_format, L"%%0%dd", line_number_size);
    }

    // Measure line number width
    HDC hdc = GetDC(NULL);
    SelectObject(hdc, editor->font);
    wchar_t line_number_buffer[32];
    if (editor->debug) {
        wsprintfW(line_number_buffer, editor->line_numbers_format, 0, 0, 0);
    } else {
        wsprintfW(line_number_buffer, editor->line_numbers_format, 0);
    }
    SIZE measure_size;
    GetTextExtentPoint32W(hdc, line_number_buffer, wcslen(line_number_buffer), &measure_size);
    DeleteDC(hdc);
    editor->line_numbers_width = editor->padding + (measure_size.cx) + editor->padding;
}

void Editor_UpdateScroll(EditorData *editor) {
    // Calculate horizontal scroll viewport
    editor->hscroll_viewport = editor->width - editor->line_numbers_width - editor->scrollbar_size;

    // Calculate horizontal scroll size
    WcharList *largest_line = editor->lines->items[0];
    for (int32_t i = 1; i < editor->lines->size; i++) {
        WcharList *line = editor->lines->items[i];
        if (line->size > largest_line->size) {
            largest_line = line;
        }
    }

    HDC hdc = GetDC(NULL);
    SelectObject(hdc, editor->font);
    SIZE measure_size;
    GetTextExtentPoint32W(hdc, largest_line->items, largest_line->size, &measure_size);
    editor->hscroll_size = measure_size.cx + editor->padding;
    DeleteDC(hdc);

    // Calculate horizontal scroll offset
    if (editor->hscroll_offset + editor->hscroll_viewport > editor->hscroll_size) {
        editor->hscroll_offset = editor->hscroll_size - editor->hscroll_viewport;

        if (editor->hscroll_offset < 0) {
            editor->hscroll_offset = 0;
        }
    }

    // Calculate vertical scroll viewport
    editor->vscroll_viewport = editor->height;

    // Calculate vertical scroll size
    editor->vscroll_size = editor->font_size * editor->lines->size +
        (editor->vscroll_viewport - editor->font_size);
}

int32_t __stdcall Editor_WndProc(HWND hwnd, uint32_t msg, WPARAM wParam, LPARAM lParam) {
    EditorData *editor = GetWindowLongPtrW(hwnd, GWLP_USERDATA);

    if (msg == WM_CREATE) {
        // Create editor data
        editor = malloc(sizeof(EditorData));
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, editor);
        #ifdef DEBUG
            editor->debug = true;
        #else
            editor->debug = false;
        #endif

        editor->background_color = 0x00342C28;
        editor->scrollbar_background_color = 0x0066564E;
        editor->scrollbar_thumb_background_color = 0x00917D74;
        editor->line_numbers_text_color = 0x00836D63;
        editor->line_numbers_active_text_color = 0x00BFB2AB;
        editor->token_normal_text_color = 0x00BFB2AB;
        editor->token_space_text_color = 0x00555555;
        editor->token_comment_text_color = 0x0070635C;
        editor->token_keyword_text_color = 0x00DD78C6;
        editor->token_constant_text_color = 0x00669AD1;
        editor->token_string_text_color = 0x0079C398;

        editor->font_name = L"Consolas";
        editor->font_size = 20;
        editor->font = CreateFontW(editor->font_size, 0, 0, 0, FW_NORMAL, false, false, false, ANSI_CHARSET,
            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, editor->font_name);

        editor->indentation_character = ' ';
        editor->indentation_size = 4;

        editor->path = NULL;
        editor->extension = NULL;

        // Create lines with one empty line
        editor->lines = List_New(128);
        List_Add(editor->lines, WcharList_New(128));

        editor->scrollbar_size = 16;
        editor->padding = 16;
        editor->line_numbers_format = malloc(sizeof(wchar_t) * 32);
        editor->hscroll_offset = 0;
        editor->hscroll_down = false;
        editor->vscroll_offset = 0;
        editor->vscroll_down = false;

        editor->cursor_x = 0;
        editor->cursor_y = 0;

        return 0;
    }

    if (msg == WM_EDITOR_OPEN_FILE) {
        // Free old lines
        for (int32_t i = 0; i < editor->lines->size; i++) {
            WcharList_Free(editor->lines->items[i]);
        }
        List_Free(editor->lines, NULL); // (void (*)(void *item))WcharList_Free);

        // Allocate new lines list
        editor->lines = List_New(128);

        // Reset cursor
        editor->cursor_x = 0;
        editor->cursor_y = 0;

        // New file
        if (wParam == NULL) {
            if (editor->path != NULL) {
                free(editor->path);
            }
            editor->path = NULL;
            editor->extension = NULL;

            List_Add(editor->lines, WcharList_New(128));

            editor->hscroll_offset = 0;
            editor->vscroll_offset = 0;
            InvalidateRect(hwnd, NULL, false);
            return 0;
        }

        // Get full file path
        wchar_t path[MAX_PATH];
        GetFullPathNameW(wParam, MAX_PATH, path, NULL);

        // Read file
        HANDLE file = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
        if (file != INVALID_HANDLE_VALUE) {
            if (editor->path != NULL) {
                free(editor->path);
            }
            editor->path = wcsdup(path);
            for (int32_t i = wcslen(editor->path); i >= 0; i--) {
                if (editor->path[i] == '/' || editor->path[i] == '\\') {
                    editor->extension = NULL;
                    break;
                }
                if (editor->path[i] == '.') {
                    editor->extension = &editor->path[i + 1];
                    break;
                }
            }

            uint32_t file_size = GetFileSize(file, NULL);
            char *file_buffer = malloc(file_size);
            uint32_t bytes_read;
            ReadFile(file, file_buffer, file_size, &bytes_read, 0);
            CloseHandle(file);

            int32_t converted_size = MultiByteToWideChar(CP_ACP, 0, file_buffer, bytes_read, NULL, 0);
            wchar_t *converted_buffer = malloc(sizeof(wchar_t) * converted_size);
            MultiByteToWideChar(CP_ACP, 0, file_buffer, bytes_read, converted_buffer, converted_size);
            free(file_buffer);

            // Create line
            WcharList *line = WcharList_New(128);
            for (int32_t i = 0; i < (int32_t)bytes_read; i++) {
                if (converted_buffer[i] == '\r') {
                    i++;
                }
                if (converted_buffer[i] == '\n') {
                    List_Add(editor->lines, line);
                    line = WcharList_New(128);
                } else {
                    WcharList_Add(line, converted_buffer[i]);
                }
            }
            List_Add(editor->lines, line);

            free(converted_buffer);
            editor->hscroll_offset = 0;
            editor->vscroll_offset = 0;
            InvalidateRect(hwnd, NULL, false);
        } else {
            MessageBoxW(HWND_DESKTOP, path, L"Can't open file", MB_OK);
        }
        return 0;
    }

    if (msg == WM_EDITOR_CHANGE_PATH) {
        // Get full file path
        wchar_t path[MAX_PATH];
        GetFullPathNameW(wParam, MAX_PATH, path, NULL);

        if (editor->path != NULL) {
            free(editor->path);
        }
        editor->path = wcsdup(path);
        for (int32_t i = wcslen(editor->path); i >= 0; i--) {
            if (editor->path[i] == '/' || editor->path[i] == '\\') {
                editor->extension = NULL;
                break;
            }
            if (editor->path[i] == '.') {
                editor->extension = &editor->path[i + 1];
                break;
            }
        }

        InvalidateRect(hwnd, NULL, false);
    }

    if (msg == WM_EDITOR_SAVE_FILE) {
        HANDLE file = CreateFileW(editor->path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (file != INVALID_HANDLE_VALUE) {
            // Insert final empty line
            WcharList *last_line = editor->lines->items[editor->lines->size - 1];
            if (last_line->size > 0) {
                List_Add(editor->lines, WcharList_New(128));
            }

            // Count largest line
            WcharList *biggest_line = editor->lines->items[0];
            for (int32_t i = 1; i < editor->lines->size; i++) {
                WcharList *line = editor->lines->items[i];
                if (line->capacity > biggest_line->capacity) {
                    biggest_line = line;
                }
            }

            char *convert_buffer = malloc(sizeof(wchar_t) * biggest_line->capacity * 2);
            for (int32_t i = 0; i < editor->lines->size; i++) {
                WcharList *line = editor->lines->items[i];

                // Trim trailing whitespace
                int32_t trimming = 0;
                for (int32_t j = line->size - 1; j >= 0; j--) {
                    if (line->items[j] == ' ') {
                        trimming++;
                    } else {
                        break;
                    }
                }
                line->size -= trimming;
                if (editor->cursor_y == i) {
                    editor->cursor_x -= trimming;
                }

                int32_t converted_size = WideCharToMultiByte(CP_UTF8, 0, line->items, line->size,
                    convert_buffer, sizeof(wchar_t) * biggest_line->capacity * 4, NULL, NULL);
                uint32_t bytes_written;
                WriteFile(file, convert_buffer, converted_size, &bytes_written, NULL);

                if (i != editor->lines->size - 1) {
                    char eol = '\n';
                    WriteFile(file, &eol, 1, &bytes_written, NULL);
                }
            }
            free(convert_buffer);

            CloseHandle(file);

            InvalidateRect(hwnd, NULL, false);
        }
    }

    if (msg == WM_SIZE) {
        // Save new editor size
        editor->width = LOWORD(lParam);
        editor->height = HIWORD(lParam);
        return 0;
    }

    if (msg == WM_LBUTTONDOWN) {
        int32_t xPos = GET_X_LPARAM(lParam);
        int32_t yPos = GET_Y_LPARAM(lParam);

        if (
            xPos >= editor->line_numbers_width && xPos < editor->width - editor->scrollbar_size &&
            yPos >= editor->vscroll_viewport - editor->scrollbar_size
        ) {
            editor->hscroll_down = true;
            SetCapture(hwnd);

            editor->hscroll_offset = (xPos - (editor->hscroll_viewport * editor->hscroll_viewport / editor->hscroll_size / 2)) * editor->hscroll_size / editor->hscroll_viewport;

            if (editor->hscroll_offset + editor->hscroll_viewport > editor->hscroll_size) {
                editor->hscroll_offset = editor->hscroll_size - editor->hscroll_viewport;
            }

            if (editor->hscroll_offset < 0) {
                editor->hscroll_offset = 0;
            }

            InvalidateRect(hwnd, NULL, false);
            return 0;
        }

        if (xPos >= editor->width - editor->scrollbar_size) {
            editor->vscroll_down = true;
            SetCapture(hwnd);

            editor->vscroll_offset = (yPos - (editor->vscroll_viewport * editor->vscroll_viewport / editor->vscroll_size / 2)) * editor->vscroll_size / editor->vscroll_viewport;

            if (editor->vscroll_offset + editor->vscroll_viewport > editor->vscroll_size) {
                editor->vscroll_offset = editor->vscroll_size - editor->vscroll_viewport;
            }

            if (editor->vscroll_offset < 0) {
                editor->vscroll_offset = 0;
            }

            InvalidateRect(hwnd, NULL, false);
            return 0;
        }

        return 0;
    }

    if (msg == WM_MOUSEMOVE) {
        int xPos = GET_X_LPARAM(lParam);
        int yPos = GET_Y_LPARAM(lParam);

        if (editor->hscroll_down && xPos >= editor->line_numbers_width && xPos < editor->width - editor->scrollbar_size) {
            editor->hscroll_offset = (xPos - (editor->hscroll_viewport * editor->hscroll_viewport / editor->hscroll_size / 2)) * editor->hscroll_size / editor->hscroll_viewport;

            if (editor->hscroll_offset + editor->hscroll_viewport > editor->hscroll_size) {
                editor->hscroll_offset = editor->hscroll_size - editor->hscroll_viewport;
            }

            if (editor->hscroll_offset < 0) {
                editor->hscroll_offset = 0;
            }

            InvalidateRect(hwnd, NULL, false);
        }

        if (editor->vscroll_down && yPos >= 0 && yPos < editor->vscroll_viewport) {
            editor->vscroll_offset = (yPos - (editor->vscroll_viewport * editor->vscroll_viewport / editor->vscroll_size / 2)) * editor->vscroll_size / editor->vscroll_viewport;

            if (editor->vscroll_offset + editor->vscroll_viewport > editor->vscroll_size) {
                editor->vscroll_offset = editor->vscroll_size - editor->vscroll_viewport;
            }

            if (editor->vscroll_offset < 0) {
                editor->vscroll_offset = 0;
            }


            InvalidateRect(hwnd, NULL, false);
        }

        return 0;
    }

    if (msg == WM_LBUTTONUP) {
        if (editor->hscroll_down) {
            editor->hscroll_down = false;
            ReleaseCapture();
        }

        if (editor->vscroll_down) {
            editor->vscroll_down = false;
            ReleaseCapture();
        }

        return 0;
    }

    if (msg == WM_MOUSEWHEEL) {
        editor->vscroll_offset -= GET_WHEEL_DELTA_WPARAM(wParam);

        if (editor->vscroll_offset + editor->vscroll_viewport > editor->vscroll_size) {
            editor->vscroll_offset = editor->vscroll_size - editor->vscroll_viewport;
        }

        if (editor->vscroll_offset < 0) {
            editor->vscroll_offset = 0;
        }

        InvalidateRect(hwnd, NULL, false);
        return 0;
    }

    if (msg == WM_MOUSEHWHEEL) {
        editor->hscroll_offset += GET_WHEEL_DELTA_WPARAM(wParam);

        if (editor->hscroll_offset + editor->hscroll_viewport > editor->hscroll_size) {
            editor->hscroll_offset = editor->hscroll_size - editor->hscroll_viewport;
        }

        if (editor->hscroll_offset < 0) {
            editor->hscroll_offset = 0;
        }

        InvalidateRect(hwnd, NULL, false);
        return 0;
    }

    if (msg == WM_KEYDOWN) {
        uint32_t key = (uintptr_t)wParam;

        if (key == VK_LEFT) {
            if (editor->cursor_x > 0) {
                editor->cursor_x--;
            } else if (editor->cursor_y > 0) {
                WcharList *previous_line = editor->lines->items[--editor->cursor_y];
                editor->cursor_x = previous_line->size;
            }
        }
        if (key == VK_RIGHT) {
            WcharList *current_line = editor->lines->items[editor->cursor_y];
            if (editor->cursor_x < current_line->size) {
                editor->cursor_x++;
            } else if (editor->cursor_y < editor->lines->size - 1) {
                editor->cursor_x = 0;
                editor->cursor_y++;
            }
        }
        if (key == VK_UP && editor->cursor_y > 0) {
            WcharList *previous_line = editor->lines->items[--editor->cursor_y];
            if (editor->cursor_x > previous_line->size) {
                editor->cursor_x = previous_line->size;
            }
        }
        if (key == VK_DOWN && editor->cursor_y < editor->lines->size - 1) {
            WcharList *next_line = editor->lines->items[++editor->cursor_y];
            if (editor->cursor_x > next_line->size) {
                editor->cursor_x = next_line->size;
            }
        }

        if (key == VK_BACK) {
            WcharList *current_line = editor->lines->items[editor->cursor_y];
            if (editor->cursor_x > 0) {
                if (editor->cursor_x >= editor->indentation_size) {
                    bool are_spaces = true;
                    for (int32_t i = 0; i < editor->indentation_size; i++) {
                        if (current_line->items[editor->cursor_x - 1 - i] != ' ') {
                            are_spaces = false;
                            break;
                        }
                    }
                    if (are_spaces) {
                        for (int32_t i = 0; i < editor->indentation_size - 1; i++) {
                            WcharList_Remove(current_line, --editor->cursor_x);
                        }
                    }
                }
                WcharList_Remove(current_line, --editor->cursor_x);
            } else if (editor->cursor_y > 0) {
                WcharList *previous_line = editor->lines->items[--editor->cursor_y];
                editor->cursor_x = previous_line->size;
                for (int32_t i = 0; i < current_line->size; i++) {
                    WcharList_Add(previous_line, current_line->items[i]);
                }
                List_Remove(editor->lines, editor->cursor_y + 1);
                WcharList_Free(current_line);
            }
        }

        if (key == VK_DELETE) {
            WcharList *current_line = editor->lines->items[editor->cursor_y];
            if (editor->cursor_x < current_line->size) {
                if (editor->cursor_x < current_line->size - editor->indentation_size) {
                    bool are_spaces = true;
                    for (int32_t i = 0; i < editor->indentation_size; i++) {
                        if (current_line->items[editor->cursor_x + i] != ' ') {
                            are_spaces = false;
                            break;
                        }
                    }
                    if (are_spaces) {
                        for (int32_t i = 0; i < editor->indentation_size - 1; i++) {
                            WcharList_Remove(current_line, editor->cursor_x);
                        }
                    }
                }
                WcharList_Remove(current_line, editor->cursor_x);
            } else if (editor->cursor_y < editor->lines->size - 1) {
                WcharList *next_line = editor->lines->items[editor->cursor_y + 1];
                for (int32_t i = 0; i < next_line->size; i++) {
                    WcharList_Add(current_line, next_line->items[i]);
                }

                List_Remove(editor->lines, editor->cursor_y + 1);
                WcharList_Free(next_line);
            }
        }

        if (key == VK_TAB) {
            if (editor->indentation_character == ' ') {
                for (int32_t i = 0; i < editor->indentation_size; i++) {
                    SendMessageW(hwnd, WM_CHAR, (void *)L' ', NULL);
                }
            }
        }

        if (key == VK_RETURN) {
            WcharList *current_line = editor->lines->items[editor->cursor_y];

            int32_t indentation_amount = 0;
            while (
                indentation_amount < current_line->size &&
                indentation_amount < editor->cursor_x &&
                current_line->items[indentation_amount] == editor->indentation_character
            ) {
                indentation_amount++;
            }

            WcharList *new_line = WcharList_New(128);
            for (int32_t i = 0; i < indentation_amount; i++) {
                WcharList_Add(new_line, editor->indentation_character);
            }
            for (int32_t i = editor->cursor_x; i < current_line->size; i++) {
                WcharList_Add(new_line, current_line->items[i]);
            }
            current_line->size = editor->cursor_x;
            List_Insert(editor->lines, ++editor->cursor_y, new_line);
            editor->cursor_x = indentation_amount;
        }

        InvalidateRect(hwnd, NULL, false);
        return 0;
    }

    if (msg == WM_CHAR) {
        wchar_t character = (intptr_t)wParam;
        if (character >= ' ' && character <= '~') {
            WcharList *line = editor->lines->items[editor->cursor_y];
            WcharList_Insert(line, editor->cursor_x++, character);
            InvalidateRect(hwnd, NULL, false);
        }
        return 0;
    }

    // Draw no background
    if (msg == WM_ERASEBKGND) {
        return true;
    }

    if (msg == WM_PAINT) {
        // Update stuff
        Editor_UpdateLines(editor);
        Editor_UpdateScroll(editor);

        // Start drawing
        PAINTSTRUCT paint_struct;
        HDC hdc = BeginPaint(hwnd, &paint_struct);

        // Create back buffer
        HDC hdc_buffer = CreateCompatibleDC(hdc);
        HBITMAP bitmap_buffer = CreateCompatibleBitmap(hdc, editor->width, editor->height);
        SelectObject(hdc_buffer, bitmap_buffer);

        // Draw background color
        HBRUSH background_brush = CreateSolidBrush(editor->background_color);
        RECT background_rect = { 0, 0, editor->width, editor->height };
        FillRect(hdc_buffer, &background_rect, background_brush);
        DeleteObject(background_brush);

        // Allocate tokens buffer of right size
        int32_t largest_line_capacity = ((WcharList *)editor->lines->items[0])->capacity;
        for (int32_t i = 1; i < editor->lines->size; i++) {
            WcharList *line = editor->lines->items[i];
            if (line->capacity > largest_line_capacity) {
                largest_line_capacity = line->capacity;
            }
        }
        uint8_t *tokens = malloc(largest_line_capacity);

        // Select right language lexer
        Lexer *lexer = NULL;
        if (editor->extension != NULL) {
            for (int32_t i = 0; i < LEXERS_SIZE; i++) {
                if (!wcscmp(lexers[i].extension, editor->extension)) {
                    lexer = &lexers[i];
                    break;
                }
            }
        }

        // Draw code lines
        SelectObject(hdc_buffer, editor->font);
        SetBkMode(hdc_buffer, TRANSPARENT);
        SetTextAlign(hdc_buffer, TA_LEFT);
        for (int32_t i = 0; i < editor->lines->size; i++) {
            WcharList *line = editor->lines->items[i];
            int32_t x = editor->line_numbers_width - editor->hscroll_offset;
            int32_t y = editor->font_size * i - editor->vscroll_offset;
            if (
                y >= -(int32_t)editor->font_size &&
                y + editor->font_size < editor->height + editor->font_size
            ) {
                if (lexer != NULL) {
                    lexer->lexer_function(line, tokens);
                } else {
                    for (int32_t j = 0; j < line->size; j++) {
                        if (line->items[j] == ' ') {
                            tokens[j] = TOKEN_SPACE;
                        } else {
                            tokens[j] = TOKEN_NORMAL;
                        }
                    }
                }

                int32_t j = 0;
                while (j < line->size) {
                    uint8_t token = tokens[j];
                    int32_t token_amount = 1;
                    while (j + token_amount < line->size && tokens[j + token_amount] == token) {
                        token_amount++;
                    }

                    if (token == TOKEN_SPACE) {
                        if (token_amount > 64) {
                            token_amount = 64;
                        }
                        wchar_t spaces[64];
                        for (int32_t i = 0; i < token_amount; i++) {
                            spaces[i] = 0x00b7;
                        }
                        SetTextColor(hdc_buffer, editor->token_space_text_color);
                        ExtTextOutW(hdc_buffer, x, y, 0, NULL, spaces, token_amount, NULL);
                        SIZE measure_size;
                        GetTextExtentPoint32W(hdc_buffer, spaces, token_amount, &measure_size);
                        x += measure_size.cx;
                        j += token_amount;
                        continue;
                    }

                    if (token == TOKEN_NORMAL) {
                        SetTextColor(hdc_buffer, editor->token_normal_text_color);
                    }
                    if (token == TOKEN_COMMENT) {
                        SetTextColor(hdc_buffer, editor->token_comment_text_color);
                    }
                    if (token == TOKEN_KEYWORD) {
                        SetTextColor(hdc_buffer, editor->token_keyword_text_color);
                    }
                    if (token == TOKEN_CONSTANT) {
                        SetTextColor(hdc_buffer, editor->token_constant_text_color);
                    }
                    if (token == TOKEN_STRING) {
                        SetTextColor(hdc_buffer, editor->token_string_text_color);
                    }
                    ExtTextOutW(hdc_buffer, x, y, 0, NULL, &line->items[j], token_amount, NULL);
                    SIZE measure_size;
                    GetTextExtentPoint32W(hdc_buffer, &line->items[j], token_amount, &measure_size);
                    x += measure_size.cx;
                    j += token_amount;
                }
            }
        }
        free(tokens);

        // Draw cursor
        SIZE measure_size;
        GetTextExtentPoint32W(hdc, L"0", 1, &measure_size);
        uint32_t character_width = measure_size.cx + 1;

        HBRUSH cursor_brush = CreateSolidBrush(RGB(255, 0, 0));
        RECT cursor_rect;
        cursor_rect.left = editor->line_numbers_width + editor->cursor_x * character_width - editor->hscroll_offset;
        cursor_rect.top = editor->cursor_y * editor->font_size - editor->vscroll_offset;
        cursor_rect.right = cursor_rect.left + 2;
        cursor_rect.bottom = cursor_rect.top + editor->font_size;
        FillRect(hdc_buffer, &cursor_rect, cursor_brush);
        DeleteObject(cursor_brush);

        // Draw line numbers rect
        HBRUSH line_numbers_brush = CreateSolidBrush(editor->background_color);
        RECT line_numbers_rect = { 0, 0, editor->line_numbers_width, editor->height };
        FillRect(hdc_buffer, &line_numbers_rect, line_numbers_brush);
        DeleteObject(line_numbers_brush);

        // Draw line numbers
        for (int32_t i = 0; i < editor->lines->size; i++) {
            WcharList *line = editor->lines->items[i];
            int32_t y = editor->font_size * i - editor->vscroll_offset;
            if (
                y >= -(int32_t)editor->font_size &&
                y + editor->font_size < editor->height + editor->font_size
            ) {
                if (editor->cursor_y == i) {
                    SetTextColor(hdc_buffer, editor->line_numbers_active_text_color);
                } else {
                    SetTextColor(hdc_buffer, editor->line_numbers_text_color);
                }
                wchar_t string_buffer[32];
                if (editor->debug) {
                    wsprintfW(string_buffer, editor->line_numbers_format, i + 1, line->size, line->capacity);
                } else {
                    wsprintfW(string_buffer, editor->line_numbers_format, i + 1);
                }
                ExtTextOutW(hdc_buffer, editor->padding, y, 0, NULL, string_buffer, wcslen(string_buffer), NULL);
            }
        }

        // Draw stats when in debug flag is set
        if (editor->debug) {
            SetTextColor(hdc_buffer, editor->line_numbers_text_color);
            wchar_t string_buffer[32];
            wsprintfW(string_buffer, L"%d/%d", editor->lines->size, editor->lines->capacity);
            SetTextAlign(hdc_buffer, TA_RIGHT);
            ExtTextOutW(hdc_buffer, editor->width - editor->scrollbar_size - editor->padding, 0, 0, NULL, string_buffer, wcslen(string_buffer), NULL);
        }

        // Draw horizontal scrollbar when needed
        if (editor->hscroll_size > editor->width) {
            // Draw horizontal scrollbar
            RECT hscrollbar_rect = { editor->line_numbers_width, editor->height - editor->scrollbar_size, editor->width - editor->scrollbar_size, editor->height };
            HBRUSH hscrollbar_brush = CreateSolidBrush(editor->scrollbar_background_color);
            FillRect(hdc_buffer, &hscrollbar_rect, hscrollbar_brush);
            DeleteObject(hscrollbar_brush);

            // Draw horizontal scrollbar thumb
            int x = editor->hscroll_viewport * editor->hscroll_offset / editor->hscroll_size;
            RECT hscrollbar_thumb_rect = { editor->line_numbers_width + x, editor->height - editor->scrollbar_size, editor->line_numbers_width + x + editor->hscroll_viewport * editor->hscroll_viewport / editor->hscroll_size, editor->height };
            HBRUSH hscrollbar_thumb_brush = CreateSolidBrush(editor->scrollbar_thumb_background_color);
            FillRect(hdc_buffer, &hscrollbar_thumb_rect, hscrollbar_thumb_brush);
            DeleteObject(hscrollbar_thumb_brush);
        }

        // Draw vertical scrollbar
        RECT vscrollbar_rect = { editor->width - editor->scrollbar_size, 0, editor->width, editor->vscroll_viewport };
        HBRUSH vscrollbar_brush = CreateSolidBrush(editor->scrollbar_background_color);
        FillRect(hdc_buffer, &vscrollbar_rect, vscrollbar_brush);
        DeleteObject(vscrollbar_brush);

        // Draw vertical scrollbar thumb
        int y = editor->vscroll_viewport * editor->vscroll_offset / editor->vscroll_size;
        RECT vscrollbar_thumb_rect = { editor->width - editor->scrollbar_size, y, editor->width, y + editor->vscroll_viewport * editor->vscroll_viewport / editor->vscroll_size };
        HBRUSH vscrollbar_thumb_brush = CreateSolidBrush(editor->scrollbar_thumb_background_color);
        FillRect(hdc_buffer, &vscrollbar_thumb_rect, vscrollbar_thumb_brush);
        DeleteObject(vscrollbar_thumb_brush);

        // Draw and delete back buffer
        BitBlt(hdc, 0, 0, editor->width, editor->height, hdc_buffer, 0, 0, SRCCOPY);
        DeleteObject(bitmap_buffer);
        DeleteDC(hdc_buffer);

        EndPaint(hwnd, &paint_struct);
        return 0;
    }

    if (msg == WM_DESTROY) {
        // Remove GDI objects
        DeleteObject(editor->font);

        // Free editor data
        free(editor);
        return 0;
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

void Editor_Register(void) {
    // Register windows editor control class
    WNDCLASSEXW wc = { 0 };
    wc.cbSize = sizeof(wc);
    wc.lpszClassName = L"plaatcode_editor";
    wc.hInstance = GetModuleHandleW(NULL);
    wc.lpfnWndProc = Editor_WndProc;
    wc.hCursor = LoadCursorW(NULL, IDC_ARROW);
    RegisterClassExW(&wc);
}
