SUBDIRS = .

EXTRA_DIST = sm2code.dat bgh.orig

if BGH_EXIST
dicdir = $(pkgdatadir)/dict/scode/bgh
nodist_dic_DATA = bgh.db sm2code.db
endif

usr_bgh.dat:
	touch $@

bgh.dat: bgh.orig usr_bgh.dat ../../src/add_usr_bgh.pl
	if [ -f bgh.orig ]; then \
		if [ -s usr_bgh.dat ]; then \
			../../src/add_usr_bgh.pl bgh.orig < usr_bgh.dat > $@; \
		else \
			cp -f bgh.orig $@; \
		fi \
	fi

bgh.db: bgh.dat
	if [ -f bgh.dat ]; then \
		LANG=C sort $< | @MAKE_DB_COMMAND@ $@; \
	fi

sm2code.db: sm2code.dat
	@MAKE_DB_COMMAND@ $@ < $<

CLEANFILES = bgh.db bgh.dat sm2code.db
