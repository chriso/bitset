#include "bitset/malloc.h"

void *bitset_malloc(size_t size) {
#if defined(has_jemalloc)
    return je_malloc(size);
#elif defined(has_tcmalloc)
    return tc_malloc(size);
#else
    return malloc(size);
#endif
}

void bitset_malloc_free(void *ptr) {
#if defined(has_jemalloc)
    return je_free(ptr);
#elif defined(has_tcmalloc)
    return tc_free(ptr);
#else
    return free(ptr);
#endif
}

void *bitset_calloc(size_t count, size_t size) {
#if defined(has_jemalloc)
    return je_calloc(count, size);
#elif defined(has_tcmalloc)
    return tc_calloc(count, size);
#else
    return calloc(count, size);
#endif
}

void *bitset_realloc(void *ptr, size_t size) {
#if defined(has_jemalloc)
    return je_realloc(ptr, size);
#elif defined(has_tcmalloc)
    return tc_realloc(ptr, size);
#else
    return realloc(ptr, size);
#endif
}

int bitset_mallopt(int param, int value) {
#if !defined(has_jemalloc) && !defined(has_tcmalloc) && defined(LINUX)
    return mallopt(param, value);
#else
    return 1;
#endif
}

