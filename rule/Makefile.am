SUBDIRS = .

PHRASE2RULE = ./phrase2rule.pl
RULE2DATA = ./rule2data.pl

ruledir = $(pkgdatadir)/rule
dist_rule_DATA = bnst_type.phrase bnst_basic.rule bnst_etc.rule bnst_after_dpnd.rule bnst_type.rule context.rule kakari_uke.rule koou.rule mrph_basic.rule mrph_ne.rule mrph_homo.rule case_analysis.rule bnst_basic.data bnst_etc.data bnst_after_dpnd.data bnst_type.data context.data kakari_uke.data koou.data mrph_basic.data mrph_ne.data mrph_homo.data case_analysis.data rule2data.pl phrase2rule.pl

.phrase.rule:
	if [ -f $@ ]; then \
	    cp -pf $@ $@.old; \
	fi; \
	$(PHRASE2RULE) < $< > $@
	if [ -r $@.old ]; then \
	    perl -pe 's/RID:\d+//' $@ > $@-tmp; \
	    perl -pe 's/RID:\d+//' $@.old > $@.old-tmp; \
	    diff $(DIFF_ENV) $@.old-tmp $@-tmp; \
	    rm -f $@.old-tmp $@-tmp; \
	fi

.rule.data:
	$(RULE2DATA) $<

rule:
	if [ -x $(RULE2DATA) ]; then \
	    $(RULE2DATA) *.rule; \
	fi

trans: diff

diff:
	if [ -r bnst_type.rule.old -a -r bnst_type.rule ]; then \
	    perl -pe 's/RID:\d+//' bnst_type.rule > bnst_type.rule-tmp; \
	    perl -pe 's/RID:\d+//' bnst_type.rule.old > bnst_type.rule.old-tmp; \
	    diff bnst_type.rule.old-tmp bnst_type.rule-tmp; \
	    rm -f bnst_type.rule.old-tmp bnst_type.rule-tmp; \
	fi

CLEANFILES = bnst_type.rule.old

rulepack: $(dist_rule_DATA)
	if [ ! -d rule ]; then \
	    mkdir rule; \
	    cp -p $(dist_rule_DATA) rule; \
	    $(AMTAR) chof - rule | GZIP=$(GZIP_ENV) gzip -c > $(PACKAGE)-rule-`date +"%Y%m%d"`.tar.gz; \
	    rm -rf rule; \
	fi
