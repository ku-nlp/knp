
MAKEINT = ../makeint/makeint
MAKEMAT = ../makemat/makemat
MAKEPAT = ../makepat/makepat
DICSORT = ../makepat/dicsort
MKDARTS = ../darts/mkdarts

GRM_FILES = JUMAN.grammar JUMAN.kankei JUMAN.katuyou
TBL_FILES = jumandic.mat jumandic.tab
GENERATED_DIC_FILES = Rendaku.dic Onomatopoeia.dic
DIC_FILES = $(sort $(wildcard *.dic) $(GENERATED_DIC_FILES))
INT_FILES = $(patsubst %.dic,%.int,$(DIC_FILES))

dicdir = $(pkgdatadir)/dic
nodist_dic_DATA = JUMAN.connect $(TBL_FILES) jumandic.dat jumandic.da jumandic.bin
dist_dic_DATA = $(GRM_FILES)
EXTRA_DIST = JUMAN.connect.c *.dic rendaku.perl onomatopoeia.perl onomatopoeia.pattern

Rendaku.dic: ContentW.dic
	perl -e 'print reverse <>' $< | perl rendaku.perl | perl -e 'print reverse <>' > $@

Onomatopoeia.dic: onomatopoeia.pattern
	perl onomatopoeia.perl < $< > $@

.dic.int: $(MAKEINT)
	$(MAKEINT) $<

clean:
	rm -f jumandic.* *.int JUMAN.connect $(GENERATED_DIC_FILES)

JUMAN.connect: JUMAN.connect.c
	@CPP@ -P $< | LANG=C sed "s/\(\#pragma\)/\;\1/" > $@

jumandic.tab: $(GRM_FILES) JUMAN.connect $(MAKEMAT)
	$(MAKEMAT)

jumandic.mat: jumandic.tab

$(INT_FILES): $(GRM_FILES) $(TBL_FILES) $(MAKEINT)

jumandic.txt: $(INT_FILES)
	cat $(INT_FILES) > $@

jumandic.dat: jumandic.txt $(DICSORT)
	$(DICSORT) jumandic.txt > $@

jumandic.done: jumandic.dat $(MKDARTS)
	$(MKDARTS) jumandic.dat jumandic.da jumandic.bin && touch $@

jumandic.da: jumandic.done
jumandic.bin: jumandic.done
