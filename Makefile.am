.PHONY: snapshot
snapshot:
	$(MAKE) dist distdir=$(PACKAGE)-`date +"%Y%m%d"`

SUBDIRS = svm system rule dict docs

knprcdir=$(sysconfdir)
nodist_knprc_DATA = knprc
dist_noinst_DATA = knprc.in
CLEANFILES = knprc
EXTRA_DIST = autogen.sh perl

knprc: knprc.in
	rm -f $@ $@.tmp
	sed 's,@pkgdatadir\@,$(pkgdatadir),g; s,@datadir\@,$(datadir),g; s,@juman_prefix\@,$(juman_prefix),g' $< > $@.tmp
	mv $@.tmp $@
