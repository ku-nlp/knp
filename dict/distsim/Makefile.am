SUBDIRS = .

EXTRA_DIST = mi.dat sim.dat

dicdir = $(pkgdatadir)/dict/distsim

if DISTSIM_MI_EXIST
nodist_dic_DATA = mi.db sim.db
dist_dic_DATA = wordlist.txt
else
nodist_dic_DATA = 
endif

mi.db: mi.dat
	@MAKE_DB_COMMAND@ $@ -z < $<

sim.db: sim.dat
	@MAKE_DB_COMMAND@ $@ < $<

CLEANFILES = mi.db sim.db
