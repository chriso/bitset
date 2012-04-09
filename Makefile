CC?=gcc
CFLAGS+=-Wall -std=c99

SOURCE=src/bitset.c src/operation.c src/probabilistic.c
OBJ=$(SOURCE:.c=.o)

TEST=test/test.o
STRESS=test/stress.o

stress: $(OBJ) $(STRESS)
	$(CC) $(LDFLAGS) $(OBJ) $(STRESS) -o ./bin/$@
	./bin/$@

test: $(OBJ) $(TEST)
	$(CC) $(LDFLAGS) $(OBJ) $(TEST) -o ./bin/$@
	./bin/$@

.c.o: init
	$(CC) -c $(CFLAGS) -Iinclude $< -o $@

init:
	test -d bin || mkdir bin

clean:
	rm -rf $(OBJ) $(TEST) $(STRESS) bin


.PHONY: test stress
.SILENT: 
