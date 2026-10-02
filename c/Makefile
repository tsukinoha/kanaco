include ../common.mk

all: libkanaco.a libkanaco.so

%.static.o: %.c kanaco.h
	$(CC) $(CFLAGS) -c -o $@ $<

%.shared.o: %.c kanaco.h
	$(CC) $(CFLAGS) -fPIC -c -o $@ $<

libkanaco.a: kanaco.static.o
	$(AR) rcs $@ $^

libkanaco.so: kanaco.shared.o
	$(CC) -shared -o $@ $^

test_kanaco: test_kanaco.c libkanaco.a
	$(CC) $(CFLAGS) -o $@ $< libkanaco.a

test: test_kanaco
	./test_kanaco ../data

clean:
	$(RM) -f *.o *.a *.so test_kanaco

.PHONY: all test clean
