SUBDIRS = .

INCLUDES = @KNP_CFLAGS@

bin_PROGRAMS = knp
prog_SOURCES = version.h
EXTRA_DIST = version.h

pkglibexecdir= $(libexecdir)/knp
pkglibexec_PROGRAMS = ipal make_db

CLEANFILES = version.h

knp_SOURCES = \
	anaphora.c \
	case_analysis.c \
	case_data.c \
	case_data.h \
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
	cky.c \
	dic.c \
	nv_mi.c

knp_LDADD = @KNP_LIBS@

make_db_SOURCES = \
	make_db.c \
	db.c \
	tools.c \
	hash.c \
	hash.h

make_db_LDADD = @MAKE_DB_LIBS@

main.c: version.h
lib_print.c: version.h
version.h:  $(top_srcdir)/.git/HEAD $(top_srcdir)/.git/index $(top_srcdir)/VERSION
	$(AM_V_GEN)( \
	echo "#define REVISION_ID \"$(shell git --git-dir=$(top_srcdir)/.git log -1 --format='%h' --date=short)\"" > $@-new; \
	echo "#define REVISION_INFO \"$(shell git --git-dir=$(top_srcdir)/.git log -1 --format='%h on %cd' --date=short)\"" >> $@-new; \
	echo "#define REVISION_VERSION \"$(shell cat $(top_srcdir)/VERSION)\"" >> $@-new; \
	cmp -s $@ $@-new || cp $@-new $@; \
	rm $@-new)
