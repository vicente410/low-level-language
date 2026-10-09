#include "parser.h"

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
    case TYPE_PTR:
        sb_appendf(&sb, "PTR\n");
        String_View type_sv = type_to_sv(type->ptr, indent + 1);
        sb_appendf(&sb, SV_FMT, SV_ARG(type_sv));
        break;
    }

    return sv_from_sb(sb);
}

String_View expr_to_sv(Expr expr, size_t indent) {
    String_Builder sb = { };

    for (size_t i = 0; i < indent; i++)
        sb_appendf(&sb, "    ");

    switch (expr.kind) {
    case EXPR_BOOL_LIT:
        if (expr.as.bool_lit) {
            sb_appendf(&sb, "TRUE");
        } else {
            sb_appendf(&sb, "FALSE");
        }
        break;
    case EXPR_INT_LIT:
        sb_appendf(&sb, "INT(%d)", expr.as.int_lit);
        break;
    case EXPR_STR_LIT:
        sb_appendf(&sb, "STR(" SV_FMT ")", SV_ARG(expr.as.str_lit));
        break;
    case EXPR_ID:
        sb_appendf(&sb, "ID(" SV_FMT ")", SV_ARG(expr.as.id));
        break;
    case EXPR_UNOP:{
            String_View expr_sv =
                expr_to_sv(*expr.as.unop.expr, indent + 1);
            sb_appendf(&sb, "UNOP(" SV_FMT ")\n", SV_ARG(expr.as.unop.op));
            sb_appendf(&sb, SV_FMT, SV_ARG(expr_sv));
        }
        break;
    case EXPR_BINOP:{
            String_View lhs_sv =
                expr_to_sv(*expr.as.binop.lhs, indent + 1);
            String_View rhs_sv =
                expr_to_sv(*expr.as.binop.rhs, indent + 1);
            sb_appendf(&sb, "BINOP(" SV_FMT ")\n",
                       SV_ARG(expr.as.binop.op));
            sb_appendf(&sb, SV_FMT "\n", SV_ARG(lhs_sv));
            sb_appendf(&sb, SV_FMT, SV_ARG(rhs_sv));
        }
        break;
    case EXPR_CALL:{
            sb_appendf(&sb, "CALL(" SV_FMT ")\n", SV_ARG(expr.as.call.id));
            for (size_t i = 0; i < expr.as.call.count; i++) {
                String_View arg_sv =
                    expr_to_sv(expr.as.call.data[i], indent + 1);
                sb_appendf(&sb, SV_FMT, SV_ARG(arg_sv));
                if (i < expr.as.call.count - 1)
                    sb_appendf(&sb, "\n");
            }
        }
        break;
    }

    return sv_from_sb(sb);
}

