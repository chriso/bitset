#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>

#include "operation.h"

int main(int argc, char **argv) {

    float start, end, size = 0;
    srand(time(NULL));

    printf("Creating 100k bitsets with 10M total bits between 1->1M\n");
    unsigned bitsets = 100000, bits = 100, max = 1000000;
    bitset **b = malloc(sizeof(bitset *) * bitsets);
    bitset_offset *offsets = malloc(sizeof(bitset_offset) * bits);

    //Create the bitsets
    start = (float) clock();
    for (unsigned i = 0; i < bitsets; i++) {
        for (unsigned j = 0; j < bits; j++) {
            offsets[j] = rand() % max;
        }
        b[i] = bitset_new_bits(bits, offsets);
        size += b[i]->length * sizeof(bitset_word);
    }
    end = ((float) clock() - start) / CLOCKS_PER_SEC;
    size /= 1024 * 1024;
    printf("Created %.2fMB in %.2fs (%.2fMB/s)\n", size, end, size/end);

    //Pop count all bitsets
    start = (float) clock();
    bitset_offset count = 0;
    for (unsigned i = 0; i < bitsets; i++) {
        count += bitset_count(b[i]);
    }
    printf("Bit count => " bitset_format "\n", count);
    end = ((float) clock() - start) / CLOCKS_PER_SEC;
    printf("Executed pop count in %.2fs (%.2fMB/s)\n", end, size/end);

    //Bitwise OR + pop count
    start = (float) clock();
    bitset_op *op = bitset_operation_new(b[0]);
    for (unsigned i = 1; i < bitsets; i++) {
        bitset_operation_add(op, b[i], BITSET_OR);
    }
    printf("Unique bit count => " bitset_format "\n", bitset_operation_count(op));
    end = ((float) clock() - start) / CLOCKS_PER_SEC;
    printf("Executed bitwise OR + pop count operation in %.2fs (%.2fMB/s)\n", end, size/end);

    //Use linear counting
    start = (float) clock();
    bitset_linear *e = bitset_linear_new(1048576);
    for (unsigned i = 0; i < bitsets; i++) {
        bitset_linear_add(e, b[i]);
    }
    printf("Unique bit count => " bitset_format "\n", bitset_linear_count(e));
    end = ((float) clock() - start) / CLOCKS_PER_SEC;
    printf("Executed linear count in %.2fs (%.2fMB/s)\n", end, size/end);
}

