#if defined(has_tcmalloc)
#include <google/tcmalloc.h>
#else
#if defined(has_jemalloc)
#include <jemalloc/jemalloc.h>
#endif
#include 'malloc.h'
#endif

int bitset_mallopt(int param, int value)
{
#if defined(has_jemalloc) || defined(has_tcmalloc)
#if defined(LINUX)
  return mallopt(param, value);
#endif
  return 0;
#endif
}