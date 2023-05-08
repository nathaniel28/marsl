#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "parser.h"
#include "types.h"
#include "sim.h"

// a tester returns 1 on success, and 0 on failure.
// if a tester prints anything, it does so to stderr.
typedef int (*tester)(int dir_fd);

void run_tests(const char *path, tester test_fn) {
	DIR *dir = opendir(path);
	int dir_fd = dirfd(dir);
	if (!dir) {
		perror("opendir():");
		return;
	}
	errno = 0;
	int passed = 0, total = 0;
	struct dirent *entry;
	while ((entry = readdir(dir)) != NULL) {
		if (entry->d_type != DT_DIR || entry->d_name[0] == '.')
			continue;

		int test_dir = openat(dir_fd, entry->d_name, O_RDONLY);
		if (test_dir == -1) {
			perror("openat():");
			errno = 0;
		} else {
			int res = test_fn(test_dir);
			const char *fmt = "failed test %s\n";
			if (res) {
				fmt = "passed test %s\n";
				passed++;
			}
			fprintf(stderr, fmt, entry->d_name);
			close(test_dir);
			total++;
		}
	}
	if (errno)
		perror("readdir()");

	fprintf(stderr, "results: %d/%d\n", passed, total);

	closedir(dir);
}

FILE *rdonly_fopenat(int dir_fd, const char *path) {
	int fd = openat(dir_fd, path, O_RDONLY);
	if (fd == -1)
		return NULL;

	return fdopen(fd, "r");
}

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
int test_using(FILE *question, FILE *answer, FILE *params) {
	// TODO: currently, the parser uses a buffer, the program uses a
	// buffer, and finally the core is a buffer... too many buffers!
	// we only need one for the compiled answer and the core. Something
	// to improve upon, eventually.
	static Program q;
	q.proc_queue[0] = 0;
	q.nprocs = 1;
	static Cell ans[CORESIZE];
	uint ans_len;
	if (parse(question, q.source_code, &q.ninstrs)) {
		fprintf(stderr, "failed to parse question\n");
		return 0;
	}
	if (parse(answer, ans, &ans_len)) {
		fprintf(stderr, "failed to parse answer\n");
		return 0;
	}
	uint steps;
	if (fscanf(params, "%u", &steps) != 1) {
		fprintf(stderr, "failed to read number of steps\n");
		return 0;
	}

	init_core();
	memcpy(core, q.source_code, sizeof(q.source_code[0]) * q.ninstrs);
	while (steps--) {
		step(&q);
	}

	for (uint i = 0; i < ans_len; i++) {
		if (
			core[i].op != ans[i].op
			|| !CMP_FIELDS(core[i].fields[0], ans[i].fields[0])
			|| !CMP_FIELDS(core[i].fields[1], ans[i].fields[1])
		) {
			fprintf(stderr, "discrepancy in cell %u:\n", i);
			// reason for mixing stdout and stderr: print_cell
			// prints to stdout, but perror (possibly called by
			// run_tests) prints to stderr. It sucks, but to
			// make sure things print in the right order, just
			// call fflush(stdout) after.
			fprintf(stdout, "expected ");
			print_cell(&ans[i]);
			fprintf(stdout, "\n   found ");
			print_cell(&core[i]);
			fprintf(stdout, "\n");
			fflush(stdout);
			return 0;
		}
	}
	return 1;
}

int test_callback(int dir_fd) {
	int res = 0;

	FILE *question = rdonly_fopenat(dir_fd, "question");
	if (!question) {
		fprintf(stderr, "failed to open question\n");
		return 0;
	}
	FILE *answer = rdonly_fopenat(dir_fd, "answer");
	if (!answer) {
		fprintf(stderr, "failed to open answer\n");
		goto no_open_ans;
	}
	FILE *params = rdonly_fopenat(dir_fd, "params");
	if (!params) {
		fprintf(stderr, "failed to open params\n");
		goto no_open_param;
	}

	res = test_using(question, answer, params);

	fclose(params);
no_open_param:
	fclose(answer);
no_open_ans:
	fclose(question);

	return res;
}

int main() {
	run_tests("tests", test_callback);

	return 0;
}
