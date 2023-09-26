#ifndef SIM_H
#define SIM_H

#include "types.h"

extern void print_cell(Cell *);

extern void print_core();

extern void init_core();

// temporarily extern, eventually should be replaced with something like
// void run_match(Program *competitors, size_t cnt)
// extern void step(Program *);

extern void run(Program *);

#endif
