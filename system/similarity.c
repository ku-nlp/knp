/*====================================================================

                         中国語類似度計算

                                                  KunYu  2007.01.22

====================================================================*/
#include "knp.h"

DBM_FILE hownet_def_db;
DBM_FILE hownet_tran_db;
DBM_FILE hownet_antonym_db;
DBM_FILE hownet_category_db;
DBM_FILE hownet_sem_def_db;
int HownetDefExist;
int HownetTranExist;
int HownetAntonymExist;
int HownetCategoryExist;
int HownetSemDefExist;

/*==================================================================*/
                       void init_hownet()
/*==================================================================*/
{
    hownet_def_db = open_dict(HOWNET_DEF_DB, HOWNET_DEF_DB_NAME, &HownetDefExist);    
    hownet_tran_db = open_dict(HOWNET_TRAN_DB, HOWNET_TRAN_DB_NAME, &HownetTranExist);    
    hownet_antonym_db = open_dict(HOWNET_ANTONYM_DB, HOWNET_ANTONYM_DB_NAME, &HownetAntonymExist);    
    hownet_category_db = open_dict(HOWNET_CATEGORY_DB, HOWNET_CATEGORY_DB_NAME, &HownetCategoryExist);    
    hownet_sem_def_db = open_dict(HOWNET_SEM_DEF_DB, HOWNET_SEM_DEF_DB_NAME, &HownetSemDefExist);    
}

/* get hownet def for word */
/*==================================================================*/
   char* get_hownet_def(char* str)
/*==================================================================*/
{
    char *key;

    if (HownetDefExist == FALSE) {
	return NULL;
    }

    key = malloc_db_buf(strlen(str)+1);

    sprintf(key, "%s", str);
    return db_get(hownet_def_db, key);
}

/* get hownet translation for word */
/*==================================================================*/
   char* get_hownet_tran(char* str)
/*==================================================================*/
{
    char *key;

    if (HownetTranExist == FALSE) {
	return NULL;
    }

    key = malloc_db_buf(strlen(str)+1);

    sprintf(key, "%s", str);
    return db_get(hownet_tran_db, key);
}

/* get hownet antonym for word */
/*==================================================================*/
   int get_hownet_antonym(char* str1, char* str2)
/*==================================================================*/
{
    char *key1, *key2, *value1, *value2;
    int ret;

    if (HownetAntonymExist == FALSE) {
	return 0;
    }

    key1 = malloc_db_buf(strlen(str1)+strlen(str2)+2);

    sprintf(key1, "%s:%s", str1, str2);
    value1 = db_get(hownet_antonym_db, key1);

    if (value1) {
	ret = atoi(value1);
	free(value1);
    }
    else {
	key2 = malloc_db_buf(strlen(str1)+strlen(str2)+2);
	sprintf(key2, "%s:%s", str2, str1);
	value2 = db_get(hownet_antonym_db, key2);
	if (value2) {
	    ret = atoi(value2);
	    free(value2);
	}
	else {
	    ret = 0;
	}
    }
    
    return ret;
}

/* get hownet def for sememe */
/*==================================================================*/
   char* get_hownet_sem_def(char *key)
/*==================================================================*/
{
    if (HownetSemDefExist == FALSE) {
	return NULL;
    }

    return db_get(hownet_sem_def_db, key);
}

/* get hownet category for word */
/*==================================================================*/
   int get_hownet_category(char *key)
/*==================================================================*/
{
    char *value;
    int ret;

    if (HownetCategoryExist == FALSE) {
	return 0;
    }

    value = db_get(hownet_category_db, key);

    if (value) {
	ret = atoi(value);
	free(value);
    }
    else {
	ret = 0;
    }
    
    return ret;
}

/* calculate word similarity */
/*==================================================================*/
     float similarity_chinese (char* str1, char* str2)
