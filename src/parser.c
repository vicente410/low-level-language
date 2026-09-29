#include "parser.h"

String_View expr_to_sv(Expr expr, size_t indent) {
    String_Builder sb = {};
    
    for (size_t i = 0; i < indent; i++) sb_appendf(&sb, "    ");
    
    switch (expr.kind) {
    case EXPR_INT_LIT: sb_appendf(&sb, "INT(%d)", expr.as.int_lit); break;
    case EXPR_ID:      sb_appendf(&sb, "ID("SV_FMT")", SV_ARG(expr.as.id)); break;
    case EXPR_OP: {
        String_View lhs_sv = expr_to_sv(*expr.as.op.lhs, indent + 1);
        String_View rhs_sv = expr_to_sv(*expr.as.op.rhs, indent + 1);
        sb_appendf(&sb, "OP("SV_FMT")\n", SV_ARG(expr.as.op.op));
        sb_appendf(&sb, SV_FMT"\n", SV_ARG(lhs_sv));
        sb_appendf(&sb, SV_FMT, SV_ARG(rhs_sv));
    } break;
    }
    
    return sv_from_sb(sb);
}

String_View stmt_to_sv(Stmt stmt, size_t indent) {
    String_Builder sb = {};
    
    for (size_t i = 0; i < indent; i++) sb_appendf(&sb, "    ");
    
    switch (stmt.kind) {
    case STMT_RET: {
        sb_appendf(&sb, "RET\n");
        String_View expr_sv = expr_to_sv(stmt.as.ret, indent + 1);
        sb_appendf(&sb, SV_FMT, SV_ARG(expr_sv));
    } break;
    case STMT_VAR: {
        sb_appendf(&sb, "VAR "SV_FMT"\n", SV_ARG(stmt.as.var.id));
        String_View expr_sv = expr_to_sv(stmt.as.var.value, indent + 1);
        sb_appendf(&sb, SV_FMT, SV_ARG(expr_sv));
    } break;
    }
    
    return sv_from_sb(sb);
}

String_View decl_to_sv(Decl decl, size_t indent) {
    String_Builder sb = {};
    
    for (size_t i = 0; i < indent; i++) sb_appendf(&sb, "    ");
    
    switch (decl.kind) {
    case DECL_FN:
        sb_appendf(&sb, "FN "SV_FMT"\n", SV_ARG(decl.as.fn.id));
        
        for (size_t i = 0; i < decl.as.fn.body.count; i++) {
            String_View stmt_sv = stmt_to_sv(decl.as.fn.body.data[i], indent + 1);
            sb_appendf(&sb, SV_FMT"\n", SV_ARG(stmt_sv));
        }
        break;
    }
    
    return sv_from_sb(sb);
}

Expr *parse_expr(Lexer *lexer, size_t precedence) {
    Expr *expr;

    if (precedence == 4) {
        Token token = next_token(lexer);
  
        expr = calloc(1, sizeof(Expr));

        switch (token.kind) {
        case TOKEN_INT_LIT:
            expr->kind = EXPR_INT_LIT;
            expr->as.int_lit = token.as.int_lit;
            break;
        case TOKEN_ID:
            expr->kind = EXPR_ID;
            expr->as.id = token.as.id;
            break;
        case TOKEN_OPEN_PAREN:
            expr = parse_expr(lexer, 0);
            expect_token(lexer, TOKEN_CLOSE_PAREN);
            break;
        default:
            String_View pos_sv = position_to_sv(token.pos);
            fprintf(stderr, SV_FMT" ERROR: invalid expression\n", SV_ARG(pos_sv));
            exit(1);
        }
    } else {
        expr = parse_expr(lexer, precedence + 1);
    }

    while (peek_token(lexer).kind == TOKEN_OP && (
        (precedence == 3 && (
            sv_eq_cstr(peek_token(lexer).as.op, "."))) ||
        (precedence == 2 && (
            sv_eq_cstr(peek_token(lexer).as.op, "*") ||
            sv_eq_cstr(peek_token(lexer).as.op, "/") ||
            sv_eq_cstr(peek_token(lexer).as.op, "%"))) ||
        (precedence == 1 && (
            sv_eq_cstr(peek_token(lexer).as.op, "+") ||
            sv_eq_cstr(peek_token(lexer).as.op, "-"))) ||
        (precedence == 0 && (
            sv_eq_cstr(peek_token(lexer).as.op, "<") ||
            sv_eq_cstr(peek_token(lexer).as.op, ">") ||
            sv_eq_cstr(peek_token(lexer).as.op, "<=") ||
            sv_eq_cstr(peek_token(lexer).as.op, ">=") ||
            sv_eq_cstr(peek_token(lexer).as.op, "==") ||
            sv_eq_cstr(peek_token(lexer).as.op, "!=")))
    )) {
        Expr *new_expr = calloc(1, sizeof(Expr));
        new_expr->kind = EXPR_OP;
        new_expr->as.op.lhs = expr;
        new_expr->as.op.op = next_token(lexer).as.op;
        new_expr->as.op.rhs = parse_expr(lexer, precedence + 1);
        expr = new_expr;
    }

    return expr;
}

Stmt parse_stmt(Lexer *lexer) {
    Stmt stmt = {};
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
        
        expect_token(lexer, TOKEN_EQUAL);
        
        stmt.as.var.value = *parse_expr(lexer, 0);
        break;
    default:
        String_View pos_sv = position_to_sv(token.pos);
        fprintf(stderr, SV_FMT" ERROR: invalid statement\n", SV_ARG(pos_sv));
        exit(1);
    }
    
    return stmt;
}

Decl parse_decl(Lexer *lexer) {
    Decl decl = {};
    Token token = next_token(lexer);
  
    switch (token.kind) {
    case TOKEN_FN:
        decl.kind = DECL_FN;
        
        token = next_token(lexer);
        assert(token.kind == TOKEN_ID);
        decl.as.fn.id = token.as.id;

        expect_token(lexer, TOKEN_OPEN_PAREN);
        expect_token(lexer, TOKEN_CLOSE_PAREN);
        expect_token(lexer, TOKEN_OPEN_CURLY);
        
        while (!accept_token(lexer, TOKEN_CLOSE_CURLY)) {
            da_push(&decl.as.fn.body, parse_stmt(lexer));
        }
        break;
    default:
        String_View pos_sv = position_to_sv(token.pos);
        fprintf(stderr, SV_FMT" ERROR: invalid declaration\n", SV_ARG(pos_sv));
        exit(1);
    }

    return decl;
}

AstProgram parse_program(Lexer *lexer) {
    AstProgram program = {};
    
    while (peek_token(lexer).kind != TOKEN_EOF) {
        da_push(&program, parse_decl(lexer));
    }

    return program;
}
