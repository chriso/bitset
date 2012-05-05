CC?=gcc
OPTIMISATION?=-O3
CFLAGS+=-Wall -std=c99 -pedantic $(OPTIMISATION)

SOURCE=src/bitset.c src/operation.c src/probabilistic.c src/vector.c
OBJ=$(SOURCE:.c=.o)

TEST=test/test.o
STRESS=test/stress.o

libbitset: $(OBJ) init
	ar -r lib/libbitset.a $(OBJ)

stress: $(OBJ) $(STRESS) init
	@$(CC) $(LDFLAGS) $(OBJ) $(STRESS) -o ./bin/$@
	@./bin/$@

test: $(OBJ) $(TEST) init
	@$(CC) $(LDFLAGS) $(OBJ) $(TEST) -o ./bin/$@
	@./bin/$@

.c.o:
	$(CC) -c $(CFLAGS) -Iinclude $< -o $@

init:
	@mkdir -p bin lib

clean:
	rm -rf $(OBJ) $(TEST) $(STRESS) bin lib

.PHONY: test stress
