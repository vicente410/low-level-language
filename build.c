#define UTILS_IMPLEMENTATION
#include "src/utils.h"

int main(int argc, char **argv) {
    Cmd cmd = { 0 };

    if (is_older_file("build", "build.c")) {
        cmd_append(&cmd, "cc");
        cmd_append(&cmd, "-o", "build");
        cmd_append(&cmd, "build.c");
        cmd_append(&cmd, "-Wall");
        cmd_append(&cmd, "-Wextra");
        if (cmd_run(&cmd) != 0) return 1;
        execvp(argv[0], argv);
    }

    if (argc == 2 && (strcmp(argv[1], "--format") == 0)) {
        cmd_append(&cmd, "indent");
        cmd_append(&cmd, "-kr");
        cmd_append(&cmd, "-nut");
        cmd_append(&cmd, "-brf");
        cmd_append(&cmd, "src/main.c", "src/utils.h",
                   "src/diagnostics.c", "src/diagnostics.h",
                   "src/lexer.c", "src/lexer.h",
                   "src/parser.c", "src/parser.h",
                   "src/semantic.c", "src/semantic.h",
                   "src/gen_asm.c", "src/gen_asm.h",
                   "src/gen_ir.c", "src/gen_ir.h");
        if (cmd_run(&cmd) != 0) return 1;
        
        remove("src/main.c~");
        remove("src/utils.h~");
        remove("src/diagnostics.c~");
        remove("src/diagnostics.h~");
        remove("src/lexer.c~");
        remove("src/lexer.h~");
        remove("src/parser.c~");
        remove("src/parser.h~");
        remove("src/semantic.c~");
        remove("src/semantic.h~");
        remove("src/gen_asm.c~");
        remove("src/gen_asm.h~");
        remove("src/gen_ir.c~");
        remove("src/gen_ir.h~");

        return 0;
    }

    cmd_append(&cmd, "cc");
    cmd_append(&cmd, "-o", "main");
    cmd_append(&cmd, "src/main.c");
    cmd_append(&cmd, "src/diagnostics.c");
    cmd_append(&cmd, "src/lexer.c");
    cmd_append(&cmd, "src/parser.c");
    //cmd_append(&cmd, "src/semantic.c");
    //cmd_append(&cmd, "src/gen_ir.c");
    //cmd_append(&cmd, "src/gen_asm.c");
    cmd_append(&cmd, "-Wall");
    cmd_append(&cmd, "-Wextra");
    if (cmd_run(&cmd) != 0) return 1;

    return 0;
}
