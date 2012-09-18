#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include <assert.h>

#include "bitset/operation.h"
#include "bitset/vector.h"
#include "bitset/estimate.h"

/**
 * Bundle a PRNG to get around dists with a tiny RAND_MAX.
 */

static unsigned bitset_rand_seed = 1;
static inline unsigned bitset_rand() {
    bitset_rand_seed = bitset_rand_seed * 1103515245 + 12345;
    return bitset_rand_seed;
}

void stress_small(unsigned bits, unsigned max, unsigned count) {
    float start, end;

    bitset_t *b, *b2, *b3, *b4;
    bitset_operation_t *o;
    bitset_offset *offsets = bitset_malloc(sizeof(bitset_offset) * bits);

    start = (float) clock();
    for (size_t j = 0; j < count; j++) {
        for (size_t j = 0; j < bits; j++) {
            offsets[j] = bitset_rand() % max;
        }
        b = bitset_new_bits(offsets, bits);
        for (size_t j = 0; j < bits; j++) {
            offsets[j] = bitset_rand() % max;
        }
        b2 = bitset_new_bits(offsets, bits);
        for (size_t j = 0; j < bits; j++) {
            offsets[j] = bitset_rand() % max;
        }
        b3 = bitset_new_bits(offsets, bits);
        o = bitset_operation_new(b);
        bitset_operation_add(o, b2, BITSET_AND);
        bitset_operation_add(o, b3, BITSET_OR);
        b4 = bitset_operation_exec(o);
        bitset_operation_free(o);
        bitset_free(b4);
        bitset_free(b);
        bitset_free(b2);
        bitset_free(b3);
    }
    end = ((float) clock() - start) / CLOCKS_PER_SEC;
    printf("Executed small ops in %.2fs\n", end);
}

void stress_vector(unsigned bitsets, unsigned bits, unsigned max) {
    float start, end, size = 0;
    size_t i;

    bitset_t **b = bitset_malloc(sizeof(bitset_t *) * bitsets);
    bitset_offset *offsets = bitset_malloc(sizeof(bitset_offset) * bits);
    bitset_vector_t *vector = bitset_vector_new();

    //Create the bitsets
    start = (float) clock();
    for (i = 0; i < bitsets; i++) {
        for (size_t j = 0; j < bits; j++) {
            offsets[j] = bitset_rand() % max;
        }
        b[i] = bitset_new_bits(offsets, bits);
        bitset_vector_push(vector, b[i], i);
        size += b[i]->length * sizeof(bitset_word);
    }
    end = ((float) clock() - start) / CLOCKS_PER_SEC;
    size /= 1024 * 1024;
    printf("Created %.2fMB in %.2fs (%.2fMB/s)\n", size, end, size/end);

    //Popcnt bitsets
    unsigned count = 0, ucount = 0;
    bitset_operation_t *o = bitset_operation_new(NULL);
    for (i = 0; i < bitsets; i++) {
        count += bitset_count(b[i]);
        bitset_operation_add(o, b[i], BITSET_OR);
    }
    ucount = bitset_operation_count(o);

    //Popcnt all bitsets
    unsigned raw, unique;
    start = (float) clock();
    bitset_vector_cardinality(vector, &raw, &unique);
    end = ((float) clock() - start) / CLOCKS_PER_SEC;
    printf("Counted %u unique bits (%u expected) from "
        "%u (%u expected) bits using an iterator in %.2fs\n", unique, ucount, raw, count, end);
    for (i = 0; i < bitsets; i++) {
        bitset_free(b[i]);
    }
    bitset_vector_free(vector);
    bitset_operation_free(o);
    bitset_malloc_free(b);
    bitset_malloc_free(offsets);
}

void stress_exec(unsigned bitsets, unsigned bits, unsigned max) {
    float start, end, size = 0;

    bitset_t **b = bitset_malloc(sizeof(bitset_t *) * bitsets);
    bitset_offset *offsets = bitset_malloc(sizeof(bitset_offset) * bits);

    //Create the bitsets
    start = (float) clock();
    for (size_t i = 0; i < bitsets; i++) {
        for (size_t j = 0; j < bits; j++) {
            offsets[j] = bitset_rand() % max;
        }
        b[i] = bitset_new_bits(offsets, bits);
        size += b[i]->length * sizeof(bitset_word);
    }
    end = ((float) clock() - start) / CLOCKS_PER_SEC;
    size /= 1024 * 1024;
    printf("Created %.2fMB in %.2fs (%.2fMB/s)\n", size, end, size/end);

    //Estimate count based on size
    start = (float) clock();
    bitset_offset count = 0;
    for (size_t i = 0; i < bitsets; i++) {
        count += b[i]->length;
    }
    printf("Estimated bit count => " bitset_format "\n", count);
    end = ((float) clock() - start) / CLOCKS_PER_SEC;
    printf("Executed count estimate in %.2fs (%.2fMB/s)\n", end, size/end);

    //Pop count all bitsets
    start = (float) clock();
    count = 0;
    for (size_t i = 0; i < bitsets; i++) {
        count += bitset_count(b[i]);
    }
    printf("Bit count => " bitset_format "\n", count);
    end = ((float) clock() - start) / CLOCKS_PER_SEC;
    printf("Executed pop count in %.2fs (%.2fMB/s)\n", end, size/end);

    //Bitwise OR + pop count
    start = (float) clock();
    bitset_operation_t *op = bitset_operation_new(b[0]);
    for (size_t i = 1; i < bitsets; i++) {
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
    bitset_linear_t *e = bitset_linear_new(max);
    for (size_t i = 0; i < bitsets; i++) {
        bitset_linear_add(e, b[i]);
    }
    printf("Unique bit count => %u\n", bitset_linear_count(e));
    end = ((float) clock() - start) / CLOCKS_PER_SEC;
    printf("Executed linear count in %.2fs (%.2fMB/s)\n", end, size/end);
    bitset_linear_free(e);

    for (size_t i = 0; i < bitsets; i++) {
        bitset_free(b[i]);
    }
    bitset_malloc_free(b);
    bitset_malloc_free(offsets);
}

int main(int argc, char **argv) {
    printf("Testing 100k small operations\n");
    stress_small(10, 1000000, 100000);

    printf("\nCreating 100k bitsets with 10M total bits between 1->1M\n");
    stress_exec(100000, 100, 1000000);

    printf("\nCreating 100k bitsets with 10M total bits between 1->10M\n");
    stress_exec(100000, 100, 10000000);

    printf("\nStress testing vector with 100k bitsets and 10M bits\n");
    stress_vector(100000, 100, 10000000);

    printf("\nCreating 1M bitsets with 100M total bits between 1->100M\n");
    stress_exec(1000000, 100, 100000000);
}

