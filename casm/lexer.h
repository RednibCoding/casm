#ifndef LEXER_H
#define LEXER_H

#include <stdbool.h>

typedef struct {
    char* value;
    int line;
} Token;

typedef struct {
    Token* tokens;
    int count;
    int capacity;
} TokenArray;

void add_token(TokenArray* token_array, const char* start, int length, int line);
bool is_token_separator(char c);
TokenArray tokenize(const char* code);
void free_token_array(TokenArray* token_array);

#endif // !LEXER_H
