#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "address.h"
#include "parser.h"
#include "types.h"
#include "ops.h"
#include "sim.h"

/*
void test() {
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
*/

int main() {
	FILE *src = fopen("prog.red", "r");
	if (!src) {
		printf("fopen failed\n");
		return -1;
	}

	Program p;
	memset(p.proc_queue + 1, 0, sizeof(p.proc_queue) - 1);
	p.proc_queue[0] = 0;
	p.cur_proc = 0;
	p.nprocs = 1;

	int err = parse(src, p.source_code, &p.ninstrs);
	fclose(src);
	if (err) {
		printf("failed to build");
		return err;
	}

	init_core();
	memcpy(core, p.source_code, sizeof(Cell) * p.ninstrs);
	print_core();

	for (int i = 0; i < 10; i++) {
		step(&p);
	}

	print_core();

	return 0;
}
