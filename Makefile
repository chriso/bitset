test:
	gcc -Wall -std=c99 -o /tmp/bitset_test src/bitset.c test/test.c && /tmp/bitset_test

.PHONY: test

