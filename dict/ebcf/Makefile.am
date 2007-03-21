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

PROBCASE_DBS = case.db cfcase.db cfex.db cfp.db renyou.db chicase.db chicase_nominal.db ctbcase.db HowNet_Def.db HowNet_Sem_Def.db HowNet_Antonym.db HowNet_Category.db

if PROB_EXIST
nodist_dic_DATA += $(PROBCASE_DBS)
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
		sort cf.idx | @MAKE_DB_COMMAND@ $@ -append /; \
	fi

noun.knpdict:
	true

noun.dat: noun.knpdict
	if [ -f noun.knpdict ]; then \
		@CF_COMMAND@ noun.idx noun.dat < noun.knpdict; \
	fi

noun.db: noun.dat
	if [ -f noun.idx ]; then \
		sort noun.idx | @MAKE_DB_COMMAND@ $@ -append /; \
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

chicase.prob:
	true

chicase.db: chicase.prob
	if [ -f $< ]; then \
		@MAKE_DB_COMMAND@ $@ < $<; \
	fi

chicase_nominal.prob:
	true

chicase_nominal.db: chicase_nominal.prob
	if [ -f $< ]; then \
		@MAKE_DB_COMMAND@ $@ < $<; \
	fi

ctbcase.prob:
	true

ctbcase.db: ctbcase.prob
	if [ -f $< ]; then \
		@MAKE_DB_COMMAND@ $@ < $<; \
	fi

HowNet.def:
	true

HowNet_Def.db: HowNet.def
	if [ -f $< ]; then \
		@MAKE_DB_COMMAND@ $@ < $<; \
	fi

HowNet.SemDef:
	true

HowNet_Sem_Def.db: HowNet.SemDef
	if [ -f $< ]; then \
		@MAKE_DB_COMMAND@ $@ < $<; \
	fi

HowNet.antonym:
	true

HowNet_Antonym.db: HowNet.antonym
	if [ -f $< ]; then \
		@MAKE_DB_COMMAND@ $@ < $<; \
	fi

HowNet.category:
	true

HowNet_Category.db: HowNet.category
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

prob: $(PROBCASE_DBS)

CLEANFILES = cf.db cf.dat cf.idx noun.db noun.dat noun.idx $(PROBCASE_DBS)
