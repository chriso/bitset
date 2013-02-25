TESTS = test

check_PROGRAMS = $(TESTS)
EXTRA_PROGRAMS = stress

LDADD= $(top_builddir)/lib/libbitset.la
AM_CPPFLAGS = -I$(top_srcdir)/include
AM_CFLAGS= -std=c99 -Wall

test_SOURCES = test.c
stress = stress.c

EXTRA_DIST = test.h
