
CFLAGS=-g -Wall
LIBS=

include Makefile.d

OBJS=address.o main.o ops.o sim.o types.o parser.tab.o
corewars: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) $(LIBS) -o corewars

parser:
	bison -t --warnings=all parser.y
	$(CC) $(CFLAGS) -c parser.tab.c

SRCS=address.c main.c ops.c sim.c types.c parser.tab.c
depend Makefile.d:
	$(CC) -MM $(SRCS) |grep : >Makefile.d
