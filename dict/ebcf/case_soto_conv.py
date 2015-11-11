#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
"""
__author__ = 'Yuta Hayashibe'
__version__ = ""
__copyright__ = ""
__license__ = ""


import optparse
import codecs
import sys
sys.stdin = codecs.getreader('UTF-8')(sys.stdin)
sys.stdout = codecs.getwriter('UTF-8')(sys.stdout)
sys.stderr = codecs.getwriter('UTF-8')(sys.stderr)


def operation(sourcef, idmapf, outf):
    targets = []
    vocab = set([])
    for line in sourcef:
        target = line.rstrip()
        targets.append(target)
        for item in target.split(u"+"):
            vocab.add(item)
    sourcef.close()

    word2id = {}
    for line in idmapf:
        sep = line.find(u" ")
        key = line[:sep]
        if key in vocab:
            val = line[sep+1 : -1]
            word2id[key] = val
            if len(word2id) == len(vocab):
                break
    idmapf.close()

    for i, target in enumerate(targets):
        if i != 0:
            outf.write(u"／")
        for j, item in enumerate(target.split(u"+")):
            if j != 0:
                outf.write(u"+")
            outf.write(word2id[item])
        outf.write(u":1")
    outf.write(u"\n")
    outf.close()

def main():
    oparser = optparse.OptionParser()
    oparser.add_option("-s", "--source", dest="source", default=None)
    oparser.add_option("-m", "--idmap", dest="idmap", default=None)
    oparser.add_option("-o", "--output", dest="output", default="-")
    (opts, args) = oparser.parse_args()

    if opts.source is None or opts.idmap is None:
        raise

    sourcef = codecs.open(opts.source, "r", "utf8")
    idmapf = codecs.open(opts.idmap, "r", "utf8")

    if opts.output == "-":
        outf = sys.stdout
    else:
        outf = codecs.open(opts.output, "w", "utf8")
    operation(sourcef, idmapf, outf)


if __name__ == '__main__':
    main()
