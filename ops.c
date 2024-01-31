#include <stdlib.h>
#include <string.h>

#include "ops.h"
#include "types.h"

// lots of stuff is static to produce warnings if I generate useless
// functions with these hack-y macros

#define GEN_FN(name) \
	static void name##A () { name(0, 0); } \
	static void name##B () { name(1, 1); }

#define GEN_FN_REV(name) \
	static void name##AB() { name(0, 1); } \
	static void name##BA() { name(1, 0); }

#define GEN_FN_EXTRA(name) \
	static void name##F () { name(0, 0); name(1, 1); } \
	static void name##X () { name(0, 1); name(1, 0); }

#define GEN_FULL(name) \
	GEN_FN(name); \
	GEN_FN_REV(name); \
	GEN_FN_EXTRA(name);

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
	        case OM_I: \
	        case OM_F: \
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
	        case OM_I: \
	        case OM_F: \
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

/*
void (Cell *src, Cell *dst, uint8_t mode) {
	switch (mode) {
	case OM_AA:
        case OM_AB:
        case OM_BA:
        case OM_BB:
        case OM_F:
        case OM_X:
        case OM_I:
	}
}
*/

/*
static void jmp() { state.ret_to = state.src; }

#define GEN_JUMP(name, condition) \
	static void name##A() { if (AFIELD(state.dst).val condition) jmp(); } \
	static void name##B() { if (BFIELD(state.dst).val condition) jmp(); } \
	static void name##F() { if (AFIELD(state.dst).val condition && BFIELD(state.dst).val condition) jmp(); }

GEN_JUMP(jmz, == 0);

GEN_JUMP(jmn, != 0);

#define DECR(u) if (u == 0) u = CORESIZE - 1; else u--;

static void djnA() { DECR(AFIELD(state.dst).val); jmnA(); }
static void djnB() { DECR(BFIELD(state.dst).val); jmnB(); }
static void djnF() {
	DECR(AFIELD(state.dst).val);
	DECR(BFIELD(state.dst).val);
	if (AFIELD(state.dst).val != 0 || BFIELD(state.dst).val != 0) jmp();
}

// TODO: please
#define GEN_SKIP(name, compare) \
	static void name(byte src_field, byte dst_field) { state.ret_to += ((state.src->fields + src_field)->val compare (state.dst->fields + dst_field)->val); } \
	GEN_FN(name); \
	static void name##F() { state.ret_to += (AFIELD(state.src).val compare AFIELD(state.dst).val && BFIELD(state.src).val compare BFIELD(state.dst).val); } \
	static void name##X() { state.ret_to += (AFIELD(state.src).val compare BFIELD(state.dst).val && BFIELD(state.src).val compare AFIELD(state.dst).val); }

#define GEN_SKIP_FULL(name, compare) \
	GEN_FN_REV(name); \
	static void name##I() { \
		state.ret_to += ( \
			state.src->op compare state.dst->op && \
			AFIELD(state.src).addr compare AFIELD(state.dst).addr && \
			BFIELD(state.src).addr compare BFIELD(state.dst).addr && \
			AFIELD(state.src).val compare AFIELD(state.dst).val && \
			BFIELD(state.src).val compare BFIELD(state.dst).val \
		); \
	}

GEN_SKIP(seq, ==);
GEN_SKIP_FULL(seq, ==);

GEN_SKIP(sne, !=);
GEN_SKIP_FULL(sne, !=);

GEN_SKIP(slt, <);

static void spl() { state.spl_to = state.src; }
*/

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
