
CFLAGS=-g3 -Wall -Wextra -Wno-unused-parameter #-O3
LIBS=

default: corewars

include Makefile.d

OBJS=address.o ops.o sim.o types.o parser.tab.o

corewars: $(OBJS) main.o
	$(CC) $(CFLAGS) $(OBJS) main.o $(LIBS) -o corewars

parser:
	bison -t --warnings=all -Wcounterexamples parser.y
	$(CC) $(CFLAGS) -c parser.tab.c

test: $(OBJS) testing.o
	make corewars
	$(CC) $(CFLAGS) $(OBJS) testing.o $(LIBS) -o test

SRCS=address.c main.c ops.c sim.c testing.c types.c parser.tab.c
depend Makefile.d:
	$(CC) -MM $(SRCS) |grep : >Makefile.d

clean:
	rm *.o
	make depend
	make parser
	make default
