//&>/dev/null;x="${0%.*}";[ ! "/tmp/$x" -ot "$0" ]||(rm -f "/tmp/$x";gcc -Wall -DNDEBUG -O3 -std=c99 -o "/tmp/$x" "$0")&&"/tmp/$x" $*;exit

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include "bitset.h"

void test_suite_macros();
void test_suite_get();
void test_suite_set();
void test_suite_count();
void test_suite_operation();

void test_bool(char *, bool, bool);
void test_ulong(char *, unsigned long, unsigned long);
bool test_bitset(bitset *, unsigned, uint32_t *);
void bitset_dump(bitset *);

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
    holds the position of the bit from the right
*/

bool bitset_set(bitset *b, unsigned long bit, bool value) {
    //TODO
    return false;
}

void test_suite_set() {
    //TODO
}

unsigned long bitset_ffs(bitset *b) {
    if (!b->length) {
        return 0;
    }
    //TODO
    return 0;
}

/**
 * Bitset operations.
 * ----------------------------------------------------------------------------
 */

bitset_op *bitset_operation_new() {
    bitset_op *ops = BITSET_MALLOC(sizeof(bitset_op));
    if (!ops) {
        BITSET_OOM;
    }
    ops->length = 0;
    return ops;
}

void bitset_operation_free(bitset_op *ops) {
    if (ops->length) {
        BITSET_FREE(ops->ops);
        BITSET_FREE(ops->bitsets);
    }
    BITSET_FREE(ops);
}

void bitset_operation_add(bitset_op *ops, bitset *b, enum bitset_operation op) {
    if (ops->length % 2 == 0) {
        if (!ops->length) {
            ops->bitsets = BITSET_MALLOC(sizeof(bitset *) * 2);
            ops->ops = BITSET_MALLOC(sizeof(enum bitset_operation) * 2);
        } else {
            ops->bitsets = BITSET_REALLOC(ops->bitsets, sizeof(bitset *) * ops->length * 2);
            ops->ops = BITSET_REALLOC(ops->ops, sizeof(enum bitset_operation) * ops->length * 2);
        }
        if (!ops->bitsets || !ops->ops) {
            BITSET_OOM;
        }
    }
    ops->bitsets[ops->length] = b;
    ops->ops[ops->length++] = op;
}

bitset *bitset_operation_exec(bitset_op *ops) {
    //TODO
    return NULL;
}

long bitset_operation_count(bitset_op *ops) {
    //TODO
    return 0;
}

void test_suite_operation() {
    //TODO
}

/**
 * Complete
 * ----------------------------------------------------------------------------
 */

bitset *bitset_new() {
    bitset *b = BITSET_MALLOC(sizeof(bitset));
    if (!b) {
        BITSET_OOM;
    }
    b->length = 0;
    return b;
}

bitset *bitset_new_array(unsigned length, uint32_t *words) {
    bitset *b = BITSET_MALLOC(sizeof(bitset));
    if (!b) {
        BITSET_OOM;
    }
    b->words = malloc(length * sizeof(uint32_t));
    if (!b->words) {
        BITSET_OOM;
    }
    memcpy(b->words, words, length * sizeof(uint32_t));
    b->length = length;
    return b;
}

void bitset_free(bitset *b) {
    if (b->length) {
        BITSET_FREE(b->words);
    }
    BITSET_FREE(b);
}

void bitset_resize(bitset *b, unsigned length) {
    if (length > b->length) {
        if (!b->length) {
            b->words = BITSET_MALLOC(sizeof(uint32_t) * length);
        } else {
            b->words = BITSET_REALLOC(b->words, sizeof(uint32_t) * length);
        }
        if (!b->words) {
            BITSET_OOM;
        }
    }
}

bitset *bitset_copy(bitset *b) {
    bitset *replica = BITSET_MALLOC(sizeof(bitset));
    if (!replica) {
        BITSET_OOM;
    }
    replica->words = BITSET_MALLOC(sizeof(unsigned) * b->length);
    if (!replica->words) {
        BITSET_OOM;
    }
    memcpy(replica->words, b->words, b->length * sizeof(uint32_t));
    replica->length = b->length;
    return replica;
}

bool bitset_get(bitset *b, unsigned long bit) {
    if (!b->length) {
        return false;
    }

    uint32_t word, *words = b->words;
    long word_offset = bit / 31;
    unsigned short position;

    for (unsigned i = 0; i < b->length; i++) {
        word = words[i];
        if (BITSET_IS_FILL_WORD(word)) {
            word_offset -= BITSET_GET_LENGTH(word);
            if (word_offset < 0) {
                return BITSET_GET_COLOUR(word);
            } else if (!word_offset) {
                position = BITSET_GET_POSITION(word);
                if (position) {
                    if (position - 1 == bit % 31) {
                        return !BITSET_GET_COLOUR(word);
                    } else if (BITSET_GET_COLOUR(word)) {
                        return true;
                    }
                    word_offset--;
                }
            }
        } else {
            return word & (0x80000000 >> ((bit % 31) + 1));
        }
    }
    return false;
}