String_View stmt_to_sv(Stmt stmt, size_t indent) {
    String_Builder sb = { };

    for (size_t i = 0; i < indent; i++)
        sb_appendf(&sb, "    ");

    switch (stmt.kind) {
    case STMT_RET:{
            sb_appendf(&sb, "RET\n");
            String_View expr_sv = expr_to_sv(stmt.as.ret, indent + 1);
            sb_appendf(&sb, SV_FMT, SV_ARG(expr_sv));
        }
        break;
    case STMT_VAR:{
            sb_appendf(&sb, "VAR " SV_FMT "\n", SV_ARG(stmt.as.var.id));
            String_View type_sv = type_to_sv(stmt.as.var.type, indent);
            sb_appendf(&sb, SV_FMT "\n", SV_ARG(type_sv));
            String_View expr_sv =
                expr_to_sv(stmt.as.var.value, indent + 1);
            sb_appendf(&sb, SV_FMT, SV_ARG(expr_sv));
        }
        break;
    case STMT_ASSIGN:{
            sb_appendf(&sb, "ASSIGN " SV_FMT "\n",
                       SV_ARG(stmt.as.assign.id));
            String_View expr_sv =
                expr_to_sv(stmt.as.assign.value, indent + 1);
            sb_appendf(&sb, SV_FMT, SV_ARG(expr_sv));
        }
        break;
    case STMT_IFTE:{
            sb_appendf(&sb, "IF\n");
            String_View expr_sv =
                expr_to_sv(stmt.as.ifte.cond, indent + 1);
            sb_appendf(&sb, SV_FMT "\n", SV_ARG(expr_sv));

            for (size_t i = 0; i < indent; i++)
                sb_appendf(&sb, "    ");
            sb_appendf(&sb, "THEN\n");
            for (size_t i = 0; i < stmt.as.ifte.then_body.count; i++) {
                String_View stmt_sv =
                    stmt_to_sv(stmt.as.ifte.then_body.data[i], indent + 1);
                sb_appendf(&sb, SV_FMT, SV_ARG(stmt_sv));
                if (i < stmt.as.ifte.then_body.count - 1)
                    sb_appendf(&sb, "\n");
            }

            if (stmt.as.ifte.else_body.count > 0) {
                sb_appendf(&sb, "\n");
                for (size_t i = 0; i < indent; i++)
                    sb_appendf(&sb, "    ");
                sb_appendf(&sb, "ELSE\n");
                for (size_t i = 0; i < stmt.as.ifte.else_body.count; i++) {
                    String_View stmt_sv =
                        stmt_to_sv(stmt.as.ifte.else_body.data[i],
                                   indent + 1);
                    sb_appendf(&sb, SV_FMT, SV_ARG(stmt_sv));
                    if (i < stmt.as.ifte.else_body.count - 1)
                        sb_appendf(&sb, "\n");
                }
            }
        }
        break;
    case STMT_WHILE:{
            sb_appendf(&sb, "WHILE\n");
            String_View expr_sv =
                expr_to_sv(stmt.as.while_stmt.cond, indent + 1);
            sb_appendf(&sb, SV_FMT "\n", SV_ARG(expr_sv));

            for (size_t i = 0; i < indent; i++)
                sb_appendf(&sb, "    ");
            sb_appendf(&sb, "DO\n");
            for (size_t i = 0; i < stmt.as.while_stmt.body.count; i++) {
                String_View stmt_sv =
                    stmt_to_sv(stmt.as.while_stmt.body.data[i],
                               indent + 1);
                sb_appendf(&sb, SV_FMT, SV_ARG(stmt_sv));
                if (i < stmt.as.while_stmt.body.count - 1)
                    sb_appendf(&sb, "\n");
            }
        }
        break;
    case STMT_CALL:{
            sb_appendf(&sb, "CALL(" SV_FMT ")\n", SV_ARG(stmt.as.call.id));
            for (size_t i = 0; i < stmt.as.call.count; i++) {
                String_View arg_sv =
                    expr_to_sv(stmt.as.call.data[i], indent + 1);
                sb_appendf(&sb, SV_FMT, SV_ARG(arg_sv));
                if (i < stmt.as.call.count - 1)
                    sb_appendf(&sb, "\n");
            }
        }
        break;
    }

    return sv_from_sb(sb);
}

String_View decl_to_sv(Decl decl, size_t indent) {
    String_Builder sb = { };

    for (size_t i = 0; i < indent; i++)
        sb_appendf(&sb, "    ");

    switch (decl.kind) {
    case DECL_FN:
        sb_appendf(&sb, "FN " SV_FMT "\n", SV_ARG(decl.as.fn.id));

        for (size_t i = 0; i < decl.as.fn.args.count; i++) {
            String_View type_sv =
                type_to_sv(decl.as.fn.args.data[i].type, indent + 1);
            sb_appendf(&sb, SV_FMT "\n",
                       SV_ARG(decl.as.fn.args.data[i].id));
            sb_appendf(&sb, SV_FMT "\n", SV_ARG(type_sv));
        }

        sb_appendf(&sb, "RET\n");
        String_View type_sv = type_to_sv(decl.as.fn.ret_type, indent + 1);
        sb_appendf(&sb, SV_FMT "\n", SV_ARG(type_sv));

        sb_appendf(&sb, "DO\n");
        for (size_t i = 0; i < decl.as.fn.body.count; i++) {
            String_View stmt_sv =
                stmt_to_sv(decl.as.fn.body.data[i], indent + 1);
            sb_appendf(&sb, SV_FMT "\n", SV_ARG(stmt_sv));
        }
        break;
    }

    return sv_from_sb(sb);
}

