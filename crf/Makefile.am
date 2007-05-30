SUBDIRS = .

EXTRA_DIST = crf.cc crf.h

if HAVE_CRFPP
noinst_LTLIBRARIES = libcrf.la

libcrf_la_SOURCES = crf.cc crf.h
libcrf_la_LDFLAGS = -version-info @LIBCRF_VERSION_INFO@
endif
