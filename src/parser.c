#include "parser.h"

#ifdef FALSE
String_View type_to_sv(Type *type, size_t indent) {
    String_Builder sb = { };

    for (size_t i = 0; i < indent; i++)
        sb_appendf(&sb, "    ");

    switch (type->kind) {
    case TYPE_NONE:
        sb_appendf(&sb, "NONE");
        break;
    case TYPE_BOOL:
        sb_appendf(&sb, "BOOL");
        break;
    case TYPE_U8:
        sb_appendf(&sb, "U8");
        break;
    case TYPE_S64:
        sb_appendf(&sb, "S64");
        break;
    case TYPE_PTR:{
            sb_appendf(&sb, "PTR\n");
            String_View type_sv = type_to_sv(type->ptr, indent + 1);
            sb_appendf(&sb, SV_FMT, SV_ARG(type_sv));
        }
        break;
    case TYPE_ID:{
            String_View type_sv = type->id;
            sb_appendf(&sb, SV_FMT, SV_ARG(type_sv));
        }
        break;
    }

    return sv_from_sb(sb);
}
#endif

String_View expr_to_sv(Expr *expr, size_t indent) {
    String_Builder sb = { };

    for (size_t i = 0; i < indent; i++)
        sb_appendf(&sb, "    ");

    switch (expr->kind) {
    case EXPR_BOOL_LIT:
        if (expr->as.bool_lit) {
            sb_appendf(&sb, "TRUE");
        } else {
            sb_appendf(&sb, "FALSE");
        }
        break;
    case EXPR_INT_LIT:
        sb_appendf(&sb, "INT(%d)", expr->as.int_lit);
        break;
    case EXPR_STR_LIT:
        sb_appendf(&sb, "STR(" SV_FMT ")", SV_ARG(expr->as.str_lit));
        break;
    case EXPR_ID:
        sb_appendf(&sb, "ID(" SV_FMT ")", SV_ARG(expr->as.id));
        break;
    case EXPR_UNOP:{
            String_View expr_sv =
                expr_to_sv(expr->as.unop.expr, indent + 1);
            sb_appendf(&sb, "UNOP(" SV_FMT ")\n",
                       SV_ARG(expr->as.unop.op));
            sb_appendf(&sb, SV_FMT, SV_ARG(expr_sv));
        }
        break;
    case EXPR_BINOP:{
            String_View lhs_sv =
                expr_to_sv(expr->as.binop.lhs, indent + 1);
            String_View rhs_sv =
                expr_to_sv(expr->as.binop.rhs, indent + 1);
            sb_appendf(&sb, "BINOP(" SV_FMT ")\n",
                       SV_ARG(expr->as.binop.op));
            sb_appendf(&sb, SV_FMT "\n", SV_ARG(lhs_sv));
            sb_appendf(&sb, SV_FMT, SV_ARG(rhs_sv));
        }
        break;
    case EXPR_CALL:{
            sb_appendf(&sb, "CALL(" SV_FMT ")\n",
                       SV_ARG(expr->as.call.id));
            for (size_t i = 0; i < expr->as.call.args.count; i++) {
                String_View arg_sv =
                    expr_to_sv(expr->as.call.args.data[i], indent + 1);
                sb_appendf(&sb, SV_FMT, SV_ARG(arg_sv));
                if (i < expr->as.call.args.count - 1)
                    sb_appendf(&sb, "\n");
            }
        }
        break;
    case EXPR_CONST_DECL:{
            sb_appendf(&sb, "CONST_DECL(" SV_FMT ")\n",
                       SV_ARG(expr->as.decl.id));

            if (expr->as.decl.type) {
                String_View arg_sv =
                    expr_to_sv(expr->as.decl.type, indent + 1);
                sb_appendf(&sb, SV_FMT, SV_ARG(arg_sv));

                if (expr->as.decl.value)
                    sb_appendf(&sb, "\n");
            }

            if (expr->as.decl.value) {
                String_View arg_sv =
                    expr_to_sv(expr->as.decl.value, indent + 1);
                sb_appendf(&sb, SV_FMT, SV_ARG(arg_sv));
            }
        }
        break;
    case EXPR_VAR_DECL:{
            sb_appendf(&sb, "VAR_DECL(" SV_FMT ")\n",
                       SV_ARG(expr->as.decl.id));

            if (expr->as.decl.type) {
                String_View arg_sv =
                    expr_to_sv(expr->as.decl.type, indent + 1);
                sb_appendf(&sb, SV_FMT, SV_ARG(arg_sv));

                if (expr->as.decl.value)
                    sb_appendf(&sb, "\n");
            }

            if (expr->as.decl.value) {
                String_View arg_sv =
                    expr_to_sv(expr->as.decl.value, indent + 1);
                sb_appendf(&sb, SV_FMT, SV_ARG(arg_sv));
            }
        }
        break;
    case EXPR_BLOCK:{
            sb_appendf(&sb, "BLOCK\n");

            for (size_t i = 0; i < expr->as.block.count; i++) {
                String_View arg_sv =
                    expr_to_sv(expr->as.block.data[i], indent + 1);
                sb_appendf(&sb, SV_FMT, SV_ARG(arg_sv));
                if (i < expr->as.block.count - 1)
                    sb_appendf(&sb, "\n");
            }
        }
        break;
    case EXPR_IFTE:{
            sb_appendf(&sb, "IF\n");
            String_View cond_sv =
                expr_to_sv(expr->as.ifte.cond, indent + 1);
            sb_appendf(&sb, SV_FMT "\n", SV_ARG(cond_sv));

            for (size_t i = 0; i < indent; i++)
                sb_appendf(&sb, "    ");

            sb_appendf(&sb, "THEN\n");
            String_View then_sv =
                expr_to_sv(expr->as.ifte.then_body, indent + 1);
            sb_appendf(&sb, SV_FMT, SV_ARG(then_sv));

            if (expr->as.ifte.else_body) {
                sb_appendf(&sb, "\n");
                for (size_t i = 0; i < indent; i++)
                    sb_appendf(&sb, "    ");

                sb_appendf(&sb, "ELSE\n");
                String_View else_sv =
                    expr_to_sv(expr->as.ifte.else_body, indent + 1);
                sb_appendf(&sb, SV_FMT, SV_ARG(else_sv));
            }
        }
        break;
    case EXPR_WHILE:{
            sb_appendf(&sb, "WHILE\n");
            String_View cond_sv =
                expr_to_sv(expr->as.whil.cond, indent + 1);
            sb_appendf(&sb, SV_FMT "\n", SV_ARG(cond_sv));

            for (size_t i = 0; i < indent; i++)
                sb_appendf(&sb, "    ");

            sb_appendf(&sb, "DO\n");
            String_View body_sv =
                expr_to_sv(expr->as.whil.body, indent + 1);
            sb_appendf(&sb, SV_FMT, SV_ARG(body_sv));
        }
        break;
    case EXPR_FN:{
            sb_appendf(&sb, "FN\n");

            for (size_t i = 0; i < expr->as.fn.args.count; i++) {
                String_View arg_sv =
                    expr_to_sv(expr->as.fn.args.data[i], indent + 1);
                sb_appendf(&sb, SV_FMT "\n", SV_ARG(arg_sv));
            }

            String_View value_sv =
                expr_to_sv(expr->as.fn.value, indent + 1);
            sb_appendf(&sb, SV_FMT, SV_ARG(value_sv));
        }
        break;
    case EXPR_STRUCT:{
            sb_appendf(&sb, "STRUCT\n");

            for (size_t i = 0; i < expr->as.struct_type.fields.count; i++) {
                String_View arg_sv =
                    expr_to_sv(expr->as.struct_type.fields.data[i],
                               indent + 1);
                sb_appendf(&sb, SV_FMT, SV_ARG(arg_sv));
                if (i < expr->as.struct_type.fields.count - 1)
                    sb_appendf(&sb, "\n");
            }
        }
        break;
    }

    return sv_from_sb(sb);
}

