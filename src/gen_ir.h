#include "parser.h"
#include "utils.h"

#ifndef GEN_IR_H
#define GEN_IR_H

typedef enum {
    IR_INT,
    IR_STR,
    IR_REG,
    IR_ARG,
} IrArgKind;

typedef struct {
    size_t size;
    IrArgKind kind;

    union {
        int int_lit;
        String_View str_lit;
        String_View reg;
        size_t arg;
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
    IR_LT,
    IR_LTE,
    IR_GT,
    IR_GTE,
    IR_EQ,
    IR_NEQ,
    IR_AND,
    IR_OR,
    IR_NOT,
    IR_JEZ,
    IR_JMP,
    IR_PARAM,
    IR_CALL,
} IrInstKind;

typedef struct {
    IrInstKind kind;
    String_View label;
    IrArg op1;
    IrArg op2;
    IrArg op3;
    bool ret;
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

String_View ir_fn_to_sv(IrFn ir);
IrProgram gen_ir_program(AstProgram ast);

#endif                          // GEN_IR_H
