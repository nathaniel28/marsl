#ifndef OPS_H
#define OPS_H

#include "types.h"

// the order of op_id better match the registry struct defined in ops.c...
enum op_id { MOV, DAT, ADD, SUB, MUL, DIV, MOD, JMP, JMZ, JMN, DJN, SEQ, SNE, SLT, SPL, NOP, OP_NB, OP_INVALID };
enum mode_id { M_A, M_B, M_AB, M_BA, M_F, M_X, M_I, M_NB, M_INVALID };

typedef struct {
	char name[3];
	operation modes[M_NB];
	operation default_mode;
} named_op;

extern const named_op op_registry[OP_NB];

extern operation op_from_name(const char *, const char *);

extern const char *name_from_op(operation);

extern enum mode_id mode_from_name(const char *);

#endif
