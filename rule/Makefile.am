SUBDIRS = .

EXTRA_DIST = \
	rule_conv.pl \
	rule_trans.pl \
	bnst_basic.rule \
	bnst_etc.rule \
	bnst_type.phrase \
	compoundnoun-auxiliary.rule \
	compoundnoun-preliminary.rule \
	compoundnoun.rule \
	context.rule \
	kakari_uke.rule \
	koou.rule \
	mrph_basic.rule \
	mrph_helpsys.rule \
	mrph_homo.rule \
	ne-juman.rule \
	ne.rule 

rule:
	if [ -x rule_conv.pl ]; then \
	    ./rule_conv.pl *.rule; \
	fi

trans:
	if [ -x rule_trans.pl ]; then \
	    if [ -f bnst_type.rule ]; then \
		cp -pf bnst_type.rule bnst_type.rule.old; \
	    fi; \
	    ./rule_trans.pl < bnst_type.phrase > bnst_type.rule; \
	fi
	$(MAKE) rule
