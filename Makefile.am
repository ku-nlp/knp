SUBDIRS = system
SHELL = sh

man_MANS = docs/knp.1

EXTRA_DIST = \
	docs/knp.1 \
	rule/rule_conv.pl \
	rule/rule_trans.pl \
	rule/bnst_basic.rule \
	rule/bnst_etc.rule \
	rule/bnst_type.phrase \
	rule/compoundnoun-auxiliary.rule \
	rule/compoundnoun-preliminary.rule \
	rule/compoundnoun.rule \
	rule/context.rule \
	rule/kakari_uke.rule \
	rule/koou.rule \
	rule/mrph_basic.rule \
	rule/mrph_helpsys.rule \
	rule/mrph_homo.rule \
	rule/ne-juman.rule \
	rule/ne.rule 

.PHONY: snapshot
snapshot:
	$(MAKE) dist distdir=$(PACKAGE)-`date +"%Y%m%d"`
