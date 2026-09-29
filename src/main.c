#include <stdio.h>
#include <ctype.h>
#include "diagnostics.h"
#include "lexer.h"
#include "parser.h"
#include "gen_ir.h"
#include "gen_asm.h"
#define UTILS_IMPLEMENTATION
#include "utils.h"

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
