#!/bin/sh

aclocal
libtoolize --force
autoheader
automake -a
autoconf

cd ./CRF++-0.58

aclocal
libtoolize
autoheader
automake -a
autoconf


