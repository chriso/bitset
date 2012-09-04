#include "bitset/malloc.h"

int bitset_mallopt(int param, int value) {
#if !defined(has_jemalloc) && !defined(has_tcmalloc) && defined(LINUX)
    return mallopt(param, value);
#else
    return 1;
#endif
}

