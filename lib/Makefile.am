INCLUDES = -I$(top_srcdir)/include
AM_CFLAGS= -std=c99 -Wall

lib_LTLIBRARIES = libbitset.la
libbitset_la_SOURCES = bitset.c estimate.c operation.c vector.c malloc.c
libbitset_la_LDFLAGS = $(AM_LDFLAGS) \
    -version-info @version_info@ \
    -no-undefined
