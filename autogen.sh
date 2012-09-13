#!/bin/sh

if test -f Makefile; then
  make maintainer-clean
fi

autoreconf -f -i