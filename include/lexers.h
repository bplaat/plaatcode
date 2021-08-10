#ifndef LEXERS_H
#define LEXERS_H

#include "editor.h"

void AssemblyLexer(EditorLine *line, uint8_t *tokens);

void CLexer(EditorLine *line, uint8_t *tokens);

#endif
