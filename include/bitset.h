#ifndef BITSET_H_
#define BITSET_H_

#include "uthash.h"

/*
The bitset structure uses PLWAH compression to encode runs of identical bits.

The encoding is based on two types of 32-bit words which can be identified
by whether the MSB is set:

Literal word: 0XXXXXXX XXXXXXXX XXXXXXXX XXXXXXXX
   Fill word: 1CPPPPPL LLLLLLLL LLLLLLLL LLLLLLLL

X = 31 uncompressed bits
L = a 25-bit length representing a span of "clean" words (all 1's or all 0's)
C = the span's colour (whether it's all 1's or 0's)
P = if the word proceeding the span contains only 1 bit, this 5-bit length
    can be used to determine which bit is set (0x80000000 >> position)
*/

typedef struct bitset_ {
    uint32_t *words;
    unsigned length;
    unsigned size;
} bitset;

enum bitset_operation {
    BITSET_AND,
    BITSET_OR,
    BITSET_XOR,
    BITSET_ANDNOT
};

typedef struct bitset_op_step_ {
    bitset *b;
    enum bitset_operation operation;
} bitset_op_step;

typedef struct bitset_op_ {
    bitset_op_step **steps;
    unsigned length;
} bitset_op;

typedef struct bitset_op_hash_ {
    unsigned long offset;
    uint32_t word;
    UT_hash_handle hh;
} bitset_op_hash;

/**
 * PLWAH compression macros for 32-bit words.
 */

#define BITSET_IS_FILL_WORD(word) ((word) & 0x80000000)
#define BITSET_IS_LITERAL_WORD(word) (((word) & 0x80000000) == 0)

#define BITSET_GET_COLOUR(word) ((word) & 0x40000000)
#define BITSET_SET_COLOUR(word) ((word) | 0x40000000)
#define BITSET_UNSET_COLOUR(word) ((word) & 0xBF000000)

#define BITSET_GET_LENGTH(word) ((word) & 0x01FFFFFF)
#define BITSET_SET_LENGTH(word, len) ((word) | (len))

#define BITSET_GET_POSITION(word) (((word) & 0x3E000000) >> 25)
#define BITSET_SET_POSITION(word, pos) ((word) | ((pos) << 25))
#define BITSET_UNSET_POSITION(word) ((word) & 0xC1FFFFFF)

#define BITSET_CREATE_FILL(len, pos) ((0x80000000 | (((pos % 31) + 1) << 25)) | (len))
#define BITSET_CREATE_EMPTY_FILL(len) (0x80000000 | (len))
#define BITSET_CREATE_LITERAL(bit) (0x40000000 >> (bit))

#define BITSET_MAX(a, b) ((a) > (b) ? (a) : (b));
#define BITSET_MIN(a, b) ((a) < (b) ? (a) : (b));

#define BITSET_GET_LITERAL_MASK(bit) (0x80000000 >> ((bit % 31) + 1))

#define BITSET_MAX_LENGTH (0x01FFFFFF)

#define BITSET_IS_POW2(word) ((word & (word - 1)) == 0)

#define BITSET_NEXT_POW2(dest, src) \
    dest = src; \
    dest--; \
    dest |= dest >> 1; \
    dest |= dest >> 2; \
    dest |= dest >> 4; \
    dest |= dest >> 8; \
    dest |= dest >> 16; \
    dest++;

#if defined(__SSE4_1__) && defined(BITSET_HARDWARE_POPCNT)
#define BITSET_POP_COUNT(count, word) \
    count += __builtin_popcount(word);
#else
#define BITSET_POP_COUNT(count, word) \
    word &= 0x7FFFFFFF; \
    word -= (word >> 1) & 0x55555555; \
    word = (word & 0x33333333) + ((word >> 2) & 0x33333333); \
    word = (word + (word >> 4)) & 0x0F0F0F0F; \
    count += (word * 0x01010101) >> 24;
#endif

/**
 * Custom out of memory behaviour.
 */

#ifndef bitset_oom
#define bitset_oom() \
    fprintf(stderr, "Out of memory\n"); \
    exit(1)
#endif

/**
 * Function prototypes.
 */

bitset *bitset_new();
void bitset_free(bitset *);
void bitset_resize(bitset *, unsigned);
bitset *bitset_new_array(unsigned, uint32_t *);
bitset *bitset_new_bits(unsigned, unsigned long *);
bitset *bitset_copy(bitset *);
bool bitset_get(bitset *, unsigned long);
unsigned long bitset_count(bitset *);
bool bitset_set(bitset *, unsigned long, bool);
unsigned long bitset_fls(bitset *);
bitset_op *bitset_operation_new(bitset *b);
void bitset_operation_free(bitset_op *);
void bitset_operation_add(bitset_op *, bitset *, enum bitset_operation);
bitset *bitset_operation_exec(bitset_op *);
unsigned long bitset_operation_count(bitset_op *);
bitset_op_hash *bitset_operation_iter(bitset_op *);
int bitset_operation_sort(bitset_op_hash *, bitset_op_hash *);
int bitset_new_bits_sort(const void *, const void *);

#endif

