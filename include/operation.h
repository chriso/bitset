#ifndef BITSET_OPERATION_H
#define BITSET_OPERATION_H

#include "bitset.h"

/**
 * Bitset operations.
 */

enum bitset_operation {
    BITSET_AND,
    BITSET_OR,
    BITSET_XOR,
    BITSET_ANDNOT
};

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
    bitset_word *words;
    unsigned size;
    unsigned count;
} bitset_hash;

/**
 * Bitset operation types.
 */

typedef struct bitset_op_step_ {
    bitset *b;
    enum bitset_operation operation;
} bitset_op_step;

typedef struct bitset_op_ {
    bitset_op_step **steps;
    bitset_hash *words;
    unsigned length;
    unsigned bit_count;
    bitset_offset bit_max;
} bitset_op;

/**
 * Create a new bitset operation.
 */

bitset_op *bitset_operation_new(const bitset *b);

/**
 * Free the bitset operation.
 */

void bitset_operation_free(bitset_op *);

/**
 * Add a bitset + operation to the queue.
 */

void bitset_operation_add(bitset_op *, const bitset *, enum bitset_operation);

/**
 * Execute the operation and return the result.
 */

bitset *bitset_operation_exec(bitset_op *);

/**
 * Get the population count of the operation result without using
 * a temporary bitset.
 */

bitset_offset bitset_operation_count(bitset_op *);

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
 * Get the word at the specified offset.
 */

bitset_word *bitset_hash_get(const bitset_hash *, bitset_offset);

#endif

