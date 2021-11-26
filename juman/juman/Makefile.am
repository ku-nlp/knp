noinst_PROGRAMS = juman
prog_SOURCES = version.h
# include_HEADERS = version.h
EXTRA_DIST = version.h
CLEANFILES = version.h

INCLUDES = -I../lib

juman_SOURCES = client.c juman.c server.c
juman_LDADD = $(top_builddir)/lib/libjuman.la @NET_LIBS@ -lstdc++

juman.c: version.h

version.h: $(top_srcdir)/VERSION.txt
	$(AM_V_GEN)(\
	      echo "#define REVISION_VERSION \"$(shell cat $(top_srcdir)/VERSION)\"" >> $@-new; \
	cmp -s $@ $@-new || cp $@-new $@; \
	rm $@-new)
