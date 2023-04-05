#ifndef ADDRESS_H
#define ADDRESS_H

#include "types.h"

// Making all of these extern is kinda gross, but they're needed by
// the parser. The parser also needs to know the operations but does
// so differently, using an enum and constant global array (see
// op_registry in ops.h and ops.c)

extern Cell *addr_immediate(Cell *, uint);
extern Cell *addr_direct(Cell *cell, uint offset);
extern Cell *addr_a_indirect(Cell *, uint);
extern Cell *addr_b_indirect(Cell *, uint);
extern Cell *addr_a_indirect_predec(Cell *, uint);
extern Cell *addr_a_indirect_postinc(Cell *, uint);
extern Cell *addr_b_indirect_predec(Cell *, uint);
extern Cell *addr_b_indirect_postinc(Cell *, uint);

extern address_mode addr_method_from_char(char c);

extern char char_from_addr_method(address_mode m);

#endif
