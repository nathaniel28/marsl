#ifndef SIM_H
#define SIM_H

#include "types.h"

extern void print_cell(Cell *);

extern void print_core();

extern void init_core();

// Step the simulation forward
// Programs are in a linked list
extern void run(Program *);

// Load each program into the core obeying MINSEPERATION
// Programs are in a linked list
// extern void load(Program *);

#endif
