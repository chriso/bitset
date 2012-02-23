#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>

#include "bitset.h"
#include "test.h"

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

    b = bitset_new(b);
    bitset_set(b, 1, true);
    bitset_set(b, 1000000000000, true);
    test_bool("Testing set where a chain of fills is required 1\n", true, bitset_get(b, 1));
    test_bool("Testing set where a chain of fills is required 2\n", true, bitset_get(b, 1000000000000));
    test_ulong("Testing set where a chain of fills is required 3\n", 2, bitset_count(b));
    bitset_free(b);

    //Test unsetting of position bit of 0-colour => fill_length++
    //Test setting & unsetting the position bit of a 1-colour fill word
    //Test append where bit becomes position in 1-colour fill
    //Test append where unset requires break of 1-colour fill position
    //Test partition where 1-colour position has to be split out
}

void test_suite_stress() {
    bitset *b = bitset_new();
    unsigned int max = 100000000, num = 1000;
    unsigned *bits = malloc(sizeof(unsigned) * num);
    srand(time(NULL));
    for (unsigned i = 0; i < num; i++) {
        bits[i] = rand() % max;
        //bits[i] = i;
        bitset_set(b, bits[i], true);
    }
    for (unsigned i = 0; i < num; i++) {
        test_bool("Checking stress test bits were set", true, bitset_get(b, bits[i]));
    }
    for (unsigned i = 0; i < 86400; i++) {
        bitset_count(b);
    }
    free(bits);
    bitset_free(b);
}

