.PHONY: clean

OBJECTS=bitset.o operation.o probabilistic.o vector.o

all: libbitset.so libbitset.a

libbitset.so: $(OBJECTS)
	$(B_LD) $(LDFLAGS) -o libbitset.so.$(VERSION) $(OBJECTS)
	@ln -s libbitset.so.$(VERSION) libbitset.so
	@ln -s libbitset.so.$(VERSION) libbitset.so.$(VERSION_MAJOR)
	@chmod 744 libbitset.so*

libbitset.a: $(OBJECTS)
	@ar rcs libbitset.a $(OBJECTS)

bitset.o:
	$(B_CC) $(CFLAGS) -c -o bitset.o bitset.c

operation.o:
	$(B_CC) $(CFLAGS) -c -o operation.o operation.c

probabilistic.o:
	$(B_CC) $(CFLAGS) -c -o probabilistic.o probabilistic.c

vector.o:
	$(B_CC) $(CFLAGS) -c -o vector.o vector.c

clean:
	@rm -rf *.dSYM *~ *.o $(OBJECTS) libbitset.*
	@echo " RM: $(OBJECTS) libbitset.*"

include ../build/env
CFLAGS+=-fPIC
