#ifndef BITSET_LIST_H_
#define BITSET_LIST_H_

/**
 * Bitset buffers can be packed together into a list and compressed using
 * length encoding.  Each list buffer consists of zero or more bitsets
 * prefixed with their offset position and length
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
 * Bitset list types.
 */

typedef struct bitset_list_ {
    char *buffer;
    unsigned length;
    unsigned size;
    unsigned count;
    char *tail;
    unsigned tail_offset;
} bitset_list;

typedef struct bitset_list_iterator_ {
    bitset **bitsets;
    unsigned *offsets;
    unsigned length;
    unsigned size;
} bitset_list_iterator;

#define BITSET_LIST_START 0
#define BITSET_LIST_END 0

/**
 * Create a new bitset list.
 */

bitset_list *bitset_list_new();

/**
 * Create a new bitset list based on an existing buffer.
 */

bitset_list *bitset_list_new_buffer(unsigned, const char *);

/**
 * Free the specified list.
 */

void bitset_list_free(bitset_list *);

/**
 * Get the byte length of the list buffer.
 */

unsigned bitset_list_length(bitset_list *);

/**
 * Get the number of bitsets in the list.
 */

unsigned bitset_list_count(bitset_list *);

/**
 * Push a bitset on to the end of the list.
 */

void bitset_list_push(bitset_list *, bitset *, unsigned);

/**
 * Create a new bitset list iterator over the range [start,end).
 */

bitset_list_iterator *bitset_list_iterator_new(bitset_list *, unsigned, unsigned);

/**
 * Iterate over all bitsets.
 */

#define BITSET_LIST_FOREACH(iterator, bitset, offset) \
    for (unsigned BITSET_TMPVAR(i, __LINE__) = 0; \
         offset = iterator->offsets[BITSET_TMPVAR(i, __LINE__)], \
         bitset = iterator->bitsets[BITSET_TMPVAR(i, __LINE__)], \
         BITSET_TMPVAR(i, __LINE__) < iterator->length; \
         BITSET_TMPVAR(i, __LINE__)++)

/**
 * Concatenate an iterator to another at the specified offset.
 */

void bitset_list_iterator_concat(bitset_list_iterator *, bitset_list_iterator *, unsigned);

/**
 * Count bits in each bitset.
 */

void bitset_list_iterator_count(bitset_list_iterator *, unsigned *, unsigned *);

/**
 * Free the list iterator.
 */

void bitset_list_iterator_free(bitset_list_iterator *);

/**
 * Helpers for foreach macro.
 */

#define BITSET_TMPVAR(i, line) BITSET_TMPVAR_(i, line)
#define BITSET_TMPVAR_(a,b) a__##b

#endif

