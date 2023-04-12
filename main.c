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
	/*
	uint steps = 10;
	while (steps--) {
		step(&p);
		print_core();
		printf("\n");
	}
	*/

	init_core();
	FILE *src = fopen("prog.red", "r");
	if (!src) {
		printf("fopen failed\n");
		return -1;
	}

	int res = parse(src, core);
	fclose(src);
	print_core();
	printf("%d\n", res);

	return 0;
}
