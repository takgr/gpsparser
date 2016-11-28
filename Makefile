# rules
export CC=gcc
export CDEFS = -DNDEBUG

# directories
BASE :=$(shell pwd)
export SRC=$(BASE)/src

all :
	cd $(SRC) && $(MAKE) all

clean :
	cd $(SRC) && $(MAKE) clean
	
doc  :
#	doxygen

