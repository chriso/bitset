#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>

#include "test.h"

#include "bitset/malloc.h"
#include "bitset/vector.h"

void bitset_dump(bitset_t *b) {
    printf("\x1B[33mDumping bitset of size %u\x1B[0m\n", (unsigned)b->length);
    for (size_t i = 0; i < b->length; i++) {
        printf("\x1B[36m%3zu.\x1B[0m %-8x\n", i, b->buffer[i]);
    }
}

#define TEST_DEFINE(pref, type, format) \
    void test_##pref(char *title, type expected, type value) { \
        if (value != expected) { \
            printf("\x1B[31m%s\x1B[0m", title); \
            printf("   expected '" format "', got '" format "'\n", expected, value); \
            exit(1); \
        } \
    }

TEST_DEFINE(int, int, "%d")
TEST_DEFINE(ulong, unsigned long, "%lu")
TEST_DEFINE(bool, bool, "%d")
TEST_DEFINE(str, char *, "%s")
TEST_DEFINE(hex, int, "%#x")

bool test_bitset(char *title, bitset_t *b, unsigned length, uint32_t *expected) {
    bool mismatch = length != b->length;
    if (!mismatch) {
        for (size_t i = 0; i < length; i++) {
            if (b->buffer[i] != expected[i]) {
                mismatch = true;
                break;
            }
        }
    }
    if (mismatch) {
        size_t length_max = BITSET_MAX(b->length, length);
        printf("\x1B[31m%s\x1B[0m\n", title);
        for (size_t i = 0; i < length_max; i++) {
            printf("  \x1B[36m%3zu.\x1B[0m ", i);
            if (i < b->length) {
                printf("%-8x ", b->buffer[i]);
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
    printf("Testing get\n");
    test_suite_get();
    printf("Testing set\n");
    test_suite_set();
    printf("Testing count\n");
    test_suite_count();
    printf("Testing operations\n");
    test_suite_operation();
    printf("Testing min / max\n");
    test_suite_min();
    test_suite_max();
    printf("Testing vector\n");
    test_suite_vector();
    printf("Testing vector operations\n");
    test_suite_vector_operation();
    printf("Testing estimate algorithms\n");
    test_suite_estimate();
    printf("Testing stress\n");
    test_suite_stress();
    printf("OK\n");
}

void test_suite_get() {
    bitset_t *b = bitset_new();
    for (size_t i = 0; i < 32; i++)
        test_bool("Testing initial bits are unset\n", false, bitset_get(b, i));
    bitset_free(b);

    uint32_t p1[] = { BITSET_CREATE_EMPTY_FILL(0), BITSET_CREATE_LITERAL(30) };
    b = bitset_new_buffer((const char *)p1, 8);
    test_bool("Testing get in the first literal 1\n", true, bitset_get(b, 30));
    test_bool("Testing get in the first literal 2\n", false, bitset_get(b, 31));
    bitset_free(b);

    uint32_t p2[] = { BITSET_CREATE_EMPTY_FILL(0), BITSET_CREATE_LITERAL(0) };
    b = bitset_new_buffer((const char *)p2, 8);
    test_bool("Testing get in the first literal 3\n", true, bitset_get(b, 0));
    test_bool("Testing get in the first literal 4\n", false, bitset_get(b, 1));
    bitset_free(b);

    uint32_t p3[] = { BITSET_CREATE_EMPTY_FILL(1), BITSET_CREATE_LITERAL(0) };
    b = bitset_new_buffer((const char *)p3, 8);
    test_bool("Testing get in the first literal with offset 1\n", false, bitset_get(b, 1));
    test_bool("Testing get in the first literal with offset 2\n", true, bitset_get(b, 31));
    bitset_free(b);

    uint32_t p4[] = {
        BITSET_CREATE_EMPTY_FILL(1), BITSET_CREATE_EMPTY_FILL(1), BITSET_CREATE_LITERAL(0)
    };
    b = bitset_new_buffer((const char *)p4, 12);
    test_bool("Testing get in the first literal with offset 4\n", false, bitset_get(b, 0));
    test_bool("Testing get in the first literal with offset 5\n", false, bitset_get(b, 31));
    test_bool("Testing get in the first literal with offset 6\n", true, bitset_get(b, 62));
    bitset_free(b);

    uint32_t p5[] = { BITSET_CREATE_FILL(1, 0) };
    b = bitset_new_buffer((const char *)p5, 4);
    test_bool("Testing get with position following a fill 1\n", false, bitset_get(b, 0));
    test_bool("Testing get with position following a fill 2\n", true, bitset_get(b, 31));
    test_bool("Testing get with position following a fill 3\n", false, bitset_get(b, 32));
    bitset_free(b);

    BITSET_NEW(b2, 1, 10, 100);
    test_int("Testing BITSET_NEW macro 1\n", 3, bitset_count(b2));
    test_bool("Testing BITSET_NEW macro 2\n", true, bitset_get(b2, 1));
    test_bool("Testing BITSET_NEW macro 3\n", true, bitset_get(b2, 10));
    test_bool("Testing BITSET_NEW macro 4\n", true, bitset_get(b2, 100));
    bitset_free(b2);

    BITSET_NEW(b3, 300);
    test_int("Testing BITSET_NEW macro 5\n", 1, bitset_count(b3));
    test_bool("Testing BITSET_NEW macro 6\n", true, bitset_get(b3, 300));
    bitset_free(b3);

    BITSET_NEW(b4b, 4000000000);
    test_int("Testing BITSET_NEW macro 7\n", 1, bitset_count(b4b));
    test_bool("Testing BITSET_NEW macro 8\n", true, bitset_get(b4b, 4000000000));
    bitset_free(b4b);

    BITSET_NEW(b4, 100, 300, 302, 305, 1000);
    bitset_iterator_t *i = bitset_iterator_new(b4);
    unsigned o, iters = 0;
    BITSET_FOREACH(i, o) {
        test_bool("Checking bitset iterator", true, o==100 || o==300 || o==302 || o==305 || o==1000);
        iters++;
    }
    test_int("Checking bitset iterator 2\n", 5, iters);
    (void)o;
    bitset_iterator_free(i);
    bitset_free(b4);
}

void test_suite_count() {
    bitset_t *b = bitset_new();
    test_ulong("Testing pop count of empty set\n", 0, bitset_count(b));
    bitset_free(b);

    uint32_t p1[] = { BITSET_CREATE_EMPTY_FILL(0), BITSET_CREATE_LITERAL(0) };
    b = bitset_new_buffer((const char *)p1, 8);
    test_ulong("Testing pop count of single literal 1\n", 1, bitset_count(b));
    bitset_free(b);

    uint32_t p2[] = { BITSET_CREATE_EMPTY_FILL(0), 0x11111111 };
    b = bitset_new_buffer((const char *)p2, 8);
    test_ulong("Testing pop count of single literal 2\n", 8, bitset_count(b));
    bitset_free(b);

    uint32_t p3[] = { BITSET_CREATE_EMPTY_FILL(1) };
    b = bitset_new_buffer((const char *)p3, 4);
    test_ulong("Testing pop count of single fill 1\n", 0, bitset_count(b));
    bitset_free(b);

    uint32_t p8[] = { BITSET_CREATE_FILL(3, 4) };
    b = bitset_new_buffer((const char *)p8, 4);
    test_ulong("Testing pop count of fill with position 1\n", 1, bitset_count(b));
    bitset_free(b);
}

void test_suite_min() {
    bitset_t *b = bitset_new();
    bitset_set_to(b, 1000, true);
    test_ulong("Test find first set 1", 1000, bitset_min(b));
    bitset_set_to(b, 300, true);
    test_ulong("Test find first set 2", 300, bitset_min(b));
    bitset_set_to(b, 299, true);
    test_ulong("Test find first set 3", 299, bitset_min(b));
    bitset_set_to(b, 298, true);
    test_ulong("Test find first set 4", 298, bitset_min(b));
    bitset_set_to(b, 290, true);
    test_ulong("Test find first set 5", 290, bitset_min(b));
    bitset_set_to(b, 240, true);
    test_ulong("Test find first set 6", 240, bitset_min(b));
    bitset_set_to(b, 12, true);
    test_ulong("Test find first set 7", 12, bitset_min(b));
    bitset_set_to(b, 3, true);
    test_ulong("Test find first set 8", 3, bitset_min(b));
    bitset_free(b);
}

void test_suite_max() {
    bitset_t *b = bitset_new();
    bitset_set_to(b, 3, true);
    test_ulong("Test find last set 8", 3, bitset_max(b));
    bitset_set_to(b, 12, true);
    test_ulong("Test find last set 7", 12, bitset_max(b));
    bitset_set_to(b, 240, true);
    test_ulong("Test find last set 6", 240, bitset_max(b));
    bitset_set_to(b, 290, true);
    test_ulong("Test find last set 5", 290, bitset_max(b));
    bitset_set_to(b, 298, true);
    test_ulong("Test find last set 4", 298, bitset_max(b));
    bitset_set_to(b, 299, true);
    test_ulong("Test find last set 3", 299, bitset_max(b));
    bitset_set_to(b, 300, true);
    test_ulong("Test find last set 2", 300, bitset_max(b));
    bitset_set_to(b, 1000, true);
    test_ulong("Test find last set 1", 1000, bitset_max(b));
    bitset_free(b);
}

void test_suite_set() {
    bitset_t *b = bitset_new();
    test_bool("Testing set on empty set 1\n", false, bitset_set_to(b, 0, true));
    test_bool("Testing set on empty set 2\n", true, bitset_get(b, 0));
    test_bool("Testing set on empty set 3\n", false, bitset_get(b, 1));
    bitset_free(b);

    b = bitset_new();
    test_bool("Testing unset on empty set 1\n", false, bitset_set_to(b, 100, false));
    test_ulong("Testing unset on empty set doesn't create it\n", 0, b->length);
    bitset_free(b);

    b = bitset_new();
    test_bool("Testing set on empty set 4\n", false, bitset_set_to(b, 31, true));
    test_bool("Testing set on empty set 5\n", true, bitset_get(b, 31));
    bitset_free(b);

    uint32_t p1[] = { BITSET_CREATE_EMPTY_FILL(1) };
    b = bitset_new_buffer((const char *)p1, 4);
    test_bool("Testing append after fill 1\n", false, bitset_set_to(b, 93, true));
    uint32_t e1[] = { BITSET_CREATE_EMPTY_FILL(1), BITSET_CREATE_FILL(2, 0) };
    test_bitset("Testing append after fill 2", b, 2, e1);
    bitset_free(b);

    uint32_t p2[] = { BITSET_CREATE_FILL(1, 0) };
    b = bitset_new_buffer((const char *)p2, 4);
    test_bool("Testing append after fill 3\n", false, bitset_set_to(b, 93, true));
    uint32_t e2[] = { BITSET_CREATE_FILL(1, 0), BITSET_CREATE_FILL(1, 0) };
    test_bitset("Testing append after fill 4", b, 2, e2);
    bitset_free(b);

    uint32_t p3[] = { BITSET_CREATE_EMPTY_FILL(1), 0 };
    b = bitset_new_buffer((const char *)p3, 8);
    test_bool("Testing set in literal 1\n", false, bitset_set_to(b, 32, true));
    test_bool("Testing set in literal 2\n", false, bitset_set_to(b, 38, true));
    test_bool("Testing set in literal 3\n", false, bitset_set_to(b, 45, true));
    test_bool("Testing set in literal 4\n", false, bitset_set_to(b, 55, true));
    test_bool("Testing set in literal 5\n", false, bitset_set_to(b, 61, true));
    uint32_t e3[] = { BITSET_CREATE_EMPTY_FILL(1), 0x20810041 };
    test_bitset("Testing set in literal 6", b, 2, e3);
    test_bool("Testing set in literal 7\n", true, bitset_set_to(b, 61, false));
    uint32_t e4[] = { BITSET_CREATE_EMPTY_FILL(1), 0x20810040 };
    test_bitset("Testing set in literal 8", b, 2, e4);
    bitset_free(b);

    uint32_t p5[] = { BITSET_CREATE_FILL(1, 0) };
    b = bitset_new_buffer((const char *)p5, 4);
    test_bool("Testing partition of fill 1\n", false, bitset_set_to(b, 32, true));
    uint32_t e5[] = { BITSET_CREATE_EMPTY_FILL(1), BITSET_CREATE_LITERAL(0) | BITSET_CREATE_LITERAL(1) };
    test_bitset("Testing partition of fill 2", b, 2, e5);
    bitset_free(b);

    uint32_t p6[] = { BITSET_CREATE_FILL(1, 0), BITSET_CREATE_FILL(1, 0) };
    b = bitset_new_buffer((const char *)p6, 8);
    test_bool("Testing partition of fill 3\n", false, bitset_set_to(b, 32, true));
    uint32_t e6[] = { BITSET_CREATE_EMPTY_FILL(1), BITSET_CREATE_LITERAL(0) | BITSET_CREATE_LITERAL(1), BITSET_CREATE_FILL(1, 0) };
    test_bitset("Testing partition of fill 4", b, 3, e6);
    bitset_free(b);

    uint32_t p7[] = { BITSET_CREATE_EMPTY_FILL(1) };
    b = bitset_new_buffer((const char *)p7, 4);
    test_bool("Testing partition of fill 5\n", false, bitset_set_to(b, 31, true));
    uint32_t e7[] = { BITSET_CREATE_FILL(1, 0) };
    test_bitset("Testing partition of fill 6", b, 1, e7);
    bitset_free(b);

    uint32_t p8[] = { BITSET_CREATE_FILL(1, 0), BITSET_CREATE_FILL(1, 2) };
    b = bitset_new_buffer((const char *)p8, 8);
    test_bool("Testing partition of fill 7\n", false, bitset_set_to(b, 0, true));
    uint32_t e8[] = { BITSET_CREATE_LITERAL(0), BITSET_CREATE_LITERAL(0), BITSET_CREATE_FILL(1, 2) };
    test_bitset("Testing partition of fill 7", b, 3, e8);
    bitset_free(b);

    uint32_t p8b[] = { BITSET_CREATE_FILL(2, 0), BITSET_CREATE_FILL(1, 2) };
    b = bitset_new_buffer((const char *)p8b, 8);
    test_bool("Testing partition of fill 7b\n", false, bitset_set_to(b, 32, true));
    uint32_t e8b[] = { BITSET_CREATE_FILL(1, 1), BITSET_CREATE_LITERAL(0), BITSET_CREATE_FILL(1, 2) };
    test_bitset("Testing partition of fill 7b - 3", b, 3, e8b);
    test_bool("Testing partition of fill 7b - 1\n", true, bitset_get(b, 32));
    test_bool("Testing partition of fill 7b - 2\n", true, bitset_get(b, 62));
    bitset_free(b);

    uint32_t p9[] = { BITSET_CREATE_FILL(3, 0), BITSET_CREATE_FILL(1, 2) };
    b = bitset_new_buffer((const char *)p9, 8);
    test_bool("Testing partition of fill 8\n", false, bitset_set_to(b, 32, true));
    uint32_t e9[] = { BITSET_CREATE_FILL(1, 1), BITSET_CREATE_FILL(1, 0), BITSET_CREATE_FILL(1, 2) };
    test_bitset("Testing partition of fill 9", b, 3, e9);
    bitset_free(b);

    uint32_t p10[] = { BITSET_CREATE_EMPTY_FILL(1), BITSET_CREATE_FILL(1, 0) };
    b = bitset_new_buffer((const char *)p10, 8);
    test_bool("Testing partition of fill 10\n", false, bitset_set_to(b, 1, true));
    uint32_t e10[] = { BITSET_CREATE_LITERAL(1), BITSET_CREATE_FILL(1, 0) };
    test_bitset("Testing partition of fill 11", b, 2, e10);
    bitset_free(b);

    uint32_t p11[] = { BITSET_CREATE_FILL(1, 0) };
    b = bitset_new_buffer((const char *)p11, 4);
    test_bool("Testing setting position bit 1\n", true, bitset_set_to(b, 31, true));
    test_bitset("Testing setting position bit 2", b, 1, p11);
    uint32_t e11[] = { BITSET_CREATE_EMPTY_FILL(1) };
    test_bool("Testing setting position bit 3\n", true, bitset_set_to(b, 31, false));
    test_bitset("Testing setting position bit 4", b, 1, e11);
    bitset_free(b);

    b = bitset_new();
    test_bool("Testing random set/get 1\n", false, bitset_set_to(b, 0, true));
    test_bool("Testing random set/get 1\n", false, bitset_set_to(b, 36, true));
    test_bool("Testing random set/get 1\n", false, bitset_set_to(b, 4, true));
    test_bool("Testing random set/get 2\n", true, bitset_get(b, 0));
    test_bool("Testing random set/get 2\n", true, bitset_get(b, 36));
    test_bool("Testing random set/get 2\n", true, bitset_get(b, 4));
    bitset_free(b);

    b = bitset_new();
    test_bool("Testing random set/get 1\n", false, bitset_set_to(b, 47, true));
    test_bool("Testing random set/get 1\n", false, bitset_set_to(b, 58, true));
    test_bool("Testing random set/get 1\n", false, bitset_set_to(b, 34, true));
    test_bool("Testing random set/get 3\n", true, bitset_get(b, 47));
    test_bool("Testing random set/get 4\n", true, bitset_get(b, 58));
    test_bool("Testing random set/get 5\n", true, bitset_get(b, 34));
    bitset_free(b);

    b = bitset_new();
    test_bool("Testing random set/get 1\n", false, bitset_set_to(b, 99, true));
    test_bool("Testing random set/get 1\n", false, bitset_set_to(b, 85, true));
    test_bool("Testing random set/get 1\n", false, bitset_set_to(b, 27, true));
    test_bool("Testing random set/get 6\n", true, bitset_get(b, 99));
    test_bool("Testing random set/get 7\n", true, bitset_get(b, 85));
    test_bool("Testing random set/get 8\n", true, bitset_get(b, 27));
    bitset_free(b);

    b = bitset_new();
    test_bool("Testing random set/get 1\n", false, bitset_set_to(b, 62, true));
    test_bool("Testing random set/get 1\n", false, bitset_set_to(b, 29, true));
    test_bool("Testing random set/get 1\n", false, bitset_set_to(b, 26, true));
    test_bool("Testing random set/get 1\n", false, bitset_set_to(b, 65, true));
    test_bool("Testing random set/get 1\n", false, bitset_set_to(b, 54, true));
    test_bool("Testing random set/get 6\n", true, bitset_get(b, 62));
    test_bool("Testing random set/get 7\n", true, bitset_get(b, 29));
    test_bool("Testing random set/get 8\n", true, bitset_get(b, 26));
    test_bool("Testing random set/get 8\n", true, bitset_get(b, 65));
    test_bool("Testing random set/get 8\n", true, bitset_get(b, 54));
    bitset_free(b);

    b = bitset_new();
    test_bool("Testing random set/get 1\n", false, bitset_set_to(b, 73, true));
    test_bool("Testing random set/get 1\n", false, bitset_set_to(b, 83, true));
    test_bool("Testing random set/get 1\n", false, bitset_set_to(b, 70, true));
    test_bool("Testing random set/get 1\n", false, bitset_set_to(b, 48, true));
    test_bool("Testing random set/get 1\n", false, bitset_set_to(b, 11, true));
    test_bool("Testing random set/get 6\n", true, bitset_get(b, 73));
    test_bool("Testing random set/get 7\n", true, bitset_get(b, 83));
    test_bool("Testing random set/get 8\n", true, bitset_get(b, 70));
    test_bool("Testing random set/get 8\n", true, bitset_get(b, 48));
    test_bool("Testing random set/get 8\n", true, bitset_get(b, 11));
    bitset_free(b);

    b = bitset_new();
    test_bool("Testing random set/get 1\n", false, bitset_set_to(b, 10, true));
    test_bool("Testing random set/get 1\n", false, bitset_set_to(b, 20, true));
    test_bool("Testing random set/get 1\n", false, bitset_set_to(b, 96, true));
    test_bool("Testing random set/get 1\n", false, bitset_set_to(b, 52, true));
    test_bool("Testing random set/get 1\n", false, bitset_set_to(b, 32, true));
    test_bool("Testing random set/get 6\n", true, bitset_get(b, 10));
    test_bool("Testing random set/get 7\n", true, bitset_get(b, 20));
    test_bool("Testing random set/get 8\n", true, bitset_get(b, 96));
    test_bool("Testing random set/get 8\n", true, bitset_get(b, 52));
    test_bool("Testing random set/get 8\n", true, bitset_get(b, 32));
    bitset_free(b);

    b = bitset_new();
    test_bool("Testing random set/get 1\n", false, bitset_set_to(b, 62, true));
    test_bool("Testing random set/get 1\n", false, bitset_set_to(b, 96, true));
    test_bool("Testing random set/get 1\n", false, bitset_set_to(b, 55, true));
    test_bool("Testing random set/get 1\n", false, bitset_set_to(b, 88, true));
    test_bool("Testing random set/get 1\n", false, bitset_set_to(b, 19, true));
    test_bool("Testing random set/get 6\n", true, bitset_get(b, 62));
    test_bool("Testing random set/get 7\n", true, bitset_get(b, 96));
    test_bool("Testing random set/get 8\n", true, bitset_get(b, 55));
    test_bool("Testing random set/get 8\n", true, bitset_get(b, 88));
    test_bool("Testing random set/get 8\n", true, bitset_get(b, 19));
    bitset_free(b);

    b = bitset_new();
    test_bool("Testing random set/get 1\n", false, bitset_set_to(b, 73, true));
    test_bool("Testing random set/get 1\n", false, bitset_set_to(b, 93, true));
    test_bool("Testing random set/get 1\n", false, bitset_set_to(b, 14, true));
    test_bool("Testing random set/get 1\n", false, bitset_set_to(b, 51, true));
    test_bool("Testing random set/get 1\n", false, bitset_set_to(b, 41, true));
    test_bool("Testing random set/get 6\n", true, bitset_get(b, 73));
    test_bool("Testing random set/get 7\n", true, bitset_get(b, 93));
    test_bool("Testing random set/get 8\n", true, bitset_get(b, 14));
    test_bool("Testing random set/get 8\n", true, bitset_get(b, 51));
    test_bool("Testing random set/get 8\n", true, bitset_get(b, 41));
    bitset_free(b);

    b = bitset_new();
    test_bool("Testing random set/get 1\n", false, bitset_set_to(b, 99, true));
    test_bool("Testing random set/get 1\n", false, bitset_set_to(b, 23, true));
    test_bool("Testing random set/get 1\n", false, bitset_set_to(b, 45, true));
    test_bool("Testing random set/get 1\n", false, bitset_set_to(b, 57, true));
    test_bool("Testing random set/get 1\n", false, bitset_set_to(b, 67, true));
    test_bool("Testing random set/get 6\n", true, bitset_get(b, 99));
    test_bool("Testing random set/get 7\n", true, bitset_get(b, 23));
    test_bool("Testing random set/get 8\n", true, bitset_get(b, 45));
    test_bool("Testing random set/get 8\n", true, bitset_get(b, 57));
    test_bool("Testing random set/get 8\n", true, bitset_get(b, 67));
    bitset_free(b);

    b = bitset_new();
    test_bool("Testing random set/get 1\n", false, bitset_set_to(b, 71, true));
    test_bool("Testing random set/get 1\n", false, bitset_set_to(b, 74, true));
    test_bool("Testing random set/get 1\n", false, bitset_set_to(b, 94, true));
    test_bool("Testing random set/get 1\n", false, bitset_set_to(b, 19, true));
    test_bool("Testing random set/get 6\n", true, bitset_get(b, 71));
    test_bool("Testing random set/get 7\n", true, bitset_get(b, 74));
    test_bool("Testing random set/get 8\n", true, bitset_get(b, 94));
    test_bool("Testing random set/get 8\n", true, bitset_get(b, 19));
    bitset_free(b);

    b = bitset_new();
    test_bool("Testing random set/get 1\n", false, bitset_set_to(b, 85, true));
    test_bool("Testing random set/get 1\n", false, bitset_set_to(b, 25, true));
    test_bool("Testing random set/get 1\n", false, bitset_set_to(b, 93, true));
    test_bool("Testing random set/get 1\n", false, bitset_set_to(b, 88, true));
    test_bool("Testing random set/get 1\n", false, bitset_set_to(b, 54, true));
    test_bool("Testing random set/get 6\n", true, bitset_get(b, 85));
    test_bool("Testing random set/get 7\n", true, bitset_get(b, 25));
    test_bool("Testing random set/get 8\n", true, bitset_get(b, 93));
    test_bool("Testing random set/get 8\n", true, bitset_get(b, 88));
    test_bool("Testing random set/get 8\n", true, bitset_get(b, 54));
    bitset_free(b);

    b = bitset_new();
    test_bool("Testing random set/get 1\n", false, bitset_set_to(b, 94, true));
    test_bool("Testing random set/get 1\n", false, bitset_set_to(b, 47, true));
    test_bool("Testing random set/get 1\n", false, bitset_set_to(b, 79, true));
    test_bool("Testing random set/get 1\n", false, bitset_set_to(b, 67, true));
    test_bool("Testing random set/get 1\n", false, bitset_set_to(b, 24, true));
    test_bool("Testing random set/get 6\n", true, bitset_get(b, 94));
    test_bool("Testing random set/get 7\n", true, bitset_get(b, 47));
    test_bool("Testing random set/get 8\n", true, bitset_get(b, 79));
    test_bool("Testing random set/get 8\n", true, bitset_get(b, 67));
    test_bool("Testing random set/get 8\n", true, bitset_get(b, 24));
    bitset_free(b);

#ifdef BITSET_64BIT_OFFSETS
    b = bitset_new(b);
    bitset_set_to(b, 1, true);
    bitset_set_to(b, 1000000000000, true);
    test_bool("Testing set where a chain of fills is required 1\n", true, bitset_get(b, 1));
    test_bool("Testing set where a chain of fills is required 2\n", true, bitset_get(b, 1000000000000));
    test_ulong("Testing set where a chain of fills is required 3\n", 2, bitset_count(b));
    bitset_free(b);
#endif
}

void test_suite_stress() {
    bitset_t *b = bitset_new();
    unsigned int max = 100000000, num = 1000;
    unsigned *bits = bitset_malloc(sizeof(unsigned) * num);
    srand(time(NULL));
    for (size_t i = 0; i < num; i++) {
        bits[i] = rand() % max;
        bitset_set_to(b, bits[i], true);
    }
    for (size_t i = 0; i < num; i++) {
        test_bool("Checking stress test bits were set", true, bitset_get(b, bits[i]));
    }
    for (size_t i = 0; i < 86400; i++) {
        bitset_count(b);
    }
    bitset_malloc_free(bits);
    bitset_free(b);
}

void test_suite_operation() {
    bitset_operation_t *ops;
    bitset_t *b1, *b2, *b3, *b4;

    b1 = bitset_new();
    b2 = bitset_new();
    b3 = bitset_new();
    bitset_set_to(b1, 100, true);
    //bitset_set_to(b1, 138, true);
    //bitset_set_to(b1, 169, true);
    bitset_set_to(b1, 200, true);
    bitset_set_to(b1, 300, true);
    bitset_set_to(b2, 100, true);
    bitset_set_to(b3, 300, true);
    bitset_set_to(b3, 400, true);
    ops = bitset_operation_new(b1);
    bitset_operation_add(ops, b2, BITSET_OR);
    bitset_operation_add(ops, b3, BITSET_OR);
    b4 = bitset_operation_exec(ops);
    test_int("Checking operation regression count 1\n", 4, bitset_count(b4));
    test_bool("Checking operation regression 1\n", true, bitset_get(b4, 100));
    test_bool("Checking operation regression 2\n", true, bitset_get(b4, 200));
    test_bool("Checking operation regression 3\n", true, bitset_get(b4, 300));
    test_bool("Checking opreation regression 4\n", true, bitset_get(b4, 400));
    bitset_operation_free(ops);
    bitset_free(b1);
    bitset_free(b2);
    bitset_free(b3);
    bitset_free(b4);

    b1 = bitset_new();
    b2 = bitset_new();
    b3 = bitset_new();
    bitset_set_to(b1, 2147483650, true);
    bitset_set_to(b1, 2147483760, true);
    bitset_set_to(b1, 3147483860, true);
    bitset_set_to(b2, 2147483760, true);
    bitset_set_to(b3, 300, true);
    bitset_set_to(b3, 3147483860, true);
    ops = bitset_operation_new(b1);
    bitset_operation_add(ops, b2, BITSET_OR);
    bitset_operation_add(ops, b3, BITSET_OR);
    b4 = bitset_operation_exec(ops);
    test_int("Checking operation regression count 2\n", 4, bitset_count(b4));
    test_bool("Checking operation regression 5\n", true, bitset_get(b4, 2147483650));
    test_bool("Checking operation regression 6\n", true, bitset_get(b4, 2147483760));
    test_bool("Checking operation regression 7\n", true, bitset_get(b4, 300));
    test_bool("Checking opreation regression 8\n", true, bitset_get(b4, 3147483860));
    bitset_operation_free(ops);
    bitset_free(b1);
    bitset_free(b2);
    bitset_free(b3);
    bitset_free(b4);

    b1 = bitset_new();
    b2 = bitset_new();
    b3 = bitset_new();
    bitset_set_to(b1, 100, true);
    bitset_set_to(b1, 200, true);
    bitset_set_to(b1, 199, true);
    bitset_set_to(b1, 300, true);
    bitset_set_to(b1, 10000, true);
    bitset_set_to(b2, 1, true);
    bitset_set_to(b2, 60, true);
    bitset_set_to(b2, 100, true);
    bitset_set_to(b2, 200, true);
    bitset_set_to(b2, 1000, true);
    bitset_set_to(b2, 2000, true);
    bitset_set_to(b2, 3000, true);
    bitset_set_to(b2, 10000, true);
    bitset_set_to(b3, 500, true);
    ops = bitset_operation_new(b1);
    bitset_operation_add(ops, b2, BITSET_AND);
    bitset_operation_add(ops, b3, BITSET_OR);
    b4 = bitset_operation_exec(ops);
    test_int("Checking operation regression count 3\n", 4, bitset_count(b4));
    test_bool("Checking operation regression 5\n", true, bitset_get(b4, 100));
    test_bool("Checking operation regression 6\n", true, bitset_get(b4, 200));
    test_bool("Checking opreation regression 7\n", true, bitset_get(b4, 500));
    test_bool("Checking operation regression 8\n", true, bitset_get(b4, 10000));
    bitset_operation_free(ops);
    bitset_free(b1);
    bitset_free(b2);
    bitset_free(b3);
    bitset_free(b4);

    b1 = bitset_new();
    b2 = bitset_new();
    b3 = bitset_new();
    bitset_set_to(b1, 100, true);
    bitset_set_to(b1, 200, true);
    bitset_set_to(b1, 199, true);
    bitset_set_to(b1, 300, true);
    bitset_set_to(b1, 10000, true);
    bitset_set_to(b2, 1, true);
    bitset_set_to(b2, 60, true);
    bitset_set_to(b2, 100, true);
    bitset_set_to(b2, 200, true);
    bitset_set_to(b2, 1000, true);
    bitset_set_to(b2, 2000, true);
    bitset_set_to(b2, 3000, true);
    bitset_set_to(b2, 10000, true);
    bitset_set_to(b3, 500, true);
    ops = bitset_operation_new(b2);
    bitset_operation_add(ops, b1, BITSET_AND);
    bitset_operation_add(ops, b3, BITSET_OR);
    b4 = bitset_operation_exec(ops);
    test_int("Checking operation regression count 4\n", 4, bitset_count(b4));
    test_bool("Checking operation regression 6\n", true, bitset_get(b4, 100));
    test_bool("Checking operation regression 7\n", true, bitset_get(b4, 200));
    test_bool("Checking opreation regression 8\n", true, bitset_get(b4, 500));
    test_bool("Checking operation regression 9\n", true, bitset_get(b4, 10000));
    bitset_operation_free(ops);
    bitset_free(b1);
    bitset_free(b2);
    bitset_free(b3);
    bitset_free(b4);

    b1 = bitset_new();
    bitset_set_to(b1, 10, true);
    b2 = bitset_new();
    bitset_set_to(b2, 20, true);
    b3 = bitset_new();
    bitset_set_to(b3, 12, true);
    ops = bitset_operation_new(b1);
    test_int("Checking initial operation length is one\n", 1, ops->length);
    test_bool("Checking primary bitset_t is added\n", true, bitset_get(&ops->steps[0]->data.bitset, 10));
    bitset_operation_add(ops, b2, BITSET_OR);
    test_int("Checking op length increases\n", 2, ops->length);
    bitset_operation_add(ops, b3, BITSET_OR);
    test_int("Checking op length increases\n", 3, ops->length);
    test_bool("Checking bitset was added correctly\n", true, bitset_get(&ops->steps[1]->data.bitset, 20));
    test_int("Checking op was added correctly\n", BITSET_OR, ops->steps[1]->type);
    test_bool("Checking bitset was added correctly\n", true, bitset_get(&ops->steps[2]->data.bitset, 12));
    test_int("Checking op was added correctly\n", BITSET_OR, ops->steps[2]->type);
    test_ulong("Checking operation count 1\n", 3, bitset_operation_count(ops));
    bitset_operation_free(ops);
    bitset_free(b1);
    bitset_free(b2);
    bitset_free(b3);

    b1 = bitset_new();
    b2 = bitset_new();
    b3 = bitset_new();
    bitset_set_to(b1, 1000, true);
    bitset_set_to(b2, 100, true);
    bitset_set_to(b3, 20, true);
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
    bitset_set_to(b1, 102, true);
    bitset_set_to(b1, 10000, true);
    bitset_set_to(b2, 100, true);
    bitset_set_to(b3, 20, true);
    bitset_set_to(b3, 101, true);
    bitset_set_to(b3, 20000, true);
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
    bitset_set_to(b1, 101, true);
    bitset_set_to(b1, 8000, true);
    bitset_set_to(b2, 100, true);
    bitset_set_to(b3, 20, true);
    bitset_set_to(b3, 101, true);
    bitset_set_to(b3, 8001, true);
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
    bitset_set_to(b1, 101, true);
    bitset_set_to(b1, 102, true);
    bitset_set_to(b2, 1000, true);
    bitset_set_to(b3, 101, true);
    bitset_set_to(b3, 1000, true);
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
    bitset_set_to(b1, 1000, true);
    bitset_set_to(b2, 100, true);
    bitset_set_to(b2, 105, true);
    bitset_set_to(b2, 130, true);
    bitset_set_to(b3, 20, true);
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
    bitset_set_to(b1, 1000, true);
    bitset_set_to(b1, 1001, true);
    bitset_set_to(b1, 1100, true);
    bitset_set_to(b1, 3, true);
    bitset_set_to(b2, 1000, true);
    bitset_set_to(b2, 1101, true);
    bitset_set_to(b2, 3, true);
    bitset_set_to(b2, 130, true);
    bitset_set_to(b3, 1000, true);
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
    b3 = bitset_new();
    bitset_set_to(b1, 100, true);
    bitset_set_to(b1, 200, true);
    bitset_set_to(b1, 300, true);
    bitset_set_to(b2, 100, true);
    bitset_set_to(b3, 300, true);
    bitset_set_to(b3, 400, true);
    ops = bitset_operation_new(b1);
    bitset_operation_t *op2 = bitset_operation_new(b2);
    bitset_operation_add(op2, b3, BITSET_OR);
    bitset_operation_add_nested(ops, op2, BITSET_AND);
    b4 = bitset_operation_exec(ops);
    test_int("Checking nested operation count 1\n", 2, bitset_count(b4));
    test_bool("Checking nested operation get 1\n", true, bitset_get(b4, 100));
    test_bool("Checking nested operation get 2\n", true, bitset_get(b4, 300));
    bitset_operation_free(ops);
    bitset_free(b1);
    bitset_free(b2);
    bitset_free(b3);
    bitset_free(b4);

    b1 = bitset_new();
    b2 = bitset_new();
    b3 = bitset_new();
    bitset_set_to(b1, 100, true);
    bitset_set_to(b1, 200, true);
    bitset_set_to(b1, 300, true);
    bitset_set_to(b2, 100, true);
    bitset_set_to(b3, 300, true);
    bitset_set_to(b3, 400, true);
    ops = bitset_operation_new(b1);
    op2 = bitset_operation_new(b2);
    bitset_operation_add(op2, b3, BITSET_OR);
    bitset_operation_add_nested(ops, op2, BITSET_OR);
    b4 = bitset_operation_exec(ops);
    test_int("Checking nested operation count 2\n", 4, bitset_count(b4));
    test_bool("Checking nested operation get 3\n", true, bitset_get(b4, 100));
    test_bool("Checking nested operation get 4\n", true, bitset_get(b4, 200));
    test_bool("Checking nested operation get 5\n", true, bitset_get(b4, 300));
    test_bool("Checking nested operation get 6\n", true, bitset_get(b4, 400));
    bitset_operation_free(ops);
    bitset_free(b1);
    bitset_free(b2);
    bitset_free(b3);
    bitset_free(b4);

#ifdef BITSET_64BIT_OFFSETS
    b1 = bitset_new();
    b2 = bitset_new();
    bitset_set_to(b1, 1, true);
    bitset_set_to(b2, 10000000000, true);
    bitset_set_to(b2, 100000000000, true);
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
#endif
}

void test_suite_vector() {

    bitset_vector_t *l, *l2, *l3;
    bitset_t *b;
    bitset_word *tmp;
    unsigned loop_count;
    unsigned offset, raw, unique;

    l = bitset_vector_new();
    test_int("Checking vector length is zero initially\n", 0, bitset_vector_length(l));
    test_int("Checking vector size is one initially\n", 1, l->size);
    test_int("Checking vector counts are initially zero\n", 0, bitset_vector_bitsets(l));
    loop_count = 0;
    BITSET_VECTOR_FOREACH(l, b, offset) {
        loop_count++;
    }
    test_int("Checking an empty iterator is safe to use with foreach\n", 0, loop_count);
    bitset_vector_free(l);

    l = bitset_vector_new();
    b = bitset_new();
    bitset_vector_push(l, b, 0);
    test_int("Checking vector bitset count 1\n", 1, bitset_vector_bitsets(l));
    test_int("Checking vector was resized properly 1\n", 4, l->size);
    test_int("Checking vector was resized properly 2\n", 4, l->length);
    test_int("Checking the offset is zero\n", 0, (unsigned char)l->buffer[0]);
    test_int("Checking the length is zero\n", 0, (unsigned char)l->buffer[1]);
    bitset_free(b);
    bitset_vector_free(l);

    l = bitset_vector_new();
    b = bitset_new();
    bitset_set_to(b, 10, true);
    bitset_vector_push(l, b, 3);
    test_int("Checking vector was resized properly 1\n", 8, l->size);
    test_int("Checking vector was resized properly 2\n", 8, l->length);
    tmp = b->buffer;
    b->buffer = (bitset_word *) (l->buffer + 4);
    test_bool("Checking bitset was added properly 1\n", true, bitset_get(b, 10));
    test_bool("Checking bitset was added properly 2\n", false, bitset_get(b, 100));
    b->buffer = tmp;
    bitset_free(b);


    b = bitset_new();
    bitset_set_to(b, 100, true);
    bitset_set_to(b, 1000, true);
    bitset_vector_push(l, b, 10);
    test_int("Checking vector bitset count 2\n", 2, bitset_vector_bitsets(l));
    test_int("Checking vector was resized properly 4\n", 32, l->size);
    test_int("Checking vector was resized properly 5\n", 20, l->length);
    tmp = b->buffer;
    b->buffer = (bitset_word *) (l->buffer + 12);
    test_bool("Checking bitset was added properly 3\n", true, bitset_get(b, 100));
    test_bool("Checking bitset was added properly 4\n", true, bitset_get(b, 1000));
    test_bool("Checking bitset was added properly 5\n", false, bitset_get(b, 10));
    b->buffer = tmp;
    bitset_free(b);

    b = bitset_new();
    bitset_set(b, 10000);
    bitset_set(b, 20000);
    l2 = bitset_vector_new();
    bitset_vector_push(l2, b, 13600);
    bitset_free(b);
    loop_count = 0;
    BITSET_VECTOR_FOREACH(l2, b, offset) {
        loop_count++;
        test_bool("Checking vector medium offset 1\n", true, offset == 13600);
        test_bool("Checking vector medium offset 2\n", true, bitset_get(b, 10000));
        test_bool("Checking vector medium offset 3\n", true, bitset_get(b, 20000));
        test_int("Checking vector medium offset 4\n", 2, bitset_count(b));
    }
    test_int("Checking it looped the right number of times a\n", 1, loop_count);
    bitset_vector_free(l2);

    loop_count = 0;
    BITSET_VECTOR_FOREACH(l, b, offset) {
        loop_count++;
        test_bool("Checking foreach works\n", true, offset == 3 || offset == 10);
        if (offset == 3) {
            test_int("Checking bitset count 1\n", 1, bitset_count(b));
            test_bool("Checking bitset was added properly to iter 6\n", true, bitset_get(b, 10));
            test_bool("Checking bitset was added properly to iter 7\n", false, bitset_get(b, 100));
        } else if (offset == 10) {
            test_int("Checking bitset count 2\n", 2, bitset_count(b));
            test_bool("Checking bitset was added properly to iter 8\n", false, bitset_get(b, 10));
            test_bool("Checking bitset was added properly to iter 9\n", true, bitset_get(b, 100));
            test_bool("Checking bitset was added properly to iter 10\n", true, bitset_get(b, 1000));
        }
    }
    test_int("Checking it looped the right number of times\n", 2, loop_count);

    b = bitset_vector_merge(l);
    test_int("Checking vector merge 1\n", 3, bitset_count(b));
    test_bool("Checking vector merge 2\n", true, bitset_get(b, 10));
    test_bool("Checking vector merge 2\n", true, bitset_get(b, 100));
    test_bool("Checking vector merge 2\n", true, bitset_get(b, 1000));
    bitset_free(b);

    l3 = bitset_vector_new();
    bitset_vector_concat(l3, l, 0, 3, 10);
    loop_count = 0;
    BITSET_VECTOR_FOREACH(l3, b, offset) {
        loop_count++;
        test_bool("Checking foreach works 2\n", true, offset == 3);
    }
    test_int("Checking it looped the right number of times 2\n", 1, loop_count);
    bitset_vector_free(l3);
    l3 = bitset_vector_new();
    bitset_vector_concat(l3, l, 0, 4, 5);
    loop_count = 0;
    BITSET_VECTOR_FOREACH(l3, b, offset) {
        loop_count++;
    }
    test_int("Checking it looped the right number of times 3\n", 0, loop_count);
    bitset_vector_free(l3);

    l3 = bitset_vector_new();
    bitset_vector_concat(l3, l, 0, BITSET_VECTOR_START, BITSET_VECTOR_END);
    bitset_vector_concat(l3, l, 11, BITSET_VECTOR_START, BITSET_VECTOR_END);
    bitset_vector_cardinality(l3, &raw, &unique);
    test_int("Checking vector bitset count 3\n", 4, bitset_vector_bitsets(l3));
    test_int("Checking vector bitset count 4\n", 6, raw);
    test_int("Checking vector bitset count 5\n", 3, unique);
    loop_count = 0;
    BITSET_VECTOR_FOREACH(l3, b, offset) {
        loop_count++;
        test_bool("Checking foreach works 3\n", true, offset == 3 ||
            offset == 10 || offset == 14 || offset == 21);
    }
    test_int("Checking it looped the right number of times 4\n", 4, loop_count);
    test_int("Checking tail offset\n", 21, l3->tail_offset);
    bitset_vector_free(l3);

    l3 = bitset_vector_new();
    bitset_vector_concat(l3, l, 100000, BITSET_VECTOR_START, BITSET_VECTOR_END);
    bitset_vector_concat(l3, l, 200000, BITSET_VECTOR_START, BITSET_VECTOR_END);
    bitset_vector_cardinality(l3, &raw, &unique);
    test_int("Checking vector bitset count 3\n", 4, bitset_vector_bitsets(l3));
    test_int("Checking vector bitset count 4\n", 6, raw);
    test_int("Checking vector bitset count 5\n", 3, unique);
    loop_count = 0;
    BITSET_VECTOR_FOREACH(l3, b, offset) {
        loop_count++;
        test_bool("Checking foreach works 3\n", true, offset == 100003 ||
            offset == 100010 || offset == 200003 || offset == 200010);
    }
    test_int("Checking it looped the right number of times 4\n", 4, loop_count);
    test_int("Checking tail offset\n", 200010, l3->tail_offset);
    bitset_vector_free(l3);

    //Make a copy of the buffer
    char *buffer = bitset_malloc(sizeof(char) * l->length);
    memcpy(buffer, l->buffer, l->length);
    unsigned length = l->length;
    bitset_vector_free(l);

    //Check the copy is the same
    l = bitset_vector_import(buffer, length);
    test_int("Check size is copied\n", 32, l->size);
    test_int("Check length is copied\n", 20, l->length);
    test_int("Check tail_offset is copied\n", 10, l->tail_offset);
    bitset_vector_free(l);
    bitset_malloc_free(buffer);
}

void test_suite_vector_operation() {
    bitset_vector_operation_t *o1, *o2;
    bitset_vector_t *v1, *v2, *v3, *v4, *v5;
    bitset_t *b1, *b2, *b3, *b4;
    unsigned offset;

    b1 = bitset_new();
    bitset_set(b1, 100);
    b2 = bitset_new();
    bitset_set(b2, 100);
    b3 = bitset_new();
    bitset_set(b3, 100);
    b4 = bitset_new();
    bitset_set(b4, 100);
    v1 = bitset_vector_new();
    bitset_vector_push(v1, b1, 1);
    bitset_vector_push(v1, b2, 2);
    bitset_vector_push(v1, b3, 3);
    bitset_vector_push(v1, b4, 4);
    bitset_free(b1);
    bitset_free(b2);
    bitset_free(b3);
    bitset_free(b4);

    b1 = bitset_new();
    bitset_set(b1, 200);
    b2 = bitset_new();
    bitset_set(b2, 100);
    bitset_set(b2, 200);
    b3 = bitset_new();
    bitset_set(b3, 200);
    b4 = bitset_new();
    bitset_set(b4, 200);
    v2 = bitset_vector_new();
    bitset_vector_push(v2, b1, 2);
    bitset_vector_push(v2, b2, 4);
    bitset_vector_push(v2, b3, 6);
    bitset_vector_push(v2, b4, 8);
    bitset_free(b1);
    bitset_free(b2);
    bitset_free(b3);
    bitset_free(b4);

    b1 = bitset_new();
    bitset_set(b1, 300);
    b2 = bitset_new();
    bitset_set(b2, 100);
    bitset_set(b2, 300);
    b3 = bitset_new();
    bitset_set(b3, 300);
    b4 = bitset_new();
    bitset_set(b4, 300);
    v3 = bitset_vector_new();
    bitset_vector_push(v3, b1, 1);
    bitset_vector_push(v3, b2, 2);
    bitset_vector_push(v3, b3, 3);
    bitset_vector_push(v3, b4, 4);
    bitset_free(b1);
    bitset_free(b2);
    bitset_free(b3);
    bitset_free(b4);

    b1 = bitset_new();
    bitset_set(b1, 400);
    b2 = bitset_new();
    bitset_set(b2, 400);
    b3 = bitset_new();
    bitset_set(b3, 400);
    b4 = bitset_new();
    bitset_set(b4, 400);
    v4 = bitset_vector_new();
    bitset_vector_push(v4, b1, 1);
    bitset_vector_push(v4, b2, 2);
    bitset_vector_push(v4, b3, 3);
    bitset_vector_push(v4, b4, 4);
    bitset_free(b1);
    bitset_free(b2);
    bitset_free(b3);
    bitset_free(b4);

    //V1 AND (V2 | V3) OR V4
    o1 = bitset_vector_operation_new(v1);
    o2 = bitset_vector_operation_new(v2);
    bitset_vector_operation_add(o2, v3, BITSET_OR);
    bitset_vector_operation_add_nested(o1, o2, BITSET_AND);
    bitset_vector_operation_add(o1, v4, BITSET_OR);
    v5 = bitset_vector_operation_exec(o1);
    bitset_vector_operation_free(o1);

    unsigned loop_count = 0;
    BITSET_VECTOR_FOREACH(v5, b1, offset) {
        loop_count++;
        if (offset == 1) {
            test_int("Check vector operation count 1\n", 1, bitset_count(b1));
            test_int("Check vector operation bitset 1\n", true, bitset_get(b1, 400));
        } else if (offset == 2) {
            test_int("Check vector operation count 2\n", 2, bitset_count(b1));
            test_int("Check vector operation bitset 2\n", true, bitset_get(b1, 100));
            test_int("Check vector operation bitset 3\n", true, bitset_get(b1, 400));
        } else if (offset == 3) {
            test_int("Check vector operation count 3\n", 1, bitset_count(b1));
            test_int("Check vector operation bitset 4\n", true, bitset_get(b1, 400));
        } else if (offset == 4) {
            test_int("Check vector operation count 4\n", 2, bitset_count(b1));
            test_int("Check vector operation bitset 5\n", true, bitset_get(b1, 100));
            test_int("Check vector operation bitset 6\n", true, bitset_get(b1, 400));
        } else {
            test_bool("", false, true);
        }
    }
    test_int("Check vector operation looped right amount of times\n", 4, loop_count);

    bitset_vector_free(v1);
    bitset_vector_free(v2);
    bitset_vector_free(v3);
    bitset_vector_free(v4);
    bitset_vector_free(v5);
}

void test_suite_estimate() {
    BITSET_NEW(b1, 100);
    BITSET_NEW(b2, 101);
    BITSET_NEW(b3, 102);
    BITSET_NEW(b4, 100, 101, 102);
    bitset_linear_t *l = bitset_linear_new(102);
    bitset_linear_add(l, b1);
    bitset_linear_add(l, b2);
    bitset_linear_add(l, b3);
    bitset_linear_add(l, b4);
    test_int("Test linear count\n", 3, bitset_linear_count(l));
    bitset_linear_free(l);
    bitset_free(b1);
    bitset_free(b2);
    bitset_free(b3);
    bitset_free(b4);

    BITSET_NEW(b5, 1, 2, 3, 40, 41, 42, 43, 51);
    BITSET_NEW(b6, 1, 3, 41, 43);
    BITSET_NEW(b7, 1);
    bitset_countn_t *c = bitset_countn_new(1, 100);
    bitset_countn_add(c, b5);
    bitset_countn_add(c, b6);
    bitset_countn_add(c, b7);
    test_int("Test countn count where N=1\n", 4, bitset_countn_count(c));
    bitset_countn_free(c);
    c = bitset_countn_new(2, 100);
    bitset_countn_add(c, b5);
    bitset_countn_add(c, b6);
    bitset_countn_add(c, b7);
    test_int("Test countn count where N=2\n", 3, bitset_countn_count(c));
    bitset_countn_free(c);
    c = bitset_countn_new(3, 100);
    bitset_countn_add(c, b5);
    bitset_countn_add(c, b6);
    bitset_countn_add(c, b7);
    test_int("Test countn count where N=3\n", 1, bitset_countn_count(c));
    unsigned *all = bitset_countn_count_all(c);
    test_int("Test countn count_all where N=1\n", 4, all[0]);
    test_int("Test countn count_all where N=2\n", 3, all[1]);
    test_int("Test countn count_all where N=3\n", 1, all[2]);
    bitset_malloc_free(all);

    BITSET_NEW(mask, 2, 4, 5, 40, 41);
    all = bitset_countn_count_mask(c, mask);
    test_int("Test countn count_mask where N=1\n", 2, all[0]);
    test_int("Test countn count_mask where N=2\n", 1, all[1]);
    test_int("Test countn count_mask where N=3\n", 0, all[2]);
    bitset_free(mask);
    bitset_malloc_free(all);

    bitset_countn_free(c);
    bitset_free(b5);
    bitset_free(b6);
    bitset_free(b7);
}

