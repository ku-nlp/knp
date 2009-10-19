SUBDIRS = .

EXTRA_DIST = auto.dat

dicdir = $(pkgdatadir)/dict/auto

nodist_dic_DATA = auto.db

all: auto.db

auto.dat:
	true

auto.db: auto.dat
	if [ -f $< ]; then \
		@MAKE_DB_COMMAND@ $@ < $<; \
	fi

CLEANFILES = auto.db
