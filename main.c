#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include "address.h"
#include "types.h"
#include "ops.h"

int closest(uint u) {
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

Cell *resolve_field(Cell *cell, Field *field) {
	return field->addr(cell, field->val);
}

void step(Program *p) {
	assert(p->cur_proc < MAXPROCESSES && p->cur_proc < p->nprocs);
	uint instruction = p->proc_queue[p->cur_proc];
	assert(instruction < CORESIZE);
	Cell *self = core + instruction;

	printf("%p: ", p);
	print_cell(self);

	state.src = resolve_field(self, &AFIELD(self));
	state.dst = resolve_field(self, &BFIELD(self));
	state.kill_proc = 0;
	state.ret_to = self + 1;

	self->op();

	if (state.kill_proc) {
		printf(" [process killed, %u remaining]\n", p->nprocs);
		// TODO: handle
		/*
		state.caller->nprocs--;
		uint *dst = &state.caller->proc_queue[state.caller->cur_proc];
		uint *src = dst + 1;
		memmove(dst, src, sizeof(uint) * (state.caller->proc_queue + state.caller->nprocs - dst));
		if (state.caller->nprocs == 0) {
		        // TODO: game over
		}
		*/
	} else {
		uint *cur_instr_offset = &p->proc_queue[p->cur_proc];
		*cur_instr_offset = (state.ret_to - core) % CORESIZE;
		printf(" [next at %u]\n", *cur_instr_offset);
		p->cur_proc = (p->cur_proc + 1) % p->nprocs;
	}
}

char *incr_while(char *s, char c) {
	while (*s && *s == c) {
		s++;
	}
	return s;
}

char *incr_until(char *s, char c) {
	while (*s && *s != c) {
		s++;
	}
	return ++s;
}

int set_args(Cell *cell, int field, char **p) {
	char *pos = *p;
	pos = incr_while(pos, ' ');
	if (!*pos) return -1;
	cell->fields[field].addr = addr_method_from_char(*pos);
	pos++;
	if (!*pos || !cell->fields[field].addr) return -1; // temp
	const char *num_start = pos;
	pos = incr_until(pos, ' ');
	char *num_end = pos;
	long num = strtol(num_start, &num_end, 10);
	if (errno == ERANGE) return -1;
	cell->fields[field].val = (unsigned long) num % CORESIZE;
	//printf("%ld -> %u\n", num, cell->fields[field].val);
	*p = pos;
	return 0;
}

int set(Cell *cell, char *pos) {
	pos = incr_while(pos, ' ');
	const char *op_name = pos;
	pos = incr_until(pos, '.');
	if (!*pos || pos - op_name != 4) return -1; // temp
	const char *mode_name = pos;
	pos = incr_until(pos, ' ');
	if (!*pos || pos - mode_name == 0 || pos - mode_name > 2) return -1; // temp
	cell->op = op_from_name(op_name, mode_name);
	if (!cell->op) return -1; // ok I get the point now... temp
	if (set_args(cell, 0, &pos)) return -1;
	if (!*pos) return -1;
	if (set_args(cell, 1, &pos)) return -1;
	return 0;
}

#define S(cell, str) if (set(cell, str)) { printf("bad code '%s'\n", str); exit(-1); }

int main() {
	static Program p; // for zero init
	p.proc_queue[0] = 1;
	p.nprocs = 1;
	p.cur_proc = 0;

	init_core();
	S(core + 0, "dat.f #2 #0");
	S(core + 1, "mov.i {-1 {-1");
	S(core + 2, "jmp.a $-2 $0");
	print_core();
	printf("\n");

	uint steps = 3;
	while (steps--) {
		step(&p);
		print_core();
		printf("\n");
	}

	return 0;
}
