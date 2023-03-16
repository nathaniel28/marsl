#ifndef TYPES_H
#define TYPES_H

#define CORESIZE 8
#define MAXPROCESSES 16

#define byte unsigned char
#define uint unsigned int

typedef struct Cell *(*address_mode)(struct Cell *, uint);

typedef void (*operation)();

typedef struct {
	address_mode addr;
	uint val;
} Field;

typedef struct Cell {
	Field fields[2];
	operation op;
} Cell;

#define AFIELD(cell) (cell->fields[0])
#define BFIELD(cell) (cell->fields[1])

typedef struct {
	Cell *source;
	uint ninstrs;
	uint proc_queue[MAXPROCESSES];
	uint nprocs;
	uint cur_proc;
} Program;

typedef struct {
	Program *caller;
	Cell *src, *dst;
} State;

extern Cell core[CORESIZE]; // defined in types.c

// bad practice ahead!

extern State state; // defined in types.c

extern const address_mode default_addr_mode; // defined in address.c

extern const operation default_op; // defined in ops.c

#endif
