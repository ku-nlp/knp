SUBDIRS = .

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
	mrph_homo.rule

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

diff:
	if [ -r bnst_type.rule.old -a -r bnst_type.rule ]; then \
	    perl -pe 's/RID:\d+//' bnst_type.rule > bnst_type.rule-tmp; \
	    perl -pe 's/RID:\d+//' bnst_type.rule.old > bnst_type.rule.old-tmp; \
	    diff bnst_type.rule.old-tmp bnst_type.rule-tmp; \
	    rm -f bnst_type.rule.old-tmp bnst_type.rule-tmp; \
	fi
