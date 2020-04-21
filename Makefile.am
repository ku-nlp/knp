AUTOMAKE_OPTIONS = foreign

.PHONY: snapshot
snapshot:
	$(MAKE) dist distdir=$(PACKAGE)-`date +"%Y%m%d"`

SUBDIRS = svm svm_light cdb crf CRF++-0.58 distsim 
if USE_INCLUDED_JUMAN
SUBDIRS += juman
endif
SUBDIRS += system rule dict doc chinese_rule

knprcdir=$(sysconfdir)
nodist_knprc_DATA = knprc
dist_noinst_DATA = knprc.in .git/HEAD .git/index VERSION
CLEANFILES = knprc
EXTRA_DIST = autogen.sh perl cdb

knprc: knprc.in config.status
	rm -f $@ $@.tmp
	sed 's,@pkgdatadir\@,$(pkgdatadir),g; s,@datadir\@,$(datadir),g; s,@juman_prefix\@,$(juman_prefix),g' $< > $@.tmp
	mv $@.tmp $@
