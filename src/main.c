#include <stdio.h>
#include <ctype.h>
#define UTILS_IMPLEMENTATION
#include "utils.h"

// --- LEXER ---

typedef struct {
    String_View filename;
    size_t line;
    size_t col;
} Position;

String_View position_to_sv(Position pos) {
    String_Builder sb = {0};
 
    sb_appendf(&sb, SV_FMT":", SV_ARG(pos.filename));
    sb_appendf(&sb, "%zu:", pos.line);
    sb_appendf(&sb, "%zu:", pos.col);
            
    return sv_from_sb(sb);
}

typedef enum {
    TOKEN_EOF,
    TOKEN_ID,
    TOKEN_COLON,
    TOKEN_EQUAL,
    TOKEN_COMMA,
    TOKEN_OPEN_PAREN,
    TOKEN_CLOSE_PAREN,
    TOKEN_OPEN_CURLY,
    TOKEN_CLOSE_CURLY,
    TOKEN_RET,
    TOKEN_VAR,
    TOKEN_IF,
    TOKEN_ELSE,
    TOKEN_FN,
    TOKEN_STRUCT,
    TOKEN_UNION,
    TOKEN_INT_LIT,
    TOKEN_STRING_LIT,
    TOKEN_SEMICOLON,
    TOKEN_OP,
} TokenKind;

typedef struct {
    TokenKind kind;
    union {
        String_View id;
        int int_lit;
        String_View string_lit;
        String_View op;
    } as;
    Position pos;
} Token;

typedef struct {
    Token *data;
    size_t count;
    size_t capacity;
} Tokens;

typedef struct {
    FILE *fp;
    Position pos;
    char peeked_char;
    bool has_peeked_char;
    Token peeked_token;
    bool has_peeked_token;
} Lexer;

bool lexer_init(Lexer *lexer, char *filename) {
    lexer->fp = fopen(filename, "r");
    lexer->pos = (Position) {
        .filename = sv_from_cstr(filename),
        .line = 0,
        .col = 0,
    };
    lexer->has_peeked_char = false;
    lexer->has_peeked_token = false;

    return lexer->fp != NULL;
}

String_View token_to_sv(Token token) {
    String_Builder sb = {};
    
    switch (token.kind) {
    case TOKEN_EOF:         sb_appendf(&sb, "TOKEN_EOF"); break;
    case TOKEN_ID:          sb_appendf(&sb, "TOKEN_ID("SV_FMT")", SV_ARG(token.as.id)); break;
    case TOKEN_COLON:       sb_appendf(&sb, "TOKEN_COLON"); break;
    case TOKEN_COMMA:       sb_appendf(&sb, "TOKEN_COMMA"); break;
    case TOKEN_OPEN_PAREN:  sb_appendf(&sb, "TOKEN_OPEN_PAREN"); break;
    case TOKEN_CLOSE_PAREN: sb_appendf(&sb, "TOKEN_CLOSE_PAREN"); break;
    case TOKEN_OPEN_CURLY:  sb_appendf(&sb, "TOKEN_OPEN_CURLY"); break;
    case TOKEN_CLOSE_CURLY: sb_appendf(&sb, "TOKEN_CLOSE_CURLY"); break;
    case TOKEN_RET:         sb_appendf(&sb, "TOKEN_RET"); break;
    case TOKEN_VAR:         sb_appendf(&sb, "TOKEN_VAR"); break;
    case TOKEN_IF:          sb_appendf(&sb, "TOKEN_IF"); break;
    case TOKEN_ELSE:        sb_appendf(&sb, "TOKEN_ELSE"); break;
    case TOKEN_FN:          sb_appendf(&sb, "TOKEN_FN"); break;
    case TOKEN_STRUCT:      sb_appendf(&sb, "TOKEN_STRUCT"); break;
    case TOKEN_UNION:       sb_appendf(&sb, "TOKEN_UNION"); break;
    case TOKEN_INT_LIT:     sb_appendf(&sb, "TOKEN_INT_LIT(%d)", token.as.int_lit); break;
    case TOKEN_STRING_LIT:  sb_appendf(&sb, "TOKEN_STRING_LIT("SV_FMT")",
                                       SV_ARG(token.as.string_lit)); break;
    case TOKEN_SEMICOLON:   sb_appendf(&sb, "TOKEN_SEMICOLON"); break;
    case TOKEN_OP:          sb_appendf(&sb, "TOKEN_OP("SV_FMT")", SV_ARG(token.as.op)); break;
    default:
        fprintf(stderr, "UNREACHABLE\n");
        exit(1);
    }
 
    return sv_from_sb(sb);
}