//http://en.wikipedia.org/wiki/Hamming_weight#Efficient_implementation
//http://www.strchr.com/crc32_popcnt

unsigned long bitset_count(bitset *b) {
    unsigned long count = 0;
    uint32_t word, *words = b->words;
    for (unsigned i = 0; i < b->length; i++) {
        word = words[i];
        if (BITSET_IS_FILL_WORD(word)) {
            if (BITSET_GET_POSITION(word)) {
                if (BITSET_GET_COLOUR(word)) {
                    count += BITSET_GET_LENGTH(word) * 31;
                    count += 30;
                } else {
                    count += 1;
                }
            } else if (BITSET_GET_COLOUR(word)) {
                count += BITSET_GET_LENGTH(word) * 31;
            }
        } else {
            word &= 0x7FFFFFFF;
            word -= (word >> 1) & 0x55555555;
            word = (word & 0x33333333) + ((word >> 2) & 0x33333333);
            word = (word + (word >> 4)) & 0x0F0F0F0F;
            count += (word * 0x01010101) >> 24;
        }
    }
    return count;
}

/**
 * Test utils
 * ----------------------------------------------------------------------------
 */

void bitset_dump(bitset *b) {
    printf("\x1B[33mDumping bitset of size %d\x1B[0m\n", b->length);
    for (unsigned i = 0; i < b->length; i++) {
        printf("\x1B[36m%3d.\x1B[0m %-8x\n", i, b->words[i]);
    }
}

#define TEST_DEFINE(pref, type, format) \
    void test_##pref(char *title, type expected, type value) { \
        if (value != expected) { \
            printf("\x1B[31m%s\x1B[0m\n", title); \
            printf("   expected '" format "', got '" format "'\n", expected, value); \
            exit(1); \
        } \
    }

TEST_DEFINE(int, int, "%d");
TEST_DEFINE(ulong, unsigned long, "%lu");
TEST_DEFINE(bool, bool, "%d");
TEST_DEFINE(str, char *, "%s");
TEST_DEFINE(hex, int, "%#x");

bool test_bitset(bitset *b, unsigned length, uint32_t *expected) {
    bool mismatch = length != b->length;
    if (!mismatch) {
        for (unsigned i = 0; i < length; i++) {
            if (b->words[i] != expected[i]) {
                mismatch = true;
                break;
            }
        }
    }
    if (mismatch) {
        unsigned length_max = BITSET_MAX(b->length, length);
        printf("\x1B[31mBitset mismatch\x1B[0m\n");
        for (unsigned i = 0; i < length_max; i++) {
            printf("  \x1B[36m%3d.\x1B[0m ", i);
            if (i < b->length) {
                printf("%-8x ", b->words[i]);
            } else {
                printf("         ");
            }
            if (i < length) {
                printf("\x1B[32m%-8x\x1B[0m", expected[i]);
            }
            putchar('\n');
        }
    }
    return !mismatch;
}

/**
 * Tests
 * ----------------------------------------------------------------------------
 */

int main(int argc, char **argv) {
    printf("Testing macros\n");
    test_suite_macros();
    printf("Testing get\n");
    test_suite_get();
    printf("Testing set\n");
    test_suite_set();
    printf("Testing count\n");
    test_suite_count();
    printf("Testing operations\n");
    test_suite_operation();
}

void test_suite_macros() {
    test_bool("Testing fill word is identified correctly 1\n", true, BITSET_IS_FILL_WORD(0xFFFFFFFF));
    test_bool("Testing fill word is identified correctly 2\n", true, BITSET_IS_FILL_WORD(0xC0372187));
    test_bool("Testing fill word is identified correctly 3\n", false, BITSET_IS_FILL_WORD(0x40367127));
    test_bool("Testing literal word is identified correctly 1\n", false, BITSET_IS_LITERAL_WORD(0xFFFFFFFF));
    test_bool("Testing literal word is identified correctly 2\n", false, BITSET_IS_LITERAL_WORD(0xC0372187));
    test_bool("Testing literal word is identified correctly 3\n", true, BITSET_IS_LITERAL_WORD(0x40367127));
    test_bool("Test the colour bit is identified correctly 1\n", true, BITSET_GET_COLOUR(0xC0000000));
    test_bool("Test the colour bit is identified correctly 2\n", false, BITSET_GET_COLOUR(0x20123217));

    test_ulong("Test fill length is extracted correctly 1\n", 3, BITSET_GET_LENGTH(0x00000003));
    test_ulong("Test fill length is extracted correctly 2\n", 0x01FFFFFF, BITSET_GET_LENGTH(0xFFFFFFFF));
    test_ulong("Test position is extracted correctly 1\n", 1, BITSET_GET_POSITION(0x02000000));
    test_ulong("Test position is extracted correctly 2\n", 31, BITSET_GET_POSITION(0x3E000000));

    test_ulong("Test set colour 1\n", 0xC0000000, BITSET_SET_COLOUR(0x80000000));
    test_ulong("Test set colour 2\n", 0x80000000, BITSET_UNSET_COLOUR(0xC0000000));
    test_ulong("Test set length 1\n", 0x00000001, BITSET_SET_LENGTH(0x00000000, 1));
    test_ulong("Test set length 2\n", 0x0001E240, BITSET_SET_LENGTH(0x00000000, 123456));
    test_ulong("Test set position 1\n", 0x3E000000, BITSET_SET_POSITION(0x00000000, 31));
    test_ulong("Test clear position 1\n", 0x00000000, BITSET_UNSET_POSITION(0x3E000000));
}

