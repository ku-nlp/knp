SUBDIRS = .

RULECONV = ./rule_conv.pl
RULETRANS = ./rule_trans.pl

EXTRA_DIST = \
	rule_conv.pl \
	rule_trans.pl \
	bnst_basic.rule \
	bnst_etc.rule \
	bnst_type.phrase \
	context.rule \
	kakari_uke.rule \
	koou.rule \
	mrph_basic.rule \
	mrph_homo.rule \
	case_analysis.rule

ruledir = $(pkgdatadir)/rule
dist_rule_DATA = bnst_type.rule bnst_basic.data bnst_etc.data bnst_type.data context.data kakari_uke.data koou.data mrph_basic.data mrph_homo.data case_analysis.data

.rule.data:
	$(RULECONV) $<

.phrase.rule:
	if [ -f $@ ]; then \
	    cp -pf $@ $@.old; \
	fi; \
	$(RULETRANS) < $< > $@
	if [ -r $@.old ]; then \
	    perl -pe 's/RID:\d+//' $@ > $@-tmp; \
	    perl -pe 's/RID:\d+//' $@.old > $@.old-tmp; \
	    diff $(DIFF_ENV) $@.old-tmp $@-tmp; \
	    rm -f $@.old-tmp $@-tmp; \
	fi

rule:
	if [ -x $(RULECONV) ]; then \
	    $(RULECONV) *.rule; \
	fi

trans: diff

diff:
	if [ -r bnst_type.rule.old -a -r bnst_type.rule ]; then \
	    perl -pe 's/RID:\d+//' bnst_type.rule > bnst_type.rule-tmp; \
	    perl -pe 's/RID:\d+//' bnst_type.rule.old > bnst_type.rule.old-tmp; \
	    diff bnst_type.rule.old-tmp bnst_type.rule-tmp; \
	    rm -f bnst_type.rule.old-tmp bnst_type.rule-tmp; \
	fi

CLEANFILES = bnst_type.rule.old $(dist_rule_DATA)

rulepack: $(dist_rule_DATA)
	if [ ! -d rule ]; then \
	    mkdir rule; \
	    cp -p $(dist_rule_DATA) rule; \
	    $(AMTAR) chof - rule | GZIP=$(GZIP_ENV) gzip -c > $(PACKAGE)-rule-`date +"%Y%m%d"`.tar.gz; \
	    rm -rf rule; \
	fi
