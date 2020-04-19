#!/bin/sh

aclocal
libtoolize --force
autoheader
automake -a
autoconf
