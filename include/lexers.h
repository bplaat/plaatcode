#ifndef LEXERS_H
#define LEXERS_H

#include "wchar_list.h"

typedef struct {
    wchar_t *extension;
    void (*lexer_function)(WcharList *line, uint8_t *tokens);
} Lexer;

#define LEXERS_SIZE 4

void Lexer_Assembly(WcharList *line, uint8_t *tokens);

void Lexer_C(WcharList *line, uint8_t *tokens);

#endif
