SUBDIRS = .

EXTRA_DIST = sm2code.dat

dicdir = $(pkgdatadir)
nodist_dic_DATA = bgh.db sm2code.db

bgh.dat:
	if [ ! -f bgh.dat -a -f "@BGH_FILE@" ]; then \
		nkf -S -e < "@BGH_FILE@" | ../src/bgh.pl > bgh.dat; \
	fi

bgh.db: bgh.dat
	if [ -f bgh.dat ]; then \
		@MAKE_DB_COMMAND@ $@ < $<; \
	fi

sm2code.db: sm2code.dat
	@MAKE_DB_COMMAND@ $@ < $<

CLEANFILES = bgh.db sm2code.db
