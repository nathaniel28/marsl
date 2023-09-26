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
			{ 0, default_addr_mode },
			{ 0, default_addr_mode },
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

#define WRAP(u, delta) \
do { \
	u = (int) (u + delta) % CORESIZE; \
	if ((int) u < 0) \
		u += CORESIZE; \
} while (0);

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

	// Order of operations:
	// NOTE: Steps 1-3 must not modify the core.
	// NOTE: 1-2 can cause changes through pre/post incriments/decriments.
	// 1. Resolve the a-field of the current instruction. NOTE: Changes MAY
	// influence b-field resolution the next step.
	// 2. Resolve the b-field of the current instruction.
	// 3. Using the source target resolved in step 1 and the destination
	// target resolved in step 2, execute the current instruction.
	// 4. Repeat steps 1-3 for each program with processes left.
	// 5. Write all buffered data to the core. I don't know what happenes if
	// two programs modify the same cell on the same frame.

	// whatever this operation is gonna do, place the result(s) in this
	// program's buffer instead of modifying the core directly

	p->src_fbuf.store = NULL;
	p->dst_fbuf.store = NULL;

	state.fbuf = &p->src_fbuf;
	state.src = resolve_field(self, &AFIELD(self));

	// TODO me, very soon: it seems that pmars resolves the first field,
	// then USING THAT RESULT (if targeted) resolves the second field.

	state.fbuf = &p->dst_fbuf;
	p->dst_cbuf.store = resolve_field(self, &BFIELD(self));
	p->dst_cbuf.buffer = *p->dst_cbuf.store;
	state.dst = &p->dst_cbuf.buffer;

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

void run(Program *start) {
	// simulate each program
	Program *p = start;
	while (p) {
		step(p);
		p = p->next;
	}

	// write the results back to the core
	// TODO: what is supposed to happen if two programs write to the same
	// cell? Currently, the last program wins the write.
	p = start;
	while (p) {
		*p->dst_cbuf.store = p->dst_cbuf.buffer;
		if (p->src_fbuf.store) {
			WRAP(*p->src_fbuf.store, p->src_fbuf.buffer);
		}
		if (p->dst_fbuf.store) {
			WRAP(*p->dst_fbuf.store, p->dst_fbuf.buffer);
		}
		p = p->next;
	}
}
