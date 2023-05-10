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
#define DB_PRINT_PQUEUE(p) for (unsigned i = 0; i < p->nprocs; i++) DB_PRINT("%u ", p->proc_queue[i])
#else
#define DB_PRINT(...)
#define DB_PRINT_CELL(self)
#define DB_PRINT_PQUEUE(p)
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

#define INCR_PROC(p) if (p->cur_proc < p->nprocs - 1) p->cur_proc++; else p->cur_proc = 0

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
		DB_PRINT_PQUEUE(p);
		DB_PRINT("], now [");

		p->nprocs--;
		if (p->nprocs == 0) {
			DB_PRINT("], %p terminated\n", p);
			fprintf(stderr, "program termination not yet implimented, so exiting\n");
			assert(0);
			// TODO: handle program termination
			return;
		}

		// TODO: check if this is handled properly
		uint *dst = &p->proc_queue[p->cur_proc];
		uint *src = dst + 1;
		memmove(dst, src, sizeof(uint) * (p->proc_queue + p->nprocs - dst));

		DB_PRINT_PQUEUE(p);
		DB_PRINT("]\n");
	} else {
		uint new_instr_offset = (state.ret_to - core) % CORESIZE;
		p->proc_queue[p->cur_proc] = new_instr_offset;

		if (state.ret_to != self + 1) {
			DB_PRINT(" (jumped to %d)", closest(new_instr_offset));
		}

		INCR_PROC(p);

		if (state.spl_to && p->nprocs < MAXPROCS) {
			uint spl_instr_offset = (state.spl_to - core) % CORESIZE;

			DB_PRINT(" (split to %d)", closest(spl_instr_offset));
			DB_PRINT("\nold queue [");
			DB_PRINT_PQUEUE(p);
			DB_PRINT("], now [");

			p->nprocs++;
			uint *src = &p->proc_queue[p->cur_proc];
			uint *dst = src + 1;
			memmove(dst, src, sizeof(uint) * (p->proc_queue + p->nprocs - dst));
			p->proc_queue[p->cur_proc] = spl_instr_offset;
			INCR_PROC(p);

			DB_PRINT_PQUEUE(p);
			DB_PRINT("]");
		}
		DB_PRINT("\n");
	}
}
