pkglibexecdir = $(libexecdir)/juman
pkglibexec_PROGRAMS = makemat

INCLUDES = -I../lib

makemat_SOURCES = makemat.c makemat.h
makemat_LDADD = $(top_builddir)/lib/libjuman.la -lstdc++
