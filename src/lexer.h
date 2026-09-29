#include <stdio.h>
#include <ctype.h>
#include "diagnostics.h"
#include "utils.h"

#ifndef LEXER_H
#define LEXER_H

typedef enum {
    TOKEN_EOF,
    TOKEN_ID,
    TOKEN_COLON,
    TOKEN_EQUAL,
    TOKEN_COMMA,
    TOKEN_OPEN_PAREN,
    TOKEN_CLOSE_PAREN,
    TOKEN_OPEN_CURLY,
    TOKEN_CLOSE_CURLY,
    TOKEN_RET,
    TOKEN_VAR,
    TOKEN_IF,
    TOKEN_ELSE,
    TOKEN_FN,
    TOKEN_STRUCT,
    TOKEN_UNION,
    TOKEN_INT_LIT,
    TOKEN_STRING_LIT,
    TOKEN_SEMICOLON,
    TOKEN_OP,
} TokenKind;

typedef struct {
    TokenKind kind;
    union {
        String_View id;
        int int_lit;
        String_View string_lit;
        String_View op;
    } as;
    Position pos;
} Token;

typedef struct {
    FILE *fp;
    Position pos;
    char peeked_char;
    bool has_peeked_char;
    Token peeked_token;
    bool has_peeked_token;
} Lexer;

bool lexer_init(Lexer *lexer, char *filename);
String_View token_to_sv(Token token);
Token next_token(Lexer *lexer);
Token peek_token(Lexer *lexer);
void expect_token(Lexer *lexer, TokenKind kind);
bool accept_token(Lexer *lexer, TokenKind kind);

#endif // LEXER_H
