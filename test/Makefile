.PHONY: clean test stress

all: test_o stress_o

test_o:
	$(CC) $(CFLAGS) -L../src -lbitset -o test test.c

stress_o:
	$(CC) $(CFLAGS) -L../src -lbitset -o stress stress.c

test: test_o
	@LD_LIBRARY_PATH=../src ./test

stress: stress_o
	@LD_LIBRARY_PATH=../src ./stress

clean:
	rm -rf *.dSYM *~ *.o $(OBJECTS) stress test

include ../build/env
CFLAGS+=-fPIC