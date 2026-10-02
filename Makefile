include common.mk

all: c cython

c:
	$(MAKE) -C c

cython: c
	$(MAKE) -C cython

test:
	$(MAKE) -C c test
	$(MAKE) -C cython test

clean:
	$(MAKE) -C c clean
	$(MAKE) -C cython clean

.PHONY: all c cython test clean
