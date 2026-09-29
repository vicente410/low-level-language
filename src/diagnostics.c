#include "diagnostics.h"
#include "utils.h"

String_View position_to_sv(Position pos) {
    String_Builder sb = {0};
              
    sb_appendf(&sb, SV_FMT":", SV_ARG(pos.filename));
    sb_appendf(&sb, "%zu:", pos.line);
    sb_appendf(&sb, "%zu:", pos.col);
                            
    return sv_from_sb(sb);
}