void test_suite_operation() {
    bitset_op *ops;
    bitset *b1, *b2, *b3, *b4;

    b1 = bitset_new();
    bitset_set(b1, 10, true);
    b2 = bitset_new();
    bitset_set(b2, 20, true);
    b3 = bitset_new();
    bitset_set(b3, 12, true);
    ops = bitset_operation_new(b1);
    test_int("Checking initial operation length is one\n", 1, ops->length);
    test_bool("Checking primary bitset is added\n", true, bitset_get(ops->steps[0]->b, 10));
    bitset_operation_add(ops, b2, BITSET_OR);
    test_int("Checking op length increases\n", 2, ops->length);
    bitset_operation_add(ops, b3, BITSET_OR);
    test_int("Checking op length increases\n", 3, ops->length);
    test_bool("Checking bitset was added correctly\n", true, bitset_get(ops->steps[1]->b, 20));
    test_int("Checking op was added correctly\n", BITSET_OR, ops->steps[1]->operation);
    test_bool("Checking bitset was added correctly\n", true, bitset_get(ops->steps[2]->b, 12));
    test_int("Checking op was added correctly\n", BITSET_OR, ops->steps[2]->operation);
    test_ulong("Checking operation count 1\n", 3, bitset_operation_count(ops));
    bitset_operation_free(ops);
    bitset_free(b1);
    bitset_free(b2);
    bitset_free(b3);

    b1 = bitset_new();
    b2 = bitset_new();
    b3 = bitset_new();
    bitset_set(b1, 1000, true);
    bitset_set(b2, 100, true);
    bitset_set(b3, 20, true);
    ops = bitset_operation_new(b1);
    bitset_operation_add(ops, b2, BITSET_OR);
    bitset_operation_add(ops, b3, BITSET_OR);
    test_ulong("Checking operation count 2\n", 3, bitset_operation_count(ops));
    bitset_operation_free(ops);
    bitset_free(b1);
    bitset_free(b2);
    bitset_free(b3);

    b1 = bitset_new();
    b2 = bitset_new();
    b3 = bitset_new();
    bitset_set(b1, 102, true);
    bitset_set(b1, 10000, true);
    bitset_set(b2, 100, true);
    bitset_set(b3, 20, true);
    bitset_set(b3, 101, true);
    bitset_set(b3, 20000, true);
    ops = bitset_operation_new(b1);
    bitset_operation_add(ops, b2, BITSET_OR);
    bitset_operation_add(ops, b3, BITSET_OR);
    test_ulong("Checking operation count 3\n", 6, bitset_operation_count(ops));
    bitset_operation_free(ops);
    bitset_free(b1);
    bitset_free(b2);
    bitset_free(b3);

    b1 = bitset_new();
    b2 = bitset_new();
    b3 = bitset_new();
    bitset_set(b1, 101, true);
    bitset_set(b1, 8000, true);
    bitset_set(b2, 100, true);
    bitset_set(b3, 20, true);
    bitset_set(b3, 101, true);
    bitset_set(b3, 8001, true);
    ops = bitset_operation_new(b1);
    bitset_operation_add(ops, b2, BITSET_OR);
    bitset_operation_add(ops, b3, BITSET_OR);
    test_ulong("Checking operation count 4\n", 5, bitset_operation_count(ops));
    bitset_operation_free(ops);
    bitset_free(b1);
    bitset_free(b2);
    bitset_free(b3);

    b1 = bitset_new();
    b2 = bitset_new();
    b3 = bitset_new();
    bitset_set(b1, 101, true);
    bitset_set(b1, 102, true);
    bitset_set(b2, 1000, true);
    bitset_set(b3, 101, true);
    bitset_set(b3, 1000, true);
    ops = bitset_operation_new(b1);
    bitset_operation_add(ops, b2, BITSET_OR);
    bitset_operation_add(ops, b3, BITSET_AND);
    test_ulong("Checking operation count 5\n", 2, bitset_operation_count(ops));
    bitset_operation_free(ops);
    bitset_free(b1);
    bitset_free(b2);
    bitset_free(b3);

    b1 = bitset_new();
    b2 = bitset_new();
    b3 = bitset_new();
    bitset_set(b1, 1000, true);
    bitset_set(b2, 100, true);
    bitset_set(b2, 105, true);
    bitset_set(b2, 130, true);
    bitset_set(b3, 20, true);
    ops = bitset_operation_new(b1);
    bitset_operation_add(ops, b2, BITSET_OR);
    bitset_operation_add(ops, b3, BITSET_OR);
    b4 = bitset_operation_exec(ops);
    test_ulong("Checking operation exec 1\n", 5, bitset_count(b4));
    test_bool("Checking operation exec get 1\n", true, bitset_get(b4, 1000));
    test_bool("Checking operation exec get 2\n", true, bitset_get(b4, 100));
    test_bool("Checking operation exec get 3\n", true, bitset_get(b4, 105));
    test_bool("Checking operation exec get 4\n", true, bitset_get(b4, 130));
    test_bool("Checking operation exec get 5\n", true, bitset_get(b4, 20));
    bitset_operation_free(ops);
    bitset_free(b1);
    bitset_free(b2);
    bitset_free(b3);
    bitset_free(b4);

    b1 = bitset_new();
    b2 = bitset_new();
    b3 = bitset_new();
    bitset_set(b1, 1000, true);
    bitset_set(b1, 1001, true);
    bitset_set(b1, 1100, true);
    bitset_set(b1, 3, true);
    bitset_set(b2, 1000, true);
    bitset_set(b2, 1101, true);
    bitset_set(b2, 3, true);
    bitset_set(b2, 130, true);
    bitset_set(b3, 1000, true);
    ops = bitset_operation_new(b1);
    bitset_operation_add(ops, b2, BITSET_AND);
    bitset_operation_add(ops, b3, BITSET_ANDNOT);
    b4 = bitset_operation_exec(ops);
    test_bool("Checking operation exec get 6\n", true, bitset_get(b4, 3));
    test_bool("Checking operation exec get 7\n", false, bitset_get(b4, 1000));
    test_bool("Checking operation exec get 8\n", false, bitset_get(b4, 130));
    test_bool("Checking operation exec get 9\n", false, bitset_get(b4, 1001));
    test_bool("Checking operation exec get 10\n", false, bitset_get(b4, 1100));
    test_bool("Checking operation exec get 11\n", false, bitset_get(b4, 1101));
    test_ulong("Checking operation exec 2\n", 1, bitset_count(b4));
    bitset_operation_free(ops);
    bitset_free(b1);
    bitset_free(b2);
    bitset_free(b3);
    bitset_free(b4);

    b1 = bitset_new();
    b2 = bitset_new();
    bitset_set(b1, 1, true);
    bitset_set(b2, 10000000000, true);
    bitset_set(b2, 100000000000, true);
    ops = bitset_operation_new(b1);
    bitset_operation_add(ops, b2, BITSET_OR);
    b4 = bitset_operation_exec(ops);
    test_ulong("Checking operation exec 2\n", 3, bitset_count(b4));
    test_bool("Checking operation exec get 12\n", true, bitset_get(b4, 1));
    test_bool("Checking operation exec get 12\n", true, bitset_get(b4, 10000000000));
    test_bool("Checking operation exec get 13\n", true, bitset_get(b4, 100000000000));
    bitset_operation_free(ops);
    bitset_free(b1);
    bitset_free(b2);
    bitset_free(b4);
}

