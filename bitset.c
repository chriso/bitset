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
void test_suite_stress();
void test_suite_count();
void test_suite_operation();
void test_suite_ffs();

void test_bool(char *, bool, bool);
void test_ulong(char *, unsigned long, unsigned long);
bool test_bitset(char *, bitset *, unsigned, uint32_t *);
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
    can be used to determine which bit is set (0x80000000 >> position)
*/

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
    b->words = BITSET_MALLOC(length * sizeof(uint32_t));
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
        b->length = length;
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
            position = BITSET_GET_POSITION(word);
            if (word_offset < 0) {
                return BITSET_GET_COLOUR(word);
            } else if (position) {
                if (!word_offset) {
                    if (position - 1 == bit % 31) {
                        return !BITSET_GET_COLOUR(word);
                    } else if (BITSET_GET_COLOUR(word)) {
                        return true;
                    }
                }
                word_offset--;
            }
        } else if (!word_offset--) {
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

unsigned long bitset_fls(bitset *b) {
    unsigned long offset = 0;
    unsigned short position;
    uint32_t word;
    for (unsigned i = 0; i < b->length; i++) {
        word = b->words[i];
        if (BITSET_IS_FILL_WORD(word)) {
            offset += BITSET_GET_LENGTH(word);
            if (BITSET_GET_COLOUR(word)) {
                return offset * 31;
            }
            position = BITSET_GET_POSITION(word);
            if (position) {
                return offset * 31 + position - 1;
            }
        } else {
            return offset * 31 + (31 - flsl(word));
        }
    }
    return 0;
}

bool bitset_set(bitset *b, unsigned long bit, bool value) {
    long word_offset = bit / 31;
    bit %= 31;

    if (b->length) {
        uint32_t word;
        unsigned long fill_length;
        unsigned short position;

        for (unsigned i = 0; i < b->length; i++) {
            word = b->words[i];
            if (BITSET_IS_FILL_WORD(word)) {
                position = BITSET_GET_POSITION(word);
                fill_length = BITSET_GET_LENGTH(word);

                if (word_offset == fill_length - 1) {
                    if (position) {
                        bitset_resize(b, b->length + 1);
                        if (i < b->length - 1) {
                            memmove(b->words+i+2, b->words+i+1, sizeof(uint32_t) * (b->length - i - 2));
                        }
                        b->words[i+1] = BITSET_CREATE_LITERAL(position - 1);
                        if (word_offset) {
                            b->words[i] = BITSET_CREATE_FILL(fill_length - 1, bit);
                        } else {
                            b->words[i] = BITSET_CREATE_LITERAL(bit);
                        }
                    } else {
                        if (fill_length - 1 > 0) {
                            b->words[i] = BITSET_CREATE_FILL(fill_length - 1, bit);
                        } else {
                            b->words[i] = BITSET_CREATE_LITERAL(bit);
                        }
                    }
                    return false;
                } else if (word_offset < fill_length) {
                    bitset_resize(b, b->length + 1);
                    if (i < b->length - 1) {
                        memmove(b->words+i+2, b->words+i+1, sizeof(uint32_t) * (b->length - i - 2));
                    }
                    if (!word_offset) {
                        b->words[i] = BITSET_CREATE_LITERAL(bit);
                    } else {
                        b->words[i] = BITSET_CREATE_FILL(word_offset, bit);
                    }
                    b->words[i+1] = BITSET_CREATE_FILL(fill_length - word_offset - 1, position - 1);
                    return false;
                }

                word_offset -= fill_length;

                if (position) {
                    if (!word_offset) {
                        if (position - 1 == bit) {
                            if (!value || BITSET_GET_COLOUR(word)) {
                                b->words[i] = BITSET_UNSET_POSITION(word);
                            }
                            return !BITSET_GET_COLOUR(word);
                        } else {
                            bitset_resize(b, b->length + 1);
                            if (i < b->length - 1) {
                                memmove(b->words+i+2, b->words+i+1, sizeof(uint32_t) * (b->length - i - 2));
                            }
                            b->words[i] = BITSET_UNSET_POSITION(word);
                            uint32_t literal = 0;
                            literal |= BITSET_GET_LITERAL_MASK(position - 1);
                            literal |= BITSET_GET_LITERAL_MASK(bit);
                            b->words[i+1] = literal;
                            return false;
                        }
                    }
                    word_offset--;
                } else if (!word_offset && i == b->length - 1) {
                    b->words[i] = BITSET_SET_POSITION(word, bit + 1);
                    return false;
                }
            } else if (!word_offset--) {
                uint32_t mask = BITSET_GET_LITERAL_MASK(bit);
                bool previous = word & mask;
                if (value) {
                    b->words[i] |= mask;
                } else {
                    b->words[i] &= ~mask;
                }
                return previous;
            }
        }

    }

    if (value) {
        if (word_offset > BITSET_MAX_LENGTH) {
            fprintf(stderr, "Fill chains are unimplemented\n");
            exit(1);
        } else {
            bitset_resize(b, b->length + 1);
            if (word_offset) {
                b->words[b->length - 1] = BITSET_CREATE_FILL(word_offset, bit);
            } else {
                b->words[b->length - 1] = BITSET_CREATE_LITERAL(bit);
            }
        }
    }

    return false;
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

bool test_bitset(char *title, bitset *b, unsigned length, uint32_t *expected) {
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
        printf("\x1B[31m%s\x1B[0m\n", title);
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
    printf("Testing ffs\n");
    test_suite_ffs();
    printf("Testing stress\n");
    test_suite_stress();
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

void test_suite_ffs() {
    bitset *b = bitset_new();
    bitset_set(b, 1000, true);
    test_ulong("Test find first set 1", 1000, bitset_fls(b));
    bitset_set(b, 300, true);
    test_ulong("Test find first set 2", 300, bitset_fls(b));
    bitset_set(b, 299, true);
    test_ulong("Test find first set 3", 299, bitset_fls(b));
    bitset_set(b, 298, true);
    test_ulong("Test find first set 4", 298, bitset_fls(b));
    bitset_set(b, 290, true);
    test_ulong("Test find first set 5", 290, bitset_fls(b));
    bitset_set(b, 240, true);
    test_ulong("Test find first set 6", 240, bitset_fls(b));
    bitset_set(b, 12, true);
    test_ulong("Test find first set 7", 12, bitset_fls(b));
    bitset_set(b, 3, true);
    test_ulong("Test find first set 8", 3, bitset_fls(b));
    bitset_free(b);
}

void test_suite_set() {
    bitset *b = bitset_new();
    test_bool("Testing set on empty set 1\n", false, bitset_set(b, 0, true));
    test_bool("Testing set on empty set 2\n", true, bitset_get(b, 0));
    test_bool("Testing set on empty set 3\n", false, bitset_get(b, 1));
    bitset_free(b);

    b = bitset_new();
    test_bool("Testing unset on empty set 1\n", false, bitset_set(b, 100, false));
    test_ulong("Testing unset on empty set doesn't create it\n", 0, b->length);
    bitset_free(b);

    b = bitset_new();
    test_bool("Testing set on empty set 4\n", false, bitset_set(b, 31, true));
    test_bool("Testing set on empty set 5\n", true, bitset_get(b, 31));
    bitset_free(b);

    uint32_t p1[] = { 0x80000001 };
    b = bitset_new_array(1, p1);
    test_bool("Testing append after fill 1\n", false, bitset_set(b, 93, true));
    uint32_t e1[] = { 0x80000001, 0x82000002 };
    test_bitset("Testing append after fill 2", b, 2, e1);
    bitset_free(b);

    uint32_t p2[] = { 0x82000001 };
    b = bitset_new_array(1, p2);
    test_bool("Testing append after fill 3\n", false, bitset_set(b, 93, true));
    uint32_t e2[] = { 0x82000001, 0x82000001 };
    test_bitset("Testing append after fill 4", b, 2, e2);
    bitset_free(b);

    uint32_t p3[] = { 0x80000001, 0x00000000 };
    b = bitset_new_array(2, p3);
    test_bool("Testing set in literal 1\n", false, bitset_set(b, 32, true));
    test_bool("Testing set in literal 2\n", false, bitset_set(b, 38, true));
    test_bool("Testing set in literal 3\n", false, bitset_set(b, 45, true));
    test_bool("Testing set in literal 4\n", false, bitset_set(b, 55, true));
    test_bool("Testing set in literal 5\n", false, bitset_set(b, 61, true));
    uint32_t e3[] = { 0x80000001, 0x20810041 };
    test_bitset("Testing set in literal 6", b, 2, e3);
    test_bool("Testing set in literal 7\n", true, bitset_set(b, 61, false));
    uint32_t e4[] = { 0x80000001, 0x20810040 };
    test_bitset("Testing set in literal 8", b, 2, e4);
    bitset_free(b);

    uint32_t p5[] = { 0x82000001 };
    b = bitset_new_array(1, p5);
    test_bool("Testing partition of fill 1\n", false, bitset_set(b, 32, true));
    uint32_t e5[] = { 0x80000001, 0x60000000 };
    test_bitset("Testing partition of fill 2", b, 2, e5);
    bitset_free(b);

    uint32_t p6[] = { 0x82000001, 0x82000001 };
    b = bitset_new_array(2, p6);
    test_bool("Testing partition of fill 3\n", false, bitset_set(b, 32, true));
    uint32_t e6[] = { 0x80000001, 0x60000000, 0x82000001 };
    test_bitset("Testing partition of fill 4", b, 3, e6);
    bitset_free(b);

    uint32_t p7[] = { 0x80000001 };
    b = bitset_new_array(1, p7);
    test_bool("Testing partition of fill 5\n", false, bitset_set(b, 31, true));
    uint32_t e7[] = { 0x82000001 };
    test_bitset("Testing partition of fill 6", b, 1, e7);
    bitset_free(b);

    uint32_t p8[] = { 0x82000001, 0x86000001 };
    b = bitset_new_array(2, p8);
    test_bool("Testing partition of fill 7\n", false, bitset_set(b, 0, true));
    uint32_t e8[] = { 0x40000000, 0x40000000, 0x86000001 };
    test_bitset("Testing partition of fill 7", b, 3, e8);
    bitset_free(b);

    uint32_t p8b[] = { 0x82000002, 0x86000001 };
    b = bitset_new_array(2, p8b);
    test_bool("Testing partition of fill 7b\n", false, bitset_set(b, 32, true));
    uint32_t e8b[] = { 0x84000001, 0x40000000, 0x86000001 };
    test_bitset("Testing partition of fill 7b - 3", b, 3, e8b);
    test_bool("Testing partition of fill 7b - 1\n", true, bitset_get(b, 32));
    test_bool("Testing partition of fill 7b - 2\n", true, bitset_get(b, 62));
    bitset_free(b);

    uint32_t p9[] = { 0x82000003, 0x86000001 };
    b = bitset_new_array(2, p9);
    test_bool("Testing partition of fill 8\n", false, bitset_set(b, 32, true));
    uint32_t e9[] = { 0x84000001, 0x82000001, 0x86000001 };
    test_bitset("Testing partition of fill 9", b, 3, e9);
    bitset_free(b);

    uint32_t p10[] = { 0x80000001, 0x82000001 };
    b = bitset_new_array(2, p10);
    test_bool("Testing partition of fill 10\n", false, bitset_set(b, 1, true));
    uint32_t e10[] = { 0x20000000, 0x82000001 };
    test_bitset("Testing partition of fill 11", b, 2, e10);
    bitset_free(b);

    uint32_t p11[] = { 0x82000001 };
    b = bitset_new_array(1, p11);
    test_bool("Testing setting position bit 1\n", true, bitset_set(b, 31, true));
    test_bitset("Testing setting position bit 2", b, 1, p11);
    uint32_t e11[] = { 0x80000001 };
    test_bool("Testing setting position bit 3\n", true, bitset_set(b, 31, false));
    test_bitset("Testing setting position bit 4", b, 1, e11);
    bitset_free(b);

    b = bitset_new();
    test_bool("Testing random set/get 1\n", false, bitset_set(b, 0, true));
    test_bool("Testing random set/get 1\n", false, bitset_set(b, 36, true));
    test_bool("Testing random set/get 1\n", false, bitset_set(b, 4, true));
    test_bool("Testing random set/get 2\n", true, bitset_get(b, 0));
    test_bool("Testing random set/get 2\n", true, bitset_get(b, 36));
    test_bool("Testing random set/get 2\n", true, bitset_get(b, 4));
    bitset_free(b);

    b = bitset_new();
    test_bool("Testing random set/get 1\n", false, bitset_set(b, 47, true));
    test_bool("Testing random set/get 1\n", false, bitset_set(b, 58, true));
    test_bool("Testing random set/get 1\n", false, bitset_set(b, 34, true));
    test_bool("Testing random set/get 3\n", true, bitset_get(b, 47));
    test_bool("Testing random set/get 4\n", true, bitset_get(b, 58));
    test_bool("Testing random set/get 5\n", true, bitset_get(b, 34));
    bitset_free(b);

    b = bitset_new();
    test_bool("Testing random set/get 1\n", false, bitset_set(b, 99, true));
    test_bool("Testing random set/get 1\n", false, bitset_set(b, 85, true));
    test_bool("Testing random set/get 1\n", false, bitset_set(b, 27, true));
    test_bool("Testing random set/get 6\n", true, bitset_get(b, 99));
    test_bool("Testing random set/get 7\n", true, bitset_get(b, 85));
    test_bool("Testing random set/get 8\n", true, bitset_get(b, 27));
    bitset_free(b);

    b = bitset_new();
    test_bool("Testing random set/get 1\n", false, bitset_set(b, 62, true));
    test_bool("Testing random set/get 1\n", false, bitset_set(b, 29, true));
    test_bool("Testing random set/get 1\n", false, bitset_set(b, 26, true));
    test_bool("Testing random set/get 1\n", false, bitset_set(b, 65, true));
    test_bool("Testing random set/get 1\n", false, bitset_set(b, 54, true));
    test_bool("Testing random set/get 6\n", true, bitset_get(b, 62));
    test_bool("Testing random set/get 7\n", true, bitset_get(b, 29));
    test_bool("Testing random set/get 8\n", true, bitset_get(b, 26));
    test_bool("Testing random set/get 8\n", true, bitset_get(b, 65));
    test_bool("Testing random set/get 8\n", true, bitset_get(b, 54));
    bitset_free(b);

    b = bitset_new();
    test_bool("Testing random set/get 1\n", false, bitset_set(b, 73, true));
    test_bool("Testing random set/get 1\n", false, bitset_set(b, 83, true));
    test_bool("Testing random set/get 1\n", false, bitset_set(b, 70, true));
    test_bool("Testing random set/get 1\n", false, bitset_set(b, 48, true));
    test_bool("Testing random set/get 1\n", false, bitset_set(b, 11, true));
    test_bool("Testing random set/get 6\n", true, bitset_get(b, 73));
    test_bool("Testing random set/get 7\n", true, bitset_get(b, 83));
    test_bool("Testing random set/get 8\n", true, bitset_get(b, 70));
    test_bool("Testing random set/get 8\n", true, bitset_get(b, 48));
    test_bool("Testing random set/get 8\n", true, bitset_get(b, 11));
    bitset_free(b);

    b = bitset_new();
    test_bool("Testing random set/get 1\n", false, bitset_set(b, 10, true));
    test_bool("Testing random set/get 1\n", false, bitset_set(b, 20, true));
    test_bool("Testing random set/get 1\n", false, bitset_set(b, 96, true));
    test_bool("Testing random set/get 1\n", false, bitset_set(b, 52, true));
    test_bool("Testing random set/get 1\n", false, bitset_set(b, 32, true));
    test_bool("Testing random set/get 6\n", true, bitset_get(b, 10));
    test_bool("Testing random set/get 7\n", true, bitset_get(b, 20));
    test_bool("Testing random set/get 8\n", true, bitset_get(b, 96));
    test_bool("Testing random set/get 8\n", true, bitset_get(b, 52));
    test_bool("Testing random set/get 8\n", true, bitset_get(b, 32));
    bitset_free(b);

    b = bitset_new();
    test_bool("Testing random set/get 1\n", false, bitset_set(b, 62, true));
    test_bool("Testing random set/get 1\n", false, bitset_set(b, 96, true));
    test_bool("Testing random set/get 1\n", false, bitset_set(b, 55, true));
    test_bool("Testing random set/get 1\n", false, bitset_set(b, 88, true));
    test_bool("Testing random set/get 1\n", false, bitset_set(b, 19, true));
    test_bool("Testing random set/get 6\n", true, bitset_get(b, 62));
    test_bool("Testing random set/get 7\n", true, bitset_get(b, 96));
    test_bool("Testing random set/get 8\n", true, bitset_get(b, 55));
    test_bool("Testing random set/get 8\n", true, bitset_get(b, 88));
    test_bool("Testing random set/get 8\n", true, bitset_get(b, 19));
    bitset_free(b);

    b = bitset_new();
    test_bool("Testing random set/get 1\n", false, bitset_set(b, 73, true));
    test_bool("Testing random set/get 1\n", false, bitset_set(b, 93, true));
    test_bool("Testing random set/get 1\n", false, bitset_set(b, 14, true));
    test_bool("Testing random set/get 1\n", false, bitset_set(b, 51, true));
    test_bool("Testing random set/get 1\n", false, bitset_set(b, 41, true));
    test_bool("Testing random set/get 6\n", true, bitset_get(b, 73));
    test_bool("Testing random set/get 7\n", true, bitset_get(b, 93));
    test_bool("Testing random set/get 8\n", true, bitset_get(b, 14));
    test_bool("Testing random set/get 8\n", true, bitset_get(b, 51));
    test_bool("Testing random set/get 8\n", true, bitset_get(b, 41));
    bitset_free(b);

    b = bitset_new();
    test_bool("Testing random set/get 1\n", false, bitset_set(b, 99, true));
    test_bool("Testing random set/get 1\n", false, bitset_set(b, 23, true));
    test_bool("Testing random set/get 1\n", false, bitset_set(b, 45, true));
    test_bool("Testing random set/get 1\n", false, bitset_set(b, 57, true));
    test_bool("Testing random set/get 1\n", false, bitset_set(b, 67, true));
    test_bool("Testing random set/get 6\n", true, bitset_get(b, 99));
    test_bool("Testing random set/get 7\n", true, bitset_get(b, 23));
    test_bool("Testing random set/get 8\n", true, bitset_get(b, 45));
    test_bool("Testing random set/get 8\n", true, bitset_get(b, 57));
    test_bool("Testing random set/get 8\n", true, bitset_get(b, 67));
    bitset_free(b);

    b = bitset_new();
    test_bool("Testing random set/get 1\n", false, bitset_set(b, 71, true));
    test_bool("Testing random set/get 1\n", false, bitset_set(b, 74, true));
    test_bool("Testing random set/get 1\n", false, bitset_set(b, 94, true));
    test_bool("Testing random set/get 1\n", false, bitset_set(b, 19, true));
    test_bool("Testing random set/get 6\n", true, bitset_get(b, 71));
    test_bool("Testing random set/get 7\n", true, bitset_get(b, 74));
    test_bool("Testing random set/get 8\n", true, bitset_get(b, 94));
    test_bool("Testing random set/get 8\n", true, bitset_get(b, 19));
    bitset_free(b);

    b = bitset_new();
    test_bool("Testing random set/get 1\n", false, bitset_set(b, 85, true));
    test_bool("Testing random set/get 1\n", false, bitset_set(b, 25, true));
    test_bool("Testing random set/get 1\n", false, bitset_set(b, 93, true));
    test_bool("Testing random set/get 1\n", false, bitset_set(b, 88, true));
    test_bool("Testing random set/get 1\n", false, bitset_set(b, 54, true));
    test_bool("Testing random set/get 6\n", true, bitset_get(b, 85));
    test_bool("Testing random set/get 7\n", true, bitset_get(b, 25));
    test_bool("Testing random set/get 8\n", true, bitset_get(b, 93));
    test_bool("Testing random set/get 8\n", true, bitset_get(b, 88));
    test_bool("Testing random set/get 8\n", true, bitset_get(b, 54));
    bitset_free(b);

    b = bitset_new();
    test_bool("Testing random set/get 1\n", false, bitset_set(b, 94, true));
    test_bool("Testing random set/get 1\n", false, bitset_set(b, 47, true));
    test_bool("Testing random set/get 1\n", false, bitset_set(b, 79, true));
    test_bool("Testing random set/get 1\n", false, bitset_set(b, 67, true));
    test_bool("Testing random set/get 1\n", false, bitset_set(b, 24, true));
    test_bool("Testing random set/get 6\n", true, bitset_get(b, 94));
    test_bool("Testing random set/get 7\n", true, bitset_get(b, 47));
    test_bool("Testing random set/get 8\n", true, bitset_get(b, 79));
    test_bool("Testing random set/get 8\n", true, bitset_get(b, 67));
    test_bool("Testing random set/get 8\n", true, bitset_get(b, 24));
    bitset_free(b);

    //Test setting & unsetting the position bit of a 1-colour fill word
    //Test append where bit becomes position in 1-colour fill
    //Test append where unset requires break of 1-colour fill position
    //Test partition where 1-colour position has to be split out
    //Test append >= 2^25
}

void test_suite_stress() {
    bitset *b = bitset_new();
    unsigned int max = 100000, num = 800;
    unsigned *bits = malloc(sizeof(unsigned) * num);
    srand(time(NULL));
    for (unsigned i = 0; i < num; i++) {
        bits[i] = rand() % max;
        //printf("%d\n", bits[i]);
        bitset_set(b, bits[i], true);
    }
    for (unsigned i = 0; i < num; i++) {
        //printf("%d\n", bits[i]);
        test_bool("Checking stress test bits were set", true, bitset_get(b, bits[i]));
    }
    free(bits);
    bitset_free(b);
}

