#include "utils.h"

#ifndef DIAGNOSTICS_H
#define DIAGNOSTICS_H

typedef struct {
    String_View filename;
    size_t line;
    size_t col;
} Position;

String_View position_to_sv(Position pos);

#endif // DIAGNOSTICS_H
