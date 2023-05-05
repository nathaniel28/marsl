
CFLAGS=-g -Wall
LIBS=

include Makefile.d

OBJS=address.o ops.o sim.o testing.o types.o parser.tab.o
corewars: $(OBJS) main.o
	$(CC) $(CFLAGS) $(OBJS) $(LIBS) -o corewars

parser:
	bison -t --warnings=all parser.y
	$(CC) $(CFLAGS) -c parser.tab.c

test: $(OBJS) testing.o
	$(CC) $(CFLAGS) $(OBJS) $(LIBS) -o test

SRCS=address.c main.c ops.c sim.c testing.c types.c parser.tab.c
depend Makefile.d:
	$(CC) -MM $(SRCS) |grep : >Makefile.d

clean:
	rm *.o
	make depend
	make parser
	make corewars
