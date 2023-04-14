#if defined(_MSC_VER) || defined(__STDC_LIB_EXT1__)
#define _CRT_SECURE_NO_WARNINGS
#pragma warning(disable: 4996)
#endif

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"
#include "lexer.h"

void add_token(TokenArray* token_array, const char* start, int length, int line) {
    if (token_array->count >= token_array->capacity) {
        token_array->capacity *= 2;
        token_array->tokens = realloc(token_array->tokens, token_array->capacity * sizeof(Token));
    }
    Token token;
    token.value = malloc((length + 1) * sizeof(char));
    strncpy(token.value, start, length);
    token.value[length] = '\0';
    token.line = line;
    token_array->tokens[token_array->count++] = token;
}

void add_eof_token(TokenArray* token_array, int line) {
    if (token_array->count >= token_array->capacity) {
        token_array->capacity *= 2;
        token_array->tokens = realloc(token_array->tokens, token_array->capacity * sizeof(Token));
    }

    Token token;
    token.value = malloc((3 + 1) * sizeof(char));

    strncpy(token.value, "EOF", 4);
    token.value[3] = '\0';
    token.line = line;
    token_array->tokens[token_array->count++] = token;
}

bool is_token_separator(char c) {
    return isspace(c) || c == '\0';
}

TokenArray tokenize(const char* code) {
    TokenArray token_array;
    token_array.count = 0;
    token_array.capacity = 16;
    token_array.tokens = malloc(token_array.capacity * sizeof(Token));

    const char* start = code;
    const char* end = code;
    int line = 1;

    while (*end != '\0') {
        if (is_token_separator(*end)) {
            if (end > start) {
                add_token(&token_array, start, end - start, line);
            }
            if (*end == '\n') {
                line++;
            }
            start = end + 1;
        }
        else if (*end == ',' || *end == ';') {
            if (end > start) {
                add_token(&token_array, start, end - start, line);
            }
            add_token(&token_array, end, 1, line);
            start = end + 1;
        }
        end++;
    }
    if (end > start) {
        add_token(&token_array, start, end - start, line);
    }

    add_eof_token(&token_array, line+1);

    return token_array;
}

void free_token_array(TokenArray* token_array) {
    for (int i = 0; i < token_array->count; i++) {
        free(token_array->tokens[i].value);
    }
    free(token_array->tokens);
}