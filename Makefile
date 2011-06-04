CFLAGS+=-g
default: procnanny

clean:
	rm -f *.o procnanny procnanny-libev

%.c: Makefile
%.h: Makefile
program.c: program.h

procnanny: LDFLAGS+=-lrt
procnanny: procnanny.o program.o
	$(CC) $(LDFLAGS) -o $@ $^

procnanny-libev.o: CFLAGS+=-I/usr/local/include

procnanny-libev: LDFLAGS+=-L/usr/local/lib -lrt -lev
procnanny-libev: procnanny-libev.o program.o
	$(CC) -o $@ $(LDFLAGS) $^
