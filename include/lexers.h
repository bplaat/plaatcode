#ifndef LEXERS_H
#define LEXERS_H

#include "editor.h"

typedef struct {
    wchar_t *extension;
    void (*lexer_function)(EditorLine *line, uint8_t *tokens);
} Lexer;

#define LEXERS_SIZE 4
// extern Lexer lexers[];

void AssemblyLexer(EditorLine *line, uint8_t *tokens);

void CLexer(EditorLine *line, uint8_t *tokens);

#endif
