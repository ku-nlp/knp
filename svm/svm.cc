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

#include <tinysvm.h>
#include "svm.h"

#define	PP_NUMBER	44

char *SVMFile[PP_NUMBER];	/* modelファイルの指定 */

TinySVM::Model *model[PP_NUMBER];

int init_svm() {
    int i;

    for (i = 0; i < PP_NUMBER; i++) {
	if (SVMFile[i]) {
	    model[i] = new TinySVM::Model;
	    if (!model[i]->read(SVMFile[i])) {
		fprintf(stderr, ";; SVM initialization error (%s is corrupted?).\n", SVMFile[i]);
		exit(1);
	    }

	    if (i == 0) { /* すべての格用 */
		break;
	    }
	}
	else {
	    model[i] = NULL;
	}
    }
    return 1;
}

double svm_classify(char *line, int pp) {
    TinySVM::Model *m;
    int i = 0;

    /* 0はすべての格用 */
    if (model[0]) {
	m = model[0];
    }
    else {
	m = model[pp];
    }

    while (isspace(line[i])) i++;
    return m->classify((const char *)(line + i));
}