char next_char(Lexer *lexer) {
    char ch;
    
    if (lexer->has_peeked_char) {
        lexer->has_peeked_char = false;
        ch = lexer->peeked_char;
    } else {
        ch = fgetc(lexer->fp);
    }

    if (ch == '\n') {
        lexer->pos.line++;
        lexer->pos.col = 0;
    } else {
        lexer->pos.col++;
    }

    return ch;
}

char peek_char(Lexer *lexer) {
    if (!(lexer->has_peeked_char)) {
        lexer->peeked_char = next_char(lexer);
        lexer->has_peeked_char = true;
    }
    return lexer->peeked_char;
}

bool accept_char(Lexer *lexer, char ch) {
    if (peek_char(lexer) == ch) {
        next_char(lexer);
        return true;
    }
    
    return false;
}

Token read_id_or_keyword(Lexer *lexer) {
    Token token = {};
    String_Builder sb = {};
    
    token.pos = lexer->pos;

    while (isalnum(peek_char(lexer))) {
        da_push(&sb, next_char(lexer));
    }

    if (sv_eq_cstr(sv_from_sb(sb), "return")) {
        token.kind = TOKEN_RET;
    } else if (sv_eq_cstr(sv_from_sb(sb), "var")) {
        token.kind = TOKEN_VAR;
    } else if (sv_eq_cstr(sv_from_sb(sb), "if")) {
        token.kind = TOKEN_IF;
    } else if (sv_eq_cstr(sv_from_sb(sb), "else")) {
        token.kind = TOKEN_ELSE;
    } else if (sv_eq_cstr(sv_from_sb(sb), "fn")) {
        token.kind = TOKEN_FN;
    } else if (sv_eq_cstr(sv_from_sb(sb), "struct")) {
        token.kind = TOKEN_STRUCT;
    } else if (sv_eq_cstr(sv_from_sb(sb), "union")) {
        token.kind = TOKEN_UNION;
    } else {
        token.kind = TOKEN_ID;
        token.as.id = sv_from_sb(sb);
    }
    
    return token;
}

Token read_int_lit(Lexer *lexer) {
    Token token = {};
    
    token.pos = lexer->pos;
    
    int int_lit = next_char(lexer) - '0';

    while(isdigit(peek_char(lexer))) {
        int_lit *= 10;
        int_lit += next_char(lexer) - '0';
    }

    token.kind = TOKEN_INT_LIT;
    token.as.int_lit = int_lit;
    
    return token;
}

Token read_string_lit(Lexer *lexer) {
    Token token = {};
    String_Builder sb = {};
    char ch;
    
    token.pos = lexer->pos;
    
    next_char(lexer);
    while ((ch = next_char(lexer)) != '"') {
        da_push(&sb, ch);
    }
    
    token.kind = TOKEN_STRING_LIT;
    token.as.string_lit = sv_from_sb(sb);
    
    return token;
}

