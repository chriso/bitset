.PHONY: clean test stress

all: test_o stress_o

test_o:
	$(B_CC) $(CFLAGS) -L../lib -lbitset -o test test.c

stress_o:
	$(B_CC) $(CFLAGS) -L../lib -lbitset -o stress stress.c

test: test_o
	@DYLD_LIBRARY_PATH=../lib LD_LIBRARY_PATH=../lib ./test

stress: stress_o
	@DYLD_LIBRARY_PATH=../lib LD_LIBRARY_PATH=../lib ./stress

clean:
	@rm -rf *.dSYM *~ *.o $(OBJECTS) stress test
	@echo " RM: *.dSYM *~ $(OBJECTS) stress test"

include ../build/env
CFLAGS+=-fPIC
