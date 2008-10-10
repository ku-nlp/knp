SUBDIRS = .

EXTRA_DIST = synonym.idx

dicdir = $(pkgdatadir)/dict/synonym

nodist_dic_DATA = synonym.db

all: synonym.db

synonym.idx:
	true

synonym.db: synonym.idx
	if [ -f $< ]; then \
	        LANG=C sort synonym.idx | @MAKE_DB_COMMAND@ $@ -append /; \
	fi

CLEANFILES = synonym.db
