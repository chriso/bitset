.PHONY: clean

OBJECTS=bitset.o operation.o probabilistic.o vector.o

all: libbitset.so libbitset.a

libbitset.so: $(OBJECTS)
	$(LD) $(LDFLAGS) -o libbitset.so.$(VERSION) $(OBJECTS)
	@ln -s libbitset.so.$(VERSION) libbitset.so
	@ln -s libbitset.so.$(VERSION) libbitset.so.$(VERSION_MAJOR)
	chmod 744 libbitset.so*

libbitset.a: $(OBJECTS)
	ar rcs libbitset.a $(OBJECTS)

bitset.o:
	$(CC) $(CFLAGS) -c -o bitset.o bitset.c

operation.o:
	$(CC) $(CFLAGS) -c -o operation.o operation.c

probalistic.o:
	$(CC) $(CFLAGS) -c -o probalistic.o probalistic.c

vector.o:
	$(CC) $(CFLAGS) -c -o vector.o vector.c

clean:
	rm -rf *.dSYM *~ *.o $(OBJECTS) libbitset.a libbitset.so*

include ../build/env
CFLAGS+=-fPIC
