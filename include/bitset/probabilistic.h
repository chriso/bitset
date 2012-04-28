#ifndef BITSET_PROBABILISTIC_H
#define BITSET_PROBABILISTIC_H

#include "bitset.h"

/**
 * Bitset linear counting type.
 */

typedef struct bitset_linear_ {
    bitset_word *words;
    unsigned count;
    size_t size;
} bitset_linear;

/**
 * Bitset HyperLogLog type.
 */

typedef struct bitset_loglog_ {
    bitset_word *words;
    unsigned count;
    size_t size;
} bitset_loglog;

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

/**
 * Estimate unique bits using the efficient HyperLogLog method.
 * See: algo.inria.fr/flajolet/Publications/FlFuGaMe07.pdf
 */

bitset_loglog *bitset_loglog_new(size_t);

/**
 * Estimate unique bits in the bitset.
 */

void bitset_loglog_add(bitset_loglog *, bitset *);

/**
 * Get the estimated unique bit count.
 */

unsigned bitset_loglog_count(bitset_loglog *);

/**
 * Free the loglog counter.
 */

void bitset_loglog_free(bitset_loglog *);

#endif

