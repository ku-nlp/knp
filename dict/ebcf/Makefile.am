SUBDIRS = .

dicdir = $(pkgdatadir)/dict/ebcf

if CF_EXIST
nodist_dic_DATA = cf.db cf.dat mrph2id.db case_soto.txt
EXTRA_DIST = cf.knpdict* mrph2id.dat* case_soto_words.txt case_soto_conv.py
else
nodist_dic_DATA = 
EXTRA_DIST = 
endif

if CF_NOUN_EXIST
nodist_dic_DATA += noun.db noun.dat
EXTRA_DIST += noun.knpdict*
endif

PROBCASE_DBS = case.db cfcase.db cfp.db
ADDITIONAL_PROBCASE_DBS = adverb.db noun_co.db renyou.db para.db

if PROB_EXIST
nodist_dic_DATA += $(PROBCASE_DBS)
EXTRA_DIST += case.prob* cfcase.prob* cfp.prob*
endif

if ADDITIONAL_PROB_EXIST
nodist_dic_DATA += $(ADDITIONAL_PROBCASE_DBS)
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



cf.dat: cf.knpdict*
	if [ -f cf.knpdict.skip ]; then \
		if [ ! -f $@ ]; then \
			echo -e '\e[1;31m.skip exists. However $@ is not found\e[m' 1>&2; exit 5;\
		fi; \
		echo -e '\e[1;31mSkip build of $@ \e[m' 1>&2 :;\
	elif [ -f cf.knpdict.xz ]; then \
		ulimit -Ss unlimited && xzcat cf.knpdict.xz |  @CF_COMMAND@ cf.idx cf.dat; \
	elif [ -f cf.knpdict ]; then \
		ulimit -Ss unlimited && @CF_COMMAND@ cf.idx cf.dat < cf.knpdict; \
	else \
		exit 2;\
	fi

cf.db: cf.dat
	if [ -f cf.idx ]; then \
		LANG=C sort cf.idx | @MAKE_DB_COMMAND@ $@ -append /; \
	fi


noun.dat: noun.knpdict*
	if [ -f noun.knpdict.skip ]; then \
		if [ ! -f $@ ]; then \
			echo -e '\e[1;31m.skip exists. However $@ is not found\e[m' 1>&2; exit 5;\
		fi; \
		echo -e '\e[1;31mSkip build of $@ \e[m' 1>&2 :;\
	elif [ -f noun.knpdict.xz ]; then \
		ulimit -Ss unlimited && xzcat noun.knpdict.xz | @CF_COMMAND@ noun.idx noun.dat; \
	elif [ -f noun.knpdict ]; then \
		ulimit -Ss unlimited && @CF_COMMAND@ noun.idx noun.dat < noun.knpdict; \
	else \
		exit 2;\
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


case.db: case.prob*
	if [ -f case.prob.skip ]; then \
		if [ ! -f $@ ]; then \
			echo -e '\e[1;31m.skip exists. However $@ is not found\e[m' 1>&2; exit 5;\
		fi; \
		echo -e '\e[1;31mSkip build of $@ \e[m' 1>&2 :;\
	elif [ -f case.prob.xz ]; then \
		xzcat case.prob.xz | @MAKE_DB_COMMAND@ $@; \
	elif [ -f case.prob ]; then \
		@MAKE_DB_COMMAND@ $@ < case.prob; \
	else \
		exit 2;\
	fi


cfcase.db: cfcase.prob*
	if [ -f cfcase.prob.skip ]; then \
		if [ ! -f $@ ]; then \
			echo -e '\e[1;31m.skip exists. However $@ is not found\e[m' 1>&2; exit 5;\
		fi; \
		echo -e '\e[1;31mSkip build of $@ \e[m' 1>&2 :;\
	elif [ -f cfcase.prob.xz ]; then \
		xzcat cfcase.prob.xz | @MAKE_DB_COMMAND@ $@; \
	elif [ -f cfcase.prob ]; then \
		@MAKE_DB_COMMAND@ $@ < cfcase.prob; \
	else \
		exit 2;\
	fi


cfp.db: cfp.prob*
	if [ -f cfp.prob.skip ]; then \
		if [ ! -f $@ ]; then \
			echo -e '\e[1;31m.skip exists. However $@ is not found\e[m' 1>&2; exit 5;\
		fi; \
		echo -e '\e[1;31mSkip build of $@ \e[m' 1>&2 :;\
	elif [ -f cfp.prob.xz ]; then \
		xzcat cfp.prob.xz | @MAKE_DB_COMMAND@ $@; \
	elif [ -f cfp.prob ]; then \
		@MAKE_DB_COMMAND@ $@ < cfp.prob; \
	else \
		exit 2;\
	fi;

renyou.prob:
	true

renyou.db: renyou.prob
	if [ -f $< ]; then \
		@MAKE_DB_COMMAND@ $@ < $<; \
	fi

para.prob:
	true

para.db: para.prob
	if [ -f $< ]; then \
		@MAKE_DB_COMMAND@ $@ < $<; \
	fi

noun_co.prob:
	true

noun_co.db: noun_co.prob
	if [ -f $< ]; then \
		@MAKE_DB_COMMAND@ $@ < $<; \
	fi

prob: $(PROBCASE_DBS)


mrph2id.db: mrph2id.dat*
	if [ -f mrph2id.dat.skip ]; then \
		if [ ! -f $@ ]; then \
			echo -e '\e[1;31m.skip exists. However $@ is not found\e[m' 1>&2; exit 5;\
		fi; \
		echo -e '\e[1;31mSkip build of $@ \e[m' 1>&2 :;\
	elif [ -f mrph2id.dat.xz ]; then \
		xzcat mrph2id.dat.xz | @MAKE_DB_COMMAND@ $@; \
	elif [ -f mrph2id.dat ]; then \
		@MAKE_DB_COMMAND@ $@ < mrph2id.dat; \
	else \
		exit 2;\
	fi

case_soto.txt: mrph2id.dat* case_soto_words.txt case_soto_conv.py
	if [ -f mrph2id.dat.skip ];then \
		if [ ! -f $@ ]; then \
			echo -e '\e[1;31m.skip exists. However $@ is not found\e[m' 1>&2; exit 5;\
		fi; \
		echo -e '\e[1;31mSkip build of $@ \e[m' 1>&2 :;\
	elif [ -f mrph2id.dat.xz ];then \
	    bash -c 'python case_soto_conv.py -s case_soto_words.txt -m <(xzcat mrph2id.dat.xz) -o $@.tmp && mv $@.tmp $@ '; \
	else \
	    python case_soto_conv.py -s case_soto_words.txt -m mrph2id.dat -o $@.tmp && mv $@.tmp $@; \
	fi


CLEANFILES = cf.db cf.dat cf.idx noun.db noun.dat noun.idx mrph2id.db case_soto.txt $(PROBCASE_DBS) $(ADDITIONAL_PROBCASE_DBS)
