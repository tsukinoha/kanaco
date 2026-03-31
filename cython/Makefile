include ../common.mk

OBJS=kanaco.c
INCLUDES=-I. -I/usr/include $$($(PYCONFIG) --includes)
CFLAGS=-Wall -O2 $$($(PYCONFIG) --cflags --libs)


%.c: %.pyx
	$(CYTHON) -3 -f -o $@ $^

all: $(OBJS)
	$(CC) $(CFLAGS) $(INCLUDES) -I../c -fPIC -c $(INCLUDES) -o kanaco.o $^

clean:
	$(RM) -rf *.c *.o __pycache__ .pytest_cache

test:
	$(PYTEST) -s -vv test_kanaco.py

.PHONY: all clean test
