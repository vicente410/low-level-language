#include <stdio.h>
#include <ctype.h>
#include "diagnostics.h"
#include "lexer.h"
#include "parser.h"
#include "semantic.h"
#include "gen_ir.h"
#include "gen_asm.h"
#define UTILS_IMPLEMENTATION
#include "utils.h"
#include "unistd.h"
#include "sys/wait.h"

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s [filename]\n", argv[0]);
        exit(1);
    }

    char *program_name = argv[1];

    Lexer lexer = { };
    lexer_init(&lexer, program_name);

    AstProgram ast = parse_program(&lexer);
    type_program(&ast);
    if (argc == 3 && strcmp(argv[2], "--ast") == 0) {
        for (size_t i = 0; i < ast.count; i++) {
            String_View ast_sv = decl_to_sv(ast.data[i], 0);
            printf(SV_FMT "\n", SV_ARG(ast_sv));
        }

        return 0;
    }

    IrProgram ir = gen_ir_program(ast);
    if (argc == 3 && strcmp(argv[2], "--ir") == 0) {
        for (size_t i = 0; i < ir.count; i++) {
            String_View ir_sv = ir_fn_to_sv(ir.data[i]);
            printf(SV_FMT "\n", SV_ARG(ir_sv));
        }

        return 0;
    }

    String_View assembly = compile_program(ir);
    if (argc == 3 && strcmp(argv[2], "--asm") == 0) {
        printf(SV_FMT "\n", SV_ARG(assembly));
        return 0;
    }

    char *asm_name = strdup(program_name);
    asm_name[strlen(asm_name) - 3] = 'a';
    asm_name[strlen(asm_name) - 2] = 's';
    asm_name[strlen(asm_name) - 1] = 'm';
    FILE *f = fopen(asm_name, "wb");
    fwrite(assembly.data, 1, assembly.count, f);
    fclose(f);

    char *const cmd[] = { "fasm", asm_name, NULL };

    pid_t pid = fork();

    switch (pid) {
    case -1:
        perror("fork");
        exit(1);
    case 0:
        execvp(cmd[0], cmd);
        perror("execvp");
        exit(1);
    default:
        wait(0);
    }

    remove(asm_name);

    return 0;
}
