SUBDIRS = .

INCLUDES = @KNP_CFLAGS@

bin_PROGRAMS = knp

pkglibexecdir= $(libexecdir)/knp
pkglibexec_PROGRAMS = ipal make_db

knp_SOURCES = \
	case_analysis.c \
	case_data.c \
	case_ipal.c \
	case_match.c \
	case_print.c \
	const.h \
	context.c \
	context.h \
	corefer.c \
	dbm.h \
	dpnd_analysis.c \
	extern.h \
	feature.c \
	knp.h \
	koou.c \
	base_phrase.c \
	lib_bgh.c \
	lib_dt.c \
	lib_print.c \
	lib_ps.c \
	lib_scase.c \
	lib_sm.c \
	main.c \
	para_analysis.c \
	para_dpnd.c \
	para_relation.c \
	para_revision.c \
	path.h \
	proper.c \
	quote.c \
	read_data.c \
	read_rule.c \
	regexp.c \
	similarity.c \
	tools.c \
	tree_conv.c \
	db.c \
	configfile.c \
	thesaurus.c \
	hash.c \
	hash.h \
	bnst_compare.c \
	lib_event.c \
	cky.c

knp_LDADD = @KNP_LIBS@

make_db_SOURCES = \
	make_db.c \
	db.c \
	tools.c \
	hash.c \
	hash.h

make_db_LDADD = @MAKE_DB_LIBS@
