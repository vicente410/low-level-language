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
    case IR_CALL:
        if (inst.op1.as.reg.count == 0) {
            return 0;
        } else {
            return 1;
        }
    case IR_LABEL:
    case IR_JMP:
    case IR_PARAM:
        return 0;
    case IR_JEZ:
    case IR_RET:
        return 1;
    case IR_NOT:
    case IR_MOV:
        return 2;
    case IR_ADD:
    case IR_SUB:
    case IR_MUL:
    case IR_DIV:
    case IR_MOD:
    case IR_LT:
    case IR_LTE:
    case IR_GT:
    case IR_GTE:
    case IR_EQ:
    case IR_NEQ:
    case IR_AND:
    case IR_OR:
        return 3;
    default:
        assert(false);
    }
}

const char *get_rax_name(size_t size) {
    switch (size) {
    case 1:
        return "al";
    case 2:
        return "ax";
    case 4:
        return "eax";
    case 8:
        return "rax";
    default:
        fprintf(stderr, "ERROR: Invalid size %zu for register\n", size);
        exit(1);
    }
}

const char *get_rbx_name(size_t size) {
    switch (size) {
    case 1:
        return "bl";
    case 2:
        return "bx";
    case 4:
        return "ebx";
    case 8:
        return "rbx";
    default:
        fprintf(stderr, "ERROR: Invalid size %zu for register\n", size);
        exit(1);
    }
}

const char *get_rdx_name(size_t size) {
    switch (size) {
    case 1:
        return "dl";
    case 2:
        return "dx";
    case 4:
        return "edx";
    case 8:
        return "rdx";
    default:
        fprintf(stderr, "ERROR: Invalid size %zu for register\n", size);
        exit(1);
    }
}

size_t allocate_registers(RegOffsets *offsets, IrFn fn) {
    size_t offset = 8;

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

                offset += arg.size;
            }
        }
    }

    return offset;
}

void append_arg(String_Builder *sb, String_Builder *data,
                RegOffsets *offsets, IrArg arg) {
    static size_t string_num = 0;

    switch (arg.kind) {
    case IR_REG:
        sb_appendf(sb, "[rbp - %zu]", get_reg_offset(offsets, arg.as.reg));
        break;
    case IR_INT:
        sb_appendf(sb, "%d", arg.as.int_lit);
        break;
    case IR_STR:
        sb_appendf(data, "string_%zu db \"" SV_FMT "\"\n", string_num,
                   SV_ARG(arg.as.str_lit));
        sb_appendf(sb, "string_%zu", string_num);
        string_num += 1;
        break;
    case IR_ARG:
        sb_appendf(sb, "[rbp + %zu]", (arg.as.arg + 1) * 8);
        break;
    }
}

