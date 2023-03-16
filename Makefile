
CFLAGS=-g -Wall
LIBS=

include Makefile.d

OBJS=address.o main.o ops.o types.o

corewars: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) $(LIBS) -o corewars

SRCS=address.c main.c ops.c types.c
depend Makefile.d:
	$(CC) -MM $(SRCS) |grep : >Makefile.d
