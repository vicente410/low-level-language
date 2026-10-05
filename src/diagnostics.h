#include "utils.h"

#ifndef DIAGNOSTICS_H
#define DIAGNOSTICS_H

typedef struct {
    String_View filename;
    size_t line;
    size_t col;
} Position;

//String_View position_to_sv(Position pos);

void diagnostic_add_file(String_View filename, String_View contents);
void diagnostic_warn(Position pos, const char *message);
void diagnostic_error(Position pos, const char *message);
bool diagnostic_print_all();

#endif                          // DIAGNOSTICS_H
