#ifndef BITSET_OPERATION_H
#define BITSET_OPERATION_H

#include "bitset/bitset.h"

#ifdef __cplusplus
extern "C" {
#endif

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
    size_t size;
    unsigned count;
} bitset_hash;

/**
 * Bitset operation types.
 */

enum bitset_operation_type {
    BITSET_AND,
    BITSET_OR,
    BITSET_XOR,
    BITSET_ANDNOT
};

typedef struct bitset_operation_ bitset_operation;

typedef struct bitset_operation_step_ {
    union {
        bitset *b;
        bitset_operation *op;
    } data;
    bool is_nested;
    bool is_operation;
    enum bitset_operation_type type;
} bitset_operation_step;

struct bitset_operation_ {
    bitset_operation_step **steps;
    size_t length;
};

/**
 * Create a new bitset operation.
 */

bitset_operation *bitset_operation_new(bitset *b);

/**
 * Free the bitset operation.
 */

void bitset_operation_free(bitset_operation *);

/**
 * Add a bitset to the operation.
 */

void bitset_operation_add(bitset_operation *, bitset *, enum bitset_operation_type);

/**
 * Add a nested operation.
 */

void bitset_operation_add_nested(bitset_operation *, bitset_operation *, enum bitset_operation_type);

/**
 * Execute the operation and return the result.
 */

bitset *bitset_operation_exec(bitset_operation *);

/**
 * Get the population count of the operation result without using
 * a temporary bitset.
 */

bitset_offset bitset_operation_count(bitset_operation *);

#ifdef __cplusplus
} //extern "C"
#endif

#endif