size_t get_op_precedence(String_View op) {
    if (sv_eq_cstr(op, "=") ||
        sv_eq_cstr(op, "+=") ||
        sv_eq_cstr(op, "-=") ||
        sv_eq_cstr(op, "*=") ||
        sv_eq_cstr(op, "/=") || sv_eq_cstr(op, "%=")) {
        return 0;
    } else if (sv_eq_cstr(op, "&&") || sv_eq_cstr(op, "||")) {
        return 1;
    } else if (sv_eq_cstr(op, "<") ||
               sv_eq_cstr(op, "<=") ||
               sv_eq_cstr(op, ">") ||
               sv_eq_cstr(op, ">=") ||
               sv_eq_cstr(op, "==") || sv_eq_cstr(op, "!=")) {
        return 2;
    } else if (sv_eq_cstr(op, "+") || sv_eq_cstr(op, "-")) {
        return 3;
    } else if (sv_eq_cstr(op, "*") ||
               sv_eq_cstr(op, "/") || sv_eq_cstr(op, "%")) {
        return 4;
    } else if (sv_eq_cstr(op, ".")) {
        return 5;
    } else {
        assert(false);
    }
}

Expr *parse_expr(Lexer *lexer, size_t precedence) {
    Expr *expr;

    if (precedence >= 6) {
        Token token = next_token(lexer);

        expr = calloc(1, sizeof(Expr));

        switch (token.kind) {
        case TOKEN_TRUE:
            expr->kind = EXPR_BOOL_LIT;
            expr->as.bool_lit = true;
            break;
        case TOKEN_FALSE:
            expr->kind = EXPR_BOOL_LIT;
            expr->as.bool_lit = false;
            break;
        case TOKEN_INT_LIT:
            expr->kind = EXPR_INT_LIT;
            expr->as.int_lit = token.as.int_lit;
            break;
        case TOKEN_STRING_LIT:
            expr->kind = EXPR_STR_LIT;
            expr->as.str_lit = token.as.string_lit;
            break;
        case TOKEN_IF:
            expr->kind = EXPR_IFTE;
            expr->as.ifte.cond = parse_expr(lexer, 0);
            expr->as.ifte.then_body = parse_expr(lexer, 0);

            if (accept_token(lexer, TOKEN_ELSE)) {
                expr->as.ifte.else_body = parse_expr(lexer, 0);
            }
            break;
        case TOKEN_WHILE:
            expr->kind = EXPR_WHILE;
            expr->as.whil.cond = parse_expr(lexer, 0);
            expr->as.whil.body = parse_expr(lexer, 0);
            break;
        case TOKEN_FN:{
                expr->kind = EXPR_FN;
                expect_token(lexer, TOKEN_OPEN_PAREN);

                while (1) {
                    if (accept_token(lexer, TOKEN_CLOSE_PAREN)) {
                        break;
                    }

                    da_push(&expr->as.fn.args, parse_expr(lexer, 0));
                    if (accept_token(lexer, TOKEN_COMMA)) {
                        continue;
                    } else if (peek_token(lexer).kind != TOKEN_CLOSE_PAREN) {
                        assert(false);
                    }
                }

                expr->as.fn.value = parse_expr(lexer, 0);
            }
            break;
        case TOKEN_ID:
            if (accept_token(lexer, TOKEN_OPEN_PAREN)) {
                expr->kind = EXPR_CALL;
                expr->as.call.id = token.as.id;
                while (1) {
                    if (accept_token(lexer, TOKEN_CLOSE_PAREN)) {
                        break;
                    }

                    da_push(&expr->as.call.args, parse_expr(lexer, 0));
                    if (accept_token(lexer, TOKEN_COMMA)) {
                        continue;
                    } else if (peek_token(lexer).kind != TOKEN_CLOSE_PAREN) {
                        assert(false);
                    }
                }
            } else if (precedence == 6 && accept_token(lexer, TOKEN_COLON)) {
                expr->as.decl.id = token.as.id;
                token = peek_token(lexer);
                if (token.kind == TOKEN_OP && sv_eq_cstr(token.as.op, "=")) {
                    next_token(lexer);
                    expr->kind = EXPR_VAR_DECL;
                    expr->as.decl.value = parse_expr(lexer, 0);
                } else if (accept_token(lexer, TOKEN_COLON)) {
                    expr->kind = EXPR_CONST_DECL;
                    expr->as.decl.value = parse_expr(lexer, 0);
                } else {
                    expr->kind = EXPR_CONST_DECL;
                    expr->as.decl.type = parse_expr(lexer, precedence + 1);
                    token = peek_token(lexer);
                    if (token.kind == TOKEN_OP
                        && sv_eq_cstr(token.as.op, "=")) {
                        next_token(lexer);
                        expr->kind = EXPR_VAR_DECL;
                        expr->as.decl.value = parse_expr(lexer, 0);
                    } else if (accept_token(lexer, TOKEN_COLON)) {
                        expr->as.decl.value = parse_expr(lexer, 0);
                    }
                }
            } else {
                expr->kind = EXPR_ID;
                expr->as.id = token.as.id;
            }
            break;
        case TOKEN_OPEN_CURLY:
            expr->kind = EXPR_BLOCK;
            while (!accept_token(lexer, TOKEN_CLOSE_CURLY)) {
                da_push(&expr->as.block, parse_expr(lexer, 0));
            }
            break;
        case TOKEN_STRUCT:
            expr->kind = EXPR_STRUCT;
            expect_token(lexer, TOKEN_OPEN_CURLY);
            while (!accept_token(lexer, TOKEN_CLOSE_CURLY)) {
                da_push(&expr->as.struct_type.fields,
                        parse_expr(lexer, 0));
            }
            break;
        case TOKEN_OP:
            if (sv_eq_cstr(token.as.op, "*")) {
                expr->kind = EXPR_ID;
                expr->as.id = token.as.id;
            } else {
                expr->kind = EXPR_UNOP;
                expr->as.unop.op = token.as.op;
                expr->as.unop.expr = parse_expr(lexer, 0);
            }
            break;
        default:
            diagnostic_error(token.pos, "invalid expression");
            diagnostic_print_all();
            exit(1);
        }
    } else {
        expr = parse_expr(lexer, precedence + 1);
    }

    while (peek_token(lexer).kind == TOKEN_OP
           && (precedence == get_op_precedence(peek_token(lexer).as.op))) {
        Expr *new_expr = calloc(1, sizeof(Expr));
        new_expr->kind = EXPR_BINOP;
        new_expr->as.binop.lhs = expr;
        new_expr->as.binop.op = next_token(lexer).as.op;
        new_expr->as.binop.rhs = parse_expr(lexer, precedence + 1);
        expr = new_expr;
    }

    return expr;
}

Exprs parse_program(Lexer *lexer) {
    Exprs program = { };
    while (peek_token(lexer).kind != TOKEN_EOF) {
        da_push(&program, parse_expr(lexer, 0));
    }

    return program;
}
