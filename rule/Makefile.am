SUBDIRS = .

PHRASE2RULE = ./phrase2rule.pl
RULE2DATA = ./rule2data.pl

ruledir = $(pkgdatadir)/rule

orig_rule_files = bnst_type.phrase bnst_basic.rule bnst_etc.rule modality.rule tag_after_dpnd_and_case.rule context.rule kakari_uke.rule koou.rule mrph_basic.rule mrph_ne.rule mrph_homo.rule mrph_filter.rule mrph_semantic_head.rule case_analysis.rule tag_postprocess.rule mrph_auto_dic.rule clause_function.phrase bnst_after_dpnd_and_case.rule search_query.rule
generated_rule_files = bnst_type.rule bnst_basic.data bnst_etc.data modality.data tag_after_dpnd_and_case.data bnst_type.data context.data kakari_uke.data koou.data mrph_basic.data mrph_ne.data mrph_homo.data mrph_filter.data mrph_semantic_head.data case_analysis.data tag_postprocess.data mrph_auto_dic.data clause_function.rule clause_function.data bnst_after_dpnd_and_case.data search_query.data
dist_rule_DATA = $(orig_rule_files) $(generated_rule_files) $(RULE2DATA) $(PHRASE2RULE)

bnst_type.rule: bnst_type.phrase $(PHRASE2RULE)
	if [ -f $@ ]; then \
	    cp -pf $@ $@.old; \
	fi; \
	$(PHRASE2RULE) --rid < $< > $@
	if [ -r $@.old ]; then \
	    perl -pe 's/RID:\d+//' $@ > $@-tmp; \
	    perl -pe 's/RID:\d+//' $@.old > $@.old-tmp; \
	    diff $(DIFF_ENV) $@.old-tmp $@-tmp; \
	    rm -f $@.old-tmp $@-tmp; \
	fi

.phrase.rule: $(PHRASE2RULE)
	if [ -f $@ ]; then \
	    cp -pf $@ $@.old; \
	fi; \
	$(PHRASE2RULE) < $< > $@
	if [ -r $@.old ]; then \
	    diff $(DIFF_ENV) $@.old $@; \
	fi; \
	:

.rule.data:
	$(RULE2DATA) $<

rule:
	if [ -x $(RULE2DATA) ]; then \
	    $(RULE2DATA) *.rule; \
	fi

trans: diff

diff:
	if [ -r bnst_type.rule.old -a -r bnst_type.rule ]; then \
	    perl -pe 's/ＴRID:\d+//' bnst_type.rule > bnst_type.rule-tmp; \
	    perl -pe 's/ＴRID:\d+//' bnst_type.rule.old > bnst_type.rule.old-tmp; \
	    diff bnst_type.rule.old-tmp bnst_type.rule-tmp; \
	    rm -f bnst_type.rule.old-tmp bnst_type.rule-tmp; \
	fi

CLEANFILES = bnst_type.rule.old

rulepack: $(dist_rule_DATA)
	if [ ! -d rule ]; then \
	    mkdir rule; \
	    cp -p $(generated_rule_files) rule; \
	    $(AMTAR) chof - rule | GZIP=$(GZIP_ENV) gzip -c > $(PACKAGE)-rule-`date +"%Y%m%d"`.tar.gz; \
	    rm -rf rule; \
	fi

win: $(orig_rule_files) $(generated_rule_files)
	nkf -sc --overwrite $(orig_rule_files) $(generated_rule_files)
