#ifndef BITSET_VECTOR_H_
#define BITSET_VECTOR_H_

#include "bitset/bitset.h"
#include "bitset/operation.h"

/**
 * Bitset buffers can be packed together into a vector which is compressed
 * using length encoding.  Each vector buffer consists of zero or more bitsets
 * prefixed with their offset and length
 *
 *    <offset1><length1><bitset_buffer1>...<offsetN><lengthN><bitset_bufferN>
 *
 * For example, adding a bitset with a length of 12 bytes at position 3
 * followed by a bitset with a length of 4 bytes at position 12 would result in
 *
 *    <offset=3><length=12><bitset1><offset=9><length=4><bitset2>
 *
 * Offsets and lengths are encoded using the following format
 *
 *    |00xxxxxx| 6 bit length
 *    |01xxxxxx|xxxxxxxx| 14 bit length
 *    |10xxxxxx|xxxxxxxx|xxxxxxxx| 22 bit length
 *    |11xxxxxx|xxxxxxxx|xxxxxxxx|xxxxxxxx| 30 bit length
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Bitset vector types.
 */

typedef struct bitset_vector_ {
    char *buffer;
    size_t length;
    size_t size;
    unsigned count;
    char *tail;
    unsigned tail_offset;
} bitset_vector;

typedef struct bitset_vector_iterator_ {
    bitset **bitsets;
    unsigned *offsets;
    size_t length;
    size_t size;
} bitset_vector_iterator;

typedef struct bitset_vector_operation_ bitset_vector_operation;

typedef struct bitset_vector_operation_step_ {
    union {
        bitset_vector_iterator *i;
        bitset_vector_operation *o;
    } data;
    bool is_nested;
    bool is_operation;
    enum bitset_operation_type type;
} bitset_vector_operation_step;

struct bitset_vector_operation_ {
    bitset_vector_operation_step **steps;
    unsigned min;
    unsigned max;
    size_t length;
};

#define BITSET_VECTOR_START 0
#define BITSET_VECTOR_END 0

/**
 * Create a new bitset vector.
 */

bitset_vector *bitset_vector_new();

/**
 * Create a new bitset vector based on an existing buffer.
 */

bitset_vector *bitset_vector_new_buffer(const char *, size_t);

/**
 * Free the specified vector.
 */

void bitset_vector_free(bitset_vector *);

/**
 * Copy a vector.
 */

bitset_vector *bitset_vector_copy(bitset_vector *);

/**
 * Get the byte length of the vector buffer.
 */

size_t bitset_vector_length(bitset_vector *);

/**
 * Get the number of bitsets in the vector.
 */

unsigned bitset_vector_count(bitset_vector *);

/**
 * Push a bitset on to the end of the vector.
 */

void bitset_vector_push(bitset_vector *, bitset *, unsigned);

/**
 * Resize the vector buffer.
 */

void bitset_vector_resize(bitset_vector *, size_t);

/**
 * Create a new bitset vector iterator over the range [start,end).
 */

bitset_vector_iterator *bitset_vector_iterator_new(bitset_vector *, unsigned, unsigned);

/**
 * Iterate over all bitsets.
 */

#define BITSET_VECTOR_FOREACH(iterator, bitset, offset) \
    for (unsigned BITSET_TMPVAR(i, __LINE__) = 0; \
         (BITSET_TMPVAR(i, __LINE__) < iterator->length \
            ? (offset = iterator->offsets[BITSET_TMPVAR(i, __LINE__)], \
               bitset = iterator->bitsets[BITSET_TMPVAR(i, __LINE__)], 1) : 0); \
         BITSET_TMPVAR(i, __LINE__)++)

/**
 * Concatenate an iterator to another at the specified offset.
 */

void bitset_vector_iterator_concat(bitset_vector_iterator *, bitset_vector_iterator *, unsigned);

/**
 * Count bits in each bitset.
 */

void bitset_vector_iterator_count(bitset_vector_iterator *, unsigned *, unsigned *);

/**
 * Compact the iterator into a vector.
 */

bitset_vector *bitset_vector_iterator_compact(bitset_vector_iterator *);

/**
 * Merge (bitwise OR) each vector bitset.
 */

bitset *bitset_vector_iterator_merge(bitset_vector_iterator *);

/**
 * Free the vector iterator.
 */

void bitset_vector_iterator_free(bitset_vector_iterator *);

/**
 * Create an empty iterator
 */

bitset_vector_iterator *bitset_vector_iterator_new_empty();

/**
 * Create a copy of the iterator.
 */

bitset_vector_iterator *bitset_vector_iterator_copy(bitset_vector_iterator *);

/**
 * Create a new vector operation.
 */

bitset_vector_operation *bitset_vector_operation_new(bitset_vector_iterator *);

/**
 * Free the specified vector operation.
 */

void bitset_vector_operation_free(bitset_vector_operation *);

/**
 * Add a vector to the operation.
 */

void bitset_vector_operation_add(bitset_vector_operation *,
    bitset_vector_iterator *, enum bitset_operation_type);

/**
 * Add a nested operation.
 */

void bitset_vector_operation_add_nested(bitset_vector_operation *,
    bitset_vector_operation *, enum bitset_operation_type);

/**
 * Execute the operation and return the result.
 */

bitset_vector_iterator *bitset_vector_operation_exec(bitset_vector_operation *);

#ifdef __cplusplus
} //extern "C"
#endif

#endif

