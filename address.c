#include <stddef.h>

#include "address.h"
#include "types.h"

Cell *addr_immediate(Cell *cell, uint offset) {
	return cell;
}

Cell *addr_direct(Cell *cell, uint offset) {
	return core + (cell - core + offset) % CORESIZE;
}

Cell *addr_a_indirect(Cell *cell, uint offset) {
	Cell *target = addr_direct(cell, offset);
	return addr_direct(target, AFIELD(target).val);
}

Cell *addr_b_indirect(Cell *cell, uint offset) {
	Cell *target = addr_direct(cell, offset);
	return addr_direct(target, BFIELD(target).val);
}

#define INCR(u) if (u >= CORESIZE - 1) u = 0; else u++
#define DECR(u) if (u == 0) u = CORESIZE - 1; else u--

Cell *addr_a_indirect_predec(Cell *cell, uint offset) {
	Cell *target = addr_direct(cell, offset);
	DECR(AFIELD(target).val);
	return addr_direct(target, AFIELD(target).val);
}

Cell *addr_a_indirect_postinc(Cell *cell, uint offset) {
	Cell *target = addr_direct(cell, offset);
	Cell *res = addr_direct(target, AFIELD(target).val);
	INCR(AFIELD(target).val);
	return res;
}

Cell *addr_b_indirect_predec(Cell *cell, uint offset) {
	Cell *target = addr_direct(cell, offset);
	DECR(BFIELD(target).val);
	return addr_direct(target, BFIELD(target).val);
}

Cell *addr_b_indirect_postinc(Cell *cell, uint offset) {
	Cell *target = addr_direct(cell, offset);
	Cell *res = addr_direct(target, BFIELD(target).val);
	INCR(BFIELD(target).val);
	return res;
}

const address_mode default_addr_mode = addr_direct;

address_mode addr_method_from_char(char c) {
	switch (c) {
	case '#': return addr_immediate;
	case '$': return addr_direct;
	case '*': return addr_a_indirect;
	case '{': return addr_a_indirect_predec;
	case '}': return addr_a_indirect_postinc;
	case '@': return addr_b_indirect;
	case '<': return addr_b_indirect_predec;
	case '>': return addr_b_indirect_postinc;
	}
	return NULL;
}

char char_from_addr_method(address_mode m) {
	if (m == addr_immediate) return '#';
	if (m == addr_direct) return '$';
	if (m == addr_a_indirect) return '*';
	if (m == addr_a_indirect_predec) return '{';
	if (m == addr_a_indirect_postinc) return '}';
	if (m == addr_b_indirect) return '@';
	if (m == addr_b_indirect_predec) return '<';
	if (m == addr_b_indirect_postinc) return '>';
	return '?';
}
