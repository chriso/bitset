#ifndef BITSET_MALLOC_H_
#define BITSET_MALLOC_H_

#include <stdlib.h>

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#if defined(has_tcmalloc)
#  include <google/tcmalloc.h>
#elif defined(has_jemalloc)
#  include <jemalloc/jemalloc.h>
#elif defined(LINUX)
#  include <malloc.h>
#endif

void *bitset_malloc(size_t);

void bitset_malloc_free(void *);

void *bitset_calloc(size_t, size_t);

void *bitset_realloc(void *, size_t);

int bitset_mallopt(int, int);

#endif

