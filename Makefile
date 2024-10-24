
SOURCES = address.c types.c ops.c sim.c parser.tab.c
OBJS = $(addsuffix .o, $(basename $(notdir $(SOURCES))))
CFLAGS = -g -Wall -Wextra -pedantic -std=gnu23
LIBS =
CC = cc

default: corewars

parser.tab.c: parser.y
	bison -t --warnings=all -Wcounterexamples parser.y

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

corewars: $(OBJS) main.o
	$(CC) $(CFLAGS) $(LIBS) $(OBJS) main.o -o $@

test: $(OBJS) testing.o
	$(CC) $(CFLAGS) $(LIBS) $(OBJS) testing.o -o $@

clean:
	rm -f *.o parser.tab.c corewars test
