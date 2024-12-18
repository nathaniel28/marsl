#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "address.h"
#include "ops.h"
#include "sim.h"
#include "types.h"

int32_t closest(uint32_t u) {
	int32_t r1 = u % CORESIZE;
	int32_t r2 = r1 - CORESIZE;
	return -r2 < r1 ? r2 : r1;
}

void print_cell(Cell *c) {
	printf(
		"%s.%s %c%d, %c%d",
		name_from_op(c->operation),
		name_from_mode(c->op_mode),
		char_from_addr_method(c->addr_modes[AFIELD]),
		closest(c->values[AFIELD]),
		char_from_addr_method(c->addr_modes[BFIELD]),
		closest(c->values[BFIELD])
	);
}

#if 1
#define DB_PRINT(...) fprintf(stderr, __VA_ARGS__)
#define DB_PRINT_CELL(self) print_cell(self); fflush(stdout)
#define DB_PRINT_PQUEUE(p) do { for (int i = 0; i < p->nprocs; i++) DB_PRINT("%u ", p->proc_queue[i]); } while (0)
#else
#define DB_PRINT(...)
#define DB_PRINT_CELL(self)
#define DB_PRINT_PQUEUE(p)
#endif

void print_core() {
	for (Cell *cur = core; cur < core + CORESIZE; cur++) {
		printf("%4lu ", cur - core);
		print_cell(cur);
		printf("\n");
	}
}

void init_core() {
	const Cell default_cell = {
		.values = { 0, 0 },
		.addr_modes = { AM_DEFAULT, AM_DEFAULT },
		.operation = OP_DEFAULT,
		.op_mode = OM_DEFAULT,
	};
	for (int i = 0; i < CORESIZE; i++) {
		core[i] = default_cell;
	}
}

uint32_t wrap(uint32_t val, uint32_t max) {
    if (val >= max)
	    return val - max;
    return val;
}

void flush_fbuf(Valuebuf *fbuf) {
	if (fbuf->store) {
		*fbuf->store = (int) (*fbuf->store + fbuf->buffer) % CORESIZE;
		if ((int) *fbuf->store < 0)
			*fbuf->store += CORESIZE;
	}
}

#define INCR_PROC(p) if (p->cur_proc < p->nprocs - 1) p->cur_proc++; else p->cur_proc = 0

_Bool valid_cell(Cell c) {
	return c.values[0] < CORESIZE && c.values[1] < CORESIZE && c.addr_modes[0] < AM_NB && c.addr_modes[1] < AM_NB && c.operation < OP_NB && c.op_mode < OM_NB;
}

