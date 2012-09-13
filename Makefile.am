ACLOCAL_AMFLAGS=-I m4

SUBDIRS=include lib test

EXTRA_DIST= README.md

MAINTAINERCLEANFILES = Makefile.in lib/Makefile.in include/Makefile.in \
	test/Makefile.in m4/config.guess m4/config.sub m4/depcomp m4/install-sh \
	m4/libtool.m4 m4/ltmain.sh m4/ltoptions.m4 m4/ltsugar.m4 m4/ltversion.m4 \
	m4/lt~obsolete.m4 m4/missing config.guess config.h.in config.h.in~ \
	configure depcomp install-sh ltmain.sh missing config.sub aclocal.m4

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = bitset.pc
