SUBDIRS = .

EXTRA_DIST = svm.cc svm.h

if HAVE_SVM
noinst_LTLIBRARIES = libsvm.la

libsvm_la_SOURCES = svm.cc svm.h
libsvm_la_LDFLAGS = -version-info @LIBSVM_VERSION_INFO@
endif
