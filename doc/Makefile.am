SUBDIRS = .

EXTRA_DIST = manual.tex
SUFFIXES = .tex .dvi .pdf

docdir = $(pkgdatadir)/doc
dist_doc_DATA = manual.pdf

.tex.dvi:
	platex $<
	platex $<

.dvi.pdf:
	dvipdfmx $<