Token read_symbol(Lexer *lexer) {
    Token token = {};
    char ch = next_char(lexer);
    
    token.pos = lexer->pos;
    
    switch (ch) {
    case ':': token.kind = TOKEN_COLON; break;
    case '=': {
        if (accept_char(lexer, '=')) {
            token.kind = TOKEN_OP;
            token.as.op = sv_from_cstr("==");
        } else {
            token.kind = TOKEN_EQUAL;
        }
    } break;
    case ',': token.kind = TOKEN_COMMA; break;
    case '(': token.kind = TOKEN_OPEN_PAREN; break;
    case ')': token.kind = TOKEN_CLOSE_PAREN; break;
    case '{': token.kind = TOKEN_OPEN_CURLY; break;
    case '}': token.kind = TOKEN_CLOSE_CURLY; break;
    case ';': token.kind = TOKEN_SEMICOLON; break;
    default:
        token.kind = TOKEN_OP;
        String_Builder sb = {};
        da_push(&sb, ch);
        while (strchr("+-*/!=<>", peek_char(lexer)) != NULL) {
            da_push(&sb, next_char(lexer));
        }
        token.as.op = sv_from_sb(sb);
    }
    
    return token;
}

Token next_token(Lexer *lexer) {
    if (lexer->has_peeked_token) {
        lexer->has_peeked_token = false;
        return lexer->peeked_token;
    }

    while (peek_char(lexer) == ' ' || peek_char(lexer) == '\n' || peek_char(lexer) == '\t') {
        next_char(lexer);
    }

    if (feof(lexer->fp)) {
    	return (Token) { .kind = TOKEN_EOF };
    } else if (isalpha(peek_char(lexer))) {
        return read_id_or_keyword(lexer);
    } else if (isdigit(peek_char(lexer))) {
        return read_int_lit(lexer);
    } else if (peek_char(lexer) == '"') {
        return read_string_lit(lexer);
    } else {
    	return read_symbol(lexer);
    }
}

Token peek_token(Lexer *lexer) {
    if (!(lexer->has_peeked_token)) {
        lexer->peeked_token = next_token(lexer);
        lexer->has_peeked_token = true;
    }
    return lexer->peeked_token;
}

void expect_token(Lexer *lexer, TokenKind kind) {
    if (next_token(lexer).kind != kind) {
        fprintf(stderr, "Error: Unexepected token");
        exit(1);
    }
}

bool accept_token(Lexer *lexer, TokenKind kind) {
    if (peek_token(lexer).kind == kind) {
        next_token(lexer);
        return true;
    }
    
    return false;
}

// --- PARSER ---

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

// --- IR ---

typedef enum {
    IR_INT,
    IR_REG,
} IrArgKind;

typedef struct {
    IrArgKind kind;
 
    union {
        int int_lit;
        String_View reg;
    } as;
} IrArg;

typedef enum {
    IR_LABEL,
    IR_MOV,
    IR_RET,
    IR_ADD,
    IR_SUB,
    IR_MUL,
    IR_DIV,
    IR_MOD,
} IrInstKind;

typedef struct {
    IrInstKind kind;
    String_View label;
    IrArg op1;
    IrArg op2;
    IrArg op3;
} IrInst;

typedef struct {
    String_View id;
    IrInst *data;
    size_t count;
    size_t capacity;
} IrFn;

typedef struct {
    IrFn *data;
    size_t count;
    size_t capacity;
} IrProgram;

String_View ir_arg_to_sv(IrArg arg) {
    String_Builder sb = {};

    switch (arg.kind) {
    case IR_INT: sb_appendf(&sb, "%d", arg.as.int_lit); break;
    case IR_REG: sb_appendf(&sb, SV_FMT, SV_ARG(arg.as.reg)); break;
    }

    return sv_from_sb(sb);
}

