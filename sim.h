#ifndef SIM_H
#define SIM_H

#include "types.h"

extern void print_cell(Cell *);

extern void print_core();

extern void init_core();

// temporarily extern
extern void step(Program *);

#endif
