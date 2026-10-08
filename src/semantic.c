#include "semantic.h"
#include <stdio.h>

typedef struct {
    String_View id;
    Type type;                  // TYPE_NONE represents a new scope
    size_t param_idx;
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

size_t get_param_idx_from_id(IdTypes id_types, String_View id) {
    for (size_t i = 0; i < id_types.count; i++) {
        if (sv_eq(id_types.data[i].id, id)) {
            return id_types.data[i].param_idx;
        }
    }

    return 0;
}

Args get_fn_args(AstProgram *program, String_View id) {
    for (size_t i = 0; i < program->count; i++) {
        if (program->data[i].kind == DECL_FN
            && sv_eq(program->data[i].as.fn.id, id)) {
            return program->data[i].as.fn.args;
        }
    }

    if (sv_eq_cstr(id, "sys_write")) {
        Args sys_write = *(Args *) calloc(1, sizeof(Args));

        da_push(&sys_write, ((Arg) {
                             .id = sv_from_cstr("fd"),.type.kind =
                             TYPE_S64,}
                ));

        Type *type_u8 = calloc(1, sizeof(Type));
        type_u8->kind = TYPE_U8;
        da_push(&sys_write, ((Arg) {
                             .id = sv_from_cstr("buf"),.type.kind =
                             TYPE_PTR,.type.ptr = type_u8,}
                ));

        da_push(&sys_write, ((Arg) {
                             .id = sv_from_cstr("count"),.type.kind =
                             TYPE_S64,}
                ));

        return sys_write;
    }


    fprintf(stderr, "ERROR: function '" SV_FMT "' not declared",
            SV_ARG(id));
    exit(1);
}

Type get_fn_ret_type(AstProgram *program, String_View id) {
    for (size_t i = 0; i < program->count; i++) {
        if (program->data[i].kind == DECL_FN
            && sv_eq(program->data[i].as.fn.id, id)) {
            return program->data[i].as.fn.ret_type;
        }
    }

    if (sv_eq_cstr(id, "sys_write")) {
        Type *type = calloc(1, sizeof(Type));
        type->kind = TYPE_S64;
        return *type;
    }

    fprintf(stderr, "ERROR: function '" SV_FMT "' not declared",
            SV_ARG(id));
    exit(1);
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

bool types_match(Type *type1, Type *type2) {
    if (type1->kind == type2->kind) {
        if (type1->kind == TYPE_PTR) {
            return types_match(type1->ptr, type2->ptr);
        } else {
            return true;
        }
    } else {
        return false;
    }
}

Type type_expr(AstProgram *program, Expr *expr, IdTypes *id_types) {
    switch (expr->kind) {
    case EXPR_BOOL_LIT:
        expr->type.kind = TYPE_BOOL;
        break;
    case EXPR_INT_LIT:
        expr->type.kind = TYPE_S64;
        break;
    case EXPR_STR_LIT:
        expr->type.kind = TYPE_PTR;
        expr->type.ptr = calloc(1, sizeof(Type));
        expr->type.ptr->kind = TYPE_U8;
        break;
    case EXPR_ID:
        expr->type = get_type_from_id(*id_types, expr->as.id);
        expr->param_idx = get_param_idx_from_id(*id_types, expr->as.id);
        if (expr->type.kind == TYPE_NONE) {
            fprintf(stderr, "ERROR: Undefined variable " SV_FMT "\n",
                    SV_ARG(expr->as.id));
            exit(1);
        }
        break;
    case EXPR_UNOP:
        if (sv_eq_cstr(expr->as.unop.op, "!")) {
            if (type_expr(program, expr->as.unop.expr, id_types).kind !=
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
    case EXPR_BINOP:
        if (sv_eq_cstr(expr->as.binop.op, "+") ||
            sv_eq_cstr(expr->as.binop.op, "-") ||
            sv_eq_cstr(expr->as.binop.op, "*") ||
            sv_eq_cstr(expr->as.binop.op, "/") ||
            sv_eq_cstr(expr->as.binop.op, "%")
            ) {
            if (type_expr(program, expr->as.binop.lhs, id_types).kind !=
                TYPE_S64) {
                fprintf(stderr, "ERROR: Invalid type\n");
                exit(1);
            } else
                if (type_expr(program, expr->as.binop.rhs, id_types).kind
                    != TYPE_S64) {
                fprintf(stderr, "ERROR: Invalid type\n");
                exit(1);
            }
            expr->type.kind = TYPE_S64;
        } else if (sv_eq_cstr(expr->as.binop.op, "<") ||
                   sv_eq_cstr(expr->as.binop.op, "<=") ||
                   sv_eq_cstr(expr->as.binop.op, ">") ||
                   sv_eq_cstr(expr->as.binop.op, ">=") ||
                   sv_eq_cstr(expr->as.binop.op, "==") ||
                   sv_eq_cstr(expr->as.binop.op, "!=")
            ) {
            if (type_expr(program, expr->as.binop.lhs, id_types).kind !=
                TYPE_S64) {
                fprintf(stderr, "ERROR: Invalid type\n");
                exit(1);
            } else
                if (type_expr(program, expr->as.binop.rhs, id_types).kind
                    != TYPE_S64) {
                fprintf(stderr, "ERROR: Invalid type\n");
                exit(1);
            }
            expr->type.kind = TYPE_BOOL;
        } else if (sv_eq_cstr(expr->as.binop.op, "&&") ||
                   sv_eq_cstr(expr->as.binop.op, "||")
            ) {
            if (type_expr(program, expr->as.binop.lhs, id_types).kind !=
                TYPE_BOOL) {
                fprintf(stderr, "ERROR: Invalid type\n");
                exit(1);
            } else
                if (type_expr(program, expr->as.binop.rhs, id_types).kind
                    != TYPE_BOOL) {
                fprintf(stderr, "ERROR: Invalid type\n");
                exit(1);
            }
            expr->type.kind = TYPE_BOOL;
        } else {
            fprintf(stderr, "ERROR: Unknown operator\n");
            exit(1);
        }
        break;
    case EXPR_CALL:
        Args fn_args = get_fn_args(program, expr->as.call.id);

        if (fn_args.count != expr->as.call.count) {
            fprintf(stderr, "ERROR: Invalid number of arguments\n");
            exit(1);
        }

        for (size_t i = 0; i < fn_args.count; i++) {
            Type arg_type =
                type_expr(program, &expr->as.call.data[i], id_types);
            if (!types_match(&arg_type, &fn_args.data[i].type)) {
                fprintf(stderr, "ERROR: Invalid argument types\n");
                exit(1);
            }
        }

        expr->type = get_fn_ret_type(program, expr->as.call.id);
        break;
    }

    return expr->type;
}

void type_stmt(AstProgram *program, Stmt *stmt, IdTypes *id_types) {
    switch (stmt->kind) {
    case STMT_RET:
        type_expr(program, &stmt->as.ret, id_types);
        break;
    case STMT_VAR:
        if (get_type_from_id(*id_types, stmt->as.var.id).kind != TYPE_NONE) {
            fprintf(stderr, "ERROR: Variable " SV_FMT " already defined\n",
                    SV_ARG(stmt->as.var.id));
            exit(1);
        }
        IdType id_type = { };
        id_type.id = stmt->as.var.id;
        id_type.type = type_expr(program, &stmt->as.var.value, id_types);
        if (stmt->as.var.type.kind != TYPE_NONE
            && !types_match(&id_type.type, &stmt->as.var.type)) {
            assert(false);
        }
        da_push(id_types, id_type);
        break;
    case STMT_ASSIGN:
        type_expr(program, &stmt->as.assign.value, id_types);
        break;
    case STMT_IFTE:
        type_expr(program, &stmt->as.ifte.cond, id_types);
        start_scope(id_types);
        for (size_t i = 0; i < stmt->as.ifte.then_body.count; i++) {
            type_stmt(program, &stmt->as.ifte.then_body.data[i], id_types);
        }
        end_scope(id_types);
        start_scope(id_types);
        for (size_t i = 0; i < stmt->as.ifte.else_body.count; i++) {
            type_stmt(program, &stmt->as.ifte.else_body.data[i], id_types);
        }
        end_scope(id_types);
        break;
    case STMT_WHILE:
        type_expr(program, &stmt->as.ifte.cond, id_types);
        start_scope(id_types);
        for (size_t i = 0; i < stmt->as.while_stmt.body.count; i++) {
            type_stmt(program, &stmt->as.while_stmt.body.data[i],
                      id_types);
        }
        end_scope(id_types);
        break;
    case STMT_CALL:
        Args fn_args = get_fn_args(program, stmt->as.call.id);

        if (fn_args.count != stmt->as.call.count) {
            fprintf(stderr, "ERROR: Invalid number of arguments\n");
            exit(1);
        }

        for (size_t i = 0; i < fn_args.count; i++) {
            Type arg_type =
                type_expr(program, &stmt->as.call.data[i], id_types);
            if (!types_match(&arg_type, &fn_args.data[i].type)) {
                fprintf(stderr, "ERROR: Invalid argument types\n");
                exit(1);
            }
        }

        break;
    }
}

void type_fn(AstProgram *program, AstFn *fn) {
    IdTypes id_types = { };

    for (size_t i = 0; i < fn->args.count; i++) {
        IdType id_type = { };
        id_type.id = fn->args.data[i].id;
        id_type.type = fn->args.data[i].type;
        id_type.param_idx = fn->args.count - i;
        da_push(&id_types, id_type);
    }

    for (size_t i = 0; i < fn->body.count; i++) {
        type_stmt(program, &fn->body.data[i], &id_types);
    }
}

void type_program(AstProgram *program) {
    for (size_t i = 0; i < program->count; i++) {
        assert(program->data[i].kind == DECL_FN);
        type_fn(program, &program->data[i].as.fn);
    }
}