void compile_fn(String_Builder *sb, String_Builder *data, IrFn fn) {
    sb_appendf(sb, SV_FMT ":\n", SV_ARG(fn.id));
    sb_appendf(sb, "    push rbp\n");
    sb_appendf(sb, "    mov rbp, rsp\n");

    RegOffsets offsets = { };
    sb_appendf(sb, "    sub rsp, %zu\n", allocate_registers(&offsets, fn));
    size_t num_params = 0;

    for (size_t i = 0; i < fn.count; i++) {
        IrInst inst = fn.data[i];
        //assert(inst.op1.kind == IR_REG);        // TODO: return might be a number
        /*if (inst.op1.kind != IR_REG) {
           fprintf(stderr, "Inst Kind: %d\n", inst.kind);
           fprintf(stderr, "Op1  Kind: %d\n", inst.op1.kind);
           exit(1);
           } */

        switch (inst.kind) {
        case IR_LABEL:
            sb_appendf(sb, SV_FMT ":\n", SV_ARG(inst.label));
            break;
        case IR_MOV:
            assert(inst.op1.size == inst.op2.size);
            sb_appendf(sb, "    mov %s, ", get_rax_name(inst.op2.size));
            append_arg(sb, data, &offsets, inst.op2);
            sb_appendf(sb, "\n    mov [rbp - %zu], %s\n",
                       get_reg_offset(&offsets, inst.op1.as.reg),
                       get_rax_name(inst.op2.size));
            break;
        case IR_RET:
            sb_appendf(sb, "    mov %s, [rbp - %zu]\n",
                       get_rax_name(inst.op1.size),
                       get_reg_offset(&offsets, inst.op1.as.reg));
            break;
        case IR_ADD:
            assert(inst.op1.size == inst.op2.size
                   && inst.op2.size == inst.op3.size);
            sb_appendf(sb, "    mov %s, ", get_rax_name(inst.op2.size));
            append_arg(sb, data, &offsets, inst.op2);
            sb_appendf(sb, "\n    add %s, ", get_rax_name(inst.op3.size));
            append_arg(sb, data, &offsets, inst.op3);
            sb_appendf(sb, "\n    mov [rbp - %zu], %s\n",
                       get_reg_offset(&offsets, inst.op1.as.reg),
                       get_rax_name(inst.op3.size));
            break;
        case IR_SUB:
            assert(inst.op1.size == inst.op2.size
                   && inst.op2.size == inst.op3.size);
            sb_appendf(sb, "    mov %s, ", get_rax_name(inst.op2.size));
            append_arg(sb, data, &offsets, inst.op2);
            sb_appendf(sb, "\n    sub %s, ", get_rax_name(inst.op3.size));
            append_arg(sb, data, &offsets, inst.op3);
            sb_appendf(sb, "\n    mov [rbp - %zu], %s\n",
                       get_reg_offset(&offsets, inst.op1.as.reg),
                       get_rax_name(inst.op3.size));
            break;
        case IR_MUL:
            assert(inst.op1.size == inst.op2.size
                   && inst.op2.size == inst.op3.size);
            sb_appendf(sb, "    mov %s, ", get_rdx_name(inst.op2.size));
            append_arg(sb, data, &offsets, inst.op2);
            sb_appendf(sb, "\n    mov %s, ", get_rax_name(inst.op3.size));
            append_arg(sb, data, &offsets, inst.op3);
            sb_appendf(sb, "\n    imul %s, %s",
                       get_rax_name(inst.op3.size),
                       get_rdx_name(inst.op2.size));
            sb_appendf(sb, "\n    mov [rbp - %zu], %s\n",
                       get_reg_offset(&offsets, inst.op1.as.reg),
                       get_rax_name(inst.op3.size));
            break;
        case IR_DIV:
            assert(inst.op1.size == inst.op2.size
                   && inst.op2.size == inst.op3.size);
            sb_appendf(sb, "    mov %s, ", get_rbx_name(inst.op3.size));
            append_arg(sb, data, &offsets, inst.op3);
            sb_appendf(sb, "\n    xor rax, rax");
            sb_appendf(sb, "\n    mov %s, ", get_rax_name(inst.op2.size));
            append_arg(sb, data, &offsets, inst.op2);
            sb_appendf(sb, "\n    xor rdx, rdx");
            sb_appendf(sb, "\n    idiv %s", get_rbx_name(inst.op3.size));
            sb_appendf(sb, "\n    mov [rbp - %zu], %s\n",
                       get_reg_offset(&offsets, inst.op1.as.reg),
                       get_rax_name(inst.op3.size));
            break;
        case IR_MOD:
            assert(inst.op1.size == inst.op2.size
                   && inst.op2.size == inst.op3.size);
            sb_appendf(sb, "    mov %s, ", get_rbx_name(inst.op3.size));
            append_arg(sb, data, &offsets, inst.op3);
            sb_appendf(sb, "\n    xor rax, rax");
            sb_appendf(sb, "\n    mov %s, ", get_rax_name(inst.op2.size));
            append_arg(sb, data, &offsets, inst.op2);
            sb_appendf(sb, "\n    xor rdx, rdx");
            sb_appendf(sb, "\n    idiv %s", get_rbx_name(inst.op3.size));
            if (inst.op1.size == 1) {
                sb_appendf(sb, "\n    mov [rbp - %zu], ah\n",
                           get_reg_offset(&offsets, inst.op1.as.reg));
            } else {
                sb_appendf(sb, "\n    mov [rbp - %zu], %s\n",
                           get_reg_offset(&offsets, inst.op1.as.reg),
                           get_rdx_name(inst.op1.size));
            }
            break;
        case IR_LT:
            assert(inst.op1.size == 1);
            assert(inst.op2.size == inst.op3.size);
            sb_appendf(sb, "    mov %s, ", get_rbx_name(inst.op2.size));
            append_arg(sb, data, &offsets, inst.op2);
            sb_appendf(sb, "\n    mov %s, ", get_rdx_name(inst.op3.size));
            append_arg(sb, data, &offsets, inst.op3);
            sb_appendf(sb, "\n    cmp %s, %s", get_rbx_name(inst.op2.size),
                       get_rdx_name(inst.op3.size));
            sb_appendf(sb, "\n    setl al");
            sb_appendf(sb, "\n    mov [rbp - %zu], al\n",
                       get_reg_offset(&offsets, inst.op1.as.reg));
            break;
        case IR_LTE:
            assert(inst.op1.size == 1);
            assert(inst.op2.size == inst.op3.size);
            sb_appendf(sb, "    mov %s, ", get_rbx_name(inst.op2.size));
            append_arg(sb, data, &offsets, inst.op2);
            sb_appendf(sb, "\n    mov %s, ", get_rdx_name(inst.op3.size));
            append_arg(sb, data, &offsets, inst.op3);
            sb_appendf(sb, "\n    cmp %s, %s", get_rbx_name(inst.op2.size),
                       get_rdx_name(inst.op3.size));
            sb_appendf(sb, "\n    setle al");
            sb_appendf(sb, "\n    mov [rbp - %zu], al\n",
                       get_reg_offset(&offsets, inst.op1.as.reg));
            break;
        case IR_GT:
            assert(inst.op1.size == 1);
            assert(inst.op2.size == inst.op3.size);
            sb_appendf(sb, "    mov %s, ", get_rbx_name(inst.op2.size));
            append_arg(sb, data, &offsets, inst.op2);
            sb_appendf(sb, "\n    mov %s, ", get_rdx_name(inst.op3.size));
            append_arg(sb, data, &offsets, inst.op3);
            sb_appendf(sb, "\n    cmp %s, %s", get_rbx_name(inst.op2.size),
                       get_rdx_name(inst.op3.size));
            sb_appendf(sb, "\n    setg al");
            sb_appendf(sb, "\n    mov [rbp - %zu], al\n",
                       get_reg_offset(&offsets, inst.op1.as.reg));
            break;
        case IR_GTE:
            assert(inst.op1.size == 1);
            assert(inst.op2.size == inst.op3.size);
            sb_appendf(sb, "    mov %s, ", get_rbx_name(inst.op2.size));
            append_arg(sb, data, &offsets, inst.op2);
            sb_appendf(sb, "\n    mov %s, ", get_rdx_name(inst.op3.size));
            append_arg(sb, data, &offsets, inst.op3);
            sb_appendf(sb, "\n    cmp %s, %s", get_rbx_name(inst.op2.size),
                       get_rdx_name(inst.op3.size));
            sb_appendf(sb, "\n    setge al");
            sb_appendf(sb, "\n    mov [rbp - %zu], al\n",
                       get_reg_offset(&offsets, inst.op1.as.reg));
            break;
        case IR_EQ:
            assert(inst.op1.size == 1);
            assert(inst.op2.size == inst.op3.size);
            sb_appendf(sb, "    mov %s, ", get_rbx_name(inst.op2.size));
            append_arg(sb, data, &offsets, inst.op2);
            sb_appendf(sb, "\n    mov %s, ", get_rdx_name(inst.op3.size));
            append_arg(sb, data, &offsets, inst.op3);
            sb_appendf(sb, "\n    cmp %s, %s", get_rbx_name(inst.op2.size),
                       get_rdx_name(inst.op3.size));
            sb_appendf(sb, "\n    sete al");
            sb_appendf(sb, "\n    mov [rbp - %zu], al\n",
                       get_reg_offset(&offsets, inst.op1.as.reg));
            break;
        case IR_NEQ:
            assert(inst.op1.size == 1);
            assert(inst.op2.size == inst.op3.size);
            sb_appendf(sb, "    mov %s, ", get_rbx_name(inst.op2.size));
            append_arg(sb, data, &offsets, inst.op2);
            sb_appendf(sb, "\n    mov %s, ", get_rdx_name(inst.op3.size));
            append_arg(sb, data, &offsets, inst.op3);
            sb_appendf(sb, "\n    cmp %s, %s", get_rbx_name(inst.op2.size),
                       get_rdx_name(inst.op3.size));
            sb_appendf(sb, "\n    setne al");
            sb_appendf(sb, "\n    mov [rbp - %zu], al\n",
                       get_reg_offset(&offsets, inst.op1.as.reg));
            break;
        case IR_AND:
            assert(inst.op1.size == 1);
            assert(inst.op2.size == 1);
            assert(inst.op3.size == 1);
            sb_appendf(sb, "    mov al, ");
            append_arg(sb, data, &offsets, inst.op2);
            sb_appendf(sb, "\n    and al, ");
            append_arg(sb, data, &offsets, inst.op3);
            sb_appendf(sb, "\n    mov [rbp - %zu], al\n",
                       get_reg_offset(&offsets, inst.op1.as.reg));
            break;
        case IR_OR:
            assert(inst.op1.size == 1);
            assert(inst.op2.size == 1);
            assert(inst.op3.size == 1);
            sb_appendf(sb, "    mov al, ");
            append_arg(sb, data, &offsets, inst.op2);
            sb_appendf(sb, "\n    or al, ");
            append_arg(sb, data, &offsets, inst.op3);
            sb_appendf(sb, "\n    mov [rbp - %zu], al\n",
                       get_reg_offset(&offsets, inst.op1.as.reg));
            break;
        case IR_NOT:
            assert(inst.op1.size == 1);
            assert(inst.op2.size == 1);
            sb_appendf(sb, "    mov al, ");
            append_arg(sb, data, &offsets, inst.op2);
            sb_appendf(sb, "\n    not al");
            sb_appendf(sb, "\n    mov [rbp - %zu], al\n",
                       get_reg_offset(&offsets, inst.op1.as.reg));
            break;
        case IR_JMP:
            sb_appendf(sb, "    jmp " SV_FMT "\n", SV_ARG(inst.label));
            break;
        case IR_JEZ:
            assert(inst.op1.size == 1);
            sb_appendf(sb, "    mov al, ");
            append_arg(sb, data, &offsets, inst.op1);
            sb_appendf(sb, "\n    test al, al");
            sb_appendf(sb, "\n    jz " SV_FMT "\n", SV_ARG(inst.label));
            break;
        case IR_PARAM:
            sb_appendf(sb, "    mov %s, ", get_rax_name(inst.op1.size));
            append_arg(sb, data, &offsets, inst.op1);
            sb_appendf(sb, "\n    push rax\n");
            num_params += 1;
            break;
        case IR_CALL:
            sb_appendf(sb, "    call " SV_FMT "\n", SV_ARG(inst.label));
            if (inst.op1.as.reg.count > 0) {
                sb_appendf(sb, "    mov ");
                append_arg(sb, data, &offsets, inst.op1);
                sb_appendf(sb, ", %s\n", get_rax_name(inst.op1.size));
            }
            sb_appendf(sb, "    add rsp, %zu\n", num_params * 8);
            num_params = 0;
            break;
        }
    }

    sb_appendf(sb, "    mov rsp, rbp\n");
    sb_appendf(sb, "    pop rbp\n");
    sb_appendf(sb, "    ret\n");
}

String_View compile_program(IrProgram program) {
    String_Builder sb = { 0 };
    String_Builder data = { 0 };

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

    sb_appendf(&data, "segment readable writeable\n");
    sb_appendf(&data, "\n");

    for (size_t i = 0; i < program.count; i++) {
        compile_fn(&sb, &data, program.data[i]);
        sb_appendf(&sb, "\n");
        sb_appendf(&data, "\n");
    }

    da_append(&sb, &data);

    return sv_from_sb(sb);
}