String_View ir_fn_to_sv(IrFn ir) {
    String_Builder sb = {};
    sb_appendf(&sb, SV_FMT":\n", SV_ARG(ir.id));
 
    for (size_t i = 0; i < ir.count; i++) {
        IrInst inst = ir.data[i];
        
        String_View sv_op1 = ir_arg_to_sv(inst.op1);
        String_View sv_op2 = ir_arg_to_sv(inst.op2);
        String_View sv_op3 = ir_arg_to_sv(inst.op3);

        switch (inst.kind) {
        case IR_LABEL: sb_appendf(&sb, SV_FMT, SV_ARG(inst.label)); break;
        case IR_MOV:   sb_appendf(&sb, "    mov "SV_FMT", "SV_FMT"\n",
                                  SV_ARG(sv_op1), SV_ARG(sv_op2)); break;
        case IR_RET:   sb_appendf(&sb, "    ret "SV_FMT"\n", SV_ARG(sv_op1)); break;
        case IR_ADD:   sb_appendf(&sb, "    add "SV_FMT", "SV_FMT", "SV_FMT"\n",
                                  SV_ARG(sv_op1), SV_ARG(sv_op2), SV_ARG(sv_op3)); break;
        case IR_SUB:   sb_appendf(&sb, "    sub "SV_FMT", "SV_FMT", "SV_FMT"\n",
                                  SV_ARG(sv_op1), SV_ARG(sv_op2), SV_ARG(sv_op3)); break;
        case IR_MUL:   sb_appendf(&sb, "    mul "SV_FMT", "SV_FMT", "SV_FMT"\n",
                                  SV_ARG(sv_op1), SV_ARG(sv_op2), SV_ARG(sv_op3)); break;
        case IR_DIV:   sb_appendf(&sb, "    div "SV_FMT", "SV_FMT", "SV_FMT"\n",
                                  SV_ARG(sv_op1), SV_ARG(sv_op2), SV_ARG(sv_op3)); break;
        case IR_MOD:   sb_appendf(&sb, "    mod "SV_FMT", "SV_FMT", "SV_FMT"\n",
                                  SV_ARG(sv_op1), SV_ARG(sv_op2), SV_ARG(sv_op3)); break;
        }
    }
 
    return sv_from_sb(sb);
}

IrArg gen_ir_expr(Expr expr, IrFn *ir_fn) {
    static size_t reg_num = 0;

    switch (expr.kind) {
    case EXPR_INT_LIT: {
        IrArg ir_arg = {};
        ir_arg.kind = IR_INT;
        ir_arg.as.int_lit = expr.as.int_lit;
        return ir_arg;
    }
    case EXPR_ID: {
        IrArg ir_arg = {};
        ir_arg.kind = IR_REG;
        ir_arg.as.reg = expr.as.id;
        return ir_arg;
    }
    case EXPR_OP: {
        IrInst inst = {};
 
        String_Builder sb = {};
        sb_appendf(&sb, "t%d", reg_num++);
        inst.op1.kind = IR_REG;
        inst.op1.as.reg = sv_from_sb(sb);
 
        inst.op2 = gen_ir_expr(*expr.as.op.lhs, ir_fn);
        inst.op3 = gen_ir_expr(*expr.as.op.rhs, ir_fn);
 
        if (sv_eq_cstr(expr.as.op.op, "+")) {
            inst.kind = IR_ADD;
        } else if (sv_eq_cstr(expr.as.op.op, "-")) {
            inst.kind = IR_SUB;
        } else if (sv_eq_cstr(expr.as.op.op, "*")) {
            inst.kind = IR_MUL;
        } else if (sv_eq_cstr(expr.as.op.op, "/")) {
            inst.kind = IR_DIV;
        } else if (sv_eq_cstr(expr.as.op.op, "%")) {
            inst.kind = IR_MOD;
        } else {
            assert(false);
        }
        
        da_push(ir_fn, inst);
        return inst.op1;
    }
    }
    
    assert(false);
}

IrFn gen_ir_fn(AstFn fn) {
    IrFn ir_fn = {};
    ir_fn.id = fn.id;
 
    for (size_t i = 0; i < fn.body.count; i++) {
        Stmt stmt = fn.body.data[i];
        IrInst inst;

        switch (stmt.kind) {
        case STMT_RET:
            inst.kind = IR_RET;
            inst.op1 = gen_ir_expr(stmt.as.ret, &ir_fn);
            break;
        case STMT_VAR:
            inst.kind = IR_MOV;
            inst.op1.kind = IR_REG;
            inst.op1.as.reg = stmt.as.var.id;
            inst.op2 = gen_ir_expr(stmt.as.var.value, &ir_fn);
            break;
        }
        da_push(&ir_fn, inst);
    }
 
    return ir_fn;
}