/*==================================================================*/
{    
    float sim;
    float p1, p2, p3, p4;
    float dis;
    float alpha;
    int nc1, nc2, ns;
    int nc1_sem, nc2_sem, ns_sem;
    float beta1, beta2, beta3, beta4;
    char *def_w1, *def_w2, *def_sem_w1, *def_sem_w2, *trans_w1, *trans_w2;

    int i, j;
    int tran_num_w1, tran_num_w2, concept_num_w1, concept_num_w2, concept_sem_num_w1, concept_sem_num_w2;
    int is_include, diff, is_sim;

    /* initialization */
    sim = 0.0;
    p1 = 0.0;
    p2 = 0.0;
    p3 = 0.0;
    p4 = 0.0;
    dis = 0.0;
    nc1 = 0;
    nc2 = 0;
    ns = 0;
    nc1_sem = 0;
    nc2_sem = 0;
    ns_sem = 0;
    diff = 0;
    is_sim = 0;
    def_w1 = NULL;
    def_w2 = NULL;
    trans_w1 = NULL;
    trans_w2 = NULL;
    def_sem_w1 = NULL;
    def_sem_w2 = NULL;

    /* set parameters */
    alpha = 1.6;
    beta1 = 0.1;
    beta2 = 0.1;
    beta3 = 0.7;
    beta4 = 0.1;

    /* get translation and concept definition for words */
    def_w1 = get_hownet_def(str1);
    def_w2 = get_hownet_def(str2);
    trans_w1 = get_hownet_tran(str1);
    trans_w2 = get_hownet_tran(str2);

    if (def_w1 == NULL || def_w2 == NULL || trans_w1 == NULL || trans_w2 == NULL) {
	return 0.0;
    }

    i = 0;
    if (trans_w1 != NULL) {
	tran_w1[0] = NULL;
	tran_w1[0] = strtok(trans_w1, ":");
	while (1) {
	    if (++i == HOWNET_TRAN_MAX) {
		fprintf(stderr, "Too many translations for one word");
		return 0;
	    }
	    tran_w1[i] = NULL;
	    tran_w1[i] = strtok(NULL, ":");
	    if (tran_w1[i] == NULL) {
		break;
	    }
	}
    }
    tran_num_w1 = i;

    i = 0;
    if (trans_w2 != NULL) {
	tran_w2[0] = NULL;
	tran_w2[0] = strtok(trans_w2, ":");
	while (1) {
	    if (++i == HOWNET_TRAN_MAX) {
		fprintf(stderr, "Too many translations for one word");
		return 0;
	    }
	    tran_w2[i] = NULL;
	    tran_w2[i] = strtok(NULL, ":");
	    if (tran_w2[i] == NULL) {
		break;
	    }
	}
    }
    tran_num_w2 = i;

    i = 0;
    if (def_w1 != NULL) {
	concept_w1[0] = NULL;
	concept_w1[0] = strtok(def_w1, ":");
	while (1) {
	    if (++i == HOWNET_CONCEPT_MAX) {
		fprintf(stderr, "Too many concept definitions for one word");
		return 0;
	    }
	    concept_w1[i] = NULL;
	    concept_w1[i] = strtok(NULL, ":");
	    if (concept_w1[i] == NULL) {
		break;
	    }
	}
    }
    concept_num_w1 = i;

    i = 0;
    if (def_w2 != NULL) {
	concept_w2[0] = NULL;
	concept_w2[0] = strtok(def_w2, ":");
	while (1) {
	    if (++i == HOWNET_CONCEPT_MAX) {
		fprintf(stderr, "Too many concept definitions for one word");
		return 0;
	    }
	    concept_w2[i] = NULL;
	    concept_w2[i] = strtok(NULL, ":");
	    if (concept_w2[i] == NULL) {
		break;
	    }
	}
    }
    concept_num_w2 = i;

    /* get concept definition for the first sememe of two words */
    def_sem_w1 = get_hownet_sem_def(concept_w1[0]);
    def_sem_w2 = get_hownet_sem_def(concept_w2[0]);

    i = 0;
    if (def_sem_w1 != NULL) {
	concept_sem_w1[0] = NULL;
	concept_sem_w1[0] = strtok(def_sem_w1, ":");
	while (1) {
	    if (++i == HOWNET_CONCEPT_MAX) {
		fprintf(stderr, "Too many concept definitions for one sememe");
		return 0;
	    }
	    concept_sem_w1[i] = NULL;
	    concept_sem_w1[i] = strtok(NULL, ":");
	    if (concept_sem_w1[i] == NULL) {
		break;
	    }
	}
    }
    concept_sem_num_w1 = i;

    i = 0;
    if (def_sem_w2 != NULL) {
	concept_sem_w2[0] = NULL;
	concept_sem_w2[0] = strtok(def_sem_w2, ":");
	while (1) {
	    if (++i == HOWNET_CONCEPT_MAX) {
		fprintf(stderr, "Too many concept definitions for one sememe");
		return 0;
	    }
	    concept_sem_w2[i] = NULL;
	    concept_sem_w2[i] = strtok(NULL, ":");
	    if (concept_sem_w2[i] == NULL) {
		break;
	    }
	}
    }
    concept_sem_num_w2 = i;
  
    /* step 1 */
    if (tran_num_w1 > 0 && tran_num_w2 > 0) {
	is_sim = 1;
    }
    for (i = 0; i < tran_num_w1; i++) {
	if (!is_sim) {
	    break;
	}
	for (j = 0; j < tran_num_w2; j++) {
	    if (tran_w1[i] != NULL && tran_w2[j] != NULL && !strcmp(tran_w1[i], tran_w2[j])) {
		continue;
	    }
	    else {
		is_sim = 0;
		break;
	    }
	}
    }
    if (is_sim) {
	sim = 1.0;
	return sim;
    }

    /* step 2 */
    if (concept_num_w1 > 0 && concept_num_w2 > 0) {
	is_sim = 1;
    }
    for (i = 0; i < concept_num_w1; i++) {
	if (!is_sim) {
	    break;
	}
	for (j = 0; j < concept_num_w2; j++) {
	    if (concept_w1[i] != NULL && concept_w2[j] != NULL && !strcmp(concept_w1[i], concept_w2[j])) {
		continue;
	    }
	    else {
		is_sim = 0;
		break;
	    }
	}
    }
    if (is_sim) {
	sim = 0.95;
	return sim;
    }

    /* step 3 */
    if (get_hownet_antonym(str1, str2)) {
	sim = 1.0;
	return sim;
    }

    /* step 5 */
    /* step 5.1 */
    is_include = 0;
    for (i = 0; i < (concept_num_w1 < concept_num_w2 ? concept_num_w1:concept_num_w2); i++) {
	for (j = 0; j < (concept_num_w1 < concept_num_w2 ? concept_num_w2:concept_num_w1); j++) {
	    if (concept_w1[(concept_num_w1 < concept_num_w2 ? i:j)] != NULL && concept_w2[(concept_num_w1 < concept_num_w2 ? j:i)] != NULL && !strcmp(concept_w1[(concept_num_w1 < concept_num_w2 ? i:j)], concept_w2[(concept_num_w1 < concept_num_w2 ? j:i)])) {
		is_include++;
		break;
	    }
	}
    }
    if (is_include == (concept_num_w1 < concept_num_w2 ? concept_num_w1:concept_num_w2)) {
	p1 = 1.0;
    }

    /* step 5.2 */
    diff = (concept_num_w1 < concept_num_w2 ? concept_num_w1:concept_num_w2);
    for (i = 0; i < (concept_num_w1 < concept_num_w2 ? concept_num_w1:concept_num_w2); i++) {
	if (concept_w1[i] != NULL && concept_w2[j] != NULL && strcmp(concept_w1[i], concept_w2[i]) != 0) {
	    diff = i;
	    break;
	}
    }
    if (diff > 0) {
	dis = get_hownet_category(concept_w1[diff-1]);
    }
    if (dis > 0) {
	p2 = 1.0*alpha/(dis + alpha);
    }

    /* step 5.3 */
    if (is_include == (concept_num_w1 < concept_num_w2 ? concept_num_w1:concept_num_w2)) {
	ns = is_include;
    }
    else {
	for (i = 0; i < concept_num_w1; i++) {
	    for (j = 0; j < concept_num_w2; j++) {
		if (concept_w1[i] != NULL && concept_w2[j] != NULL && !strcmp(concept_w1[i], concept_w2[j])) {
		    ns++;
		}
	    }
	}
    }
    nc1 = concept_num_w1;
    nc2 = concept_num_w2;
    if (nc1 != 0 || nc2 != 0) {
	p3 = 2.0*ns/(nc1+nc2);
    }
    else {
	p3 = 0.0;
    }

    /* step 5.4 */
    for (i = 0; i < concept_sem_num_w1; i++) {
	for (j = 0; j < concept_sem_num_w2; j++) {
	    if (concept_sem_w1[i] != NULL && concept_sem_w2[j] != NULL && !strcmp(concept_sem_w1[i], concept_sem_w2[j])) {
		ns_sem++;
	    }
	}
    }
    nc1_sem = concept_sem_num_w1;
    nc2_sem = concept_sem_num_w2;
    if (nc1_sem != 0 || nc2_sem != 0) {
	p4 = 2.0*ns_sem/(nc1_sem+nc2_sem);
    }
    else {
	p4 = 0.0;
    }

    /* step 5.5 */
    sim = p1*beta1 + p2*beta2 + p3*beta3 + p4*beta4;

    /* free memory */
    if (trans_w1) {
	free(trans_w1);
    }
    if (trans_w2) {
	free(trans_w2);
    }
    if (def_w1) {
	free(def_w1);
    }
    if (def_w2) {
	free(def_w2);
    }
    if (def_sem_w1) {
	free(def_sem_w1);
    }
    if (def_sem_w2) {
	free(def_sem_w2);
    }

    return sim;
}
