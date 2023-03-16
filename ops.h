#ifndef OPS_H
#define OPS_H

#include "types.h"

extern operation op_from_name(const char *, const char *);

extern const char *name_from_op(operation op);

#endif
