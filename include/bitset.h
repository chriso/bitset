#ifndef BITSET_H_
#define BITSET_H_

#include <stdint.h>
#include "uthash.h"

/**
 * The bitset structure uses a form of word-aligned run-length encoding.
 *
 * The compression technique is optimised for sparse bitsets where runs of
 * clean words are typically followed by a word with only one set bit. We
 * can exploit the fact that runs are usually less than 2^25 words long and
 * use the extra space in the previous word to encode the position of this bit.
 *
 * There are two types of words identified by the most significant bit
 *
 *     Literal word: 0XXXXXXX XXXXXXXX XXXXXXXX XXXXXXXX
 *        Fill word: 1CPPPPPL LLLLLLLL LLLLLLLL LLLLLLLL
 *
 * X = Uncompressed bits
 * L = represents the length of the span of clean words
 * C = the colour of the span (all 1's or 0's)
 * P = if the word proceeding the span contains only 1 bit, this 5-bit length
 *     stores the position of the bit so that the next literal can be omitted
 */

#define bitset_word                    uint32_t

#define BITSET_WORD_LENGTH             (sizeof(bitset_word) * 8)
#define BITSET_POSITION_LENGTH         BITSET_LOG2(BITSET_WORD_LENGTH)
#define BITSET_FILL_BIT                (1 << (BITSET_WORD_LENGTH - 1))
#define BITSET_COLOUR_BIT              (1 << (BITSET_WORD_LENGTH - 2))
#define BITSET_SPAN_LENGTH             (BITSET_WORD_LENGTH - BITSET_POSITION_LENGTH - 2)
#define BITSET_POSITION_MASK           (((1 << (BITSET_POSITION_LENGTH)) - 1) << (BITSET_SPAN_LENGTH))
#define BITSET_LENGTH_MASK             ((1 << (BITSET_SPAN_LENGTH)) - 1)
#define BITSET_LITERAL_LENGTH          (BITSET_WORD_LENGTH - 1)

#define BITSET_IS_FILL_WORD(word)      ((word) & BITSET_FILL_BIT)
#define BITSET_IS_LITERAL_WORD(word)   (((word) & BITSET_FILL_BIT) == 0)
#define BITSET_GET_COLOUR(word)        ((word) & BITSET_COLOUR_BIT)
#define BITSET_SET_COLOUR(word)        ((word) | BITSET_COLOUR_BIT)
#define BITSET_UNSET_COLOUR(word)      ((word) & ~BITSET_COLOUR_BIT)
#define BITSET_GET_LENGTH(word)        ((word) & BITSET_LENGTH_MASK)
#define BITSET_SET_LENGTH(word, len)   ((word) | (len))
#define BITSET_GET_POSITION(word)      (((word) & BITSET_POSITION_MASK) >> BITSET_SPAN_LENGTH)
#define BITSET_SET_POSITION(word, pos) ((word) | ((pos) << BITSET_SPAN_LENGTH))
#define BITSET_UNSET_POSITION(word)    ((word) & ~BITSET_POSITION_MASK)
#define BITSET_CREATE_FILL(len, pos)   BITSET_SET_POSITION(BITSET_FILL_BIT | (len), (pos) + 1)
#define BITSET_CREATE_EMPTY_FILL(len)  (BITSET_FILL_BIT | (len))
#define BITSET_CREATE_LITERAL(bit)     (BITSET_COLOUR_BIT >> (bit))
#define BITSET_MAX_LENGTH              BITSET_LENGTH_MASK

#define BITSET_MAX(a, b)               ((a) > (b) ? (a) : (b));
#define BITSET_MIN(a, b)               ((a) < (b) ? (a) : (b));
#define BITSET_IS_POW2(word)           ((word & (word - 1)) == 0)
#define BITSET_LOG2(v)                 (8 - 90/(((v)/4+14)|1) - 2/((v)/2+1))
#define BITSET_NEXT_POW2(d,s)          d=s;d--;d|=d>>1;d|=d>>2;d|=d>>4;d|=d>>8;d|=d>>16;d++;
#define BITSET_FLS(word)               ((BITSET_LITERAL_LENGTH) - fls(word))
#define BITSET_POP_COUNT(c,w)          w&=P1;w-=(w>>1)&P2;w=(w&P3)+((w>>2)&P3);w=(w+(w>>4))\
                                       &P4;c+=(w*P5)>>(BITSET_WORD_LENGTH-8);
#define P1 0x7FFFFFFF
#define P2 0x55555555
#define P3 0x33333333
#define P4 0x0F0F0F0F
#define P5 0x01010101

/**
 * 64-bit offsets are supported.
 */

#ifndef BITSET_64BIT_OFFSETS
#  define bitset_offset uint32_t
#else
#  define bitset_offset uint64_t
#endif

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
 * Bitset types.
 */

typedef struct bitset_ {
    bitset_word *words;
    unsigned length;
    unsigned size;
} bitset;

typedef struct bitset_op_step_ {
    bitset *b;
    enum bitset_operation operation;
} bitset_op_step;

typedef struct bitset_op_ {
    bitset_op_step **steps;
    unsigned length;
} bitset_op;

typedef struct bitset_op_hash_ {
    bitset_offset offset;
    bitset_word word;
    UT_hash_handle hh;
} bitset_op_hash;

/**
 * Create a new bitset.
 */

bitset *bitset_new();

/**
 * Free the specified bitset.
 */

void bitset_free(bitset *);

/**
 * Create a new bitset from an array of compressed words.
 */

bitset *bitset_new_array(unsigned, bitset_word *);

/**
 * Create a new bitset from an array of bits.
 */

bitset *bitset_new_bits(unsigned, bitset_offset *);

/**
 * Create a copy of the specified bitset.
 */

bitset *bitset_copy(bitset *);

/**
 * Check whether a bit is set.
 */

bool bitset_get(bitset *, bitset_offset);

/**
 * Get the population count of the bitset (number of set bits).
 */

bitset_offset bitset_count(bitset *);

/**
 * Set or unset the specified bit.
 */

bool bitset_set(bitset *, bitset_offset, bool);

/**
 * Find the lowest set bit.
 */

bitset_offset bitset_fls(bitset *);

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

/**
 * Custom out of memory behaviour.
 */

#ifndef bitset_oom
#  define bitset_oom() fprintf(stderr, "Out of memory\n"); exit(1)
#endif

#endif

