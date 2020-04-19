
MAKEINT = ../makeint/makeint
MAKEMAT = ../makemat/makemat
MAKEPAT = ../makepat/makepat
DICSORT = ../makepat/dicsort
MKDARTS = ../darts/mkdarts

GRM_FILES = ../dic/JUMAN.grammar ../dic/JUMAN.kankei ../dic/JUMAN.katuyou
TBL_FILES = ../dic/jumandic.mat ../dic/jumandic.tab
INT_FILES := $(patsubst %.dic,%.int,$(wildcard *.dic))

dicdir = $(pkgdatadir)/wikipediadic
nodist_dic_DATA = jumandic.dat jumandic.da jumandic.bin
EXTRA_DIST = Wikipedia.dic

.dic.int: $(MAKEINT)
	$(MAKEINT) $<

clean:
	rm -f jumandic.* *.int

$(INT_FILES): $(GRM_FILES) $(TBL_FILES) $(MAKEINT)

jumandic.txt: $(INT_FILES)
	cat $(INT_FILES) > $@

jumandic.dat: jumandic.txt $(DICSORT)
	$(DICSORT) jumandic.txt > $@

jumandic.done: jumandic.dat $(MKDARTS)
	$(MKDARTS) jumandic.dat jumandic.da jumandic.bin && touch $@

jumandic.da: jumandic.done
jumandic.bin: jumandic.done
