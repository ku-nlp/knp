SUBDIRS = .

EXTRA_DIST = mi.dat

dicdir = $(pkgdatadir)/dict/distsim

if DISTSIM_MI_EXIST
nodist_dic_DATA = mi.db
else
nodist_dic_DATA = 
endif

mi.db: mi.dat
	@MAKE_DB_COMMAND@ $@ -z < $<

CLEANFILES = mi.db
