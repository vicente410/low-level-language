#include "semantic.h"
#include <stdio.h>

typedef struct {
    Decl *data;
    size_t count;
    size_t capacity;
} Scope;

typedef struct {
    Scope *data;
    size_t count;
    size_t capacity;
} Scopes;

static Type type_nil = (Type) {
    .kind = TYPE_NIL,
};

static Type type_bool = (Type) {
    .kind = TYPE_BOOL,
};

static Type type_u8 = (Type) {
    .kind = TYPE_U8,
};

static Type type_u8_ptr = (Type) {
    .kind = TYPE_PTR,
    .as.ptr = &type_u8,
};

static Type type_s64 = (Type) {
    .kind = TYPE_S64,
};

Type *get_type_from_id(Scopes scopes, String_View id) {
    for (size_t i = 0; i < scopes.count; i++) {
        for (size_t j = 0; j < scopes.data[i].count; j++) {
            if (sv_eq(scopes.data[i].data[j].id, id)) {
                return scopes.data[i].data[j].type;
            }
	}
    }

    assert(false);
}

/*void start_scope(IdTypes *id_types) {
    da_push(id_types, ((IdType) {
                       .type = &type_none}));
}

void end_scope(IdTypes *id_types) {
    while (da_last(id_types).type->kind != TYPE_NONE) {
        (void) da_pop(id_types);
    }
    (void) da_pop(id_types);
}*/

bool types_match(Type *type1, Type *type2) {
    if (type1->kind == type2->kind) {
        if (type1->kind == TYPE_PTR) {
            return types_match(type1->as.ptr, type2->as.ptr);
	} else if (type1->kind == TYPE_STRUCT) {
            assert(false && "TODO: implement")
        } else {
            return true;
        }
    } else {
        return false;
    }
}

