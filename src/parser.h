#include "diagnostics.h"
#include "lexer.h"
#include "utils.h"

#ifndef PARSER_H
#define PARSER_H

typedef enum {
    EXPR_INT_LIT,
    EXPR_BOOL_LIT,
    EXPR_ID,
    EXPR_UNOP,
    EXPR_BINOP,
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

typedef struct Expr {
    ExprKind kind;
    union {
        int int_lit;
        bool bool_lit;
        String_View id;
        UnOp unop;
        BinOp binop;
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
    String_View id;
    Expr value;
} Assign;

typedef struct {
    Expr cond;
    Stmts then_body;
    Stmts else_body;
} Ifte;

typedef struct {
    Expr cond;
    Stmts body;
} While;

typedef enum {
    STMT_RET,
    STMT_VAR,
    STMT_ASSIGN,
    STMT_IFTE,
    STMT_WHILE,
} StmtKind;

typedef struct Stmt {
    StmtKind kind;
    union {
        Expr ret;
        Var var;
        Assign assign;
        Ifte ifte;
        While while_stmt;
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
