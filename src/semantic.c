#include "semantic.h"
#include <stdio.h>

typedef struct {
    String_View id;
    Type type;                  // TYPE_NONE represents a new scope
} IdType;

typedef struct {
    IdType *data;
    size_t count;
    size_t capacity;
} IdTypes;

Type get_type_from_id(IdTypes id_types, String_View id) {
    for (size_t i = 0; i < id_types.count; i++) {
        if (sv_eq(id_types.data[i].id, id)) {
            return id_types.data[i].type;
        }
    }

    return (Type) {
    .kind = TYPE_NONE};
}

void start_scope(IdTypes *id_types) {
    da_push(id_types, ((IdType) {
                       .type.kind = TYPE_NONE}));
}

void end_scope(IdTypes *id_types) {
    while (da_last(id_types).type.kind != TYPE_NONE) {
        (void) da_pop(id_types);
    }
    (void) da_pop(id_types);
}

Type type_expr(Expr *expr, IdTypes *id_types) {
    switch (expr->kind) {
    case EXPR_BOOL_LIT:
        expr->type.kind = TYPE_BOOL;
        break;
    case EXPR_INT_LIT:
        expr->type.kind = TYPE_INT;
        break;
    case EXPR_ID:
        expr->type = get_type_from_id(*id_types, expr->as.id);
        if (expr->type.kind == TYPE_NONE) {
            fprintf(stderr, "ERROR: Undefined variable " SV_FMT "\n",
                    SV_ARG(expr->as.id));
            exit(1);
        }
        break;
    case EXPR_UNOP:
        if (sv_eq_cstr(expr->as.unop.op, "!")) {
            if (type_expr(expr->as.unop.expr, id_types).kind != TYPE_BOOL) {
                fprintf(stderr, "ERROR: Invalid type\n");
                exit(1);
            }
            expr->type.kind = TYPE_BOOL;
        } else {
            fprintf(stderr, "ERROR: Unknown operator\n");
            exit(1);
        }
        break;
    case EXPR_BINOP:
        if (sv_eq_cstr(expr->as.binop.op, "+") ||
            sv_eq_cstr(expr->as.binop.op, "-") ||
            sv_eq_cstr(expr->as.binop.op, "*") ||
            sv_eq_cstr(expr->as.binop.op, "/") ||
            sv_eq_cstr(expr->as.binop.op, "%")
            ) {
            if (type_expr(expr->as.binop.lhs, id_types).kind != TYPE_INT) {
                fprintf(stderr, "ERROR: Invalid type\n");
                exit(1);
            } else if (type_expr(expr->as.binop.rhs, id_types).kind !=
                       TYPE_INT) {
                fprintf(stderr, "ERROR: Invalid type\n");
                exit(1);
            }
            expr->type.kind = TYPE_INT;
        } else if (sv_eq_cstr(expr->as.binop.op, "<") ||
                   sv_eq_cstr(expr->as.binop.op, "<=") ||
                   sv_eq_cstr(expr->as.binop.op, ">") ||
                   sv_eq_cstr(expr->as.binop.op, ">=") ||
                   sv_eq_cstr(expr->as.binop.op, "==") ||
                   sv_eq_cstr(expr->as.binop.op, "!=")
            ) {
            if (type_expr(expr->as.binop.lhs, id_types).kind != TYPE_INT) {
                fprintf(stderr, "ERROR: Invalid type\n");
                exit(1);
            } else if (type_expr(expr->as.binop.rhs, id_types).kind !=
                       TYPE_INT) {
                fprintf(stderr, "ERROR: Invalid type\n");
                exit(1);
            }
            expr->type.kind = TYPE_BOOL;
        } else if (sv_eq_cstr(expr->as.binop.op, "&&") ||
                   sv_eq_cstr(expr->as.binop.op, "||")
            ) {
            if (type_expr(expr->as.binop.lhs, id_types).kind != TYPE_BOOL) {
                fprintf(stderr, "ERROR: Invalid type\n");
                exit(1);
            } else if (type_expr(expr->as.binop.rhs, id_types).kind !=
                       TYPE_BOOL) {
                fprintf(stderr, "ERROR: Invalid type\n");
                exit(1);
            }
            expr->type.kind = TYPE_BOOL;
        } else {
            fprintf(stderr, "ERROR: Unknown operator\n");
            exit(1);
        }
        break;
    }

    return expr->type;
}

void type_stmt(Stmt *stmt, IdTypes *id_types) {
    switch (stmt->kind) {
    case STMT_RET:
        type_expr(&stmt->as.ret, id_types);
        break;
    case STMT_VAR:
        if (get_type_from_id(*id_types, stmt->as.var.id).kind != TYPE_NONE) {
            fprintf(stderr, "ERROR: Variable " SV_FMT " already defined\n",
                    SV_ARG(stmt->as.var.id));
            exit(1);
        }
        IdType id_type = { };
        id_type.id = stmt->as.var.id;
        id_type.type = type_expr(&stmt->as.var.value, id_types);
        da_push(id_types, id_type);
        break;
    case STMT_ASSIGN:
        type_expr(&stmt->as.assign.value, id_types);
        break;
    case STMT_IFTE:
        type_expr(&stmt->as.ifte.cond, id_types);
        start_scope(id_types);
        for (size_t i = 0; i < stmt->as.ifte.then_body.count; i++) {
            type_stmt(&stmt->as.ifte.then_body.data[i], id_types);
        }
        end_scope(id_types);
        start_scope(id_types);
        for (size_t i = 0; i < stmt->as.ifte.else_body.count; i++) {
            type_stmt(&stmt->as.ifte.else_body.data[i], id_types);
        }
        end_scope(id_types);
        break;
    case STMT_WHILE:
        type_expr(&stmt->as.ifte.cond, id_types);
        start_scope(id_types);
        for (size_t i = 0; i < stmt->as.while_stmt.body.count; i++) {
            type_stmt(&stmt->as.while_stmt.body.data[i], id_types);
        }
        end_scope(id_types);
        break;
    }
}

void type_fn(AstFn *fn) {
    IdTypes id_types = { };

    for (size_t i = 0; i < fn->body.count; i++) {
        type_stmt(&fn->body.data[i], &id_types);
    }
}

void type_program(AstProgram *program) {
    for (size_t i = 0; i < program->count; i++) {
        assert(program->data[i].kind == DECL_FN);
        type_fn(&program->data[i].as.fn);
    }
}
