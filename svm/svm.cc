#include "common.h"
#include "misc.h"
#include "model.h"
#include "example.h"
#include "base_example.h"
#include "kernel.h"
#include "param.h"
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

    while (isspace (line[i])) i++;
    /* 
    y = atof (line + i);
    while (i < len && !isspace (line[i])) i++;
    while (i < len && isspace (line[i])) i++; */
    dist = model.classify((const char *)(line + i));

    return dist;
}

