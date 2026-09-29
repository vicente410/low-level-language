#include "lexer.h"
#include "utils.h"

bool lexer_init(Lexer *lexer, char *filename) {
    lexer->fp = fopen(filename, "r");
    lexer->pos = (Position) {
    .filename = sv_from_cstr(filename),.line = 0,.col = 0,};
    lexer->has_peeked_char = false;
    lexer->has_peeked_token = false;

    return lexer->fp != NULL;
}

String_View token_to_sv(Token token) {
    String_Builder sb = { };

    switch (token.kind) {
    case TOKEN_EOF:
        sb_appendf(&sb, "TOKEN_EOF");
        break;
    case TOKEN_ID:
        sb_appendf(&sb, "TOKEN_ID(" SV_FMT ")", SV_ARG(token.as.id));
        break;
    case TOKEN_COLON:
        sb_appendf(&sb, "TOKEN_COLON");
        break;
    case TOKEN_COMMA:
        sb_appendf(&sb, "TOKEN_COMMA");
        break;
    case TOKEN_OPEN_PAREN:
        sb_appendf(&sb, "TOKEN_OPEN_PAREN");
        break;
    case TOKEN_CLOSE_PAREN:
        sb_appendf(&sb, "TOKEN_CLOSE_PAREN");
        break;
    case TOKEN_OPEN_CURLY:
        sb_appendf(&sb, "TOKEN_OPEN_CURLY");
        break;
    case TOKEN_CLOSE_CURLY:
        sb_appendf(&sb, "TOKEN_CLOSE_CURLY");
        break;
    case TOKEN_RET:
        sb_appendf(&sb, "TOKEN_RET");
        break;
    case TOKEN_VAR:
        sb_appendf(&sb, "TOKEN_VAR");
        break;
    case TOKEN_IF:
        sb_appendf(&sb, "TOKEN_IF");
        break;
    case TOKEN_ELSE:
        sb_appendf(&sb, "TOKEN_ELSE");
        break;
    case TOKEN_FN:
        sb_appendf(&sb, "TOKEN_FN");
        break;
    case TOKEN_STRUCT:
        sb_appendf(&sb, "TOKEN_STRUCT");
        break;
    case TOKEN_UNION:
        sb_appendf(&sb, "TOKEN_UNION");
        break;
    case TOKEN_INT_LIT:
        sb_appendf(&sb, "TOKEN_INT_LIT(%d)", token.as.int_lit);
        break;
    case TOKEN_STRING_LIT:
        sb_appendf(&sb, "TOKEN_STRING_LIT(" SV_FMT ")",
                   SV_ARG(token.as.string_lit));
        break;
    case TOKEN_SEMICOLON:
        sb_appendf(&sb, "TOKEN_SEMICOLON");
        break;
    case TOKEN_OP:
        sb_appendf(&sb, "TOKEN_OP(" SV_FMT ")", SV_ARG(token.as.op));
        break;
    default:
        fprintf(stderr, "UNREACHABLE\n");
        exit(1);
    }

    return sv_from_sb(sb);
}

char next_char(Lexer *lexer) {
    char ch;

    if (lexer->has_peeked_char) {
        lexer->has_peeked_char = false;
        ch = lexer->peeked_char;
    } else {
        ch = fgetc(lexer->fp);
    }

    if (ch == '\n') {
        lexer->pos.line++;
        lexer->pos.col = 0;
    } else {
        lexer->pos.col++;
    }

    return ch;
}

char peek_char(Lexer *lexer) {
    if (!(lexer->has_peeked_char)) {
        lexer->peeked_char = next_char(lexer);
        lexer->has_peeked_char = true;
    }
    return lexer->peeked_char;
}

bool accept_char(Lexer *lexer, char ch) {
    if (peek_char(lexer) == ch) {
        next_char(lexer);
        return true;
    }

    return false;
}

