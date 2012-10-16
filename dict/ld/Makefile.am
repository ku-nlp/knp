SUBDIRS = .

EXTRA_DIST = id2lex.txt rep2id_da.txt

dicdir = $(pkgdatadir)/dict/ld

nodist_dic_DATA = id2lex.db rep2id.da

id2lex.txt:
	true

id2lex.db: id2lex.txt
	if [ -f $< ]; then \
		@MAKE_DB_COMMAND@ $@ < $<; \
	fi

rep2id_da.txt:
	true

rep2id.da: rep2id_da.txt
	if [ -f $< ]; then \
		@MKDARTS_COMMAND@ $< $@; \
	fi

CLEANFILES = rep2id.da id2lex.db
