
OBJ=lib/nanny.js lib/program.js lib/interface.js controllers/program.js \
    public/js/procnanny.js

all: $(OBJ)

lib/interface.js: controllers/program.js
lib/nanny.js: lib/program.js lib/interface.js 

%.js: %.coffee
	~/node_modules/coffee-script/bin/coffee -c $<
