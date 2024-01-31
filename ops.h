#ifndef OPS_H
#define OPS_H

#include "types.h"

typedef struct {
	char name[4];
	uint8_t default_mode;
} named_op;

extern uint8_t op_from_name(const char *);
extern const char *name_from_op(uint8_t op);

extern uint8_t mode_from_name(const char *name);
extern const char *name_from_mode(uint8_t mode);

extern uint8_t default_op_mode(uint8_t op);

extern void mov(Cell *src, Cell *dst, uint8_t mode);
extern void add(Cell *src, Cell *dst, uint8_t mode);
extern void sub(Cell *src, Cell *dst, uint8_t mode);
extern void mul(Cell *src, Cell *dst, uint8_t mode);

extern _Bool div_(Cell *src, Cell *dst, uint8_t mode);
extern _Bool mod(Cell *src, Cell *dst, uint8_t mode);

#endif
