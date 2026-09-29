#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/wait.h>

int run_command(char *const cmd[]) {
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
        int status;
        wait(&status);
        
        if (WIFEXITED(status)) {
            return WEXITSTATUS(status);
        } else {
            exit(1);
        }
    }
}

int main() {
    struct stat c_attr, bin_attr;
    stat("build.c", &c_attr);
    stat("build", &bin_attr);
    
    if (c_attr.st_mtime > bin_attr.st_mtime) {
        char *const recompile_cmd[] = { "cc", "-o", "build", "build.c", NULL };
        run_command(recompile_cmd);
        char *const new_bin_cmd[] = { "./build", NULL };
        execvp(new_bin_cmd[0], new_bin_cmd);
    }

    char *const cmd[] = {
        "cc",
        "-o", "main",
        "src/main.c",
        "src/diagnostics.c",
        "src/lexer.c",
        "src/parser.c",
        "src/gen_ir.c",
        "src/gen_asm.c",
        "-Wall",
        "-Wextra",
        NULL
    };
    return run_command(cmd);
}