void step(Program *p) {
	// TODO: buffer source, destination, and self cells
	assert(p->nprocs < MAXPROCS && p->cur_proc < p->nprocs);
	int instr = p->proc_queue[p->cur_proc];
	assert(instr < CORESIZE);

	Cell *self = core + instr;

	// Order of operations:
	// NOTE: Steps 1-3 must not modify the core.
	// NOTE: 1-2 can cause changes through pre/post incriments/decriments.
	// 1. Resolve the a-field of the current instruction. NOTE: Changes MAY
	// influence b-field resolution the next step.
	// 2. Resolve the b-field of the current instruction.
	// 3. Using the source target resolved in step 1 and the destination
	// target resolved in step 2, execute the current instruction.
	// 4. Repeat steps 1-3 for each program with processes left.
	// 5. Write all buffered data to the core. I don't know what happenes if
	// two programs modify the same cell on the same frame.

	// whatever this operation is gonna do, place the result(s) in this
	// program's buffer instead of modifying the core directly

	p->dst_cbuf.store = NULL;
	p->src_fbuf.store = NULL;
	p->dst_fbuf.store = NULL;

	_Bool kill_proc = 0;
	int ret_to = 1; // relative to instr; instr will be added on later
	_Bool split = 0;
	int spl_to; // not relative

	// TODO me, very soon: it seems that pmars resolves the first field,
	// then USING THAT RESULT (if targeted) resolves the second field.
	int src_offset = resolve_field(instr, AFIELD, &p->src_fbuf);
	int dst_offset = resolve_field(instr, BFIELD, &p->dst_fbuf);

	DB_PRINT("%p %u@%d/%d: ", (void *) p, p->cur_proc + 1, instr, p->nprocs);
	DB_PRINT_CELL(self);
	DB_PRINT(" (%d %d)\n", src_offset - instr, dst_offset - instr);

	Cell *src = core + src_offset;
	p->dst_cbuf.store = core + dst_offset;
	p->dst_cbuf.buffer = *p->dst_cbuf.store;

	switch (self->operation) {
	case OP_DAT:
		kill_proc = 1;
		break;
	case OP_MOV:
		mov(src, &p->dst_cbuf.buffer, self->op_mode);
		break;
	case OP_ADD:
		add(src, &p->dst_cbuf.buffer, self->op_mode);
		break;
	case OP_SUB:
		sub(src, &p->dst_cbuf.buffer, self->op_mode);
		break;
	case OP_MUL:
		mul(src, &p->dst_cbuf.buffer, self->op_mode);
		break;
	case OP_DIV:
		kill_proc = div_(src, &p->dst_cbuf.buffer, self->op_mode);
		break;
	case OP_MOD:
		kill_proc = mod(src, &p->dst_cbuf.buffer, self->op_mode);
		break;
	case OP_JMP:
		ret_to = src_offset - instr; // src_offset is an absolute position in the core; we need one relative to instr
		break;
	case OP_JMZ:
		ret_to = jmz(src_offset - instr, &p->dst_cbuf.buffer, self->op_mode);
		break;
	case OP_JMN:
		ret_to = jmn(src_offset - instr, &p->dst_cbuf.buffer, self->op_mode);
		break;
	case OP_DJN:
		ret_to = djn(src_offset - instr, &p->dst_cbuf.buffer, self->op_mode);
		break;
	case OP_SEQ:
		ret_to += seq(src, &p->dst_cbuf.buffer, self->op_mode);
		break;
	case OP_SNE:
		ret_to += sne(src, &p->dst_cbuf.buffer, self->op_mode);
		break;
	case OP_SLT:
		ret_to += slt(src, &p->dst_cbuf.buffer, self->op_mode);
		break;
	case OP_SPL:
		split = 1;
		spl_to = src_offset;
		break;
	case OP_NOP:
		break;
	default:
		// TODO: print error
		abort();
	}

	assert(valid_cell(p->dst_cbuf.buffer));

	if (kill_proc) {
		DB_PRINT("process killed: old queue [");
		DB_PRINT_PQUEUE(p);
		DB_PRINT("], now [");

		p->nprocs--;
		if (p->nprocs == 0) {
			DB_PRINT("], %p terminated\n", (void *) p);
			fprintf(stderr, "program termination not yet implimented, so exiting\n");
			exit(1);
			// TODO: handle program termination
			return;
		}

		// TODO: check if this is handled properly
		int *dst = &p->proc_queue[p->cur_proc];
		int *src = dst + 1;
		memmove(dst, src, sizeof(int) * (p->proc_queue + p->nprocs - dst));
		// replace (p->proc_queue + p->nprocs - dst) with (p->nprocs - p->cur_proc)?

		if (p->cur_proc == p->nprocs) {
			p->cur_proc = 0;
		}

		DB_PRINT_PQUEUE(p);
		DB_PRINT("]\n");
	} else {
		ret_to += instr;
		int new_instr_offset = wrap(ret_to, CORESIZE);
		p->proc_queue[p->cur_proc] = new_instr_offset;

		if (ret_to != instr + 1) {
			DB_PRINT("jumped to %d.\n", closest(new_instr_offset));
		}

		INCR_PROC(p);

		if (split && p->nprocs < MAXPROCS) {
			int spl_instr_offset = wrap(spl_to, CORESIZE);
			DB_PRINT("split to %d: old queue [", closest(spl_instr_offset));
			DB_PRINT_PQUEUE(p);
			DB_PRINT("], now [");

			p->nprocs++;
			int *src = &p->proc_queue[p->cur_proc];
			int *dst = src + 1;
			memmove(dst, src, sizeof(int) * (p->proc_queue + p->nprocs - dst)); // (p->nprocs - p->cur_proc - 1) == (p->proc_queue + p->nprocs - dst)
			p->proc_queue[p->cur_proc] = spl_instr_offset;
			INCR_PROC(p);

			DB_PRINT_PQUEUE(p);
			DB_PRINT("]\n");
		}
	}
}

void run(Program *start) {
	// simulate each program
	Program *p = start;
	while (p) {
		step(p);
		p = p->next;
	}

	// write the results back to the core
	// TODO: what is supposed to happen if two programs write to the same
	// cell? Currently, the last program wins the write.
	p = start;
	while (p) {
		/*
		DB_PRINT("replace [");
		DB_PRINT_CELL(p->dst_cbuf.store);
		DB_PRINT("]@%ld with [", p->dst_cbuf.store - core);
		DB_PRINT_CELL(&p->dst_cbuf.buffer);
		DB_PRINT("]\n");
		*/
		*p->dst_cbuf.store = p->dst_cbuf.buffer;
		flush_fbuf(&p->src_fbuf);
		flush_fbuf(&p->dst_fbuf);
		p = p->next;
	}
	//print_core();
}
