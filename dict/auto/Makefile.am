SUBDIRS = .

EXTRA_DIST = auto.dat

dicdir = $(pkgdatadir)/dict/auto

nodist_dic_DATA = auto.db

if NV_MI_EXIST
nodist_dic_DATA += nv_mi.db
endif

all: auto.db

auto.dat:
	true

auto.db: auto.dat
	if [ -f $< ]; then \
		@MAKE_DB_COMMAND@ $@ < $<; \
	fi

nv_mi.db: nv_mi.dat
	if [ -f $< ]; then \
		@MAKE_DB_COMMAND@ $@ < $<; \
	fi

CLEANFILES = auto.db nv_mi.db
