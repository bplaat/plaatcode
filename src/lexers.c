#include "lexers.h"

void AssemblyLexer(EditorLine *line, uint8_t *tokens) {
    wchar_t *keywords[] = {
        L"db", L"dw", L"dd", L"dq", L"equ", L"times", L"align",
        L"BYTE", L"WORD", L"DWORD", L"QWORD",

        L"mov", L"cmp", L"add", L"sub", L"jmp", L"call", L"push", L"pop",

        L"eax", L"ebx", L"ecx", L"edx", L"esi", L"edi", L"ebp", L"esp",
        L"rax", L"rbx", L"rcx", L"rdx", L"rsi", L"rdi", L"rbp", L"rsp",
        L"r8", L"r8d", L"r9", L"r9d", L"r10", L"r10d", L"r11", L"r11d",
        L"r12", L"r12d", L"r13", L"r13d", L"r14", L"r14d", L"r15", L"r15d"
    };

    int32_t j = 0;
    while (j < line->size) {
        wchar_t character = line->text[j];

        // Single tokens
        if (
            j == 0 || line->text[j - 1] == ' ' || line->text[j - 1] == '(' ||
            line->text[j - 1] == '[' || line->text[j - 1] == '{'
        ) {
            // Keywords
            for (int32_t k = 0; k < (int32_t)(sizeof(keywords) / sizeof(wchar_t *)); k++) {
                if (character == keywords[k][0]) {
                    bool same = true;
                    int32_t keyword_size = wcslen(keywords[k]);
                    for (int32_t l = 0; l < keyword_size; l++) {
                        if (line->text[j + l] != keywords[k][l]) {
                            same = false;
                            break;
                        }
                    }
                    if (same) {
                        for (int32_t l = 0; l < keyword_size; l++) {
                            tokens[j++] = TOKEN_KEYWORD;
                        }
                        continue;
                    }
                }
            }

            // Number constants
            if (character >= '0' && character <= '9') {
                while (j < line->size && (
                    (character >= '0' && character <= '9') || (character >= 'A' && character <= 'F') ||
                    (character >= 'a' && character <= 'f') || character == 'x'
                )) {
                    tokens[j++] = TOKEN_CONSTANT;
                    character = line->text[j];
                }
                continue;
            }
        }

        // Spaces
        if (character == ' ') {
            while (character == ' ') {
                tokens[j++] = TOKEN_SPACE;
                character = line->text[j];
            }
            continue;
        }

        // Strings
        wchar_t string_starters[] = { '\"', '\'', '`' };
        bool next = false;
        for (int32_t i = 0; i < (int32_t)(sizeof(string_starters) / sizeof(wchar_t)); i++) {
            if (character == string_starters[i]) {
                tokens[j++] = TOKEN_STRING;
                character = line->text[j];
                while (j < line->size && character != string_starters[i]) {
                    tokens[j++] = TOKEN_STRING;
                    character = line->text[j];
                }
                tokens[j++] = TOKEN_STRING;
                character = line->text[j];
                next = true;
                break;
            }
        }
        if (next) {
            continue;
        }

        // Comments
        if (character == ';') {
            while (j < line->size) {
                tokens[j++] = TOKEN_COMMENT;
            }
            return;
        }

        // Everthing else
        tokens[j++] = TOKEN_NORMAL;
    }
}

void CLexer(EditorLine *line, uint8_t *tokens) {
    wchar_t *keywords[] = {
        L"auto", L"break", L"case", L"char", L"const", L"continue",
        L"default", L"do", L"double", L"else", L"enum", L"extern",
        L"float", L"for", L"goto", L"if", L"inline", L"int", L"long",
        L"register", L"restrict", L"return", L"short", L"signed",
        L"sizeof", L"static", L"struct", L"switch", L"typedef",
        L"union", L"unsigned", L"void", L"volatile", L"while",
        L"_Alignas", L"_Alignof", L"_Atomic", L"_Bool", L"_Complex",
        L"_Generic", L"_Imaginary", L"_Noreturn",
        L"_Static_assert", L"_Thread_local",

        L"#include", L"#define", L"#ifdef", L"#ifndef", L"#endif"
    };

    int32_t j = 0;
    while (j < line->size) {
        wchar_t character = line->text[j];

        // Single tokens
        if (
            j == 0 || line->text[j - 1] == ' ' || line->text[j - 1] == '(' ||
            line->text[j - 1] == '[' || line->text[j - 1] == '{'
        ) {
            // Keywords
            for (int32_t k = 0; k < (int32_t)(sizeof(keywords) / sizeof(wchar_t *)); k++) {
                if (character == keywords[k][0]) {
                    bool same = true;
                    int32_t keyword_size = wcslen(keywords[k]);
                    for (int32_t l = 0; l < keyword_size; l++) {
                        if (line->text[j + l] != keywords[k][l]) {
                            same = false;
                            break;
                        }
                    }
                    if (same) {
                        for (int32_t l = 0; l < keyword_size; l++) {
                            tokens[j++] = TOKEN_KEYWORD;
                        }
                        continue;
                    }
                }
            }

            // Number constants
            if (character >= '0' && character <= '9') {
                while (j < line->size && (
                    (character >= '0' && character <= '9') || (character >= 'A' && character <= 'F') ||
                    (character >= 'a' && character <= 'f') || character == 'x'
                )) {
                    tokens[j++] = TOKEN_CONSTANT;
                    character = line->text[j];
                }
                continue;
            }
        }

        // Spaces
        if (character == ' ') {
            while (character == ' ') {
                tokens[j++] = TOKEN_SPACE;
                character = line->text[j];
            }
            continue;
        }

        // Strings
        wchar_t string_starters[] = { '\"', '\'' };
        bool next = false;
        for (int32_t i = 0; i < (int32_t)(sizeof(string_starters) / sizeof(wchar_t)); i++) {
            if (character == string_starters[i]) {
                tokens[j++] = TOKEN_STRING;
                character = line->text[j];
                while (j < line->size && character != string_starters[i]) {
                    tokens[j++] = TOKEN_STRING;
                    character = line->text[j];
                }
                tokens[j++] = TOKEN_STRING;
                character = line->text[j];
                next = true;
                break;
            }
        }
        if (next) {
            continue;
        }

        // Comments
        if (j < line->size - 1 && character == '/' && line->text[j + 1] == '/') {
            while (j < line->size) {
                tokens[j++] = TOKEN_COMMENT;
            }
            return;
        }

        // Everthing else
        tokens[j++] = TOKEN_NORMAL;
    }
}
