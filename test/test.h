#ifndef TEST_H_
#define TEST_H_

void test_suite_macros();
void test_suite_get();
void test_suite_set();
void test_suite_stress();
void test_suite_count();
void test_suite_operation();
void test_suite_min();
void test_suite_max();
void test_suite_vector();
void test_suite_vector_operation();
void test_suite_estimate();

void test_bool(char *, bool, bool);
void test_ulong(char *, unsigned long, unsigned long);
void test_int(char *, int, int);
bool test_bitset(char *, bitset_t *, unsigned, uint32_t *);
void bitset_dump(bitset_t *);

#endif
