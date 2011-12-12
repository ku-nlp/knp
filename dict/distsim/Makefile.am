SUBDIRS = .

EXTRA_DIST = all-compound-1-201101-mi.dat

dicdir = $(pkgdatadir)/dict/distsim

if DISTSIM_MI_EXIST
nodist_dic_DATA = all-compound-1-201101-mi
else
nodist_dic_DATA = 
endif

all-compound-1-201101-mi: all-compound-1-201101-mi.dat
	@MAKE_DB_COMMAND@ $@ -z < $<

CLEANFILES = all-compound-1-201101-mi
