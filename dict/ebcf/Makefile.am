SUBDIRS = .

dicdir = $(pkgdatadir)/dict/ebcf

if CF_EXIST
nodist_dic_DATA = cf.db cf.dat
else
nodist_dic_DATA = 
endif

if CF_NOUN_EXIST
nodist_dic_DATA += noun.db noun.dat
endif

IPALV1_FILE = /share/dict/ipal/orig/dousi1/dousi1.txt
IPALV2_FILE = /share/dict/ipal/orig/dousi2/dousi2.txt
IPALA1_FILE = /share/dict/ipal/orig/keiyou/keiyou1.txt
IPALA2_FILE = /share/dict/ipal/orig/keiyou/keiyou2.txt
IPALN_DIR   = /share/dict/ipal_noun_31/3.1/data

# database_ipal:
# 	if [ -f "$(IPALV1_FILE)" -a "$(IPALV2_FILE)" ]; then \
# 		cat $(IPALV1_FILE) $(IPALV2_FILE) | ../src/ipalv.pl > ipalv.dat; \
# 	fi
# 	if [ -f "$(IPALA1_FILE)" -a "$(IPALA2_FILE)" ]; then \
# 		../src/ipala.pl $(IPALA1_FILE) $(IPALA2_FILE) > ipala.dat; \
# 	fi
# 	if [ -n "$(IPALN_DIR)" -a -d "$(IPALN_DIR)" ]; then \
# 		cat $(IPALN_DIR)/n31.* | ../src/ipals.pl | ../src/ipals_add_voice.pl > ipals.dat; \
# 	fi
# 	if [ -f ipalv.dat -o -f ipals.dat -o -f ipala.dat ]; then \
# 		cat ipalv.dat ipals.dat ipala.dat | @CF_COMMAND@ ipal.idx ipal.dat; \
# 		cat ipalv.dat ipals.dat ipala.dat | ../src/ipal2scase.pl >> ../gcf/scase.dat; \
# 	fi


cf.knpdict:
	true

cf.dat: cf.knpdict
	if [ -f cf.knpdict ]; then \
		@CF_COMMAND@ cf.idx cf.dat < cf.knpdict; \
	fi

cf.db: cf.dat
	if [ -f cf.idx ]; then \
		@MAKE_DB_COMMAND@ $@ -append / < cf.idx; \
	fi

noun.knpdict:
	true

noun.dat: noun.knpdict
	if [ -f noun.knpdict ]; then \
		@CF_COMMAND@ noun.idx noun.dat < noun.knpdict; \
	fi

noun.db: noun.dat
	if [ -f noun.idx ]; then \
		@MAKE_DB_COMMAND@ $@ -append / < noun.idx; \
	fi

case.prob:
	true

case.db: case.prob
	if [ -f $< ]; then \
		@MAKE_DB_COMMAND@ $@ < $<; \
	fi

cfcase.prob:
	true

cfcase.db: cfcase.prob
	if [ -f $< ]; then \
		@MAKE_DB_COMMAND@ $@ < $<; \
	fi

cfex.logprob:
	true

cfex.db: cfex.logprob
	if [ -f $< ]; then \
		@MAKE_DB_COMMAND@ $@ < $<; \
	fi

cfp.prob:
	true

cfp.db: cfp.prob
	if [ -f $< ]; then \
		@MAKE_DB_COMMAND@ $@ < $<; \
	fi

renyou.prob:
	true

renyou.db: renyou.prob
	if [ -f $< ]; then \
		@MAKE_DB_COMMAND@ $@ < $<; \
	fi

prob: case.db cfcase.db cfex.db cfp.db renyou.db

CLEANFILES = cf.db cf.dat cf.idx noun.db noun.dat noun.idx case.db cfcase.db cfex.db cfp.db renyou.db
