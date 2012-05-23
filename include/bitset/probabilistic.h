#ifndef BITSET_PROBABILISTIC_H
#define BITSET_PROBABILISTIC_H

#include "bitset.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Bitset linear counting type.
 */

typedef struct bitset_linear_ {
    bitset_word *words;
    unsigned count;
    size_t size;
} bitset_linear;

/**
 * Estimate unique bits using an uncompressed bitset of the specified size
 * (bloom filter where n=1).
 */

bitset_linear *bitset_linear_new(size_t);

/**
 * Estimate unique bits in the bitset.
 */

void bitset_linear_add(bitset_linear *, bitset *);

/**
 * Get the estimated unique bit count.
 */

unsigned bitset_linear_count(bitset_linear *);

/**
 * Free the linear counter.
 */

void bitset_linear_free(bitset_linear *);

#ifdef __cplusplus
} //extern "C"
#endif

#endif

