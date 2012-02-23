CC?=gcc
CFLAGS+=-Wall -std=c99 -DBITSET_64BIT_OFFSETS
SOURCES=src/bitset.c test/test.c
OBJ=$(SOURCES:.c=.o)

bitset_test: $(OBJ)
	@$(CC) $(LDFLAGS) $(OBJ) -o $@
	./$@

.c.o:
	@$(CC) -c $(CFLAGS) -Iinclude $< -o $@

clean:
	@rm src/bitset.o test/test.o bitset_test

.PHONY: bitest_test
