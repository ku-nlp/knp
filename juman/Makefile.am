SUBDIRS = darts lib makeint makemat makepat juman dic autodic wikipediadic doc

jumanrcdir=$(sysconfdir)
nodist_jumanrc_DATA = jumanrc
dist_noinst_DATA = jumanrc.in VERSION
CLEANFILES = jumanrc
EXTRA_DIST = perl

jumanrc: jumanrc.in config.status
	rm -f $@ $@.tmp
	LANG=C sed 's,@pkgdatadir\@,$(pkgdatadir),g' $< > $@.tmp
	mv $@.tmp $@
