include_HEADERS = juman.h juman_pat.h

lib_LTLIBRARIES = libjuman.la

libjuman_la_CFLAGS = -I../darts
libjuman_la_SOURCES = connect.c const.h getid.c grammar.c iotool.c katuyou.c lisp.c pat.c patfile.c zentohan.c juman.h juman_lib.c juman_pat.h japanese.c
libjuman_la_LDFLAGS = -version-info @LIBJUMAN_VERSION_INFO@
libjuman_la_LIBADD = $(top_builddir)/darts/libdarts.la
