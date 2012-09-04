#ifndef BITSET_MALLOC_H_
#define BITSET_MALLOC_H_

#if defined(has_tcmalloc)
#  include <google/tcmalloc.h>
#elif defined(has_jemalloc)
#  include <jemalloc/jemalloc.h>
#elif defined(LINUX)
#  include <malloc.h>
#endif

int bitset_mallopt(int, int);

#endif

