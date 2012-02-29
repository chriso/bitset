#ifndef BITSET_HASH_H
#define BITSET_HASH_H

#include <stdbool.h>

#include "bitset.h"

/**
 * Bitset hash types.
 */

typedef struct bucket_ {
    bitset_offset offset;
    bitset_word word;
    struct bucket_ *next;
} bitset_hash_bucket;

typedef struct hash_ {
    bitset_hash_bucket **buckets;
    unsigned size;
    unsigned count;
} bitset_hash;

/**
 * Create a new hash with the specified number of buckets.
 */

bitset_hash *bitset_hash_new(unsigned);

/**
 * Free the specified hash.
 */

void bitset_hash_free(bitset_hash *);

/**
 * Insert a non-existent offset into the hash.
 */

bool bitset_hash_insert(bitset_hash *, bitset_offset, bitset_word);

/**
 * Replace the word at an offset that exists in the hash.
 */

bool bitset_hash_replace(bitset_hash *, bitset_offset, bitset_word);

/**
 * Get the word at the specified offset.
 */

bitset_word bitset_hash_get(bitset_hash *, bitset_offset);

#endif

