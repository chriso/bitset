#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include <assert.h>

#include "bitset/operation.h"
#include "bitset/vector.h"
#include "bitset/probabilistic.h"

void stress_vector(unsigned bitsets, unsigned bits, unsigned max) {
    float start, end, size = 0;
    unsigned i;

    bitset **b = malloc(sizeof(bitset *) * bitsets);
    bitset_offset *offsets = malloc(sizeof(bitset_offset) * bits);
    bitset_vector *vector = bitset_vector_new();

    //Create the bitsets
    start = (float) clock();
    for (i = 0; i < bitsets; i++) {
        for (unsigned j = 0; j < bits; j++) {
            offsets[j] = rand() % max;
        }
        b[i] = bitset_new_bits(offsets, bits);
        bitset_vector_push(vector, b[i], i);
        size += b[i]->length * sizeof(bitset_word);
    }
    end = ((float) clock() - start) / CLOCKS_PER_SEC;
    size /= 1024 * 1024;
    printf("Created %.2fMB in %.2fs (%.2fMB/s)\n", size, end, size/end);

    //Popcnt bitsets
    bitset_offset count = 0, ucount = 0;
    bitset_operation *o = bitset_operation_new(NULL);
    for (i = 0; i < bitsets; i++) {
        count += bitset_count(b[i]);
        bitset_operation_add(o, b[i], BITSET_OR);
    }
    ucount = bitset_operation_count(o);

    //Popcnt bitsets using an iterator
    bitset_vector_iterator *iter = bitset_vector_iterator_new(vector,
        BITSET_VECTOR_START, BITSET_VECTOR_END);
    start = (float) clock();
    bitset_offset raw, unique;
    bitset_vector_iterator_count(iter, &raw, &unique);
    end = ((float) clock() - start) / CLOCKS_PER_SEC;
    printf("Counted " bitset_format " unique bits (" bitset_format " expected) from "
        bitset_format " (" bitset_format \
        " expected) bits using an iterator in %.2fs\n", unique, ucount, raw, count, end);
    for (i = 0; i < bitsets; i++) {
        bitset_free(b[i]);
    }
    bitset_vector_iterator_free(iter);
    bitset_vector_free(vector);
    bitset_operation_free(o);
    free(b);
    free(offsets);
}

void stress_exec(unsigned bitsets, unsigned bits, unsigned max) {
    float start, end, size = 0;

    bitset **b = malloc(sizeof(bitset *) * bitsets);
    bitset_offset *offsets = malloc(sizeof(bitset_offset) * bits);

    //Create the bitsets
    start = (float) clock();
    for (unsigned i = 0; i < bitsets; i++) {
        for (unsigned j = 0; j < bits; j++) {
            offsets[j] = rand() % max;
        }
        b[i] = bitset_new_bits(offsets, bits);
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
    bitset_operation *op = bitset_operation_new(b[0]);
    for (unsigned i = 1; i < bitsets; i++) {
        bitset_operation_add(op, b[i], BITSET_OR);
    }
    printf("Unique bit count => " bitset_format "\n", bitset_operation_count(op));
    end = ((float) clock() - start) / CLOCKS_PER_SEC;
    printf("Executed bitwise OR + pop count operation in %.2fs (%.2fMB/s)\n", end, size/end);
    printf("Unique bit count => " bitset_format "\n", bitset_count(bitset_operation_exec(op)));
    end = ((float) clock() - start) / CLOCKS_PER_SEC;
    printf("Executed bitwise OR into temporary + pop count operation in %.2fs (%.2fMB/s)\n", end, size/end);
    bitset_operation_free(op);

    //Use linear counting
    start = (float) clock();
    bitset_linear *e = bitset_linear_new(330000000);
    for (unsigned i = 0; i < bitsets; i++) {
        bitset_linear_add(e, b[i]);
    }
    printf("Unique bit count => " bitset_format "\n", bitset_linear_count(e));
    end = ((float) clock() - start) / CLOCKS_PER_SEC;
    printf("Executed linear count in %.2fs (%.2fMB/s)\n", end, size/end);
    bitset_linear_free(e);

    for (unsigned i = 0; i < bitsets; i++) {
        bitset_free(b[i]);
    }
    free(b);
    free(offsets);
}

int main(int argc, char **argv) {
    srand(1);

    printf("Creating 100k bitsets with 10M total bits between 1->1M\n");
    stress_exec(100000, 100, 1000000);

    printf("\nCreating 100k bitsets with 10M total bits between 1->10M\n");
    stress_exec(100000, 100, 10000000);

    printf("\nStress testing vector with 100k bitsets\n");
    stress_vector(100000, 100, 10000000);

    printf("\nCreating 1M bitsets with 100M total bits between 1->100M\n");
    stress_exec(1000000, 100, 100000000);
}

