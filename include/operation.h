#ifndef BITSET_OPERATION_H
#define BITSET_OPERATION_H

#include "bitset.h"
#include "hash.h"

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
} bitset_op;

/**
 * Create a new bitset operation.
 */

bitset_op *bitset_operation_new(bitset *b);

/**
 * Free the bitset operation.
 */

void bitset_operation_free(bitset_op *);

/**
 * Add a bitset + operation to the queue.
 */

void bitset_operation_add(bitset_op *, bitset *, enum bitset_operation);

/**
 * Execute the operation and return the result.
 */

bitset *bitset_operation_exec(bitset_op *);

/**
 * Get the population count of the operation result without using
 * a temporary bitset.
 */

bitset_offset bitset_operation_count(bitset_op *);

#endif

