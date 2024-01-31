%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "types.h"
#include "address.h"
#include "parser.h"
#include "ops.h"

typedef struct {
	char *label;
	uint32_t val;
	uint8_t addr_mode;
} pfield;

typedef struct {
	char *label;
	pfield fields[2];
	uint8_t op;
	uint8_t op_mode;
} line;

// parse state, sorta using this like a namespace thing, container for globals
static struct {
	line lines[MAXPROGRAMLEN];
	line *current;
	pfield *active_field;
	FILE *file;
} pstate = {};

void init_pstate(FILE *fp) {
	memset(pstate.lines, 0, sizeof pstate.lines);
	pstate.current = &pstate.lines[0];
	pstate.active_field = &pstate.current->fields[0];
	pstate.file = fp;
}

#define SET_FIELD(v) do { \
		if (pstate.active_field >= &pstate.current->fields[2]) { \
			fprintf(stderr, "instruction has too many fields\n"); \
			YYABORT; \
		} \
		pstate.active_field->addr_mode = v; \
		pstate.active_field++; \
	} while (0);


//int yydebug = 1;

int yylex();
void yyerror(char const *);
%}

%define api.value.type union
%token <uint32_t> NUM
%token <char *> STR
%token <uint8_t> OP

%%

// I'm not really sure how to indent for this section

line: '\n' { /* do nothing */ }
    | line '\n' { /* do nothing */ }
    | operation field ',' field '\n' {
	if (++pstate.current >= pstate.lines + MAXPROGRAMLEN) {
		fprintf(stderr, "program is too long (max length: %u instructions)\n", MAXPROGRAMLEN);
		YYABORT;
	}
	pstate.active_field = &pstate.current->fields[0];
}
    ;

// macros defined in types.h
field: '#' location { SET_FIELD(AM_IMMEDIATE); }
     | '$' location { SET_FIELD(AM_DIRECT); }
     | '*' location { SET_FIELD(AM_INDIRECT_A); }
     | '@' location { SET_FIELD(AM_INDIRECT_B); }
     | '{' location { SET_FIELD(AM_INDIRECT_A_PREDEC); }
     | '<' location { SET_FIELD(AM_INDIRECT_B_PREDEC); }
     | '}' location { SET_FIELD(AM_INDIRECT_A_POSTINC); }
     | '>' location { SET_FIELD(AM_INDIRECT_A_POSTINC); }
     ;

location: NUM { pstate.active_field->label = NULL; pstate.active_field->val = $1; }
	| STR { pstate.active_field->label = $1; /* val set later when label is resolved */ }
	;

// prefix or prefix <mode>
operation: prefix { pstate.current->op_mode = default_op_mode(pstate.current->op); }
	 | prefix '.' STR {
	uint8_t m = mode_from_name($3);
	free($3);
	if (m == OM_INVALID) {
		fprintf(stderr, "invalid addressing mode\n");
		YYABORT;
	}
	pstate.current->op_mode = m;
}
	 ;

// <op> or <label> <op>
// note: line is a valid preceeding token because I can't figure out how to
// pop a line from the stack after it gets parsed...
prefix: OP { pstate.current->label = NULL; pstate.current->op = $1; }
      | STR OP { pstate.current->label = $1; pstate.current->op = $2; }
      | line OP { pstate.current->label = NULL; pstate.current->op = $2; }
      | line STR OP { pstate.current->label = $2; pstate.current->op = $3; }
      ;

%%

#include <ctype.h>
#include <stdlib.h> // TODO, NOTE, >1 imports of this
#include <string.h> // TODO, NOTE, >1 imports of this too

int yylex() {
	int c = fgetc(pstate.file);

	// Skip white space.
	while (c == ' ' || c == '\t') {
		c = fgetc(pstate.file);
	}

	// Skip comments.
	if (c == ';') {
		do {
			c = fgetc(pstate.file);
		} while (c != '\n');
//printf("comment\n");
		return c; // c is always '\n' here
	}

	// Process numbers.
	if (isdigit(c) || c == '-') {
		ungetc(c, pstate.file);
		long n;
		if (fscanf(pstate.file, "%lu", &n) != 1) {
			fprintf(stderr, "fscanf failed to match long integer\n");
			abort();
		}
		// in C, -1 % 5 (both signed) == -1, NOT 4, as % is the
		// remainder operator, not a modulo operator. Thus, a slight
		// workaround is neccesary to make sure the result is the
		// correct positive version.
		n = n % CORESIZE;
		if (n < 0) {
			n += CORESIZE;
		}
		yylval.NUM = n;
//printf("number %ld (%u)\n", n, yylval.NUM);
		return NUM;
	}

	if (isalpha(c) || c == '_') {
		char *buf = NULL;
		size_t len = 0;
		size_t pos = 0;
		do {
			if (len <= pos) {
				len = 2 * len + 8;
				char *realloced = realloc(buf, len);
				if (!realloced) {
					fprintf(stderr, "failed to resize string buffer, out of memory\n");
					free(buf);
					abort();
				}
				buf = realloced;
			}
			buf[pos] = tolower(c);
			pos++;
			c = fgetc(pstate.file);
		} while (isalnum(c) || c == '_');

		ungetc(c, pstate.file);
		buf[pos] = '\0';
		if (pos == 3) { // names of ops are 3 chars long
			uint8_t op = op_from_name(buf);
			if (op != OP_INVALID) {
				free(buf);
				yylval.OP = op;
				return OP;
			}
		}

//printf("string \"%s\"\n", buf);
		yylval.STR = buf;
		return STR;
	}

	if (c == EOF) {
		// Return end-of-input.
//printf("EOF\n");
		return YYEOF;
	}

//printf("char '%c'\n", c);
	// Return a single char.
	return c;
}

// Called by yyparse on error.
void yyerror(char const *s) {
	fprintf(stderr, "%s\n", s);
}

// 1 is success, 0 is fail
int resolve_label(line *here, pfield *f) {
	if (f->label) {
		line *l = &pstate.lines[0];
		while (l < pstate.current) {
			if (l->label && !strcmp(f->label, l->label)) {
				int offset = (l - here) % CORESIZE;
				if (offset < 0) {
					offset += CORESIZE;
				}
				f->val = offset;
				return 1;
			}
			l++;
		}
		return 0;
	}
	return 1;
}

// fp is the open file to parse from;
// the parsed program will be written to buf, which is of size MAXPROGRAMLEN
// len is the location to set the length of the parsed program
int parse(FILE *fp, Cell *buf, int *len) {
	init_pstate(fp);

	// The parser fails on an empty input, but an empty input should produce
	// a zero-length output
	int c = fgetc(fp);
	if (c == EOF) {
		*len = 0;
		return 0;
	}
	ungetc(c, fp);

	int err = yyparse();

	line *l;

	if (!err) {
		l = &pstate.lines[0];
		while (l < pstate.current) {
			if (!resolve_label(l, &l->fields[0]) || !resolve_label(l, &l->fields[1])) {
				fprintf(stderr, "failed to resolve label\n");
				err = -1;
				break;
			}

			buf->operation = l->op;
			buf->values[0] = l->fields[0].val;
			buf->addr_modes[0] = l->fields[0].addr_mode;
			buf->values[1] = l->fields[1].val;
			buf->addr_modes[1] = l->fields[1].addr_mode;

			buf++;
			l++;
		}
		*len = l - pstate.lines;
	}

	l = &pstate.lines[0];
	while (l < pstate.current) {
		free(l->label); // something or NULL, so OK to free
		free(l->fields[0].label); // likewise
		free(l->fields[1].label); // likewise
		l++;
	}

	return err;
}
