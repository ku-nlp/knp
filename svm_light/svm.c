#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef STDC_HEADERS
#include <stdio.h>
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#include <svm_light/svm_common.h>

#define	PP_NUMBER	44
#define NE_MODEL_NUMBER	33

char *SVMFile[PP_NUMBER];	/* modelファイルの指定 */
char *SVMFileNE[NE_MODEL_NUMBER]; /* ne解析用modelファイルの指定 */

MODEL *model[PP_NUMBER];
MODEL *modelNE[NE_MODEL_NUMBER];

int count_max_words_doc(char *line) {
    int wol = 0;

    while(*line) {
	if(space_or_null((int)(*line))) {
	    wol++;
	}
	line++;
    }

    return wol + 3;
}

double _svm_classify(char *line, MODEL *m)
{
    WORD *words;
    DOC *doc;
    char *r_line, *comment = "";
    int j, max_words_doc;
    double costfactor, doc_label, dist;
    long queryid, slackid, wnum;

    r_line = (char *)my_malloc(sizeof(char) * (strlen(line) + 4));
    sprintf(r_line, "0 %s", line);

    max_words_doc = count_max_words_doc(r_line);
    words = (WORD *)my_malloc(sizeof(WORD) * (max_words_doc + 10));

    parse_document(r_line, words, &doc_label, &queryid, &slackid, &costfactor, &wnum,
		   max_words_doc, &comment);

    if (m->kernel_parm.kernel_type == 0) {   /* linear kernel */
	for(j = 0; (words[j]).wnum != 0; j++) {  /* Check if feature numbers   */
	    if ((words[j]).wnum > m->totwords) /* are not larger than in     */
		(words[j]).wnum = 0;               /* model. Remove feature if   */
	}                                        /* necessary.                 */
	doc = create_example(-1, 0, 0, 0.0, create_svector(words, comment, 1.0));

	dist = classify_example_linear(m, doc);

	free_example(doc, 1);
    }
    else {                             /* non-linear kernel */
	doc = create_example(-1, 0, 0, 0.0, create_svector(words, comment, 1.0));
	dist = classify_example(m, doc);
	free_example(doc, 1);
    }

    free(r_line);
    free(words);
    return dist;
}

int init_svm_for_anaphora() {
    int i;

    for (i = 0; i < PP_NUMBER; i++) {
	if (SVMFile[i]) {
	    model[i] = read_model(SVMFile[i]);
	    if (model[i]->kernel_parm.kernel_type == 0) { /* linear kernel */
		/* compute weight vector */
		add_weight_vector_to_linear_model(model[i]);
	    }

	    if (i == 0) { /* すべての格用 */
		return 0;
	    }
	}
	else {
	    model[i] = NULL;
	}
    }
    return 0;
}

int init_svm_for_NE() {
    int i;

    for (i = 0; i < NE_MODEL_NUMBER; i++) {
	modelNE[i] = read_model(SVMFileNE[i]);
	if (modelNE[i]->kernel_parm.kernel_type == 0) { /* linear kernel */
	    /* compute weight vector */
	    add_weight_vector_to_linear_model(modelNE[i]);
	}	
    }
    return 0;
}

double svm_classify_for_anaphora(char *line, int pp)
{
    MODEL *m;

    /* 0はすべての格用 */
    if (model[0]) {
	m = model[0];
	pp = 0;
    }
    else {
	m = model[pp];
    }

    if (!m) {
	fprintf(stderr, ";; SVM model[%d] cannot be read.\n", pp);
	exit(1);
    }

    return _svm_classify(line, m);
}

double svm_classify_for_NE(char *line, int n)
{
    MODEL *m;
    
    m = modelNE[n];

    if (!m) {
	fprintf(stderr, ";; SVM model for NE [%d] cannot be read.\n", n);
	exit(1);
    }

    return _svm_classify(line, m);
}
