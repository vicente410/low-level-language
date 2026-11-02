#include "diagnostics.h"
#include "lexer.h"
#include "utils.h"

#ifndef PARSER_H
#define PARSER_H

struct Expr;

typedef struct {
    struct Expr **data;
    size_t count;
    size_t capacity;
} Exprs;

typedef enum {
    EXPR_BOOL_LIT,
    EXPR_INT_LIT,
    EXPR_STR_LIT,
    EXPR_ID,
    EXPR_UNOP,
    EXPR_BINOP,
    EXPR_CALL,
    EXPR_CONST_DECL,
    EXPR_VAR_DECL,
    EXPR_BLOCK,
    EXPR_IFTE,
    EXPR_WHILE,
    EXPR_FN,
    EXPR_STRUCT,
} ExprKind;

typedef struct {
    String_View op;
    struct Expr *expr;
} UnOp;

typedef struct {
    String_View op;
    struct Expr *lhs;
    struct Expr *rhs;
} BinOp;

typedef struct {
    String_View id;
    Exprs args;
} Call;

typedef struct {
    String_View id;
    struct Expr *type;
    struct Expr *value;
} Decl;

typedef struct {
    struct Expr *cond;
    struct Expr *then_body;
    struct Expr *else_body;
} Ifte;

typedef struct {
    struct Expr *cond;
    struct Expr *body;
} While;

typedef struct {
    Exprs args;
    struct Expr *value;
} Fn;

typedef struct {
    Exprs fields;
} Struct;

typedef struct Expr {
    //Type *type;
    ExprKind kind;
    size_t param_idx;
    union {
        bool bool_lit;
        int int_lit;
        String_View str_lit;
        String_View id;
        UnOp unop;
        BinOp binop;
        Call call;
        Decl decl;
        Exprs block;
        Ifte ifte;
        While whil;
        Fn fn;
        Struct struct_type;
    } as;
} Expr;

String_View expr_to_sv(Expr * expr, size_t indent);
Exprs parse_program(Lexer * lexer);

#endif                          // PARSER_H
