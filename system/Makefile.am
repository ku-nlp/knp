SUBDIRS = .

EXTRA_DIST = \
	Makefile.orig \
	ipal.c \
	make_db.c

INCLUDES = @KNP_CFLAGS@

noinst_PROGRAMS = knp ipal make_db

knp_SOURCES = \
	case_analysis.c \
	case_data.c \
	case_ipal.c \
	case_match.c \
	case_print.c \
	const.h \
	context.c \
	corpus.c \
	dbm.h \
	dpnd_analysis.c \
	extern.h \
	feature.c \
	knp.h \
	koou.c \
	lib_bgh.c \
	lib_lib.c \
	lib_print.c \
	lib_ps.c \
	lib_scase.c \
	lib_sm.c \
	main.c \
	para_analysis.c \
	para_dpnd.c \
	para_relation.c \
	para_revision.c \
	para_similarity.c \
	path.h \
	proper.c \
	quote.c \
	read_data.c \
	read_rule.c \
	regexp.c \
	tools.c \
	tree_conv.c \
	unsupervised.c \
	db.c \
	japanese.c \
	japanese.h \
	configfile.c \
	noun.c

knp_LDADD = -ljuman @KNP_LIBS@

make_db_SOURCES = \
	make_db.c \
	db.c \
	tools.c

make_db_LDADD = @MAKE_DB_LIBS@
