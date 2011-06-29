FILES=procnanny.coffee lib/interface.coffee controllers/program.coffee public/js/procnanny.coffee
STATIC=public/ views/
OBJECTS=$(addprefix build/, $(subst .coffee,.js,$(FILES)))

STATIC_FILES=$(shell find $(STATIC) -type f | egrep '\.(css|js|jade|jpg)$$')
STATIC_OBJECTS=$(addprefix build/, $(STATIC_FILES))

COFFEE=./node_modules/coffee-script/bin/coffee

VPATH=src
all: $(OBJECTS)

clean:
	rm -f $(OBJECTS)
	rm -r build/

%.coffee: Makefile

build/%.js: %.coffee | build
	@echo "Compiling $< ($@)"
	$(QUIET)[ -d $(shell dirname $@) ] || mkdir -p $(shell dirname $@)
	$(QUIET)$(COFFEE) -c -o $(shell dirname $@) $<

build/%.css: %.css | build
	$(QUIET)[ -d $(shell dirname $@) ] || mkdir -p $(shell dirname $@)
	$(QUIET)cp $< $@

build/%.js: %.js | build
	$(QUIET)[ -d $(shell dirname $@) ] || mkdir -p $(shell dirname $@)
	$(QUIET)cp $< $@

build/%.jade: %.jade | build
	$(QUIET)[ -d $(shell dirname $@) ] || mkdir -p $(shell dirname $@)
	$(QUIET)cp $< $@

build/%.jpg: %.jpg | build
	$(QUIET)[ -d $(shell dirname $@) ] || mkdir -p $(shell dirname $@)
	$(QUIET)cp $< $@

build: 
	$(QUIET)mkdir build

all: $(OBJ) $(STATIC_OBJECTS)
