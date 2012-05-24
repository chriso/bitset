#ifndef BITSET_PROBABILISTIC_H
#define BITSET_PROBABILISTIC_H

#include "bitset/bitset.h"

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
 * Bitset count N type.
 */

typedef struct bitset_countn_ {
    bitset_word **words;
    unsigned n;
    size_t size;
} bitset_countn;

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
 * Estimate the number of bits that occur N times.
 */

bitset_countn *bitset_countn_new(unsigned, size_t);

/**
 * Add a bitset to the counter.
 */

void bitset_countn_add(bitset_countn *, bitset *);

/**
 * Count the number of bits that occur N times.
 */

unsigned bitset_countn_count(bitset_countn *);

/**
 * Count the number of bits that occur 0..N times.
 */

unsigned *bitset_countn_count_all(bitset_countn *);

/**
 * Free the counter.
 */

void bitset_countn_free(bitset_countn *);

#ifdef __cplusplus
} //extern "C"
#endif

#endif

