SUBDIRS = .

EXTRA_DIST = svm.c svm_common.h

if HAVE_SVMLIGHT
noinst_LTLIBRARIES = libsvm.la

libsvm_la_SOURCES = svm.c svm.h
libsvm_la_LDFLAGS = -version-info @LIBSVM_VERSION_INFO@
endif
