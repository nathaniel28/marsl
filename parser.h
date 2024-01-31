#ifndef PARSER_H
#define PARSER_H

#include <stdio.h>

#include "types.h"

extern int parse(FILE *fp, Cell *buf, int *len);

#endif
