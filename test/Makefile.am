TESTS= test stress

check_PROGRAMS=$(TESTS)

LDADD= $(top_builddir)/lib/libbitset.la
AM_CPPFLAGS = -I$(top_srcdir)/include
AM_CFLAGS= -std=c99 -Wall

test_SOURCES=test.c
stress_SOURCES=stress.c

EXTRA_DIST = test.h
