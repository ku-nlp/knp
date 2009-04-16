SUBDIRS = .

EXTRA_DIST = chidpnd.dict chi_pa.dict

dicdir = $(pkgdatadir)/dict/ebcf

if CF_EXIST
nodist_dic_DATA = cf.db cf.dat
else
nodist_dic_DATA = 
endif

if CF_NOUN_EXIST
nodist_dic_DATA += noun.db noun.dat
endif

nodist_dic_DATA +=  chidpnd.db chidpnd_prob.db chidpnd_stru.db chi_dis_comma.db chi_arg.db

PROBCASE_DBS = adverb.db case.db cfcase.db cfp.db renyou.db chi_pa.db

HOWNET_DBS = hownet_def.db hownet_sem_def.db hownet_antonym.db hownet_category.db hownet_tran.db

if PROB_EXIST
nodist_dic_DATA += $(PROBCASE_DBS)
endif

if HOWNET_EXIST
nodist_dic_DATA += $(HOWNET_DBS)
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
		ulimit -Ss unlimited && @CF_COMMAND@ cf.idx cf.dat < cf.knpdict; \
	fi

cf.db: cf.dat
	if [ -f cf.idx ]; then \
		LANG=C sort cf.idx | @MAKE_DB_COMMAND@ $@ -append /; \
	fi

noun.knpdict:
	true

noun.dat: noun.knpdict
	if [ -f noun.knpdict ]; then \
		ulimit -Ss unlimited && @CF_COMMAND@ noun.idx noun.dat < noun.knpdict; \
	fi

noun.db: noun.dat
	if [ -f noun.idx ]; then \
		LANG=C sort noun.idx | @MAKE_DB_COMMAND@ $@ -append /; \
	fi

adverb.prob:
	true

adverb.db: adverb.prob
	if [ -f $< ]; then \
		@MAKE_DB_COMMAND@ $@ < $<; \
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

chi_pa.dict:
	true

chi_pa.db: chi_pa.dict
	if [ -f $< ]; then \
		@MAKE_DB_COMMAND@ $@ < $<; \
	fi

chidpnd.dict:
	true

chidpnd.db: chidpnd.dict
	if [ -f $< ]; then \
		@MAKE_DB_COMMAND@ $@ < $<; \
	fi

chidpnd_prob.dict:
	true

chidpnd_prob.db: chidpnd_prob.dict
	if [ -f $< ]; then \
		@MAKE_DB_COMMAND@ $@ < $<; \
	fi

chi_arg.dict:
	true

chi_arg.db: chi_arg.dict
	if [ -f $< ]; then \
		@MAKE_DB_COMMAND@ $@ < $<; \
	fi

chidpnd_stru.dict:
	true

chidpnd_stru.db: chidpnd_stru.dict
	if [ -f $< ]; then \
		@MAKE_DB_COMMAND@ $@ < $<; \
	fi

chi_dis_comma.dict:
	true

chi_dis_comma.db: chi_dis_comma.dict
	if [ -f $< ]; then \
		@MAKE_DB_COMMAND@ $@ < $<; \
	fi

HowNet.def:
	true

hownet_def.db: HowNet.def
	if [ -f $< ]; then \
		@MAKE_DB_COMMAND@ $@ < $<; \
	fi

HowNet.tran:
	true

hownet_tran.db: HowNet.tran
	if [ -f $< ]; then \
		@MAKE_DB_COMMAND@ $@ < $<; \
	fi

HowNet.antonym:
	true

hownet_antonym.db: HowNet.antonym
	if [ -f $< ]; then \
		@MAKE_DB_COMMAND@ $@ < $<; \
	fi

HowNet.category:
	true

hownet_category.db: HowNet.category
	if [ -f $< ]; then \
		@MAKE_DB_COMMAND@ $@ < $<; \
	fi

HowNet.SemDef:
	true

hownet_sem_def.db: HowNet.SemDef
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
