SUBDIRS = .

EXTRA_DIST = darts_for_juman.cc darts_for_juman.h darts.h

noinst_LTLIBRARIES = libdarts.la

libdarts_la_SOURCES = darts_for_juman.cc darts_for_juman.h darts.h mmap.h
libdarts_la_LDFLAGS = -version-info 0:0:1

pkglibexecdir= $(libexecdir)/juman
pkglibexec_PROGRAMS = mkdarts

mkdarts_SOURCES = mkdarts.cc
