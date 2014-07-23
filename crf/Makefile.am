SUBDIRS = .

EXTRA_DIST = crf.cc crf.h

INCLUDES = @KNP_CFLAGS@
noinst_LTLIBRARIES = libcrf.la

libcrf_la_SOURCES = crf.cc crf.h
libcrf_la_LDFLAGS = -version-info @LIBCRF_VERSION_INFO@

all:
	$(MAKE) -C ../CRF++-0.58

clean:
	rm -rf crf.lo crf.o libcrf.la .libs
	$(MAKE) -C ../CRF++-0.58 clean
