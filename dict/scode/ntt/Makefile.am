SUBDIRS = .

EXTRA_DIST = transform.cpp

if NTT_EXIST
dicdir = $(pkgdatadir)/dict/scode/ntt
nodist_dic_DATA = word2code.db sm2code.db code2sm.db smp2smg.db
endif

usr_word2code.dat:
	touch $@

word2code.orig:
	true

transform: transform.cpp
	$(CXX) $< -o $@

word2code.dat: transform usr_word2code.dat sm2code.dat word2code.orig
	if [ -f word2code.orig ]; then \
		./transform usr_word2code.dat sm2code.dat word2code.orig > word2code.dat; \
	fi

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