Token read_id_or_keyword(Lexer *lexer) {
    Token token = { };
    String_Builder sb = { };

    token.pos = lexer->pos;

    while (isalnum(peek_char(lexer))) {
        da_push(&sb, next_char(lexer));
    }

    if (sv_eq_cstr(sv_from_sb(sb), "return")) {
        token.kind = TOKEN_RET;
    } else if (sv_eq_cstr(sv_from_sb(sb), "var")) {
        token.kind = TOKEN_VAR;
    } else if (sv_eq_cstr(sv_from_sb(sb), "if")) {
        token.kind = TOKEN_IF;
    } else if (sv_eq_cstr(sv_from_sb(sb), "else")) {
        token.kind = TOKEN_ELSE;
    } else if (sv_eq_cstr(sv_from_sb(sb), "fn")) {
        token.kind = TOKEN_FN;
    } else if (sv_eq_cstr(sv_from_sb(sb), "struct")) {
        token.kind = TOKEN_STRUCT;
    } else if (sv_eq_cstr(sv_from_sb(sb), "union")) {
        token.kind = TOKEN_UNION;
    } else {
        token.kind = TOKEN_ID;
        token.as.id = sv_from_sb(sb);
    }

    return token;
}

Token read_int_lit(Lexer *lexer) {
    Token token = { };

    token.pos = lexer->pos;

    int int_lit = next_char(lexer) - '0';

    while (isdigit(peek_char(lexer))) {
        int_lit *= 10;
        int_lit += next_char(lexer) - '0';
    }

    token.kind = TOKEN_INT_LIT;
    token.as.int_lit = int_lit;

    return token;
}

Token read_string_lit(Lexer *lexer) {
    Token token = { };
    String_Builder sb = { };
    char ch;

    token.pos = lexer->pos;

    next_char(lexer);
    while ((ch = next_char(lexer)) != '"') {
        da_push(&sb, ch);
    }

    token.kind = TOKEN_STRING_LIT;
    token.as.string_lit = sv_from_sb(sb);

    return token;
}

Token read_symbol(Lexer *lexer) {
    Token token = { };
    char ch = next_char(lexer);

    token.pos = lexer->pos;

    switch (ch) {
    case ':':
        token.kind = TOKEN_COLON;
        break;
    case '=':{
            if (accept_char(lexer, '=')) {
                token.kind = TOKEN_OP;
                token.as.op = sv_from_cstr("==");
            } else {
                token.kind = TOKEN_EQUAL;
            }
        }
        break;
    case ',':
        token.kind = TOKEN_COMMA;
        break;
    case '(':
        token.kind = TOKEN_OPEN_PAREN;
        break;
    case ')':
        token.kind = TOKEN_CLOSE_PAREN;
        break;
    case '{':
        token.kind = TOKEN_OPEN_CURLY;
        break;
    case '}':
        token.kind = TOKEN_CLOSE_CURLY;
        break;
    case ';':
        token.kind = TOKEN_SEMICOLON;
        break;
    default:
        token.kind = TOKEN_OP;
        String_Builder sb = { };
        da_push(&sb, ch);
        while (strchr("+-*/!=<>", peek_char(lexer)) != NULL) {
            da_push(&sb, next_char(lexer));
        }
        token.as.op = sv_from_sb(sb);
    }

    return token;
}

Token next_token(Lexer *lexer) {
    if (lexer->has_peeked_token) {
        lexer->has_peeked_token = false;
        return lexer->peeked_token;
    }

    while (peek_char(lexer) == ' ' || peek_char(lexer) == '\n'
           || peek_char(lexer) == '\t') {
        next_char(lexer);
    }

    if (feof(lexer->fp)) {
        return (Token) {
        .kind = TOKEN_EOF};
    } else if (isalpha(peek_char(lexer))) {
        return read_id_or_keyword(lexer);
    } else if (isdigit(peek_char(lexer))) {
        return read_int_lit(lexer);
    } else if (peek_char(lexer) == '"') {
        return read_string_lit(lexer);
    } else {
        return read_symbol(lexer);
    }
}

Token peek_token(Lexer *lexer) {
    if (!(lexer->has_peeked_token)) {
        lexer->peeked_token = next_token(lexer);
        lexer->has_peeked_token = true;
    }
    return lexer->peeked_token;
}

void expect_token(Lexer *lexer, TokenKind kind) {
    if (next_token(lexer).kind != kind) {
        fprintf(stderr, "Error: Unexepected token");
        exit(1);
    }
}

bool accept_token(Lexer *lexer, TokenKind kind) {
    if (peek_token(lexer).kind == kind) {
        next_token(lexer);
        return true;
    }

    return false;
}
