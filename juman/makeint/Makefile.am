pkglibexecdir = $(libexecdir)/juman
pkglibexec_PROGRAMS = makeint

INCLUDES = -I../lib

makeint_SOURCES = makeint.c trans.c makeint.h
makeint_LDADD = $(top_builddir)/lib/libjuman.la -lstdc++
