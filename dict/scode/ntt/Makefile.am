SUBDIRS = .

dicdir = $(pkgdatadir)/dict/scode/ntt
nodist_dic_DATA = word2code.db sm2code.db code2sm.db smp2smg.db

usr_word2code.dat:
	true

transform: transform.cpp
	g++ transform.cpp -o transform

word2code.dat: transform usr_word2code.dat sm2code.dat word2code.orig
	./transform usr_word2code.dat sm2code.dat word2code.orig > word2code.dat

word2code.db: word2code.dat
	if [ -f $< ]; then \
		@MAKE_DB_COMMAND@ $@ < $<; \
	fi

sm2code.dat:
	true

sm2code.db: sm2code.dat
	if [ -f $< ]; then \
		@MAKE_DB_COMMAND@ $@ < $<; \
	fi

code2sm.dat:
	true

code2sm.db: code2sm.dat
	if [ -f $< ]; then \
		@MAKE_DB_COMMAND@ $@ < $<; \
	fi

smp2smg.dat:
	true

smp2smg.db: smp2smg.dat
	if [ -f $< ]; then \
		@MAKE_DB_COMMAND@ $@ -append / < $<; \
	fi

CLEANFILES = word2code.db sm2code.db code2sm.db smp2smg.db
