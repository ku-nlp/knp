SUBDIRS = .

dicdir = $(pkgdatadir)/scode/ntt
nodist_dic_DATA = word2code.db word2code_add.db word2code_del.db sm2code.db code2sm.db smp2smg.db

word2code.dat:
	true

word2code.db: word2code.dat
	if [ -f $< ]; then \
		@MAKE_DB_COMMAND@ $@ < $<; \
	fi

word2code_add.rule:
	true

word2code_add.dat: transform word2code_add.rule
	./transform word2code_add

word2code_add.db: word2code_add.dat
	if [ -f $< ]; then \
		@MAKE_DB_COMMAND@ $@ < $<; \
	fi

word2code_del.rule:
	true

word2code_del.dat: transform word2code_del.rule
	./transform word2code_del

word2code_del.db: word2code_del.dat
	if [ -f $< ]; then \
		@MAKE_DB_COMMAND@ $@ < $<; \
	fi

transform: transform.cpp
	g++ transform.cpp -o transform

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
