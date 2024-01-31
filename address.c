#include <stddef.h>
#include <stdlib.h>

#include "address.h"
#include "types.h"

int offset(int cell, int delta) {
        cell = (cell + delta) % CORESIZE;
        if (cell < 0)
                return cell + CORESIZE;
        return cell;
}

int indirect(int cell, int field) {
        return offset(cell, core[cell].values[field]);
}

int resolve_field(int cell, int field, Valuebuf *vb) {
        int delta;
        switch (core[cell].addr_modes[field]) {
        case AM_IMMEDIATE:
                return 0;
        case AM_DIRECT:
                return indirect(cell, field);
        case AM_INDIRECT_A:
                cell = indirect(cell, field);
                return indirect(cell, AFIELD);
        case AM_INDIRECT_B:
                cell = indirect(cell, field);
                return indirect(cell, BFIELD);
        case AM_INDIRECT_A_PREDEC:
                cell = indirect(cell, field);
                delta = core[cell].values[AFIELD] - 1;
                if (unlikely(delta < 0))
                        delta = CORESIZE - 1;
                vb->store = &core[cell].values[AFIELD];
                vb->buffer = delta;
                return offset(cell, delta);
        case AM_INDIRECT_B_PREDEC:
                cell = indirect(cell, field);
                delta = core[cell].values[BFIELD] - 1;
                if (unlikely(delta < 0))
                        delta = CORESIZE - 1;
                vb->store = &core[cell].values[BFIELD];
                vb->buffer = delta;
                return offset(cell, delta);
        case AM_INDIRECT_A_POSTINC:
                cell = indirect(cell, field);
                delta = core[cell].values[AFIELD] + 1;
                if (unlikely(delta >= CORESIZE))
                        delta = 0;
                vb->store = &core[cell].values[AFIELD];
                vb->buffer = delta;
                return indirect(cell, AFIELD);
        case AM_INDIRECT_B_POSTINC:
                cell = indirect(cell, field);
                delta = core[cell].values[BFIELD] + 1;
                if (unlikely(delta >= CORESIZE))
                        delta = 0;
                vb->store = &core[cell].values[BFIELD];
                vb->buffer = delta;
                return indirect(cell, BFIELD);
        default:
                // TODO: say an error
                abort();
        }
}

/*
#define INCR(u) (u >= CORESIZE - 1 ? 0 : u + 1)
#define DECR(u) (u == 0 ? CORESIZE - 1 : u - 1)

Cell *addr_a_indirect_predec(Cell *cell, uint offset) {
	Cell *target = addr_direct(cell, offset);
	offset = DECR(AFIELD(target).val);
	state.fbuf->buffer = -1;
	state.fbuf->store = &AFIELD(target).val;
	return addr_direct(target, offset);
}

Cell *addr_a_indirect_postinc(Cell *cell, uint offset) {
	Cell *target = addr_direct(cell, offset);
	Cell *res = addr_direct(target, AFIELD(target).val);
	state.fbuf->buffer = 1;
	state.fbuf->store = &AFIELD(target).val;
	return res;
}

Cell *addr_b_indirect_predec(Cell *cell, uint offset) {
	Cell *target = addr_direct(cell, offset);
	offset = DECR(BFIELD(target).val);
	state.fbuf->buffer = -1;
	state.fbuf->store = &BFIELD(target).val;
	return addr_direct(target, offset);
}

Cell *addr_b_indirect_postinc(Cell *cell, uint offset) {
	Cell *target = addr_direct(cell, offset);
	Cell *res = addr_direct(target, BFIELD(target).val);
	state.fbuf->buffer = 1;
	state.fbuf->store = &BFIELD(target).val;
	return res;
}
*/

uint8_t addr_method_from_char(char c) {
	switch (c) {
	case '#': return AM_IMMEDIATE;
	case '$': return AM_DIRECT;
	case '*': return AM_INDIRECT_A;
	case '@': return AM_INDIRECT_B;
	case '{': return AM_INDIRECT_A_PREDEC;
	case '<': return AM_INDIRECT_B_PREDEC;
	case '}': return AM_INDIRECT_A_POSTINC;
	case '>': return AM_INDIRECT_B_POSTINC;
	}
	return AM_INVALID;
}

char char_from_addr_method(uint8_t m) {
	switch (m) {
	case AM_IMMEDIATE: return '#';
	case AM_DIRECT: return '$';
	case AM_INDIRECT_A: return '*';
	case AM_INDIRECT_B: return '@';
	case AM_INDIRECT_A_PREDEC: return '{';
	case AM_INDIRECT_B_PREDEC: return '<';
	case AM_INDIRECT_A_POSTINC: return '}';
	case AM_INDIRECT_B_POSTINC: return '>';
	}
	return '?';
}
