EXTRA_DIST = sm2code.dat

if BGH_EXIST
dicdir = $(pkgdatadir)/dict/scode/bgh
nodist_dic_DATA = bgh.db sm2code.db
endif

bgh.dat:
	if [ ! -f bgh.dat -a -f "@BGH_FILE@" ]; then \
		../../src/bgh.pl < "@BGH_FILE@" > bgh.dat; \
	fi

bgh.db: bgh.dat
	if [ -f bgh.dat ]; then \
		@MAKE_DB_COMMAND@ $@ < $<; \
	fi

sm2code.db: sm2code.dat
	@MAKE_DB_COMMAND@ $@ < $<

CLEANFILES = bgh.db sm2code.db
