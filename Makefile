CFLAGS?=-g

MSGPACK?=/opt/msgpack/0.5.5
LIBEV?=/opt/libev/4.04
ZEROMQ?=/opt/zeromq/2.1.7/

CFLAGS+=-I$(MSGPACK)/include -I/usr/local/include
LDFLAGS+=-L$(MSGPACK)/lib -L/usr/local/lib
CFLAGS+=-I$(LIBEV)/include -I/usr/local/include
LDFLAGS+=-L$(LIBEV)/lib -L/usr/local/lib
CFLAGS+=-I$(ZEROMQ)/include -I/usr/local/include
LDFLAGS+=-L$(ZEROMQ)/lib -L/usr/local/lib

QUIET?=@

FILES=msgpack_helpers.c pn_api.c pn_util.c process.c procnanny.c program.c
OBJECTS=$(addprefix build/, $(subst .c,.o,$(FILES)))

VPATH=src
all: default
default: procnanny

clean:
	rm -f $(OBJECTS)
	rm -r build/

%.c %.h: Makefile
program.c: program.h process.h
process.c: program.h process.h
pn_api.c: pn_api.h procnanny.h
procnanny.c: procnanny.h
msgpack_helpers.c: msgpack_helpers.h

build/%.o: %.c build
	@echo "Compiling $< ($@)"
	$(QUIET)$(CC) $(CFLAGS) -c $< -o $@

build: 
	@mkdir build

.PHONY: compile
compile: $(OBJECTS)

procnanny: LDFLAGS+=-lrt -lev -lmsgpack -lzmq
procnanny: LDFLAGS+=-Xlinker -rpath=$(MSGPACK)/lib -Xlinker -rpath=$(LIBEV)/lib
procnanny: LDFLAGS+=-Xlinker -rpath=$(ZEROMQ)/lib
procnanny: $(OBJECTS)
	@echo "Linking $@"
	$(QUIET)$(CC) -o $@ $(LDFLAGS) $^
