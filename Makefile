CFLAGS+=-g
default: procnanny

clean:
	rm -f *.o procnanny procnanny-libev

%.c: Makefile
%.h: Makefile

program.c: program.h process.h
process.c: program.h process.h
pn_api.c: pn_api.h procnanny.h
procnanny.c: procnanny.h

procnanny: LDFLAGS+=-lrt
procnanny: procnanny.o program.o process.o
	$(CC) $(LDFLAGS) -o $@ $^

procnanny-libev.o: CFLAGS+=-I/usr/local/include
pn_api: CFLAGS+=-I/usr/local/include

procnanny-libev: LDFLAGS+=-L/usr/local/lib -lrt -lev -lmsgpack
procnanny-libev: procnanny-libev.o program.o process.o pn_util.o pn_api.o
	$(CC) -o $@ $(LDFLAGS) $^

procnanny-libevzmq.o: CFLAGS+=-I/usr/local/include

procnanny-libevzmq: LDFLAGS+=-L/usr/local/lib -lrt -lev -lzmq
procnanny-libevzmq: procnanny-libevzmq.o program.o process.o
	$(CC) -o $@ $(LDFLAGS) $^


test.o: CFLAGS+=-I/usr/local/include
test: LDFLAGS+=-L/usr/local/lib -lrt -lev -lmsgpack
test: test.o
	$(CC) -o $@ $(LDFLAGS) $^
