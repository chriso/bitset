#!/bin/sh

if test -f Makefile; then
  make distclean
fi

mkdir -p m4

rm -rf *~ *.cache *.m4 config.guess config.log \
	config.status config.sub depcomp ltmain.sh

autoreconf -f -i