IrProgram gen_ir_program(AstProgram ast) {
    IrProgram ir;
 
    for (size_t i = 0; i < ast.count; i++) {
        assert(ast.data[i].kind == DECL_FN);
        da_push(&ir, gen_ir_fn(ast.data[i].as.fn));
    }
 
    return ir;
}

// --- COMPILER ---

typedef struct {
    String_View reg;
    size_t offset;
} RegOffset;

typedef struct {
    RegOffset *data;
    size_t count;
    size_t capacity;
} RegOffsets;

int get_reg_offset(RegOffsets *offsets, String_View reg) {
    for (size_t i = 0; i < offsets->count; i++) {
        if (sv_eq(offsets->data[i].reg, reg)) {
            return offsets->data[i].offset;
        }
    }
    
    return -1;
}

size_t get_num_ops(IrInst inst) {
    switch (inst.kind) {
    case IR_LABEL: return 0;
    case IR_MOV:   return 2;
    case IR_RET:   return 1;
    case IR_ADD:   return 3;
    case IR_SUB:   return 3;
    case IR_MUL:   return 3;
    case IR_DIV:   return 3;
    case IR_MOD:   return 3;
    default: assert(false);
    }
}

size_t allocate_registers(RegOffsets *offsets, IrFn fn) {
    size_t offset = 0;

    for (size_t i = 0; i < fn.count; i++) {
        IrInst inst = fn.data[i];
        size_t num_ops = get_num_ops(inst);
        
        for (size_t op = 1; op <= num_ops; op++) {
            IrArg arg;
            
            if (op == 1) arg = inst.op1;
            else if (op == 2) arg = inst.op2;
            else if (op == 3) arg = inst.op3;
            else assert(false);
            
            if (arg.kind == IR_REG && get_reg_offset(offsets, arg.as.reg) == -1) {
                da_push(offsets, ((RegOffset) {
                    .reg = arg.as.reg,
                    .offset = offset,
                }));
                
                offset += 8;
            }
        }
    }
    
    return offset;
}

void append_arg(String_Builder *sb, RegOffsets *offsets, IrArg arg) {
    switch (arg.kind) {
    case IR_REG:
        sb_appendf(sb, "[rbp - %zu]", get_reg_offset(offsets, arg.as.reg));
        break;
    case IR_INT:
        sb_appendf(sb, "%d", arg.as.int_lit);
        break;
    }
}

