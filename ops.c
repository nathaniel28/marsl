#include <stdlib.h>
#include <string.h>

#include "ops.h"
#include "types.h"

void mov(Cell *src, Cell *dst, uint8_t mode) {
	switch (mode) {
	case OM_AA:
		dst->values[AFIELD] = src->values[AFIELD];
		return;
	case OM_AB:
		dst->values[BFIELD] = src->values[AFIELD];
		return;
	case OM_BA:
		dst->values[AFIELD] = src->values[BFIELD];
		return;
	case OM_BB:
		dst->values[BFIELD] = src->values[BFIELD];
		return;
	case OM_F:
		dst->values[AFIELD] = src->values[AFIELD];
		dst->values[BFIELD] = src->values[BFIELD];
		return;
	case OM_X:
		dst->values[AFIELD] = src->values[BFIELD];
		dst->values[BFIELD] = src->values[AFIELD];
		return;
	case OM_I:
		*dst = *src;
		return;
	default:
		// TODO: print error
		abort();
	}
}

#define GENERATE_MATH_OP(name, helper) \
	void name(Cell *src, Cell *dst, uint8_t mode) { \
		switch (mode) { \
		case OM_AA: \
			dst->values[AFIELD] = helper(src->values[AFIELD], dst->values[AFIELD]); \
			return; \
		case OM_AB: \
			dst->values[BFIELD] = helper(src->values[AFIELD], dst->values[BFIELD]); \
			return; \
		case OM_BA: \
			dst->values[AFIELD] = helper(src->values[BFIELD], dst->values[AFIELD]); \
			return; \
		case OM_BB: \
			dst->values[BFIELD] = helper(src->values[BFIELD], dst->values[BFIELD]); \
			return; \
		case OM_F: \
		case OM_I: \
			dst->values[AFIELD] = helper(src->values[AFIELD], dst->values[AFIELD]); \
			dst->values[BFIELD] = helper(src->values[BFIELD], dst->values[BFIELD]); \
			return; \
		case OM_X: \
			dst->values[AFIELD] = helper(src->values[BFIELD], dst->values[AFIELD]); \
			dst->values[BFIELD] = helper(src->values[AFIELD], dst->values[BFIELD]); \
			return; \
		default: \
			abort(); \
		} \
	}

uint32_t help_add(uint32_t a, uint32_t b) {
	uint32_t res = a + b;
	if (res >= CORESIZE)
		return res - CORESIZE;
	return res;
}
GENERATE_MATH_OP(add, help_add);

uint32_t help_sub(uint32_t a, uint32_t b) {
	int32_t res = a - b; // signed, converted to uint32_t during return
	if (res < 0)
		return res + CORESIZE;
	return res;
}
GENERATE_MATH_OP(sub, help_sub);

uint32_t help_mul(uint32_t a, uint32_t b) {
	return (a * b) % CORESIZE;
}
GENERATE_MATH_OP(mul, help_mul);

#define GENERATE_ZERO_FAIL_MATH_OP(name, helper) \
	_Bool name(Cell *src, Cell *dst, uint8_t mode) { \
		switch (mode) { \
		case OM_AA: \
			if (dst->values[AFIELD] == 0) \
				return 1; \
			dst->values[AFIELD] = helper(src->values[AFIELD], dst->values[AFIELD]); \
			return 0; \
		case OM_AB: \
			if (dst->values[BFIELD] == 0) \
				return 1; \
			dst->values[BFIELD] = helper(src->values[AFIELD], dst->values[BFIELD]); \
			return 0; \
		case OM_BA: \
			if (dst->values[AFIELD] == 0) \
				return 1; \
			dst->values[AFIELD] = helper(src->values[BFIELD], dst->values[AFIELD]); \
			return 0; \
		case OM_BB: \
			if (dst->values[BFIELD] == 0) \
				return 1; \
			dst->values[BFIELD] = helper(src->values[BFIELD], dst->values[BFIELD]); \
			return 0; \
		case OM_F: \
		case OM_I: \
			if (dst->values[AFIELD] == 0 || dst->values[BFIELD] == 0) \
				return 1; \
			dst->values[AFIELD] = helper(src->values[AFIELD], dst->values[AFIELD]); \
			dst->values[BFIELD] = helper(src->values[BFIELD], dst->values[BFIELD]); \
			return 0; \
		case OM_X: \
			if (dst->values[AFIELD] == 0 || dst->values[BFIELD] == 0) \
				return 1; \
			dst->values[AFIELD] = helper(src->values[BFIELD], dst->values[AFIELD]); \
			dst->values[BFIELD] = helper(src->values[AFIELD], dst->values[BFIELD]); \
			return 0; \
		default: \
			abort(); \
		} \
	}

uint32_t help_div(uint32_t a, uint32_t b) {
	return (a / b) % CORESIZE;
}
GENERATE_ZERO_FAIL_MATH_OP(div_, help_div);

uint32_t help_mod(uint32_t a, uint32_t b) {
	return (a % b) % CORESIZE;
}
GENERATE_ZERO_FAIL_MATH_OP(mod, help_mod);

#define GENERATE_CONDITIONAL_JMP(name, cond) \
	int name(int to, Cell *dst, uint8_t mode) { \
		_Bool res; \
		switch (mode) { \
		case OM_AA: \
		case OM_BA: \
			res = (dst->values[AFIELD] cond); \
			break; \
		case OM_BB: \
		case OM_AB: \
			res = (dst->values[BFIELD] cond); \
			break; \
		case OM_F: \
		case OM_X: \
		case OM_I: \
			res = (dst->values[AFIELD] cond && dst->values[BFIELD] cond); \
			break; \
		default: \
			abort(); \
		} \
		if (res) \
			return to; \
		return 1; \
	}