void test_suite_get() {
    bitset *b = bitset_new();
    for (unsigned i = 0; i < 32; i++)
        test_bool("Testing initial bits are unset\n", false, bitset_get(b, i));
    bitset_free(b);

    uint32_t p1[] = { 0x80000000, 0x00000001 };
    b = bitset_new_array(2, p1);
    test_bool("Testing get in the first literal 1\n", true, bitset_get(b, 30));
    test_bool("Testing get in the first literal 2\n", false, bitset_get(b, 31));
    bitset_free(b);

    uint32_t p2[] = { 0x80000000, 0x40000000 };
    b = bitset_new_array(2, p2);
    test_bool("Testing get in the first literal 3\n", true, bitset_get(b, 0));
    test_bool("Testing get in the first literal 4\n", false, bitset_get(b, 1));
    bitset_free(b);

    uint32_t p3[] = { 0x80000001, 0x40000000 };
    b = bitset_new_array(2, p3);
    test_bool("Testing get in the first literal with offset 1\n", false, bitset_get(b, 1));
    test_bool("Testing get in the first literal with offset 2\n", true, bitset_get(b, 31));
    bitset_free(b);

    uint32_t p4[] = { 0x80000001, 0x80000001, 0x40000000 };
    b = bitset_new_array(3, p4);
    test_bool("Testing get in the first literal with offset 4\n", false, bitset_get(b, 0));
    test_bool("Testing get in the first literal with offset 5\n", false, bitset_get(b, 31));
    test_bool("Testing get in the first literal with offset 6\n", true, bitset_get(b, 62));
    bitset_free(b);

    uint32_t p5[] = { 0x82000001 };
    b = bitset_new_array(1, p5);
    test_bool("Testing get with position following a fill 1\n", false, bitset_get(b, 0));
    test_bool("Testing get with position following a fill 2\n", true, bitset_get(b, 31));
    test_bool("Testing get with position following a fill 3\n", false, bitset_get(b, 32));
    bitset_free(b);

    uint32_t p6[] = { 0xC2000001 };
    b = bitset_new_array(1, p6);
    test_bool("Testing get with position following a fill 4\n", true, bitset_get(b, 0));
    test_bool("Testing get with position following a fill 5\n", true, bitset_get(b, 30));
    test_bool("Testing get with position following a fill 6\n", false, bitset_get(b, 31));
    test_bool("Testing get with position following a fill 7\n", true, bitset_get(b, 32));
    bitset_free(b);
}

void test_suite_count() {
    bitset *b = bitset_new();
    test_ulong("Testing pop count of empty set\n", 0, bitset_count(b));
    bitset_free(b);

    uint32_t p1[] = { 0x80000000, 0x00000001 };
    b = bitset_new_array(2, p1);
    test_ulong("Testing pop count of single literal 1\n", 1, bitset_count(b));
    bitset_free(b);

    uint32_t p2[] = { 0x80000000, 0x11111111 };
    b = bitset_new_array(2, p2);
    test_ulong("Testing pop count of single literal 2\n", 8, bitset_count(b));
    bitset_free(b);

    uint32_t p3[] = { 0x80000001 };
    b = bitset_new_array(1, p3);
    test_ulong("Testing pop count of single fill 1\n", 0, bitset_count(b));
    bitset_free(b);

    uint32_t p4[] = { 0xC0000001 };
    b = bitset_new_array(1, p4);
    test_ulong("Testing pop count of single fill 2\n", 31, bitset_count(b));
    bitset_free(b);

    uint32_t p5[] = { 0xC0000002 };
    b = bitset_new_array(1, p5);
    test_ulong("Testing pop count of single fill 3\n", 62, bitset_count(b));
    bitset_free(b);

    uint32_t p6[] = { 0xC0000001, 0xC0000001 };
    b = bitset_new_array(2, p6);
    test_ulong("Testing pop count of double fill 1\n", 62, bitset_count(b));
    bitset_free(b);

    uint32_t p7[] = { 0x80000010, 0xC0000001 };
    b = bitset_new_array(2, p7);
    test_ulong("Testing pop count of double fill 2\n", 31, bitset_count(b));
    bitset_free(b);

    uint32_t p8[] = { 0x8C000011 };
    b = bitset_new_array(1, p8);
    test_ulong("Testing pop count of fill with position 1\n", 1, bitset_count(b));
    bitset_free(b);

    uint32_t p9[] = { 0xCC000001 };
    b = bitset_new_array(1, p9);
    test_ulong("Testing pop count of fill with position 2\n", 61, bitset_count(b));
    bitset_free(b);
}

