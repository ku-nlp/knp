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

#define	ModelFile	"/home/kawahara/CHUN-LI/knp/system/hoge.model"

TinySVM::Model model;

int init_svm() {
    if (!model.read(ModelFile)) {
	return 0;
    }
    return 1;
}

double svm_classify(char *line) {
    double y = 0, dist;
    int len = strlen(line);
    int i = 0;

    while (isspace(line[i])) i++;
    dist = model.classify((const char *)(line + i));

    return dist;
}

