#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef STDC_HEADERS
#include <stdio.h>
#endif

#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#ifdef HAVE_SIGNAL_H
#include <signal.h>
#endif

#ifdef HAVE_SETJMP_H
#include <setjmp.h>
#endif

#ifdef HAVE_MATH_H
#include <math.h>
#endif

#ifdef HAVE_SYS_FILE_H
#include <sys/file.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_CTYPE_H
#include <ctype.h>
#endif

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif

#include <crfpp.h>
#include <iostream>
#include "crf.h"

char *CRFFileNE;
CRFPP::Tagger *tagger;

int init_crf_for_NE() {
    char argc = 5, *argv[5];

    /* make an argument of CRFPP::createTagger */
    argv[0] = const_cast<char*>("knp");
    argv[1] = const_cast<char*>("-m");
    argv[2] = CRFFileNE;
    argv[3] = const_cast<char*>("-v"); /* -v 2: access marginal probablilities */
    argv[4] = const_cast<char*>("2");
    
    tagger = CRFPP::createTagger(argc, argv);
    if (!tagger) {
	fprintf(stderr, ";; CRF initialization error (%s is corrupted?).\n", CRFFileNE);
	exit(1);
    }
    return 0;
}

void crf_add(char *line) {
    tagger->add(line);
}

void clear_crf() {
    tagger->clear();
}

void crf_parse() {
    if (!tagger->parse()) {
	fprintf(stderr, ";; CRF parse error.\n");
	exit(1);
    }
}

void get_crf_prob(int i, int j, double *prob) {
    tagger->prob(); // 何故か必要
    *prob = tagger->prob(i, j);
}
