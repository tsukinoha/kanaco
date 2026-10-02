include ../common.mk

# Python extensions are built with Python's own suffix,
# e.g. kanaco.cpython-313-x86_64-linux-gnu.so
EXT_SUFFIX:=$(shell $(PYCONFIG) --extension-suffix)
PY_INCLUDES:=$(shell $(PYCONFIG) --includes)
TARGET=kanaco$(EXT_SUFFIX)

all: $(TARGET)

kanaco.c: kanaco.pyx ../c/kanaco.h
	$(CYTHON) -3 -o $@ $<

# Cython-generated code is not strict C99, so it gets plain warnings only.
kanaco.o: kanaco.c ../c/kanaco.h
	$(CC) -Wall -O2 -fPIC $(PY_INCLUDES) -I../c -c -o $@ $<

../c/kanaco.shared.o: ../c/kanaco.c ../c/kanaco.h
	$(MAKE) -C ../c kanaco.shared.o

$(TARGET): kanaco.o ../c/kanaco.shared.o
	$(CC) -shared -o $@ $^

test: $(TARGET)
	$(PYTEST) -v test_kanaco.py

clean:
	$(RM) -rf kanaco.c *.o *.so __pycache__ .pytest_cache

.PHONY: all test clean
