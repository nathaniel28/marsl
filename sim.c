#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "address.h"
#include "ops.h"
#include "sim.h"

static int closest(uint u) {
	int r1 = u % CORESIZE;
	int r2 = r1 - CORESIZE;
	return -r2 < r1 ? r2 : r1;
}

void print_cell(Cell *c) {
	printf(
		"%s %c%d, %c%d",
		name_from_op(c->op),
		char_from_addr_method(AFIELD(c).addr),
		closest(AFIELD(c).val),
		char_from_addr_method(BFIELD(c).addr),
		closest(BFIELD(c).val)
	);
}

#if 1
#define DB_PRINT(...) fprintf(stderr, __VA_ARGS__)
#define DB_PRINT_CELL(self) print_cell(self); fflush(stdout)
#else
#define DB_PRINT(...)
#define DB_PRINT_CELL(self)
#endif

void print_core() {
	for (Cell *cur = core; cur < core + CORESIZE; cur++) {
		printf("%4lu ", cur - core);
		print_cell(cur);
		printf("\n");
	}
}

void init_core() {
	const Cell default_cell = {
		.fields = {
			{ default_addr_mode, 0 },
			{ default_addr_mode, 0 },
		},
		.op = default_op,
	};
	for (Cell *cur = core; cur < core + CORESIZE; cur++) {
		*cur = default_cell;
	}
}

static Cell *resolve_field(Cell *cell, Field *field) {
	return field->addr(cell, field->val);
}

void step(Program *p) {
	// TODO: buffer source, destination, and self cells
	assert(p->nprocs < MAXPROCS && p->cur_proc < p->nprocs);
	uint instruction = p->proc_queue[p->cur_proc];
	assert(instruction < CORESIZE);
	Cell *self = core + instruction;
	assert(self->op && AFIELD(self).addr && BFIELD(self).addr);

	DB_PRINT("%p (%u@%lu/%u): ", p, p->cur_proc + 1, self - core, p->nprocs);
	DB_PRINT_CELL(self);

	state.src = resolve_field(self, &AFIELD(self));
	state.dst = resolve_field(self, &BFIELD(self));
	state.kill_proc = 0;
	state.ret_to = self + 1;
	state.spl_to = NULL;

	self->op();

	if (state.kill_proc) {
		DB_PRINT(" (process killed)");
		DB_PRINT("\nold queue [");
		for (unsigned i = 0; i < p->nprocs; i++) {
			DB_PRINT("%u ", p->proc_queue[i]);
		}
		DB_PRINT("], now [");

		p->nprocs--;
		if (p->nprocs == 0) {
			DB_PRINT("], %p terminated\n", p);
			// TODO: handle program termination
			return;
		}

		// TODO: check if this is handled properly
		uint *dst = &p->proc_queue[p->cur_proc];
		uint *src = dst + 1;
		memmove(dst, src, sizeof(uint) * (p->proc_queue + p->nprocs - dst));

		for (unsigned i = 0; i < p->nprocs; i++) {
			DB_PRINT("%u ", p->proc_queue[i]);
		}
		DB_PRINT("]\n");
	} else {
		uint *cur_instr_offset = &p->proc_queue[p->cur_proc];
		*cur_instr_offset = (state.ret_to - core) % CORESIZE;
		if (state.ret_to != self + 1) {
			DB_PRINT(" (jumped to %d)", closest(*cur_instr_offset));
		}

		if (state.spl_to && p->nprocs < MAXPROCS) {
			p->proc_queue[p->nprocs] = (state.spl_to - core) % CORESIZE;
			DB_PRINT(" (split at %u)", p->proc_queue[p->nprocs]);
			p->nprocs++;
		}
		DB_PRINT("\n");
	}
	p->cur_proc = (p->cur_proc + 1) % p->nprocs;
}
