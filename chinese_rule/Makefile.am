SUBDIRS = .

RULE2DATA = ../rule/rule2data.pl

ruledir = $(pkgdatadir)/chinese_rule

orig_rule_files = bnst_basic.rule kakari_uke.rule mrph_basic.rule koou.rule
generated_rule_files = bnst_basic.data kakari_uke.data mrph_basic.data koou.data
dist_rule_DATA = $(orig_rule_files) $(generated_rule_files)

.rule.data:
	$(RULE2DATA) $<

rule:
	if [ -x $(RULE2DATA) ]; then \
	    $(RULE2DATA) *.rule; \
	fi
