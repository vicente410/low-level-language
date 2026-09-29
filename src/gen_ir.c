#include "gen_ir.h"
#include "utils.h"

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
