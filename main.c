#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <dirent.h>

#include "address.h"
#include "parser.h"
#include "types.h"
#include "ops.h"
#include "sim.h"

/*
int test(Program *p, uint steps, Cell *answer) {
		assert(p->ninstrs < MAXPROGRAMLEN);
		//memset(p->proc_queue, 0, sizeof(uint)*(MAXPROCS-1));

		Cell *src = p->source_code, *dst = core;
		while (src < p->source_code + p->ninstrs) {
			*dst++ = *src++;
		}

		while (steps--) {
			step(p);
		}

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
	return 0;
}
*/

#define CMP_FIELDS(a, b) ((a).addr == (b).addr && (a).val == (b).val)

/*
 * tests should appear as such:
 * /the_dir_passed_to_run_tests
 * 	/example_test_name_0
 * 		params
 * 		question
 * 		answer
 * 	/example_test_name_1
 * 		params
 * 		question
 * 		answer
 * 	...
 */
// 0 == fail, 1 == success
int test(FILE *question, FILE *answer, FILE *params) {
	/*
	uint total = 0, passed = 0;
	struct dirent *entry;
	while ((entry = readdir(dir)) != NULL) {
		if (entry->d_type != DT_DIR || entry->d_name[0] == '.') {
			// skip anything not a directory as well as . and ..
			// for simplicity, just skip anything starting with .
			// collateral damage is acceptable
			continue;
		}
		FILE 
		printf("%s\n", entry->d_name);
	}
	*/
	Program q;
	static Cell a[CORESIZE];
	uint a_len;
	if (!parse(question, q.source_code, &q.ninstrs)) {
		printf("failed to parse question\n");
		return 0;
	}
	if (!parse(answer, a, &a_len)) {
		printf("failed to parse answer\n");
		return 0;
	}
	uint steps;
	if (fscanf(params, "%u", &steps) != 1) {
		printf("failed to read number of steps\n");
		return 0;
	}

	while (steps--) {
		step(&q);
	}

	for (int i = 0; i < a_len; i++) {
		if (
			core[i].op != a[i].op
			|| !CMP_FIELDS(core[i].fields[0], a[i].fields[0])
			|| !CMP_FIELDS(core[i].fields[1], a[i].fields[1])
		) {
			return 0;
		}
	}
	return 1;
}

int main() {
	FILE *question = fopen("tests/0/question", "r");
	FILE *answer = fopen("tests/0/answer", "r");
	FILE *params = fopen("tests/0/params", "r");
	if (!question || !answer || !params) {
		return -1; // who cares about closing, this is a test
	}
	int res = test(question, answer, params);
	printf("%d\n", res);
	fclose(question);
	fclose(answer);
	fclose(params);

	/*
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
	*/

	return 0;
}