GENERATE_CONDITIONAL_JMP(jmz, == 0);

GENERATE_CONDITIONAL_JMP(jmn, != 0);

int djn(int to, Cell *dst, uint8_t mode) {
	_Bool res;
	switch (mode) {
	case OM_AA:
	case OM_BA:
		dst->values[AFIELD] = help_sub(dst->values[AFIELD], 1);
		res = (dst->values[AFIELD] != 0);
		break;
	case OM_BB:
	case OM_AB:
		dst->values[BFIELD] = help_sub(dst->values[BFIELD], 1);
		res = (dst->values[BFIELD] != 0);
		break;
	case OM_F:
	case OM_X:
	case OM_I:
		dst->values[AFIELD] = help_sub(dst->values[AFIELD], 1);
		dst->values[BFIELD] = help_sub(dst->values[BFIELD], 1);
		res = (dst->values[AFIELD] != 0) || (dst->values[BFIELD] != 0);
		break;
	default:
		abort();
	}
	if (res)
		return to;
	return 1;
}

_Bool seq(Cell *src, Cell *dst, uint8_t mode) {
	switch (mode) {
	case OM_AA:
		return src->values[AFIELD] == dst->values[AFIELD];
	case OM_AB:
		return src->values[AFIELD] == dst->values[BFIELD];
	case OM_BA:
		return src->values[BFIELD] == dst->values[AFIELD];
	case OM_BB:
		return src->values[BFIELD] == dst->values[BFIELD];
	case OM_F:
		return (src->values[AFIELD] == dst->values[AFIELD]) && (src->values[BFIELD] == dst->values[BFIELD]);
	case OM_X:
		return (src->values[AFIELD] == dst->values[BFIELD]) && (src->values[BFIELD] == dst->values[AFIELD]);
	case OM_I:
		return (src->operation == dst->operation)
			&& (src->op_mode == dst->op_mode)
			&& (src->values[AFIELD] == dst->values[AFIELD])
			&& (src->values[BFIELD] == dst->values[BFIELD])
			&& (src->addr_modes[AFIELD] == dst->addr_modes[AFIELD])
			&& (src->addr_modes[BFIELD] == dst->addr_modes[BFIELD]);
	default:
		abort();
	}
}

_Bool sne(Cell *src, Cell *dst, uint8_t mode) {
	return !seq(src, dst, mode);
}

_Bool slt(Cell *src, Cell *dst, uint8_t mode) {
	switch (mode) {
	case OM_AA:
		return src->values[AFIELD] < dst->values[AFIELD];
	case OM_AB:
		return src->values[AFIELD] < dst->values[BFIELD];
	case OM_BA:
		return src->values[BFIELD] < dst->values[AFIELD];
	case OM_BB:
		return src->values[BFIELD] < dst->values[BFIELD];
	case OM_F:
	case OM_I:
		return (src->values[AFIELD] < dst->values[AFIELD]) && (src->values[BFIELD] < dst->values[BFIELD]);
	case OM_X:
		return (src->values[AFIELD] < dst->values[BFIELD]) && (src->values[BFIELD] < dst->values[AFIELD]);
	default:
		abort();
	}
}

typedef struct {
	char name[4];
	uint8_t default_mode;
} named_op;

named_op op_registry[OP_NB] = {
	[OP_DAT] = { "dat", OM_F  },
	[OP_MOV] = { "mov", OM_I  },
	[OP_ADD] = { "add", OM_AB },
	[OP_SUB] = { "sub", OM_AB },
	[OP_MUL] = { "mul", OM_AB },
	[OP_DIV] = { "div", OM_AB },
	[OP_MOD] = { "mod", OM_AB },
	[OP_JMP] = { "jmp", OM_BB },
	[OP_JMZ] = { "jmz", OM_BB },
	[OP_JMN] = { "jmn", OM_BB },
	[OP_DJN] = { "djn", OM_BB },
	[OP_SEQ] = { "seq", OM_I  },
	[OP_SNE] = { "sne", OM_I  },
	[OP_SLT] = { "slt", OM_BB },
	[OP_SPL] = { "spl", OM_BB },
	[OP_NOP] = { "nop", OM_F  },
};

// name must point to at least 2 chars
uint8_t mode_from_name(const char *name) {
	uint8_t res;
	switch (name[0]) {
	case 'a':
		if (name[1] == 'b') return OM_AB;
		res = OM_AA;
		break;
	case 'b':
		if (name[1] == 'a') return OM_BA;
		res = OM_BB;
		break;
	case 'f':
		res = OM_F;
		break;
	case 'x':
		res = OM_X;
		break;
	case 'i':
		res = OM_I;
		break;
	default:
		return OM_INVALID;
	}
	if (name[1] == '\0') {
		return res;
	}
	return OM_INVALID;
}

const char *name_from_mode(uint8_t mode) {
	switch (mode) {
	case OM_AA: return "a ";
	case OM_AB: return "ab";
	case OM_BA: return "ba";
	case OM_BB: return "b ";
	case OM_F: return "f ";
	case OM_X: return "x ";
	case OM_I: return "i ";
	}
	return "? ";
}

// name must be a pointer to at least 3 characters.
uint8_t op_from_name(const char *name) {
	for (uint8_t i = 0; i < OP_NB; i++) {
		if (memcmp(name, op_registry[i].name, 3)) {
			continue;
		}
		return i;
	}
	return OP_INVALID;
}

const char *name_from_op(uint8_t op) {
	if (op >= OP_NB) // actually, an assertion is probably better
		return "invalid operation";
	return op_registry[op].name;
}

uint8_t default_op_mode(uint8_t op) {
	if (op >= OP_NB) // actually, an assertion is probably better
		return -1;
	return op_registry[op].default_mode;
}
