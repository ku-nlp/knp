SUBDIRS = .

EXTRA_DIST = darts_for_knp.cc darts_for_knp.h darts.h

noinst_LTLIBRARIES = libdarts.la

libdarts_la_SOURCES = darts_for_knp.cc darts_for_knp.h darts.h
libdarts_la_LDFLAGS = -version-info 0:0:1

pkglibexecdir= $(libexecdir)/knp
pkglibexec_PROGRAMS = mkdarts

mkdarts_SOURCES = mkdarts.cc
