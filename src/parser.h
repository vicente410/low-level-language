#include "diagnostics.h"
#include "lexer.h"
#include "utils.h"

#ifndef PARSER_H
#define PARSER_H

typedef enum {
    EXPR_INT_LIT,
    EXPR_ID,
    EXPR_OP,
} ExprKind;

typedef struct {
    String_View op;
    struct Expr *lhs;
    struct Expr *rhs;
} Op;

typedef struct Expr {
    ExprKind kind;
    union {
        int int_lit;
        String_View id;
        Op op;
    } as;
} Expr;

struct Stmt;

typedef struct {
    struct Stmt *data;
    size_t count;
    size_t capacity;
} Stmts;

typedef struct {
    String_View id;
    Expr value;
} Var;

typedef struct {
    Expr cond;
    Stmts then_body;
    Stmts else_body;
} Ifte;

typedef enum {
    STMT_RET,
    STMT_VAR,
    STMT_IFTE,
} StmtKind;

typedef struct Stmt {
    StmtKind kind;
    union {
        Expr ret;
        Var var;
        Ifte ifte;
    } as;
} Stmt;

typedef struct {
    String_View id;
    Stmts body;
} AstFn;

typedef enum {
    DECL_FN
} DeclKind;

typedef struct {
    DeclKind kind;
    union {
        AstFn fn;
    } as;
} Decl;

typedef struct {
    Decl *data;
    size_t count;
    size_t capacity;
} AstProgram;

String_View expr_to_sv(Expr expr, size_t indent);
String_View stmt_to_sv(Stmt stmt, size_t indent);
String_View decl_to_sv(Decl decl, size_t indent);
AstProgram parse_program(Lexer * lexer);

#endif                          // PARSER_H