void compile_fn(String_Builder *sb, IrFn fn) {
    sb_appendf(sb, SV_FMT":\n", SV_ARG(fn.id));
    sb_appendf(sb, "    push rbp\n");
    sb_appendf(sb, "    mov rbp, rsp\n");
 
    RegOffsets offsets = {};
    sb_appendf(sb, "    sub rsp, %zu\n", allocate_registers(&offsets, fn));

    for (size_t i = 0; i < fn.count; i++) {
        IrInst inst = fn.data[i];
        assert(inst.op1.kind == IR_REG);

        switch (inst.kind) {
        case IR_LABEL:
            sb_appendf(sb, SV_FMT":\n", SV_ARG(inst.label));
            break;
        case IR_MOV:
            sb_appendf(sb, "    mov rax, ");
            append_arg(sb, &offsets, inst.op2);
            sb_appendf(sb, "\n    mov [rbp - %zu], rax\n", get_reg_offset(&offsets, inst.op1.as.reg));
            break;
        case IR_RET:
            sb_appendf(sb, "    mov rax, [rbp - %zu]\n", get_reg_offset(&offsets, inst.op1.as.reg));
            break;
        case IR_ADD:
            sb_appendf(sb, "    mov rax, ");
            append_arg(sb, &offsets, inst.op2);
            sb_appendf(sb, "\n    add rax, ");
            append_arg(sb, &offsets, inst.op3);
            sb_appendf(sb, "\n    mov [rbp - %zu], rax\n", get_reg_offset(&offsets, inst.op1.as.reg));
            break;
        case IR_SUB:
            sb_appendf(sb, "    mov rax, ");
            append_arg(sb, &offsets, inst.op2);
            sb_appendf(sb, "\n    sub rax, ");
            append_arg(sb, &offsets, inst.op3);
            sb_appendf(sb, "\n    mov [rbp - %zu], rax\n", get_reg_offset(&offsets, inst.op1.as.reg));
            break;
        case IR_MUL:
            sb_appendf(sb, "    mov rdx, ");
            append_arg(sb, &offsets, inst.op2);
            sb_appendf(sb, "\n    mov rax, ");
            append_arg(sb, &offsets, inst.op3);
            sb_appendf(sb, "\n    imul rax, rdx");
            sb_appendf(sb, "\n    mov [rbp - %zu], rax\n", get_reg_offset(&offsets, inst.op1.as.reg));
            break;
        case IR_DIV:
            sb_appendf(sb, "    mov rbx, ");
            append_arg(sb, &offsets, inst.op2);
            sb_appendf(sb, "\n    mov rax, ");
            append_arg(sb, &offsets, inst.op3);
            sb_appendf(sb, "\n    xor rdx, rdx");
            sb_appendf(sb, "\n    idiv rbx");
            sb_appendf(sb, "\n    mov [rbp - %zu], rax\n", get_reg_offset(&offsets, inst.op1.as.reg));
            break;
        case IR_MOD:
            sb_appendf(sb, "    mov rbx, ");
            append_arg(sb, &offsets, inst.op2);
            sb_appendf(sb, "\n    mov rax, ");
            append_arg(sb, &offsets, inst.op3);
            sb_appendf(sb, "\n    xor rdx, rdx");
            sb_appendf(sb, "\n    idiv rbx");
            sb_appendf(sb, "\n    mov [rbp - %zu], rdx\n", get_reg_offset(&offsets, inst.op1.as.reg));
            break;
        }
    }
 
    sb_appendf(sb, "    mov rsp, rbp\n");
    sb_appendf(sb, "    pop rbp\n");
    sb_appendf(sb, "    ret\n");
}

String_View compile_program(IrProgram program) {
    String_Builder sb = {0};

    sb_appendf(&sb, "format ELF64 executable 3\n");
    sb_appendf(&sb, "\n");
    sb_appendf(&sb, "segment readable executable\n");
    sb_appendf(&sb, "entry _start\n");
    sb_appendf(&sb, "_start:\n");
    sb_appendf(&sb, "    call main\n");
    sb_appendf(&sb, "    mov rdi, rax\n");
    sb_appendf(&sb, "    mov rax, 60\n");
    sb_appendf(&sb, "    syscall\n");
    sb_appendf(&sb, "\n");

    for (size_t i = 0; i < program.count; i++) {
        compile_fn(&sb, program.data[i]);
        sb_appendf(&sb, "\n");
    }
    
    return sv_from_sb(sb);
}

// --- MAIN ---

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s [filename]\n", argv[0]);
        exit(1);
    }

    char* program_name = argv[1];

    Lexer lexer = {};
    lexer_init(&lexer, program_name);
    
    AstProgram ast = parse_program(&lexer);
    IrProgram ir = gen_ir_program(ast);
    //for (size_t i = 0; i < ir.count; i++) {
    //    String_View ir_sv = ir_fn_to_sv(ir.data[i]);
    //    printf(SV_FMT"\n", SV_ARG(ir_sv));
    //}
    String_View assembly = compile_program(ir);
    printf(SV_FMT"\n", SV_ARG(assembly));
    
    /*for (size_t i = 0; i < program.count; i++) {
        String_View decl_sv = decl_to_sv(program.data[i], 0);
        printf(SV_FMT"\n", SV_ARG(decl_sv));
    }*/
    
    /*for (size_t i = 0; i < asts.count; i++) {
        String_View ast_sv = ast_to_sv(asts.data[i]);
        printf(SV_FMT"\n", SV_ARG(ast_sv));
    }*/

    /*while (true) {
        Token token = next_token(&lexer);
        if (token.kind == TOKEN_EOF) break;
        String_View token_sv = token_to_sv(token);
        printf(SV_FMT"\n", SV_ARG(token_sv));
    }*/

    return 0;
}
