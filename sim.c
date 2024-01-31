#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "address.h"
#include "ops.h"
#include "sim.h"

int32_t closest(uint32_t u) {
	int32_t r1 = u % CORESIZE;
	int32_t r2 = r1 - CORESIZE;
	return -r2 < r1 ? r2 : r1;
}

void print_cell(Cell *c) {
	printf(
		"%s %c%d, %c%d",
		name_from_op(c->operation),
		char_from_addr_method(c->addr_modes[AFIELD]),
		closest(c->values[AFIELD]),
		char_from_addr_method(c->addr_modes[BFIELD]),
		closest(c->addr_modes[BFIELD])
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

#define WRAP(u, delta) \
do { \
	u = (int) (u + delta) % CORESIZE; \
	if ((int) u < 0) \
		u += CORESIZE; \
} while (0);

#define INCR_PROC(p) if (p->cur_proc < p->nprocs - 1) p->cur_proc++; else p->cur_proc = 0

void step(Program *p) {
	// TODO: buffer source, destination, and self cells
	assert(p->nprocs < MAXPROCS && p->cur_proc < p->nprocs);
	int instr = p->proc_queue[p->cur_proc];
	assert(instr < CORESIZE);

	Cell *self = core + instr;

	DB_PRINT("%p (%u@%d/%d): ", p, p->cur_proc + 1, instr, p->nprocs);
	DB_PRINT_CELL(self);

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

	// TODO me, very soon: it seems that pmars resolves the first field,
	// then USING THAT RESULT (if targeted) resolves the second field.

	/*
	state.kill_proc = 0;
	state.ret_to = self + 1;
	state.spl_to = NULL;
	*/

	_Bool kill_proc = 0;
	int ret_to = instr + 1;
	int spl_to = -1;

	Cell *src = core + resolve_field(instr, AFIELD, &p->src_fbuf);
	Cell *dst = core + resolve_field(instr, BFIELD, &p->dst_fbuf);

	switch (self->operation) {
	case OP_DAT:
		kill_proc = 1;
                break;
        case OP_MOV:
		mov(src, dst, self->op_mode);
		break;
        case OP_ADD:
		add(src, dst, self->op_mode);
		break;
        case OP_SUB:
		sub(src, dst, self->op_mode);
		break;
        case OP_MUL:
		mul(src, dst, self->op_mode);
		break;
        case OP_DIV:
		kill_proc = div_(src, dst, self->op_mode);
		break;
        case OP_MOD:
		kill_proc = mod(src, dst, self->op_mode);
		break;
	/*
        case OP_JMP:
		ret_to = jmp(src, dst, self->op_mode);
		break;
        case OP_JMZ:
		ret_to = jmz(src, dst, self->op_mode);
		break;
        case OP_JMN:
		ret_to = jmz(src, dst, self->op_mode);
		break;
        case OP_DJN:
		ret_to = djn(src, dst, self->op_mode);
		break;
        case OP_SEQ:
		ret_to = seq(src, dst, self->op_mode);
		break;
        case OP_SNE:
		ret_to = sne(src, dst, self->op_mode);
		break;
        case OP_SLT:
		ret_to = slt(src, dst, self->op_mode);
		break;
        case OP_SPL:
		spl_to = spl(src, dst, self->op_mode);
		break;
	*/
        case OP_NOP:
                break;
        default:
                // TODO: print error
                abort();
	}

	if (kill_proc) {
		DB_PRINT(" (process killed)");
		DB_PRINT("\nold queue [");
		DB_PRINT_PQUEUE(p);
		DB_PRINT("], now [");

		p->nprocs--;
		if (p->nprocs == 0) {
			DB_PRINT("], %p terminated\n", p);
			fprintf(stderr, "program termination not yet implimented, so exiting\n");
			exit(1);
			// TODO: handle program termination
			return;
		}

		// TODO: check if this is handled properly
		int *dst = &p->proc_queue[p->cur_proc];
		int *src = dst + 1;
		memmove(dst, src, sizeof(uint) * (p->proc_queue + p->nprocs - dst));

		DB_PRINT_PQUEUE(p);
		DB_PRINT("]\n");
	} else {
		int new_instr_offset = ret_to % CORESIZE; // a wrap
		if (new_instr_offset < 0)
			new_instr_offset += CORESIZE;
		p->proc_queue[p->cur_proc] = new_instr_offset;

		if (ret_to != instr + 1) {
			DB_PRINT(" (jumped to %d)", closest(new_instr_offset));
		}

		INCR_PROC(p);

		if (spl_to != -1 && p->nprocs < MAXPROCS) {
			int spl_instr_offset = spl_to % CORESIZE; // a wrap
			if (spl_instr_offset < 0)
				spl_instr_offset += CORESIZE;
			DB_PRINT(" (split to %d)", closest(spl_instr_offset));
			DB_PRINT("\nold queue [");
			DB_PRINT_PQUEUE(p);
			DB_PRINT("], now [");

			p->nprocs++;
			int *src = &p->proc_queue[p->cur_proc];
			int *dst = src + 1;
			memmove(dst, src, sizeof(uint) * (p->proc_queue + p->nprocs - dst)); // (p->nprocs - p->cur_proc - 1) == (p->proc_queue + p->nprocs - dst)
			p->proc_queue[p->cur_proc] = spl_instr_offset;
			INCR_PROC(p);

			DB_PRINT_PQUEUE(p);
			DB_PRINT("]");
		}
		DB_PRINT("\n");
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
		*p->dst_cbuf.store = p->dst_cbuf.buffer;
		if (p->src_fbuf.store) {
			WRAP(*p->src_fbuf.store, p->src_fbuf.buffer);
		}
		if (p->dst_fbuf.store) {
			WRAP(*p->dst_fbuf.store, p->dst_fbuf.buffer);
		}
		p = p->next;
	}
}
