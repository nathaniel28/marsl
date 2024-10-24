#include <stdio.h>
#include <unistd.h>

#include "parser.h"
#include "types.h"
#include "sim.h"

int main(int argc, char **argv) {
	if (argc != 2) {
		printf("Specify one program.\n");
		return 1;
	}

	init_core();

	FILE *fp = fopen(argv[1], "r");
	if (!fp) {
		printf("Failed to open %s.\n", argv[1]);
		return 1;
	}
	int len;
	int err = parse(fp, core, &len);
	fclose(fp);
	if (err) {
		printf("Failed to parse %s.\n", argv[1]);
		return 1;
	}

	write(STDOUT_FILENO, core, len * sizeof(Cell));

	return 0;
}
