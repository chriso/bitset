#ifndef BITSET_VECTOR_H_
#define BITSET_VECTOR_H_

/**
 * Bitset buffers can be packed together into a vector which is compressed
 * using length encoding.  Each vector buffer consists of zero or more bitsets
 * prefixed with their offset and length
 *
 *    <offset1><length1><bitset_buffer1><offsetN><lengthN><bitset_bufferN>
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
 * Create a new bitset vector iterator over the range [start,end).
 */

bitset_vector_iterator *bitset_vector_iterator_new(bitset_vector *, unsigned, unsigned);

/**
 * Iterate over all bitsets.
 */

#define BITSET_VECTOR_FOREACH(iterator, bitset, offset) \
    for (unsigned BITSET_TMPVAR(i, __LINE__) = 0; \
         offset = iterator->offsets[BITSET_TMPVAR(i, __LINE__)], \
         bitset = iterator->bitsets[BITSET_TMPVAR(i, __LINE__)], \
         BITSET_TMPVAR(i, __LINE__) < iterator->length; \
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
 * Free the vector iterator.
 */

void bitset_vector_iterator_free(bitset_vector_iterator *);

/**
 * Helpers for foreach macro.
 */

#define BITSET_TMPVAR(i, line) BITSET_TMPVAR_(i, line)
#define BITSET_TMPVAR_(a,b) a__##b

#endif

