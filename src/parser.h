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

struct Stmts;

typedef struct {
    String_View id;
    Expr value;
} Var;

typedef enum {
    STMT_RET,
    STMT_VAR,
} StmtKind;

typedef struct {
    StmtKind kind;
    union {
        Expr ret;
        Var var;
    } as;
} Stmt;

typedef struct Stmts {
    Stmt *data;
    size_t count;
    size_t capacity;
} Stmts;

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
AstProgram parse_program(Lexer *lexer);

#endif // PARSER_H
