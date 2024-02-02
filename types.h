#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>

#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

#define CORESIZE 80
#define MAXPROCS 16
#define MAXPROGRAMLEN 30
//#define MINSEPERATION
//#define MAXCYCLES

// address modes
#define AM_IMMEDIATE 0
#define AM_DIRECT 1
#define AM_INDIRECT_A 2
#define AM_INDIRECT_B 3
#define AM_INDIRECT_A_PREDEC 4
#define AM_INDIRECT_B_PREDEC 5
#define AM_INDIRECT_A_POSTINC 6
#define AM_INDIRECT_B_POSTINC 7
#define AM_DEFAULT AM_DIRECT
#define AM_INVALID 255

// operations
#define OP_DAT 0
#define OP_MOV 1
#define OP_ADD 2
#define OP_SUB 3
#define OP_MUL 4
#define OP_DIV 5
#define OP_MOD 6
#define OP_JMP 7
#define OP_JMZ 8
#define OP_JMN 9
#define OP_DJN 10
#define OP_SEQ 11
#define OP_SNE 12
#define OP_SLT 13
#define OP_SPL 14
#define OP_NOP 15
#define OP_NB 16
#define OP_DEFAULT OP_DAT
#define OP_INVALID 255

// operation modes
#define OM_AA 0
#define OM_AB 1
#define OM_BA 2
#define OM_BB 3
#define OM_F 4
#define OM_X 5
#define OM_I 6
#define OM_DEFAULT OM_F
#define OM_INVALID 255

// a and b must be unsigned
#define ADDMOD(a, b) (a + b >= CORESIZE ? a + b - CORESIZE : a + b)
#define SUBMOD(a, b) (a - b >= CORESIZE ? a - b + CORESIZE : a - b)

typedef struct {
	uint32_t values[2];
	uint8_t addr_modes[2]; // see AM_* macros
	uint8_t operation; // see OP_* macros
	uint8_t op_mode; // see OM_* macros
} Cell;

#define AFIELD 0
#define BFIELD 1

typedef struct {
	Cell *store;
	Cell buffer;
} Cellbuf;

typedef struct {
	uint32_t *store;
	uint32_t buffer;
} Valuebuf;

typedef struct Program {
	Cell source_code[MAXPROGRAMLEN];
	int ninstrs;

	int proc_queue[MAXPROCS];
	int nprocs;
	int cur_proc;

	Cellbuf dst_cbuf;
	Valuebuf src_fbuf, dst_fbuf;

	struct Program *next; // who's turn it is next. TODO: impliment this
} Program;

// Set in types.c
extern Cell core[CORESIZE];

#endif