Type *parse_type(Lexer *lexer) {
    Type *type = calloc(1, sizeof(Type));
    Token token = next_token(lexer);

    if (token.kind == TOKEN_ID) {
        if (sv_eq_cstr(token.as.id, "s64")) {
            type->kind = TYPE_S64;
        } else if (sv_eq_cstr(token.as.id, "u8")) {
            type->kind = TYPE_U8;
        } else if (sv_eq_cstr(token.as.id, "bool")) {
            type->kind = TYPE_BOOL;
        } else {
            assert(false);
        }
    } else if (token.kind == TOKEN_OP && sv_eq_cstr(token.as.op, "*")) {
        type->kind = TYPE_PTR;
        type->ptr = parse_type(lexer);
    } else {
        assert(false);
    }

    return type;
}

size_t get_op_precedence(String_View op) {
    if (sv_eq_cstr(op, "&&") || sv_eq_cstr(op, "||")) {
        return 0;
    } else if (sv_eq_cstr(op, "<") ||
               sv_eq_cstr(op, "<=") ||
               sv_eq_cstr(op, ">") ||
               sv_eq_cstr(op, ">=") ||
               sv_eq_cstr(op, "==") || sv_eq_cstr(op, "!=")) {
        return 1;
    } else if (sv_eq_cstr(op, "+") || sv_eq_cstr(op, "-")) {
        return 2;
    } else if (sv_eq_cstr(op, "*") ||
               sv_eq_cstr(op, "/") || sv_eq_cstr(op, "%")) {
        return 3;
    } else if (sv_eq_cstr(op, ".")) {
        return 4;
    } else {
        assert(false);
    }
}

