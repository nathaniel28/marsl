#ifndef OPS_H
#define OPS_H

#include "types.h"

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

extern int jmz(int to, Cell *dst, uint8_t mode);
extern int jmn(int to, Cell *dst, uint8_t mode);

extern int djn(int to, Cell *dst, uint8_t mode);

extern _Bool seq(Cell *src, Cell *dst, uint8_t mode);
extern _Bool sne(Cell *src, Cell *dst, uint8_t mode);

extern _Bool slt(Cell *src, Cell *dst, uint8_t mode);

extern const char *invalid_op_str;

#endif
