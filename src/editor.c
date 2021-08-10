#include "editor.h"
#include "lexers.h"

void UpdateLines(EditorData *editor) {
    // Generate line number format string
    int32_t line_number_size = 1;
    int32_t c = editor->lines_size;
    while (c > 10) {
        c /= 10;
        line_number_size++;
    }
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

void UpdateScroll(EditorData *editor) {
    // Calculate horizontal scroll viewport
    editor->hscroll_viewport = editor->width - editor->line_numbers_width - editor->scrollbar_size;

    // Calculate horizontal scroll size
    int32_t largest_line = 0;
    for (int32_t i = 1; i < editor->lines_size; i++) {
        if (editor->lines[i]->size > editor->lines[largest_line]->size) {
            largest_line = i;
        }
    }

    HDC hdc = GetDC(NULL);
    SelectObject(hdc, editor->font);
    SIZE measure_size;
    GetTextExtentPoint32W(hdc, editor->lines[largest_line]->text, editor->lines[largest_line]->size, &measure_size);
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
    editor->vscroll_size = editor->font_size * editor->lines_size +
        (editor->vscroll_viewport - editor->font_size);
}

int32_t __stdcall EditorWndProc(HWND hwnd, uint32_t msg, WPARAM wParam, LPARAM lParam) {
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

        // Create lines with one empty line
        editor->lines_capacity = 128;
        editor->lines_size = 0;
        editor->lines = malloc(sizeof(EditorLine *) * editor->lines_capacity);

        EditorLine *line = malloc(sizeof(EditorLine));
        line->capacity = 128;
        line->size = 0;
        line->text = malloc(sizeof(wchar_t) * line->capacity);
        editor->lines[editor->lines_size++] = line;

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
        for (int32_t i = 0; i < editor->lines_size; i++) {
            free(editor->lines[i]->text);
            free(editor->lines[i]);
        }
        free(editor->lines);

        // Allocate new lines list
        editor->lines_capacity = 128;
        editor->lines_size = 0;
        editor->lines = malloc(sizeof(EditorLine *) * editor->lines_capacity);

        // Reset cursor
        editor->cursor_x = 0;
        editor->cursor_y = 0;

        // New file
        if (wParam == NULL) {
            if (editor->path != NULL) {
                free(editor->path);
            }
            editor->path = NULL;

            EditorLine *line = malloc(sizeof(EditorLine));
            line->capacity = 128;
            line->size = 0;
            line->text = malloc(sizeof(wchar_t) * line->capacity);
            editor->lines[editor->lines_size++] = line;

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

            uint32_t file_size = GetFileSize(file, NULL);
            char *file_buffer = malloc(file_size);
            uint32_t bytes_read;
            ReadFile(file, file_buffer, file_size, &bytes_read, 0);
            CloseHandle(file);

            int32_t converted_size = MultiByteToWideChar(CP_ACP, 0, file_buffer, file_size, NULL, 0);
            wchar_t *converted_buffer = malloc(sizeof(wchar_t) * converted_size);
            MultiByteToWideChar(CP_ACP, 0, file_buffer, bytes_read, converted_buffer, converted_size);
            free(file_buffer);

            // Create line
            EditorLine *line = malloc(sizeof(EditorLine));
            line->capacity = 128;
            line->size = 0;
            line->text = malloc(sizeof(wchar_t) * line->capacity);

            wchar_t *current = converted_buffer;
            while (*current != '\0') {
                if (*current == '\r') {
                    current++;
                }
                if (*current == '\n') {
                    // Add line to lines
                    if (editor->lines_size == editor->lines_capacity) {
                        editor->lines_capacity *= 2;
                        editor->lines = realloc(editor->lines, sizeof(EditorLine *) * editor->lines_capacity);
                    }
                    editor->lines[editor->lines_size++] = line;

                    // Create a new line
                    line = malloc(sizeof(EditorLine));
                    line->capacity = 128;
                    line->size = 0;
                    line->text = malloc(sizeof(wchar_t) * line->capacity);

                    current++;
                } else {
                    // Add character to current line
                    if (line->size == line->capacity) {
                        line->capacity *= 2;
                        line->text = realloc(line->text, sizeof(wchar_t) * line->capacity);
                    }
                    line->text[line->size++] = *current++;
                }
            }

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
    }

    if (msg == WM_EDITOR_SAVE_FILE) {
        HANDLE file = CreateFileW(editor->path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (file != INVALID_HANDLE_VALUE) {
            // Insert final empty line
            if (editor->lines[editor->lines_size - 1]->size > 0) {
                EditorLine *line = malloc(sizeof(EditorLine));
                line->capacity = 128;
                line->size = 0;
                line->text = malloc(sizeof(wchar_t) * line->capacity);
                if (editor->lines_size == editor->lines_capacity) {
                    editor->lines_capacity *= 2;
                    editor->lines = realloc(editor->lines, sizeof(EditorLine *) * editor->lines_capacity);
                }
                editor->lines[editor->lines_size++] = line;
            }

            // Count largest line
            int32_t largest_line_capacity = editor->lines[0]->capacity;
            for (int32_t i = 0; i < editor->lines_size; i++) {
                if (editor->lines[i]->capacity > largest_line_capacity) {
                    largest_line_capacity = editor->lines[i]->capacity;
                }
            }

            char *convert_buffer = malloc(sizeof(wchar_t) * largest_line_capacity * 2);
            for (int32_t i = 0; i < editor->lines_size; i++) {
                EditorLine *line = editor->lines[i];

                // Trim trailing whitespace
                int32_t trimming = 0;
                for (int32_t j = line->size - 1; j >= 0; j--) {
                    if (line->text[j] == ' ') {
                        trimming++;
                    } else {
                        break;
                    }
                }
                line->size -= trimming;
                if (editor->cursor_y == i) {
                    editor->cursor_x -= trimming;
                }

                int32_t converted_size = WideCharToMultiByte(CP_UTF8, 0, line->text, line->size,
                    convert_buffer, sizeof(wchar_t) * largest_line_capacity * 4, NULL, NULL);
                uint32_t bytes_written;
                WriteFile(file, convert_buffer, converted_size, &bytes_written, NULL);
                char eol = '\n';
                WriteFile(file, &eol, 1, &bytes_written, NULL);
            }
            free(convert_buffer);

            CloseHandle(file);

            InvalidateRect(hwnd, NULL, false);
        }
    }

    if (msg == WM_SIZE) {
        // Save new window size
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
                editor->cursor_y--;
                editor->cursor_x = editor->lines[editor->cursor_y]->size;
            }
        }
        if (key == VK_RIGHT) {
            if (editor->cursor_x < editor->lines[editor->cursor_y]->size) {
                editor->cursor_x++;
            } else if (editor->cursor_y < editor->lines_size) {
                editor->cursor_x = 0;
                editor->cursor_y++;
            }
        }
        if (key == VK_UP && editor->cursor_y > 0) {
            editor->cursor_y--;
            if (editor->cursor_x > editor->lines[editor->cursor_y]->size) {
                editor->cursor_x = editor->lines[editor->cursor_y]->size;
            }
        }
        if (key == VK_DOWN && editor->cursor_y < editor->lines_size) {
            editor->cursor_y++;
            if (editor->cursor_x > editor->lines[editor->cursor_y]->size) {
                editor->cursor_x = editor->lines[editor->cursor_y]->size;
            }
        }

        if (key == VK_BACK) {
            EditorLine *line = editor->lines[editor->cursor_y];
            if (editor->cursor_x > 0) {
                if (editor->cursor_x >= editor->indentation_size) {
                    bool are_spaces = true;
                    for (int32_t i = 0; i < editor->indentation_size; i++) {
                        if (line->text[editor->cursor_x - 1 - i] != ' ') {
                            are_spaces = false;
                            break;
                        }
                    }

                    if (are_spaces) {
                        for (int32_t i = 0; i < editor->indentation_size - 1; i++) {
                            for (int32_t j = editor->cursor_x - 1; j < line->size; j++) {
                                line->text[j] = line->text[j + 1];
                            }
                            line->size--;
                            editor->cursor_x--;
                        }
                    }
                }

                for (int32_t i = editor->cursor_x - 1; i < line->size; i++) {
                    line->text[i] = line->text[i + 1];
                }
                line->size--;
                editor->cursor_x--;
            } else if (editor->cursor_y > 0) {
                editor->cursor_y--;
                EditorLine *prev_line = editor->lines[editor->cursor_y];
                editor->cursor_x = prev_line->size;

                for (int32_t i = 0; i < line->size; i++) {
                    if (prev_line->size == prev_line->capacity) {
                        prev_line->capacity *= 2;
                        prev_line->text = realloc(prev_line->text, sizeof(wchar_t) * prev_line->capacity);
                    }
                    prev_line->text[prev_line->size++] = line->text[i];
                }

                free(line);
                for (int32_t i = editor->cursor_y + 1; i < editor->lines_size; i++) {
                    editor->lines[i] = editor->lines[i + 1];
                }
                editor->lines_size--;
            }
        }

        if (key == VK_DELETE) {
            EditorLine *line = editor->lines[editor->cursor_y];
            if (editor->cursor_x < line->size) {
                if (editor->cursor_x < line->size - editor->indentation_size) {
                    bool are_spaces = true;
                    for (int32_t i = 0; i < editor->indentation_size; i++) {
                        if (line->text[editor->cursor_x + i] != ' ') {
                            are_spaces = false;
                            break;
                        }
                    }

                    if (are_spaces) {
                        for (int32_t i = 0; i < editor->indentation_size - 1; i++) {
                            for (int32_t j = editor->cursor_x; j < line->size; j++) {
                                line->text[j] = line->text[j + 1];
                            }
                            line->size--;
                        }
                    }
                }

                for (int32_t i = editor->cursor_x; i < line->size; i++) {
                    line->text[i] = line->text[i + 1];
                }
                line->size--;
            } else if (editor->cursor_y < editor->lines_size) {
                EditorLine *next_line = editor->lines[editor->cursor_y + 1];
                for (int32_t i = 0; i < next_line->size; i++) {
                    if (line->size == line->capacity) {
                        line->capacity *= 2;
                        line->text = realloc(line->text, sizeof(wchar_t) * line->capacity);
                    }
                    line->text[line->size++] = next_line->text[i];
                }

                free(next_line);
                for (int32_t i = editor->cursor_y + 1; i < editor->lines_size; i++) {
                    editor->lines[i] = editor->lines[i + 1];
                }
                editor->lines_size--;
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
            EditorLine *line = editor->lines[editor->cursor_y];

            int32_t indentation_amount = 0;
            while (indentation_amount < line->size && line->text[indentation_amount] == editor->indentation_character) {
                indentation_amount++;
            }

            EditorLine *new_line = malloc(sizeof(EditorLine));
            new_line->capacity = line->capacity;
            new_line->size = 0;
            new_line->text = malloc(sizeof(wchar_t) * line->capacity);
            for (int32_t i = 0; i < indentation_amount; i++) {
                new_line->text[new_line->size++] = editor->indentation_character;
            }
            for (int32_t i = editor->cursor_x; i < line->size; i++) {
                new_line->text[new_line->size++] = line->text[i];
            }
            line->size = editor->cursor_x;

            if (editor->lines_size == editor->lines_capacity) {
                editor->lines_capacity *= 2;
                editor->lines = realloc(editor->lines, sizeof(EditorLine *) * editor->lines_capacity);
            }
            for (int32_t i = editor->lines_size - 1; i >= editor->cursor_y; i--) {
                editor->lines[i + 1] = editor->lines[i];
            }

            editor->lines[++editor->cursor_y] = new_line;
            editor->lines_size++;
            editor->cursor_x = indentation_amount;
        }

        InvalidateRect(hwnd, NULL, false);
        return 0;
    }

    if (msg == WM_CHAR) {
        wchar_t character = (uintptr_t)wParam;
        if (
            // character == '\t' ||
            (character >= ' ' && character <= '~')) {
            EditorLine *line = editor->lines[editor->cursor_y];

            if (line->size == line->capacity) {
                line->capacity *= 2;
                line->text = realloc(line->text, sizeof(wchar_t) * line->capacity);
            }

            for (int32_t i = line->size - 1; i >= editor->cursor_x; i--) {
                line->text[i + 1] = line->text[i];
            }
            line->size++;
            line->text[editor->cursor_x] = character;
            // if (character == '\t') {
            //     editor->cursor_x += editor->indentation_size;
            // } else {
                editor->cursor_x++;
            // }
            InvalidateRect(hwnd, NULL, false);
        }
        return 0;
    }

    if (msg == WM_ERASEBKGND) {
        // Draw no background
        return true;
    }

    if (msg == WM_PAINT) {
        UpdateLines(editor);
        UpdateScroll(editor);

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

        // Draw code lines
        SelectObject(hdc_buffer, editor->font);
        SetBkMode(hdc_buffer, TRANSPARENT);
        SetTextAlign(hdc_buffer, TA_LEFT);

        SIZE measure_size;
        GetTextExtentPoint32W(hdc, L"0", 1, &measure_size);
        uint32_t character_width = measure_size.cx + 1;


        int32_t largest_line_capacity = editor->lines[0]->capacity;
        for (int32_t i = 0; i < editor->lines_size; i++) {
            if (editor->lines[i]->capacity > largest_line_capacity) {
                largest_line_capacity = editor->lines[i]->capacity;
            }
        }
        uint8_t *tokens = malloc(largest_line_capacity);

        for (int32_t i = 0; i < editor->lines_size; i++) {
            EditorLine *line = editor->lines[i];
            int32_t x = editor->line_numbers_width - editor->hscroll_offset;
            int32_t y = editor->font_size * i - editor->vscroll_offset;
            if (
                y >= -(int32_t)editor->font_size &&
                y + editor->font_size < editor->height + editor->font_size
            ) {
                AssemblyLexer(line, tokens);
                // CLexer(line, tokens);

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
                    ExtTextOutW(hdc_buffer, x, y, 0, NULL, &line->text[j], token_amount, NULL);
                    SIZE measure_size;
                    GetTextExtentPoint32W(hdc_buffer, &line->text[j], token_amount, &measure_size);
                    x += measure_size.cx;
                    j += token_amount;
                }
            }
        }
        free(tokens);

        // Draw cursor
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
        for (int32_t i = 0; i < editor->lines_size; i++) {
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
                    wsprintfW(string_buffer, editor->line_numbers_format, i + 1, editor->lines[i]->size, editor->lines[i]->capacity);
                } else {
                    wsprintfW(string_buffer, editor->line_numbers_format, i + 1);
                }
                ExtTextOutW(hdc_buffer, editor->padding, y, 0, NULL, string_buffer, wcslen(string_buffer), NULL);
            }
        }

        if (editor->debug) {
            // Draw stats
            SetTextColor(hdc_buffer, editor->line_numbers_text_color);
            wchar_t string_buffer[32];
            wsprintfW(string_buffer, L"%d/%d", editor->lines_size, editor->lines_capacity);
            SetTextAlign(hdc_buffer, TA_RIGHT);
            ExtTextOutW(hdc_buffer, editor->width - editor->scrollbar_size - editor->padding, 0, 0, NULL, string_buffer, wcslen(string_buffer), NULL);
        }

        // Draw horizontal scrollbar
        if (editor->hscroll_size > editor->width) {
            RECT hscrollbar_rect = { editor->line_numbers_width, editor->height - editor->scrollbar_size, editor->width - editor->scrollbar_size, editor->height };
            HBRUSH hscrollbar_brush = CreateSolidBrush(editor->scrollbar_background_color);
            FillRect(hdc_buffer, &hscrollbar_rect, hscrollbar_brush);
            DeleteObject(hscrollbar_brush);

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

void InitEditorControl(void) {
    WNDCLASSEXW wc = { 0 };
    wc.cbSize = sizeof(wc);
    wc.lpszClassName = L"plaatcode_editor";
    wc.hInstance = GetModuleHandleW(NULL);
    wc.lpfnWndProc = EditorWndProc;
    wc.hCursor = LoadCursorW(NULL, IDC_ARROW);
    RegisterClassExW(&wc);
}
