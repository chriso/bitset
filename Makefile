CC?=gcc
CFLAGS+=-Wall -std=c99

SOURCE=src/bitset.c
OBJ=$(SOURCE:.c=.o)

TEST=test/test.o
STRESS=test/stress.o

$(shell [ -d "bin" ] || mkdir bin)

stress: $(OBJ) $(STRESS)
	@$(CC) $(LDFLAGS) $(OBJ) $(STRESS) -o ./bin/$@
	@./bin/$@

test: $(OBJ) $(TEST)
	@$(CC) $(LDFLAGS) $(OBJ) $(TEST) -o ./bin/$@
	@./bin/$@

.c.o:
	@$(CC) -c $(CFLAGS) -Iinclude $< -o $@

clean:
	@rm -rf $(OBJ) $(TEST) $(STRESS) bin

.PHONY: test stress
