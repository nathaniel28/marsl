#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "address.h"
#include "types.h"
#include "ops.h"
#include "sim.h"

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

#define OP(id, mode_id, a_addr, a, b_addr, b) \
	(Cell) { \
		.op = op_registry[id].modes[mode_id], \
		.fields = { \
			(Field) {.addr = addr_method_from_char(a_addr), .val = a}, \
			(Field) {.addr = addr_method_from_char(b_addr), .val = b} \
		} \
	}

// that's a cast, not a parameter
#define END (Cell) { .op = NULL }

uint cnt_instrs(Cell *prog) {
	uint res = 0;
	while ((prog++)->op) res++;
	return res;
}

void test() {
	// yeah, programs sorta being null terminated is bad, but the array
	// degrades to a pointer within the struct. Just for testing, so
	// not too bad?
	const struct {
		Cell *input;
		Cell *answer;
		uint steps;
	} tests[] = {
		{
			(Cell []) {
				OP(MOV, M_I, '$', 0, '$', 1),
				END
			},
			(Cell []) {
				OP(MOV, M_I, '$', 0, '$', 1),
				OP(MOV, M_I, '$', 0, '$', 1),
				OP(MOV, M_I, '$', 0, '$', 1),
				END
			},
			3
		},
	};

	for (int i = 0; i < sizeof tests/sizeof *tests; i++) {
		init_core();
		Program p = {
			.source_code = tests[i].input,
			.ninstrs = cnt_instrs(tests[i].input),
			.nprocs = 1,
			.cur_proc = 0,
		};
		assert(p.ninstrs < MAXPROGRAMLEN);
		memset(p.proc_queue, 0, sizeof(uint)*(MAXPROCS-1));

		for (uint l = 0; l < p.ninstrs; l++) {
			*(core + l) = *(tests[i].input + l);
		}

		uint steps = tests[i].steps;
		while (steps--) {
			step(&p);
		}

		char *result = NULL;
		Cell *expected = tests[i].answer;
		for (uint pos = 0; expected[pos].op; pos++) {
			if (pos >= CORESIZE) {
				printf("uh oh\n");
				exit(1);
			}
			Cell *a = &expected[pos], *b = &core[pos];
			if (
				a->op != b->op ||
				a->fields[0].addr != b->fields[0].addr ||
				a->fields[0].val != b->fields[0].val ||
				a->fields[1].addr != b->fields[1].addr ||
				a->fields[1].val != b->fields[1].val
			) {
				result = "failed";
				break;
			}
		}
		if (!result) {
			result = "passed";
		}
		
		printf("%d: len %u, %u steps, %s\n", i, p.ninstrs, tests[i].steps, result); 
	}
}

#define S(cell, str) if (set(cell, str)) { printf("bad code '%s'\n", str); exit(-1); }

int main() {
	/*
	static Program p; // for zero init
	p.proc_queue[0] = 0;
	p.nprocs = 1;
	p.cur_proc = 0;

	init_core();
	S(core + 0, "spl.a $1 #0");
	S(core + 1, "spl.a $1 #0");
	S(core + 2, "jmp.a $0 #0");
	print_core();
	printf("\n");

	uint steps = 10;
	while (steps--) {
		step(&p);
		print_core();
		printf("\n");
	}
	*/

	test();

	return 0;
}
