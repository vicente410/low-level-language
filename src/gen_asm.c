#include "gen_asm.h"
#include "utils.h"

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
    case IR_LABEL:
        return 0;
    case IR_MOV:
        return 2;
    case IR_RET:
        return 1;
    case IR_ADD:
        return 3;
    case IR_SUB:
        return 3;
    case IR_MUL:
        return 3;
    case IR_DIV:
        return 3;
    case IR_MOD:
        return 3;
    case IR_JMP:
        return 0;
    case IR_JEZ:
        return 1;
    default:
        assert(false);
    }
}

size_t allocate_registers(RegOffsets *offsets, IrFn fn) {
    size_t offset = 0;

    for (size_t i = 0; i < fn.count; i++) {
        IrInst inst = fn.data[i];
        size_t num_ops = get_num_ops(inst);

        for (size_t op = 1; op <= num_ops; op++) {
            IrArg arg;

            if (op == 1)
                arg = inst.op1;
            else if (op == 2)
                arg = inst.op2;
            else if (op == 3)
                arg = inst.op3;
            else
                assert(false);

            if (arg.kind == IR_REG
                && get_reg_offset(offsets, arg.as.reg) == -1) {
                da_push(offsets, ((RegOffset) {
                                  .reg = arg.as.reg,.offset = offset,}
                        ));

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
    sb_appendf(sb, SV_FMT ":\n", SV_ARG(fn.id));
    sb_appendf(sb, "    push rbp\n");
    sb_appendf(sb, "    mov rbp, rsp\n");

    RegOffsets offsets = { };
    sb_appendf(sb, "    sub rsp, %zu\n", allocate_registers(&offsets, fn));

    for (size_t i = 0; i < fn.count; i++) {
        IrInst inst = fn.data[i];
        assert(inst.op1.kind == IR_REG);

        switch (inst.kind) {
        case IR_LABEL:
            sb_appendf(sb, SV_FMT ":\n", SV_ARG(inst.label));
            break;
        case IR_MOV:
            sb_appendf(sb, "    mov rax, ");
            append_arg(sb, &offsets, inst.op2);
            sb_appendf(sb, "\n    mov [rbp - %zu], rax\n",
                       get_reg_offset(&offsets, inst.op1.as.reg));
            break;
        case IR_RET:
            sb_appendf(sb, "    mov rax, [rbp - %zu]\n",
                       get_reg_offset(&offsets, inst.op1.as.reg));
            break;
        case IR_ADD:
            sb_appendf(sb, "    mov rax, ");
            append_arg(sb, &offsets, inst.op2);
            sb_appendf(sb, "\n    add rax, ");
            append_arg(sb, &offsets, inst.op3);
            sb_appendf(sb, "\n    mov [rbp - %zu], rax\n",
                       get_reg_offset(&offsets, inst.op1.as.reg));
            break;
        case IR_SUB:
            sb_appendf(sb, "    mov rax, ");
            append_arg(sb, &offsets, inst.op2);
            sb_appendf(sb, "\n    sub rax, ");
            append_arg(sb, &offsets, inst.op3);
            sb_appendf(sb, "\n    mov [rbp - %zu], rax\n",
                       get_reg_offset(&offsets, inst.op1.as.reg));
            break;
        case IR_MUL:
            sb_appendf(sb, "    mov rdx, ");
            append_arg(sb, &offsets, inst.op2);
            sb_appendf(sb, "\n    mov rax, ");
            append_arg(sb, &offsets, inst.op3);
            sb_appendf(sb, "\n    imul rax, rdx");
            sb_appendf(sb, "\n    mov [rbp - %zu], rax\n",
                       get_reg_offset(&offsets, inst.op1.as.reg));
            break;
        case IR_DIV:
            sb_appendf(sb, "    mov rbx, ");
            append_arg(sb, &offsets, inst.op2);
            sb_appendf(sb, "\n    mov rax, ");
            append_arg(sb, &offsets, inst.op3);
            sb_appendf(sb, "\n    xor rdx, rdx");
            sb_appendf(sb, "\n    idiv rbx");
            sb_appendf(sb, "\n    mov [rbp - %zu], rax\n",
                       get_reg_offset(&offsets, inst.op1.as.reg));
            break;
        case IR_MOD:
            sb_appendf(sb, "    mov rbx, ");
            append_arg(sb, &offsets, inst.op2);
            sb_appendf(sb, "\n    mov rax, ");
            append_arg(sb, &offsets, inst.op3);
            sb_appendf(sb, "\n    xor rdx, rdx");
            sb_appendf(sb, "\n    idiv rbx");
            sb_appendf(sb, "\n    mov [rbp - %zu], rdx\n",
                       get_reg_offset(&offsets, inst.op1.as.reg));
            break;
        case IR_JMP:
            sb_appendf(sb, "    jmp " SV_FMT "\n", SV_ARG(inst.label));
            break;
        case IR_JEZ:
            sb_appendf(sb, "    mov rax, ");
            append_arg(sb, &offsets, inst.op1);
            sb_appendf(sb, "\n    test rax, rax");
            sb_appendf(sb, "\n    jz " SV_FMT "\n", SV_ARG(inst.label));
            break;
        }
    }

    sb_appendf(sb, "    mov rsp, rbp\n");
    sb_appendf(sb, "    pop rbp\n");
    sb_appendf(sb, "    ret\n");
}

String_View compile_program(IrProgram program) {
    String_Builder sb = { 0 };

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
