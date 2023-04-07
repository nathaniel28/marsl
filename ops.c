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

static void dat() { state.kill_proc = 1; }

static void mov(byte src_field, byte dst_field) {
	(state.dst->fields + dst_field)->val = (state.src->fields + src_field)->val;
}
GEN_FULL(mov);
static void movI() { *state.dst = *state.src; }

static void add(byte src_field, byte dst_field) {
	uint *dst_u = &(state.dst->fields + dst_field)->val;
	*dst_u = (*dst_u + (state.src->fields + src_field)->val) % CORESIZE;
}
GEN_FULL(add);

static void sub(byte src_field, byte dst_field) {
	uint *dst_u = &(state.dst->fields + dst_field)->val;
	*dst_u = (*dst_u - (state.src->fields + src_field)->val) % CORESIZE;
}
GEN_FULL(sub);

static void mul(byte src_field, byte dst_field) {
	uint *dst_u = &(state.dst->fields + dst_field)->val;
	*dst_u = ((*dst_u) * ((state.src->fields + src_field)->val)) % CORESIZE;
}
GEN_FULL(mul);

static void div(byte src_field, byte dst_field) {
	uint divisor = (state.src->fields + src_field)->val;
	if (divisor == 0) {
		state.kill_proc = 1;
	} else {
		uint *dst_u = &(state.dst->fields + dst_field)->val;
		*dst_u = (*dst_u / divisor) % CORESIZE;
	}
}
GEN_FULL(div);

static void mod(byte src_field, byte dst_field) {
	uint divisor = (state.src->fields + src_field)->val;
	if (divisor == 0) {
		state.kill_proc = 1;
	} else {
		uint *dst_u = &(state.dst->fields + dst_field)->val;
		*dst_u = (*dst_u % divisor) % CORESIZE;
	}
}
GEN_FULL(mod);

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

static void nop() { /* TODO */ }

const operation default_op = dat;

const named_op op_registry[OP_NB] = {
	{"mov", {movA, movB, movAB, movBA, movF, movX, movI}, movI },
	{"dat", {dat , dat , dat  , dat  , dat , dat , dat }, dat  },
	{"add", {addA, addB, addAB, addBA, addF, addX, addF}, addAB},
	{"sub", {subA, subB, subAB, subBA, subF, subX, subF}, subAB},
	{"mul", {mulA, mulB, mulAB, mulBA, mulF, mulX, mulF}, mulAB},
	{"div", {divA, divB, divAB, divBA, divF, divX, divF}, divAB},
	{"mod", {modA, modB, modAB, modBA, modF, modX, modF}, modAB},
	{"jmp", {jmp , jmp , jmp  , jmp  , jmp , jmp , jmp }, jmp  },
	{"jmz", {jmzA, jmzB, jmzB , jmzA , jmzF, jmzF, jmzF}, jmzB },
	{"jmn", {jmnA, jmnB, jmnB , jmnA , jmnF, jmnF, jmnF}, jmnB },
	{"djn", {djnA, djnB, djnB , djnA , djnF, djnF, djnF}, djnB },
	{"seq", {seqA, seqB, seqAB, seqBA, seqF, seqX, seqI}, seqI },
	{"sne", {sneA, sneB, sneAB, sneBA, sneF, sneX, sneI}, sneI },
	{"slt", {sltA, sltB, sltB , sltA , sltF, sltX, sltF}, sltB },
	{"spl", {spl , spl , spl  , spl  , spl , spl , spl }, spl  },
	{"nop", {nop , nop , nop  , nop  , nop , nop , nop }, nop  },
};

// name must point to at least 2 chars (16 bytes)
enum mode_id mode_from_name(const char *name) {
	enum mode_id res;
	switch (name[0]) {
	case 'a':
		if (name[1] == 'b') return M_AB;
		res = M_A;
		break;
	case 'b':
		if (name[1] == 'a') return M_BA;
		res = M_B;
		break;
	case 'f':
		res = M_F;
		break;
	case 'x':
		res = M_X;
		break;
	case 'i':
		res = M_I;
		break;
	default:
		return M_INVALID;
	}
	if (name[1] == '\0') {
		return res;
	}
	return M_INVALID;
}

// name must be a pointer to 3 characters.
// mode_name must be a pointer to 2 characters.
operation op_from_name(const char *name, const char *mode_name) {
	for (int i = 0; i < OP_NB; i++) {
		if (memcmp(name, op_registry[i].name, 3)) {
			continue;
		}
		enum mode_id m = mode_from_name(mode_name);
		if (m != M_INVALID) {
			return op_registry[i].modes[m];
		}
		return NULL;
	}
	return NULL;
}

const char *name_from_op(operation op) {
	static char res[7];
	// mind the static
	// [0,2] store the operation name
	// 3 stores a '.'
	// 4 stores the first character of the operation mode
	// 5 stores the second character of the operation node or ' '
	// 6 stores a '\0'
	
	for (int i = 0; i < OP_NB; i++) {
		for (int j = 0; j < M_NB; j++) {
			if (op != op_registry[i].modes[j]) {
				continue;
			}
			memcpy(&res[0], op_registry[i].name, 3);
			res[3] = '.';
			switch (j) {
			case M_A:
				memcpy(&res[4], "a ", 2);
				break;
			case M_B:
				memcpy(&res[4], "b ", 2);
				break;
			case M_AB:
				memcpy(&res[4], "ab", 2);
				break;
			case M_BA:
				memcpy(&res[4], "ba", 2);
				break;
			case M_F:
				memcpy(&res[4], "f ", 2);
				break;
			case M_X:
				memcpy(&res[4], "x ", 2);
				break;
			case M_I:
				memcpy(&res[4], "i ", 2);
				break;
			}
			return res;
		}
	}

	// string literals have static storage duration btw
	return "invalid operation";
}
