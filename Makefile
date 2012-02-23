test:
	@gcc -Wall -std=c99 -DBITSET_64BIT_OFFSETS -Iinclude -o /tmp/bitset_test src/bitset.c test/test.c && /tmp/bitset_test

.PHONY: test

