#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

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

#define S(cell, str) if (set(cell, str)) { printf("bad code '%s'\n", str); exit(-1); }

int main() {
	static Program p; // for zero init
	p.proc_queue[0] = 0;
	p.nprocs = 1;
	p.cur_proc = 0;

	init_core();
	//S(core + 0, "mov.i $0 $1");
	/*
	S(core + 0, "dat.f #2 #0");
	S(core + 1, "mov.i {-1 {-1");
	S(core + 2, "jmp.a $-2 $0");
	*/
	/*
	S(core + 0, "spl.a $2 #0");
	S(core + 1, "jmp.a $0 $0");
	S(core + 2, "mov.i $1 $-1");
	S(core + 3, "jmp.a $1 $0");
	S(core + 4, "jmp.a $0 $0");
	*/
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

	return 0;
}
