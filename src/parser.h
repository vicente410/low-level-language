#include "diagnostics.h"
#include "lexer.h"
#include "utils.h"

#ifndef PARSER_H
#define PARSER_H

typedef enum {
    TYPE_NONE,
    TYPE_BOOL,
    TYPE_U8,
    TYPE_S64,
    TYPE_PTR,
} TypeKind;

typedef struct Type {
    TypeKind kind;
    struct Type *ptr;
} Type;

typedef enum {
    EXPR_BOOL_LIT,
    EXPR_INT_LIT,
    EXPR_STR_LIT,
    EXPR_ID,
    EXPR_UNOP,
    EXPR_BINOP,
    EXPR_CALL,
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
    struct Expr *data;
    size_t count;
    size_t capacity;
} Call;

typedef struct Expr {
    Type *type;
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
    Type *type;
    Expr value;
} VarDecl;

typedef struct {
    Expr target;
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
    STMT_VAR_DECL,
    STMT_ASSIGN,
    STMT_IFTE,
    STMT_WHILE,
    STMT_CALL,
} StmtKind;

typedef struct Stmt {
    StmtKind kind;
    union {
        Expr ret;
        VarDecl var_decl;
        Assign assign;
        Ifte ifte;
        While while_stmt;
        Call call;
    } as;
} Stmt;

typedef struct {
    String_View id;
    Type *type;
} Arg;

typedef struct {
    Arg *data;
    size_t count;
    size_t capacity;
} Args;

typedef struct {
    String_View id;
    Args args;
    Type *ret_type;
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
