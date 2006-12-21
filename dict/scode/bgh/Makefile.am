SUBDIRS = .

EXTRA_DIST = sm2code.dat

if BGH_EXIST
dicdir = $(pkgdatadir)/dict/scode/bgh
nodist_dic_DATA = bgh.db sm2code.db
endif

usr_bgh.dat:
	touch $@

bgh.orig:
	if [ ! -f bgh.dat -a -f "@BGH_FILE@" ]; then \
		../../src/bgh.pl < "@BGH_FILE@" > bgh.orig; \
	fi

bgh.dat: usr_bgh.dat ../../src/add_usr_bgh.pl
	if [ -f bgh.orig -a -f usr_bgh.dat ]; then \
		../../src/add_usr_bgh.pl bgh.orig < usr_bgh.dat > $@; \
	fi

bgh.db: bgh.dat
	if [ -f bgh.dat ]; then \
		sort $< | @MAKE_DB_COMMAND@ $@; \
	fi

sm2code.db: sm2code.dat
	@MAKE_DB_COMMAND@ $@ < $<

CLEANFILES = bgh.db bgh.dat sm2code.db