Type *type_expr(Expr *expr, Scopes *scopes) {
    switch (expr->kind) {
    case EXPR_BOOL_LIT:
        expr->type = &type_bool;
        break;
    case EXPR_INT_LIT:
        expr->type = &type_s64;
        break;
    case EXPR_STR_LIT:
        expr->type = &type_u8_ptr;
        break;
    /*case EXPR_ID:
        expr->type = get_type_from_id(*id_types, expr->as.id);
        expr->param_idx = get_param_idx_from_id(*id_types, expr->as.id);
        if (expr->type->kind == TYPE_NONE) {
            fprintf(stderr, "ERROR: Undefined variable " SV_FMT "\n",
                    SV_ARG(expr->as.id));
            exit(1);
        }
        break;
    case EXPR_UNOP:
        if (sv_eq_cstr(expr->as.unop.op, "!")) {
            if (type_expr(program, expr->as.unop.expr, id_types)->kind !=
                TYPE_BOOL) {
                fprintf(stderr, "ERROR: Invalid type\n");
                exit(1);
            }
            expr->type = &type_bool;
        } else if (sv_eq_cstr(expr->as.unop.op, "&")) {
            expr->type = calloc(1, sizeof(Type));
            expr->type->kind = TYPE_PTR;
            expr->type->ptr =
                type_expr(program, expr->as.unop.expr, id_types);
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
            if (type_expr(program, expr->as.binop.lhs, id_types)->kind !=
                TYPE_S64) {
                fprintf(stderr, "ERROR: Invalid type\n");
                exit(1);
            } else
                if (type_expr(program, expr->as.binop.rhs, id_types)->kind
                    != TYPE_S64) {
                fprintf(stderr, "ERROR: Invalid type\n");
                exit(1);
            }
            expr->type = &type_s64;
        } else if (sv_eq_cstr(expr->as.binop.op, "<") ||
                   sv_eq_cstr(expr->as.binop.op, "<=") ||
                   sv_eq_cstr(expr->as.binop.op, ">") ||
                   sv_eq_cstr(expr->as.binop.op, ">=") ||
                   sv_eq_cstr(expr->as.binop.op, "==") ||
                   sv_eq_cstr(expr->as.binop.op, "!=")
            ) {
            if (type_expr(program, expr->as.binop.lhs, id_types)->kind !=
                TYPE_S64) {
                fprintf(stderr, "ERROR: Invalid type\n");
                exit(1);
            } else
                if (type_expr(program, expr->as.binop.rhs, id_types)->kind
                    != TYPE_S64) {
                fprintf(stderr, "ERROR: Invalid type\n");
                exit(1);
            }
            expr->type = &type_bool;
        } else if (sv_eq_cstr(expr->as.binop.op, "&&") ||
                   sv_eq_cstr(expr->as.binop.op, "||")
            ) {
            if (type_expr(program, expr->as.binop.lhs, id_types)->kind !=
                TYPE_BOOL) {
                fprintf(stderr, "ERROR: Invalid type\n");
                exit(1);
            } else
                if (type_expr(program, expr->as.binop.rhs, id_types)->kind
                    != TYPE_BOOL) {
                fprintf(stderr, "ERROR: Invalid type\n");
                exit(1);
            }
            expr->type = &type_bool;
        } else if (sv_eq_cstr(expr->as.binop.op, ".") &&
                   expr->as.binop.rhs->kind == EXPR_ID &&
                   sv_eq_cstr(expr->as.binop.rhs->as.id, "*")) {
            if (type_expr(program, expr->as.binop.lhs, id_types)->kind !=
                TYPE_PTR) {
                fprintf(stderr, "ERROR: Dereferencing a non-pointer\n");
                exit(1);
            }
            expr->type = expr->as.binop.lhs->type->ptr;
        } else {
            fprintf(stderr, "ERROR: Unknown operator\n");
            exit(1);
        }
        break;
    case EXPR_CALL:
	Fn fn = get_env(env, expr->as.call.id);
        //Exprs fn_args = get_fn_args(program, expr->as.call.id);

        if (fn.args.count != expr->as.call.count) {
            fprintf(stderr, "ERROR: Invalid number of arguments\n");
            exit(1);
        }

        for (size_t i = 0; i < fn.args.count; i++) {
            Type *arg_type =
                type_expr(program, &expr->as.call.data[i], id_types);
            if (!types_match(arg_type, fn.args.data[i].type)) {
                fprintf(stderr, "ERROR: Invalid argument types\n");
                exit(1);
            }
        }

        expr->type = get_fn_ret_type(program, expr->as.call.id);
        break;*/
    case EXPR_CONST_DECL:
	type_expr(expr->as.decl.value);
	da_append(&scopes.data[scopes.count - 1], expr->as.decl);
        expr->type = &type_nil;
	break;
    case EXPR_VAR_DECL:
	type_expr(expr->as.decl.value);
	da_append(&scopes.data[scopes.count - 1], expr->as.decl);
        expr->type = &type_nil;
	break;
    }

    return expr->type;
}

#ifdef FALSE
void type_stmt(AstProgram *program, Stmt *stmt, IdTypes *id_types) {
    switch (stmt->kind) {
    case STMT_RET:
        type_expr(program, &stmt->as.ret, id_types);
        break;
    case STMT_VAR_DECL:
        if (get_type_from_id(*id_types, stmt->as.var_decl.id)->kind !=
            TYPE_NONE) {
            fprintf(stderr, "ERROR: Variable " SV_FMT " already defined\n",
                    SV_ARG(stmt->as.var_decl.id));
            exit(1);
        }
        IdType id_type = { };
        id_type.id = stmt->as.var_decl.id;
        id_type.type =
            type_expr(program, &stmt->as.var_decl.value, id_types);
        if (stmt->as.var_decl.type
            && stmt->as.var_decl.type->kind != TYPE_NONE
            && !types_match(id_type.type, stmt->as.var_decl.type)) {
            assert(false);
        }
        da_push(id_types, id_type);
        break;
    case STMT_ASSIGN:
        type_expr(program, &stmt->as.assign.target, id_types);
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
            Type *arg_type =
                type_expr(program, &stmt->as.call.data[i], id_types);
            if (!types_match(arg_type, fn_args.data[i].type)) {
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
#endif

void type_program(Exprs *program) {
    Scopes scopes = { };

    for (size_t i = 0; i < program->count; i++) {
	if (program->data[i]->kind != EXPR_CONST_DECL &&
	    program->data[i]->kind != EXPR_VAR_DECL) {
		assert(false && "TODO: implement");
	}

        type_expr(program->data[i], &scopes);
    }
}