Expr *parse_expr(Lexer *lexer, size_t precedence) {
    Expr *expr;

    if (precedence == 5) {
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
        case TOKEN_ID:
            if (accept_token(lexer, TOKEN_OPEN_PAREN)) {
                expr->kind = EXPR_CALL;
                expr->as.call.id = token.as.id;

                while (1) {
                    if (accept_token(lexer, TOKEN_CLOSE_PAREN)) {
                        break;
                    }

                    da_push(&expr->as.call, *parse_expr(lexer, 0));

                    if (accept_token(lexer, TOKEN_COMMA)) {
                        continue;
                    } else if (peek_token(lexer).kind != TOKEN_CLOSE_PAREN) {
                        assert(false);
                    }
                }
            } else {
                expr->kind = EXPR_ID;
                expr->as.id = token.as.id;
            }
            break;
        case TOKEN_OPEN_PAREN:
            expr = parse_expr(lexer, 0);
            expect_token(lexer, TOKEN_CLOSE_PAREN);
            break;
        case TOKEN_OP:
            expr->kind = EXPR_UNOP;
            expr->as.unop.op = token.as.op;
            expr->as.unop.expr = parse_expr(lexer, 0);
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

Stmt parse_stmt(Lexer *lexer) {
    Stmt stmt = { };
    Token token = next_token(lexer);

    switch (token.kind) {
    case TOKEN_RET:
        stmt.kind = STMT_RET;
        stmt.as.ret = *parse_expr(lexer, 0);
        break;
    case TOKEN_VAR:
        stmt.kind = STMT_VAR;

        token = next_token(lexer);
        assert(token.kind == TOKEN_ID);
        stmt.as.var.id = token.as.id;

        if (accept_token(lexer, TOKEN_COLON)) {
            stmt.as.var.type = parse_type(lexer);
        }
        expect_token(lexer, TOKEN_EQUAL);

        stmt.as.var.value = *parse_expr(lexer, 0);
        break;
    case TOKEN_IF:
        stmt.kind = STMT_IFTE;
        stmt.as.ifte.cond = *parse_expr(lexer, 0);

        expect_token(lexer, TOKEN_OPEN_CURLY);
        while (!accept_token(lexer, TOKEN_CLOSE_CURLY)) {
            da_push(&stmt.as.ifte.then_body, parse_stmt(lexer));
        }

        if (accept_token(lexer, TOKEN_ELSE)) {
            expect_token(lexer, TOKEN_OPEN_CURLY);
            while (!accept_token(lexer, TOKEN_CLOSE_CURLY)) {
                da_push(&stmt.as.ifte.else_body, parse_stmt(lexer));
            }
        }

        break;
    case TOKEN_WHILE:
        stmt.kind = STMT_WHILE;
        stmt.as.while_stmt.cond = *parse_expr(lexer, 0);

        expect_token(lexer, TOKEN_OPEN_CURLY);
        while (!accept_token(lexer, TOKEN_CLOSE_CURLY)) {
            da_push(&stmt.as.while_stmt.body, parse_stmt(lexer));
        }

        break;
    case TOKEN_ID:
        if (accept_token(lexer, TOKEN_OPEN_PAREN)) {
            stmt.kind = STMT_CALL;
            stmt.as.call.id = token.as.id;
            while (1) {
                if (accept_token(lexer, TOKEN_CLOSE_PAREN)) {
                    break;
                }

                da_push(&stmt.as.call, *parse_expr(lexer, 0));

                if (accept_token(lexer, TOKEN_COMMA)) {
                    continue;
                } else if (peek_token(lexer).kind != TOKEN_CLOSE_PAREN) {
                    assert(false);
                }
            }
        } else {
            stmt.kind = STMT_ASSIGN;
            stmt.as.assign.id = token.as.id;

            expect_token(lexer, TOKEN_EQUAL);

            stmt.as.assign.value = *parse_expr(lexer, 0);
        }
        break;
    default:
        diagnostic_error(token.pos, "invalid statement");
        diagnostic_print_all();
        exit(1);
    }

    return stmt;
}

Decl parse_decl(Lexer *lexer) {
    Decl decl = { };
    Token token = next_token(lexer);

    switch (token.kind) {
    case TOKEN_FN:
        decl.kind = DECL_FN;

        token = next_token(lexer);
        assert(token.kind == TOKEN_ID);
        decl.as.fn.id = token.as.id;

        expect_token(lexer, TOKEN_OPEN_PAREN);
        while (1) {
            if (accept_token(lexer, TOKEN_CLOSE_PAREN)) {
                break;
            }
            Arg arg = { };

            token = next_token(lexer);
            assert(token.kind == TOKEN_ID);
            arg.id = token.as.id;

            expect_token(lexer, TOKEN_COLON);
            arg.type = parse_type(lexer);

            da_push(&decl.as.fn.args, arg);

            if (accept_token(lexer, TOKEN_COMMA)) {
                continue;
            } else if (peek_token(lexer).kind != TOKEN_CLOSE_PAREN) {
                assert(false);
            }
        }

        if (accept_token(lexer, TOKEN_ARROW)) {
            decl.as.fn.ret_type = parse_type(lexer);
        }

        expect_token(lexer, TOKEN_OPEN_CURLY);

        while (!accept_token(lexer, TOKEN_CLOSE_CURLY)) {
            da_push(&decl.as.fn.body, parse_stmt(lexer));
        }
        break;
    default:
        diagnostic_error(token.pos, "invalid declaration");
        diagnostic_print_all();
        exit(1);
    }

    return decl;
}

AstProgram parse_program(Lexer *lexer) {
    AstProgram program = { };

    while (peek_token(lexer).kind != TOKEN_EOF) {
        da_push(&program, parse_decl(lexer));
    }

    return program;
}
