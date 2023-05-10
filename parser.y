%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "types.h"
#include "address.h"
#include "parser.h"
#include "ops.h"

typedef struct {
	address_mode addr;
	char *label;
	uint val;
} pfield;

typedef struct {
	char *label;
	enum op_id op;
	//enum mode_id mode;
	operation resolved_op;
	pfield fields[2];
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
		pstate.active_field->addr = v; \
		pstate.active_field++; \
	} while (0);


//int yydebug = 1;

int yylex();
void yyerror(char const *);
%}

%define api.value.type union
%token <uint> NUM
%token <char *> STR
%token <enum op_id> OP

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

// using functions defined in address.h
field: '#' location { SET_FIELD(addr_immediate); }
     | '$' location { SET_FIELD(addr_direct); }
     | '*' location { SET_FIELD(addr_a_indirect); }
     | '@' location { SET_FIELD(addr_b_indirect); }
     | '}' location { SET_FIELD(addr_a_indirect_postinc); }
     | '{' location { SET_FIELD(addr_a_indirect_predec); }
     | '>' location { SET_FIELD(addr_b_indirect_postinc); }
     | '<' location { SET_FIELD(addr_b_indirect_predec); }
     ;

location: NUM { pstate.active_field->label = NULL; pstate.active_field->val = $1; }
	| STR { pstate.active_field->label = $1; /* val set later when label is resolved */ }
	;

// prefix or prefix <mode>
operation: prefix { pstate.current->resolved_op = op_registry[pstate.current->op].default_mode; }
	 | prefix '.' STR {
	enum mode_id m = mode_from_name($3);
	free($3);
	if (m == M_INVALID) {
		fprintf(stderr, "invalid addressing mode\n");
		YYABORT;
	}
	pstate.current->resolved_op = op_registry[pstate.current->op].modes[m];
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
			for (uint i = 0; i < OP_NB; i++) {
				// again, names of ops are 3 chars long
				if (!memcmp(buf, op_registry[i].name, 3)) {
//printf("operation %s\n", buf);
					free(buf);
					yylval.OP = i;
					return OP;
				}
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

int parse(FILE *fp, Cell *buf, uint *len) {
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

			buf->op = l->resolved_op;
			buf->fields[0].addr = l->fields[0].addr;
			buf->fields[0].val = l->fields[0].val;
			buf->fields[1].addr = l->fields[1].addr;
			buf->fields[1].val = l->fields[1].val;

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
