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

int main() {
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
