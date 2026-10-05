#include "diagnostics.h"
#include "utils.h"

typedef struct {
    String_View *data;
    size_t count;
    size_t capacity;
} Lines;

typedef struct {
    String_View filename;
    Lines lines;
} Source_File;

typedef struct {
    Source_File *data;
    size_t count;
    size_t capacity;
} Source_Files;

Source_Files source_files;

typedef enum {
    LEVEL_WARNING,
    LEVEL_ERROR,
} Diagnostic_Level;

typedef struct {
    Diagnostic_Level level;
    Position position;
    const char *message;
} Diagnostic;

typedef struct {
    Diagnostic *data;
    size_t count;
    size_t capacity;
} Diagnostics;

Diagnostics diagnostics;

String_View get_line_from_position(Position position) {
    for (size_t i = 0; i < source_files.count; i++) {
        if (sv_eq(source_files.data[i].filename, position.filename)) {
            assert(position.line - 1 < source_files.data[i].lines.count);
            return source_files.data[i].lines.data[position.line - 1];
        }
    }

    assert(false);
}

void diagnostic_add_file(String_View filename, String_View contents) {
    Source_File source_file = { 0 };
    String_View line = { 0 };

    source_file.filename = filename;

    while ((line = sv_split_delim(&contents, '\n')).count != 0) {
        da_push(&source_file.lines, line);
    }

    da_push(&source_files, source_file);
}

void diagnostic_warn(Position position, const char *message) {
    Diagnostic diagnostic = { 0 };
    diagnostic.level = LEVEL_WARNING;
    diagnostic.position = position;
    diagnostic.message = message;
    da_push(&diagnostics, diagnostic);
}

void diagnostic_error(Position position, const char *message) {
    Diagnostic diagnostic = { 0 };
    diagnostic.level = LEVEL_ERROR;
    diagnostic.position = position;
    diagnostic.message = message;
    da_push(&diagnostics, diagnostic);
}

void diagnostic_print(Diagnostic diagnostic) {
    Position pos = diagnostic.position;
    String_View line = get_line_from_position(pos);

    printf(SV_FMT ":%zu:%zu: ", SV_ARG(pos.filename), pos.line, pos.col);

    switch (diagnostic.level) {
    case LEVEL_WARNING:
        printf("warning: ");
        break;
    case LEVEL_ERROR:
        printf("error: ");
        break;
    default:
        assert(false);
    }

    printf("%s\n", diagnostic.message);
    printf("| " SV_FMT "\n", SV_ARG(line));
    printf("| %*s^\n", (int) pos.col - 1, "");
}

bool diagnostic_print_all() {
    bool has_error = false;

    for (size_t i = 0; i < diagnostics.count; i++) {
        if (diagnostics.data[i].level == LEVEL_ERROR) {
            has_error = true;
        }

        diagnostic_print(diagnostics.data[i]);
    }

    diagnostics.count = 0;

    return has_error;
}
