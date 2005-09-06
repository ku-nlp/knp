EXTRA_DIST = scase.dat

dicdir = $(pkgdatadir)/dict/gcf
nodist_dic_DATA = scase.db


EDR_FILE = /share/dict/EDR/tdict/jwd/JWD.DIC

# scase.dat:
# 	if [ ! -f scase.dat -a -f "$(EDR_FILE)" ]; then \
# 		../src/edr.pl < "$(EDR_FILE)" > scase.dat; \
# 	fi


scase.db: scase.dat
	@MAKE_DB_COMMAND@ $@ -or < $<

CLEANFILES = scase.db
