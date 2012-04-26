#ifndef list_H_
#define list_H_

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

typedef struct bitset_list_iter_ {
    bitset_list *c;
    bitset **bitsets;
    unsigned *offsets;
    unsigned count;
} bitset_list_iter;

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
 * Create a new bitset list iterator using the specified start/end offsets.
 */

bitset_list_iter *bitset_list_iter_new(bitset_list *);

/**
 * Iterate over all bitsets.
 */

#define BITSET_LIST_FOREACH(iterator, bitset, offset) \
    for (unsigned TMPVAR(i, __LINE__) = 0; \
         offset = iterator->offsets[TMPVAR(i, __LINE__)], \
         bitset = iterator->bitsets[TMPVAR(i, __LINE__)], \
         TMPVAR(i, __LINE__) < iterator->count; \
         TMPVAR(i, __LINE__)++)

/**
 * Free the list iterator.
 */

void bitset_list_iter_free(bitset_list_iter *);

/**
 * Helpers for foreach macro.
 */

#define TMPSTRINGIFY(a,b) a__##b
#define TMPVAR(i, line) TMPSTRINGIFY(i, line)

#endif

