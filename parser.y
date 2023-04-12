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
	unsigned val;
} pfield;

typedef struct {
	char *label;
	enum op_id op;
	//enum mode_id mode;
	operation resolved_op;
	pfield fields[2];
} line;

// parse state, sorta using this like a namespace thing, container for globals
// immaculate, flawless, sparklingly clean practice right here
struct {
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

void set_field(address_mode v) {
	if (pstate.active_field >= &pstate.current->fields[2]) {
		printf("TODO: turn this into an error instead of a program termination. anyway, more fields found than needed\n");
		exit(-1);
	}
	pstate.active_field->addr = v;
	pstate.active_field++;
}

int yydebug = 1; // I'm debugging now

int yylex();
void yyerror(char const *);
%}

%define api.value.type union
%token <unsigned> NUM
%token <char *> STR
%token <enum op_id> OP

%%

// I'm not really sure how to indent for this section

line: '\n'
    | line '\n'
    | operation field field '\n' {
	if (pstate.current - pstate.lines >= MAXPROGRAMLEN) {
		// TODO: I'm a bit worried about this section. Is it
		// off-by-one? Is pstate.current the next active element
		// (as it should (and better) be) or the final finished
		// element? I'll need to test that.
		fprintf(stderr, "program is too long (max length: %u instructions)\n", MAXPROGRAMLEN);
		YYABORT;
	}
	pstate.current++;
	pstate.active_field = &pstate.current->fields[0];
}
    ;

// using functions defined in address.h
field: '#' location { set_field(addr_immediate); }
     | '$' location { set_field(addr_direct); }
     | '*' location { set_field(addr_a_indirect); }
     | '@' location { set_field(addr_b_indirect); }
     | '}' location { set_field(addr_a_indirect_postinc); }
     | '{' location { set_field(addr_a_indirect_predec); }
     | '>' location { set_field(addr_b_indirect_postinc); }
     | '<' location { set_field(addr_b_indirect_predec); }
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
			for (unsigned i = 0; i < OP_NB; i++) {
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
				f->val = offset; // is this right?
				return 1;
			}
			l++;
		}
		return 0;
	}
	return 1;
}

int parse(FILE *fp, Cell *buf) {
	init_pstate(fp);
	int err = yyparse();

	line *l;

	if (!err) {
		// the following is worst-case order n^2 because of how labels
		// are resolved. This may be bad; n log n could be achieved if
		// labels were kept in a sorted array. But is it worth the
		// trouble?
		l = &pstate.lines[0];
		while (l < pstate.current) {
			if (!resolve_label(l, &l->fields[0]) || !resolve_label(l, &l->fields[1])) {
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
	}

	l = &pstate.lines[0];
	while (l < pstate.current) {
		free(l->label); // something or NULL, so OK to free
		free(l->fields[0].label);
		free(l->fields[1].label);
		l++;
	}

	return err;
}
