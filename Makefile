
default: procnanny

%.c: Makefile
%.h: Makefile
program.c: program.h

procnanny: LDFLAGS+=-lrt
procnanny: procnanny.o program.o
	$(CC) $(LDFLAGS) -o $@ $^
