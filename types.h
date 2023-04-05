#ifndef TYPES_H
#define TYPES_H

#define CORESIZE 80
#define MAXPROCS 16
#define MAXPROGRAMLEN 20
//#define MINSEPERATION
//#define MAXCYCLES

#define byte unsigned char
#define uint unsigned int

typedef struct Cell *(*address_mode)(struct Cell *, uint);

typedef void (*operation)();

typedef struct {
	// see https://corewar-docs.readthedocs.io/en/latest/redcode/addressing_modes
	// and see address.c for the implimentation
	address_mode addr;
	uint val;
} Field;

typedef struct Cell {
	// the A field is fields[0]
	// the B field is fields[1]
	// ...see below macros AFIELD and BFIELD
	Field fields[2];
	operation op;
} Cell;

#define AFIELD(cell) (cell->fields[0])
#define BFIELD(cell) (cell->fields[1])

typedef struct Program {
	Cell *source_code;
	uint ninstrs;
	uint proc_queue[MAXPROCS];
	uint nprocs;
	uint cur_proc;
	/*
	Cell *procs[MAXPROCS];
	uint nprocs;
	Cell *cur_proc;
	*/
	// buffer so that multiple programs don't read output
	// from another program on the same turn
	Cell task, src, dst;
	struct Program *next; // who's turn it is next
} Program;

typedef struct {
	// src and dst store the operands for the current instruction after being
	// deduced by Cell->addr. 
	Cell *src, *dst;

	// kill_proc is set to 0 by default-- operations do NOT need to set it if
	// the process should NOT be killed. When set to a nonzero value, the
	// program's current process will be terminated.
	byte kill_proc;

	// ret_to defaults to 1 cell after the current instruction-- operations do
	// NOT need to set it unless the operation is some form of jump (jmp, seq...)
	// The current process's next instruction will be that of ret_to (wraps
	// around to be something from core to core[CORESIZE - 1]
	Cell *ret_to;

	Cell *spl_to;
} State;

// Set in types.c
extern Cell core[CORESIZE];

// bad practice ahead! ...

// used by operations to store their arguments. Set in types.c
extern State state;

// Set in address.c
extern const address_mode default_addr_mode;

// Set in ops.c
extern const operation default_op;

#endif
