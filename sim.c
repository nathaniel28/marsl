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

// Crazy idea: instead of calling the functions at Cell->fields[*].addr and
// Cell->op, jump to them. No need to create a new stack frame, and they have
// no local variables and I've never done anything like that before and it
// probably won't work for some technical reason I'm unaware of, but maybe it
// can be done with some inline assembly. TODO, perhaps.
void step(Program *p) {
	// TODO: buffer source, destination, and self cells
	assert(p->cur_proc < MAXPROCS && p->cur_proc < p->nprocs);
	uint instruction = p->proc_queue[p->cur_proc];
	assert(instruction < CORESIZE);
	Cell *self = core + instruction;
	assert(self->op && AFIELD(self).addr && BFIELD(self).addr);

	printf("%p (%u@%lu/%u): ", p, p->cur_proc + 1, self - core, p->nprocs);
	print_cell(self);

	state.src = resolve_field(self, &AFIELD(self));
	state.dst = resolve_field(self, &BFIELD(self));
	state.kill_proc = 0;
	state.ret_to = self + 1;
	state.spl_to = NULL;

	self->op();

	if (state.kill_proc) {
		printf("\n");
		for (unsigned i = 0; i < p->nprocs; i++) {
			printf("%u\n", p->proc_queue[i]);
		}
		// TODO: check if this is handled properly
		p->nprocs--;
		printf(" [process killed, %u remaining]\n", p->nprocs);
		if (p->nprocs == 0) {
			// TODO: handle program termination
		}
		uint *dst = &p->proc_queue[p->cur_proc];
		uint *src = dst + 1;
		memmove(dst, src, sizeof(uint) * (p->proc_queue + p->nprocs - dst));
		for (unsigned i = 0; i < p->nprocs; i++) {
			printf("%u\n", p->proc_queue[i]);
		}
	} else {
		uint *cur_instr_offset = &p->proc_queue[p->cur_proc];
		*cur_instr_offset = (state.ret_to - core) % CORESIZE;
		printf(" [next at %u", *cur_instr_offset);
		p->cur_proc = (p->cur_proc + 1) % p->nprocs;
		if (state.spl_to && p->nprocs < MAXPROCS) {
			p->proc_queue[p->nprocs] = (state.spl_to - core) % CORESIZE;
			printf(", split at %u", p->proc_queue[p->nprocs]);
			p->nprocs++;
		}
		printf("]\n");
	}
}
