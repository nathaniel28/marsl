#ifndef ADDRESS_H
#define ADDRESS_H

#include "types.h"

extern int resolve_field(int cell, int field, Valuebuf *vb);

extern uint8_t addr_method_from_char(char c);

extern char char_from_addr_method(uint8_t m);

#endif
