/*====================================================================

                                 依存構造解析
 
                                                S.Kurohashi 93. 5.31

       $Id$

====================================================================*/
#include "knp.h"

/* DB file for Chinese dpnd rule */
DBM_FILE chi_dpnd_db;
int     CHIDpndExist;
DBM_FILE chi_dpnd_prob_db;
int     CHIDpndProbExist;
DBM_FILE chi_dis_comma_db;
int     CHIDisCommaExist;
DBM_FILE chi_case_db;
int     CHICaseExist;
DBM_FILE chi_pos_db;
int     CHIPosExist;

int Possibility;	/* 依存構造の可能性の何番目か */
static int dpndID = 0;

double giga_weight = 0.4;
double prob_bk_weight_1= 0.8;
double prob_bk_weight_2 = 0.5;

double fprob_LtoR, fprob_RtoL;

/*==================================================================*/
             void assign_dpnd_rule(SENTENCE_DATA *sp)
/*==================================================================*/
{
  int 	i, j;
  BNST_DATA	*b_ptr;
  DpndRule 	*r_ptr;

  for (i = 0, b_ptr = sp->bnst_data; i < sp->Bnst_num; i++, b_ptr++) {
    for (j = 0, r_ptr = DpndRuleArray; j < CurDpndRuleSize; j++, r_ptr++) {
      if (feature_pattern_match(&(r_ptr->dependant), b_ptr->f, NULL, b_ptr) 
	  == TRUE) {
	b_ptr->dpnd_rule = r_ptr; 
	break;
      }
    }

    if (b_ptr->dpnd_rule == NULL) {
      fprintf(stderr, ";; No DpndRule for %dth bnst (", i);
      print_feature(b_ptr->f, stderr);
      fprintf(stderr, ")\n");

      /* DpndRuleArray[0] はマッチしない時用 */
      b_ptr->dpnd_rule = DpndRuleArray;
    }
  }
}

/*==================================================================*/
                      void close_chi_dpnd_db()
/*==================================================================*/
{
  if (!OptChiGenerative) {
    DB_close(chi_dpnd_db);
  }
  else {
    DB_close(chi_dpnd_prob_db);
    DB_close(chi_dis_comma_db);
    DB_close(chi_case_db);
    DB_close(chi_pos_db);
  }
}

/*==================================================================*/
                     void init_chi_dpnd_db()
/*==================================================================*/
{
  if (!OptChiGenerative) {
    chi_dpnd_db = open_dict(CHI_DPND_DB, CHI_DPND_DB_NAME, &CHIDpndExist);
  }
  else {
    chi_dpnd_prob_db = open_dict(CHI_DPND_PROB_DB, CHI_DPND_PROB_DB_NAME, &CHIDpndProbExist);
    chi_dis_comma_db = open_dict(CHI_DIS_COMMA_DB, CHI_DIS_COMMA_DB_NAME, &CHIDisCommaExist);
    chi_case_db = open_dict(CHI_CASE_DB, CHI_CASE_DB_NAME, &CHICaseExist);
    chi_pos_db = open_dict(CHI_POS_DB, CHI_POS_DB_NAME, &CHIPosExist);
  }
}

/*==================================================================*/
                       void free_chi_type()
/*==================================================================*/
{
  int i;
  for (i = 0; i < CHI_POS_MAX; i++) {
    if (Chi_word_type[i]) {
      free(Chi_word_type[i]);
      Chi_word_type[i] = NULL;
    }
  }
}

/*==================================================================*/
                       void init_chi_type()
/*==================================================================*/
{
  int i;
  for (i = 0; i < CHI_POS_MAX; i++) {
    Chi_word_type[i] = (char *)malloc(sizeof(char)*(CHI_TYPE_LEN_MAX + 1));
  }

  strcpy(Chi_word_type[0], "adv");
  strcpy(Chi_word_type[1], "verbMarker");
  strcpy(Chi_word_type[2], "ba");
  strcpy(Chi_word_type[3], "coor");
  strcpy(Chi_word_type[4], "num");
  strcpy(Chi_word_type[5], "subord");
  strcpy(Chi_word_type[6], "relDe");
  strcpy(Chi_word_type[7], "assoDe");
  strcpy(Chi_word_type[8], "vDe");
  strcpy(Chi_word_type[9], "vDe");
  strcpy(Chi_word_type[10], "det");
  strcpy(Chi_word_type[11], "nounMarker");
  strcpy(Chi_word_type[12], "foreign");
  strcpy(Chi_word_type[13], "other");
  strcpy(Chi_word_type[14], "adj");
  strcpy(Chi_word_type[15], "lBei");
  strcpy(Chi_word_type[16], "post");
  strcpy(Chi_word_type[17], "measure");
  strcpy(Chi_word_type[18], "particle");
  strcpy(Chi_word_type[19], "noun");
  strcpy(Chi_word_type[20], "noun");
  strcpy(Chi_word_type[21], "tempNoun");
  strcpy(Chi_word_type[22], "num");
  strcpy(Chi_word_type[23], "sound");
  strcpy(Chi_word_type[24], "prep");
  strcpy(Chi_word_type[25], "noun");
  strcpy(Chi_word_type[26], "punc");
  strcpy(Chi_word_type[27], "sBei");
  strcpy(Chi_word_type[28], "sentMarker");
  strcpy(Chi_word_type[29], "verb");
  strcpy(Chi_word_type[30], "verb");
  strcpy(Chi_word_type[31], "verb");
  strcpy(Chi_word_type[32], "verb");
}

/*==================================================================*/
                      void free_chi_pos()
/*==================================================================*/
{
  int i;
  for (i = 0; i < CHI_POS_MAX; i++) {
    if (Chi_word_pos[i]) {
      free(Chi_word_pos[i]);
      Chi_word_pos[i] = NULL;
    }
  }
}

/*==================================================================*/
                        void init_chi_pos()
/*==================================================================*/
{
  int i;
  for (i = 0; i < CHI_POS_MAX; i++) {
    Chi_word_pos[i] = (char *)malloc(sizeof(char)*(CHI_POS_LEN_MAX + 1));
  }

  strcpy(Chi_word_pos[0], "AD");
  strcpy(Chi_word_pos[1], "AS");
  strcpy(Chi_word_pos[2], "BA");
  strcpy(Chi_word_pos[3], "CC");
  strcpy(Chi_word_pos[4], "CD");
  strcpy(Chi_word_pos[5], "CS");
  strcpy(Chi_word_pos[6], "DEC");
  strcpy(Chi_word_pos[7], "DEG");
  strcpy(Chi_word_pos[8], "DER");
  strcpy(Chi_word_pos[9], "DEV");
  strcpy(Chi_word_pos[10], "DT");
  strcpy(Chi_word_pos[11], "ETC");
  strcpy(Chi_word_pos[12], "FW");
  strcpy(Chi_word_pos[13], "IJ");
  strcpy(Chi_word_pos[14], "JJ");
  strcpy(Chi_word_pos[15], "LB");
  strcpy(Chi_word_pos[16], "LC");
  strcpy(Chi_word_pos[17], "M");
  strcpy(Chi_word_pos[18], "MSP");
  strcpy(Chi_word_pos[19], "NN");
  strcpy(Chi_word_pos[20], "NR");
  strcpy(Chi_word_pos[21], "NT");
  strcpy(Chi_word_pos[22], "OD");
  strcpy(Chi_word_pos[23], "ON");
  strcpy(Chi_word_pos[24], "P");
  strcpy(Chi_word_pos[25], "PN");
  strcpy(Chi_word_pos[26], "PU");
  strcpy(Chi_word_pos[27], "SB");
  strcpy(Chi_word_pos[28], "SP");
  strcpy(Chi_word_pos[29], "VA");
  strcpy(Chi_word_pos[30], "VC");
  strcpy(Chi_word_pos[31], "VE");
  strcpy(Chi_word_pos[32], "VV");
}

/* get dpnd rule for Chinese */
/*==================================================================*/
char* get_chi_dpnd_rule(char *word1, char *pos1, char *word2, char *pos2, int distance, int comma)
/*==================================================================*/
{
  char *key;

  if ((!OptChiGenerative && CHIDpndExist == FALSE) || (OptChiGenerative && CHIDpndProbExist == FALSE)) {
    return NULL;
  }

  if (OptChiGenerative) {
    if (!strcmp(word2, "ROOT") && OptChiGenerative) {
      key = malloc_db_buf(strlen(word1) + strlen(pos1) + 7);
      sprintf(key, "%s_%s_ROOT", pos1, word1);
    }
    else {
      if (distance == 0) {
	key = malloc_db_buf(strlen(word1) + strlen(word2) + strlen(pos1) + strlen(pos2) + 4);
	sprintf(key, "%s_%s_%s_%s", pos1, word1, pos2, word2);
      }
      else {
	key = malloc_db_buf(strlen(word1) + strlen(word2) + strlen(pos1) + strlen(pos2) + 8);
	sprintf(key, "%s_%s_%s_%s_%d_%d", pos1, word1, pos2, word2, distance, comma);
      }
    }
  }
  else {
    key = malloc_db_buf(strlen(word1) + strlen(word2) + strlen(pos1) + strlen(pos2) + 6);
    sprintf(key, "%s_%s_%s_%s_%d", pos1, word1, pos2, word2, distance);
  }
  if (!OptChiGenerative) {
    return db_get(chi_dpnd_db, key);
  }
  else {
    if (distance == 0 || !strcmp(word2, "ROOT")) {
      return db_get(chi_dpnd_prob_db, key);
    }
    else {
      return db_get(chi_dis_comma_db, key);
    }
  }
}

/*==================================================================*/
              void calc_dpnd_matrix(SENTENCE_DATA *sp)
/*==================================================================*/
{
  int i, j, k, l, s, value, first_uke_flag;
  BNST_DATA *k_ptr, *u_ptr;
  char *lex_rule, *pos_rule_1, *pos_rule_2, *pos_rule;
  char *type, *probL, *probR, *occur, *dpnd;
  int count;
  char *rule;
  char *curRule[CHI_DPND_TYPE_MAX];
  int appear_LtoR_2, appear_RtoL_2, appear_LtoR_3, appear_RtoL_3, total_2, total_3;
  int distance;

  char  	direction_1[CHI_DPND_TYPE_MAX]; /* store different directions for each type */
  char  	direction_2[CHI_DPND_TYPE_MAX]; /* store different directions for each type */
  char  	direction_3[CHI_DPND_TYPE_MAX]; /* store different directions for each type */
  char  	direction_4[CHI_DPND_TYPE_MAX]; /* store different directions for each type */
  double         prob_LtoR_1[CHI_DPND_TYPE_MAX]; /* store different probability for each type */
  double         prob_RtoL_1[CHI_DPND_TYPE_MAX];
  double         prob_LtoR_2[CHI_DPND_TYPE_MAX]; /* store different probability for each type */
  double         prob_RtoL_2[CHI_DPND_TYPE_MAX];
  double         prob_LtoR_3[CHI_DPND_TYPE_MAX]; /* store different probability for each type */
  double         prob_RtoL_3[CHI_DPND_TYPE_MAX];
  double         prob_LtoR_4[CHI_DPND_TYPE_MAX]; /* store different probability for each type */
  double         prob_RtoL_4[CHI_DPND_TYPE_MAX];
  char        type_1[CHI_DPND_TYPE_MAX][CHI_DPND_TYPE_LEN_MAX]; /* store different dpnd type */
  char        type_2[CHI_DPND_TYPE_MAX][CHI_DPND_TYPE_LEN_MAX]; /* store different dpnd type */
  char        type_3[CHI_DPND_TYPE_MAX][CHI_DPND_TYPE_LEN_MAX]; /* store different dpnd type */
  char        type_4[CHI_DPND_TYPE_MAX][CHI_DPND_TYPE_LEN_MAX]; /* store different dpnd type */
  double         occur_1[CHI_DPND_TYPE_MAX]; /* store occur time of different dpnd type */
  double         occur_2[CHI_DPND_TYPE_MAX]; /* store occur time of different dpnd type */
  double         occur_3[CHI_DPND_TYPE_MAX]; /* store occur time of different dpnd type */
  double         occur_4[CHI_DPND_TYPE_MAX]; /* store occur time of different dpnd type */
  double         occur_RtoL_1[CHI_DPND_TYPE_MAX]; /* store occur time of different dpnd type */
  double         occur_RtoL_2[CHI_DPND_TYPE_MAX]; /* store occur time of different dpnd type */
  double         occur_RtoL_3[CHI_DPND_TYPE_MAX]; /* store occur time of different dpnd type */
  double         occur_RtoL_4[CHI_DPND_TYPE_MAX]; /* store occur time of different dpnd type */
  int         count_1; /* number of dpnd type */
  int         count_2; /* number of dpnd type */
  int         count_3; /* number of dpnd type */
  int         count_4; /* number of dpnd type */
  double      lamda1[CHI_DPND_TYPE_MAX]; /* parameter of each dpnd type */
  double      lamda2[CHI_DPND_TYPE_MAX]; /* parameter of each dpnd type */

  /* initialization */
  lex_rule = NULL;
  pos_rule_1 = NULL;
  pos_rule_2 = NULL;
  pos_rule = NULL;
  rule = NULL;
  for (i = 0; i < CHI_DPND_TYPE_MAX; i++) {
    prob_LtoR_1[i] = 0;
    prob_LtoR_2[i] = 0;
    prob_LtoR_3[i] = 0;
    prob_LtoR_4[i] = 0;
    occur_1[i] = 0;
    occur_2[i] = 0;
    occur_3[i] = 0;
    occur_4[i] = 0;
    prob_RtoL_1[i] = 0;
    prob_RtoL_2[i] = 0;
    prob_RtoL_3[i] = 0;
    prob_RtoL_4[i] = 0;
    occur_RtoL_1[i] = 0;
    occur_RtoL_2[i] = 0;
    occur_RtoL_3[i] = 0;
    occur_RtoL_4[i] = 0;
    direction_1[i] = 0;
    direction_2[i] = 0;
    direction_3[i] = 0;
    direction_4[i] = 0;
    lamda1[i] = 0;
    lamda2[i] = 0;
  }

  count_1 = 0;
  count_2 = 0;
  count_3 = 0;
  count_4 = 0;

  for (i = 0; i < sp->Bnst_num; i++) {
    k_ptr = sp->bnst_data + i;
    first_uke_flag = 1;
    for (j = i + 1; j < sp->Bnst_num; j++) {
      u_ptr = sp->bnst_data + j;
      lex_rule = NULL;
      pos_rule_1 = NULL;
      pos_rule_2 = NULL;
      pos_rule = NULL;

      Dpnd_matrix[i][j] = 0;
      Chi_dpnd_matrix[i][j].count = 0;

      if (Language != CHINESE) {
	for (k = 0; k_ptr->dpnd_rule->dpnd_type[k]; k++) {
	  value = feature_pattern_match(&(k_ptr->dpnd_rule->governor[k]),
					u_ptr->f, k_ptr, u_ptr);
	  if (value == TRUE) {
	    Dpnd_matrix[i][j] = (int)k_ptr->dpnd_rule->dpnd_type[k];
	    first_uke_flag = 0;
	    break;
	  }
	}
      }
      else {
	if (j == i + 1) {
	  distance = 1;
	}
	else {
	  distance = 2;
	}
	/* read dpnd rule from DB for Chinese */
	lex_rule = get_chi_dpnd_rule(k_ptr->head_ptr->Goi, k_ptr->head_ptr->Pos, u_ptr->head_ptr->Goi, u_ptr->head_ptr->Pos, distance, 0);
	pos_rule_1 = get_chi_dpnd_rule(k_ptr->head_ptr->Goi, k_ptr->head_ptr->Pos, "X", u_ptr->head_ptr->Pos, distance, 0);
	pos_rule_2 = get_chi_dpnd_rule("X", k_ptr->head_ptr->Pos, u_ptr->head_ptr->Goi, u_ptr->head_ptr->Pos, distance, 0);
	pos_rule = get_chi_dpnd_rule("X", k_ptr->head_ptr->Pos, "X", u_ptr->head_ptr->Pos,distance, 0);

	if (lex_rule != NULL) {
	  count = 0;
	  rule = NULL;
	  rule = strtok(lex_rule, ":");
	  while (rule) {
	    curRule[count] = malloc(strlen(rule) + 1);
	    strcpy(curRule[count], rule);
	    count++;
	    rule = NULL;
	    rule = strtok(NULL, ":");
	  }

	  count_1 = count;
	  for (k = 0; k < count; k++) {
	    type = NULL;
	    probL = NULL;
	    probR = NULL;
	    occur = NULL;
			    
	    type = strtok(curRule[k], "_");
	    probR = strtok(NULL, "_");
	    probL = strtok(NULL, "_");
	    occur = strtok(NULL, "_");
	    dpnd = strtok(NULL, "_");
			    
	    if (!strcmp(type, "R")) {
	      direction_1[k] = 'R';
	    }
	    else if (!strcmp(type, "L")) {
	      direction_1[k] = 'L';
	    }
	    else if (!strcmp(type, "B")) {
	      direction_1[k] = 'B';
	    }
	    occur_1[k] = atof(occur);
	    strcpy(type_1[k], dpnd);
	    prob_LtoR_1[k] = atof(probR);
	    prob_RtoL_1[k] = atof(probL);

	    if (curRule[k]) {
	      free(curRule[k]);
	      curRule[k] = NULL;
	    }
	  }
	}

	if (pos_rule_1 != NULL) {
	  count = 0;
	  rule = NULL;
	  rule = strtok(pos_rule_1, ":");
	  while (rule) {
	    curRule[count] = malloc(strlen(rule) + 1);
	    strcpy(curRule[count], rule);
	    count++;
	    rule = NULL;
	    rule = strtok(NULL, ":");
	  }

	  count_2 = count;
	  for (k = 0; k < count; k++) {
	    type = NULL;
	    probL = NULL;
	    probR = NULL;
	    occur = NULL;
			    
	    type = strtok(curRule[k], "_");
	    probR = strtok(NULL, "_");
	    probL = strtok(NULL, "_");
	    occur = strtok(NULL, "_");
	    dpnd = strtok(NULL, "_");
			    
	    if (!strcmp(type, "R")) {
	      direction_2[k] = 'R';
	    }
	    else if (!strcmp(type, "L")) {
	      direction_2[k] = 'L';
	    }
	    else if (!strcmp(type, "B")) {
	      direction_2[k] = 'B';
	    }
	    occur_2[k] = atof(occur);
	    strcpy(type_2[k], dpnd);
	    prob_LtoR_2[k] = atof(probR);
	    prob_RtoL_2[k] = atof(probL);

	    if (curRule[k]) {
	      free(curRule[k]);
	      curRule[k] = NULL;
	    }
	  }
	}

	if (pos_rule_2 != NULL) {
	  count = 0;
	  rule = NULL;
	  rule = strtok(pos_rule_2, ":");
	  while (rule) {
	    curRule[count] = malloc(strlen(rule) + 1);
	    strcpy(curRule[count], rule);
	    count++;
	    rule = NULL;
	    rule = strtok(NULL, ":");
	  }

	  count_3 = count;
	  for (k = 0; k < count; k++) {
	    type = NULL;
	    probL = NULL;
	    probR = NULL;
	    occur = NULL;
			    
	    type = strtok(curRule[k], "_");
	    probR = strtok(NULL, "_");
	    probL = strtok(NULL, "_");
	    occur = strtok(NULL, "_");
	    dpnd = strtok(NULL, "_");
			    
	    if (!strcmp(type, "R")) {
	      direction_3[k] = 'R';
	    }
	    else if (!strcmp(type, "L")) {
	      direction_3[k] = 'L';
	    }
	    else if (!strcmp(type, "B")) {
	      direction_3[k] = 'B';
	    }
	    occur_3[k] = atof(occur);
	    strcpy(type_3[k], dpnd);
	    prob_LtoR_3[k] = atof(probR);
	    prob_RtoL_3[k] = atof(probL);

	    if (curRule[k]) {
	      free(curRule[k]);
	      curRule[k] = NULL;
	    }
	  }
	}

	if (pos_rule != NULL) {
	  count = 0;
	  rule = NULL;
	  rule = strtok(pos_rule, ":");
	  while (rule) {
	    curRule[count] = malloc(strlen(rule) + 1);
	    strcpy(curRule[count], rule);
	    count++;
	    rule = NULL;
	    rule = strtok(NULL, ":");
	  }

	  count_4 = count;
	  for (k = 0; k < count; k++) {
	    type = NULL;
	    probL = NULL;
	    probR = NULL;
	    occur = NULL;
			    
	    type = strtok(curRule[k], "_");
	    probR = strtok(NULL, "_");
	    probL = strtok(NULL, "_");
	    occur = strtok(NULL, "_");
	    dpnd = strtok(NULL, "_");
			    
	    if (!strcmp(type, "R")) {
	      direction_4[k] = 'R';
	    }
	    else if (!strcmp(type, "L")) {
	      direction_4[k] = 'L';
	    }
	    else if (!strcmp(type, "B")) {
	      direction_4[k] = 'B';
	    }
	    occur_4[k] = atof(occur);
	    strcpy(type_4[k], dpnd);
	    prob_LtoR_4[k] = atof(probR);
	    prob_RtoL_4[k] = atof(probL);

	    if (curRule[k]) {
	      free(curRule[k]);
	      curRule[k] = NULL;
	    }
	  }
	}

	Chi_dpnd_matrix[i][j].prob_pos_LtoR = 0;
	Chi_dpnd_matrix[i][j].prob_pos_RtoL = 0;
	/* calculate pos probability */
	if (pos_rule != NULL) {
	  for (k = 0; k < count_4; k++) {
	    Chi_dpnd_matrix[i][j].prob_pos_LtoR += prob_LtoR_4[k];
	    Chi_dpnd_matrix[i][j].prob_pos_RtoL += prob_RtoL_4[k];
	    Chi_dpnd_matrix[i][j].occur_pos += occur_4[k];
	  }
	}
	Chi_dpnd_matrix[i][j].prob_pos_LtoR = 1.0 * Chi_dpnd_matrix[i][j].prob_pos_LtoR / Chi_dpnd_matrix[i][j].occur_pos;
	Chi_dpnd_matrix[i][j].prob_pos_RtoL = 1.0 * Chi_dpnd_matrix[i][j].prob_pos_RtoL / Chi_dpnd_matrix[i][j].occur_pos;

	/* calculate probability */
	if (lex_rule != NULL) {
	  Chi_dpnd_matrix[i][j].count = count_1;
	  for (k = 0; k < Chi_dpnd_matrix[i][j].count; k++) {
	    lamda1[k] = (1.0 * occur_1[k])/(occur_1[k] + 1);
	    appear_LtoR_2 = 0;
	    appear_RtoL_2 = 0;
	    appear_LtoR_3 = 0;
	    appear_RtoL_3 = 0;
	    total_2 = 0;
	    total_3 = 0;
	    for (l = 0; l < count_2; l++) {
	      if (!strcmp(type_1[k], type_2[l])) {
		appear_LtoR_2 = prob_LtoR_2[l];
		appear_RtoL_2 = prob_RtoL_2[l];
		total_2 = occur_2[l];
		break;
	      }
	    }
	    for (l = 0; l < count_3; l++) {
	      if (!strcmp(type_1[k], type_3[l])) {
		appear_LtoR_3 = prob_LtoR_3[l];
		appear_RtoL_3 = prob_RtoL_3[l];
		total_3 = occur_3[l];
		break;
	      }
	    }
	    if (total_2 != 0 || total_3 != 0) {
	      Chi_dpnd_matrix[i][j].prob_LtoR[k] = 1.0 * lamda1[k] * (1.0 * prob_LtoR_1[k]/occur_1[k]) +
		(1.0 * (1 - lamda1[k]) * (1.0 * (appear_LtoR_2 + appear_LtoR_3)/(total_2 + total_3)));
	      Chi_dpnd_matrix[i][j].prob_RtoL[k] = 1.0 * lamda1[k] * (1.0 * prob_RtoL_1[k]/occur_1[k]) +
		(1.0 * (1 - lamda1[k]) * (1.0 * (appear_RtoL_2 + appear_RtoL_3)/(total_2 + total_3)));
	    }
	    else {
	      Chi_dpnd_matrix[i][j].prob_LtoR[k] = 1.0 * lamda1[k] * (1.0 * prob_LtoR_1[k]/occur_1[k]);
	      Chi_dpnd_matrix[i][j].prob_RtoL[k] = 1.0 * lamda1[k] * (1.0 * prob_RtoL_1[k]/occur_1[k]);
	    }

	    Chi_dpnd_matrix[i][j].direction[k] = direction_1[k];
	    strcpy(Chi_dpnd_matrix[i][j].type[k],type_1[k]);
	  }
	}
	else if (pos_rule_1 != NULL || pos_rule_2 != NULL) {
	  if (pos_rule_1 != NULL) {
	    Chi_dpnd_matrix[i][j].count = count_2;
	    for (k = 0; k < Chi_dpnd_matrix[i][j].count; k++) {
	      appear_LtoR_2 = prob_LtoR_2[k];
	      appear_RtoL_2 = prob_RtoL_2[k];
	      appear_LtoR_3 = 0;
	      appear_RtoL_3 = 0;
	      total_2 = occur_2[k];
	      total_3 = 0;
	      for (l = 0; l < count_3; l++) {
		if (!strcmp(type_2[k], type_3[l])) {
		  appear_LtoR_3 = prob_LtoR_3[l];
		  appear_RtoL_3 = prob_RtoL_3[l];
		  total_3 = occur_3[l];
		  break;
		}
	      }
	      lamda2[k] = (1.0 * (total_2 + total_3))/(total_2 + total_3 + 1);
	      Chi_dpnd_matrix[i][j].prob_LtoR[k] = 1.0 * lamda2[k] * (1.0 * (appear_LtoR_2 + appear_LtoR_3)/(total_2 + total_3));
	      Chi_dpnd_matrix[i][j].prob_RtoL[k] = 1.0 * lamda2[k] * (1.0 * (appear_RtoL_2 + appear_RtoL_3)/(total_2 + total_3));
	      strcpy(Chi_dpnd_matrix[i][j].type[k],type_2[k]);
	      for (l = 0; l < count_4; l++) {
		if (!strcmp(type_2[k], type_4[l])) {
		  Chi_dpnd_matrix[i][j].prob_LtoR[k] += (1 - lamda2[k]) * (1.0 * prob_LtoR_4[l] / occur_4[l]);
		  Chi_dpnd_matrix[i][j].prob_RtoL[k] += (1 - lamda2[k]) * (1.0 * prob_RtoL_4[l] / occur_4[l]);
		  break;
		}
	      }
	      Chi_dpnd_matrix[i][j].direction[k] = direction_2[k];

	      Chi_dpnd_matrix[i][j].prob_LtoR[k] *= prob_bk_weight_1;
	      Chi_dpnd_matrix[i][j].prob_RtoL[k] *= prob_bk_weight_1;
	    }
	  }
	  else if (pos_rule_2 != NULL) {
	    Chi_dpnd_matrix[i][j].count = count_3;
	    for (k = 0; k < Chi_dpnd_matrix[i][j].count; k++) {
	      lamda2[k] = (1.0 * occur_3[k]) / (occur_3[k] + 1);
	      Chi_dpnd_matrix[i][j].prob_LtoR[k] = 1.0 * lamda2[k] * (1.0 * prob_LtoR_3[k] / occur_3[k]);
	      Chi_dpnd_matrix[i][j].prob_RtoL[k] = 1.0 * lamda2[k] * (1.0 * prob_RtoL_3[k] / occur_3[k]);
	      strcpy(Chi_dpnd_matrix[i][j].type[k], type_3[k]);
	      for (l = 0; l < count_4; l++) {
		if (!strcmp(type_3[k], type_4[l])) {
		  Chi_dpnd_matrix[i][j].prob_LtoR[k] += (1 - lamda2[k]) * (1.0 * prob_LtoR_4[l] / occur_4[k]);
		  Chi_dpnd_matrix[i][j].prob_RtoL[k] += (1 - lamda2[k]) * (1.0 * prob_RtoL_4[l] / occur_4[k]);
		  break;
		}
	      }
	      Chi_dpnd_matrix[i][j].direction[k] = direction_3[k];

	      Chi_dpnd_matrix[i][j].prob_LtoR[k] *= prob_bk_weight_1;
	      Chi_dpnd_matrix[i][j].prob_RtoL[k] *= prob_bk_weight_1;
	    }
	  }
	}
	else {
	  Chi_dpnd_matrix[i][j].count = count_4;
	  for (k = 0; k < Chi_dpnd_matrix[i][j].count; k++) {
	    Chi_dpnd_matrix[i][j].prob_LtoR[k] = (1.0 * prob_LtoR_4[k]) / occur_4[k];
	    Chi_dpnd_matrix[i][j].prob_RtoL[k] = (1.0 * prob_RtoL_4[k]) / occur_4[k];
	    Chi_dpnd_matrix[i][j].direction[k] = direction_4[k];

	    Chi_dpnd_matrix[i][j].prob_LtoR[k] *= prob_bk_weight_2;
	    Chi_dpnd_matrix[i][j].prob_RtoL[k] *= prob_bk_weight_2;

	    strcpy(Chi_dpnd_matrix[i][j].type[k], type_4[k]);
	  }
	}

	if (lex_rule != NULL || pos_rule_1 != NULL || pos_rule_2 != NULL || pos_rule != NULL) {
	  first_uke_flag = 0;
	  if (Chi_dpnd_matrix[i][j].count > 0) {
	    Dpnd_matrix[i][j] = 'O';
	  }
	}
      }
    }
  }

  if (Language == CHINESE) {
    /* free memory */
    if (lex_rule) {
      free(lex_rule);
      lex_rule = NULL;
    }
    if (pos_rule_1) {
      free(pos_rule_1);
      pos_rule_1 = NULL;
    }
    if (pos_rule_2) {
      free(pos_rule_2);
      pos_rule_2 = NULL;
    }
    if (pos_rule) {
      free(pos_rule);
      pos_rule = NULL;
    }
    if (rule) {
      free(rule);
      rule = NULL;
    }
  }
}

/* calculate dpnd and dis_comma probability */
/*==================================================================*/
void get_prob(SENTENCE_DATA *sp, int left, int right, int distance, int comma)
/*==================================================================*/
{
  int i, j, k;
  char *lex_rule, *pos_rule_1, *pos_rule_2, *pos_rule;
  char *probLtoR, *probRtoL, *occurLtoR, *occurRtoL, *dpnd, *direction, *rule;
  char *curRule[CHI_DPND_TYPE_MAX];
  int count;
  double prob[2];
  BNST_DATA *k_ptr, *u_ptr;
    
  double      prob_LtoR_1[2];
  double      prob_LtoR_2[2];
  double      prob_LtoR_3[2];
  double      prob_LtoR_4[2];
  double      occur_LtoR_1[2];
  double      occur_LtoR_2[2];
  double      occur_LtoR_3[2];
  double      occur_LtoR_4[2];
  double      prob_RtoL_1[2];
  double      prob_RtoL_2[2];
  double      prob_RtoL_3[2];
  double      prob_RtoL_4[2];
  double      occur_RtoL_1[2];
  double      occur_RtoL_2[2];
  double      occur_RtoL_3[2];
  double      occur_RtoL_4[2];
  double      lamda; /* parameter of each dpnd type */

  /* initialization */
  k_ptr = sp->bnst_data + left;
  u_ptr = sp->bnst_data + right;

  lex_rule = NULL;
  pos_rule_1 = NULL;
  pos_rule_2 = NULL;
  pos_rule = NULL;

  prob_LtoR_1[0] = 0;
  prob_LtoR_1[1] = 0;
  prob_LtoR_2[0] = 0;
  prob_LtoR_2[1] = 0;
  prob_LtoR_3[0] = 0;
  prob_LtoR_3[1] = 0;
  prob_LtoR_4[0] = 0;
  prob_LtoR_4[1] = 0;
  occur_LtoR_1[0] = 0;
  occur_LtoR_1[1] = 0;
  occur_LtoR_2[0] = 0;
  occur_LtoR_2[1] = 0;
  occur_LtoR_3[0] = 0;
  occur_LtoR_3[1] = 0;
  occur_LtoR_4[0] = 0;
  occur_LtoR_4[1] = 0;
  prob_RtoL_1[0] = 0;
  prob_RtoL_1[1] = 0;
  prob_RtoL_2[0] = 0;
  prob_RtoL_2[1] = 0;
  prob_RtoL_3[0] = 0;
  prob_RtoL_3[1] = 0;
  prob_RtoL_4[0] = 0;
  prob_RtoL_4[1] = 0;
  occur_RtoL_1[0] = 0;
  occur_RtoL_1[1] = 0;
  occur_RtoL_2[0] = 0;
  occur_RtoL_2[1] = 0;
  occur_RtoL_3[0] = 0;
  occur_RtoL_3[1] = 0;
  occur_RtoL_4[0] = 0;
  occur_RtoL_4[1] = 0;
  lamda = 0;
  fprob_LtoR = 0.0;
  fprob_RtoL = 0.0;

  /* read rule from DB for Chinese */
  /* for each pair, [0] store TRAIN, [1] store GIGA */
  lex_rule = get_chi_dpnd_rule(k_ptr->head_ptr->Goi, k_ptr->head_ptr->Pos, u_ptr->head_ptr->Goi, u_ptr->head_ptr->Pos, distance, comma);
  pos_rule_1 = get_chi_dpnd_rule(k_ptr->head_ptr->Goi, k_ptr->head_ptr->Pos, "XX", u_ptr->head_ptr->Pos, distance, comma);
  pos_rule_2 = get_chi_dpnd_rule("XX", k_ptr->head_ptr->Pos, u_ptr->head_ptr->Goi, u_ptr->head_ptr->Pos, distance, comma);
  pos_rule = get_chi_dpnd_rule("XX", k_ptr->head_ptr->Pos, "XX", u_ptr->head_ptr->Pos, distance, comma);

  if (lex_rule != NULL) {
    count = 0;
    rule = NULL;
    rule = strtok(lex_rule, ":");
    while (rule) {
      curRule[count] = malloc(strlen(rule) + 1);
      strcpy(curRule[count], rule);
      count++;
      rule = NULL;
      rule = strtok(NULL, ":");
    }

    for (k = 0; k < count; k++) {
      direction = NULL;
      probLtoR = NULL;
      occurLtoR = NULL;
      probRtoL = NULL;
      occurRtoL = NULL;
      dpnd = NULL;
	
      direction = strtok(curRule[k], "_");
      probLtoR = strtok(NULL, "_");
      occurLtoR = strtok(NULL, "_");
      probRtoL = strtok(NULL, "_");
      occurRtoL = strtok(NULL, "_");
      dpnd = strtok(NULL, "_");
	
      if (!strcmp(dpnd, "TRAIN")) {
	occur_LtoR_1[0] = atof(occurLtoR);
	occur_RtoL_1[0] = atof(occurRtoL);
	prob_LtoR_1[0] = atof(probLtoR);
	prob_RtoL_1[0] = atof(probRtoL);
      }
      else if (!strcmp(dpnd, "GIGA")) {
	occur_LtoR_1[1] = atof(occurLtoR);
	occur_RtoL_1[1] = atof(occurRtoL);
	prob_LtoR_1[1] = atof(probLtoR);
	prob_RtoL_1[1] = atof(probRtoL);
      }
      if (curRule[k]) {
	free(curRule[k]);
	curRule[k] = NULL;
      }
    }
  }

  if (pos_rule_1 != NULL) {
    count = 0;
    rule = NULL;
    rule = strtok(pos_rule_1, ":");
    while (rule) {
      curRule[count] = malloc(strlen(rule) + 1);
      strcpy(curRule[count], rule);
      count++;
      rule = NULL;
      rule = strtok(NULL, ":");
    }

    for (k = 0; k < count; k++) {
      direction = NULL;
      probLtoR = NULL;
      occurLtoR = NULL;
      probRtoL = NULL;
      occurRtoL = NULL;
      dpnd = NULL;
			    
      direction = strtok(curRule[k], "_");
      probLtoR = strtok(NULL, "_");
      occurLtoR = strtok(NULL, "_");
      probRtoL = strtok(NULL, "_");
      occurRtoL = strtok(NULL, "_");
      dpnd = strtok(NULL, "_");
			    
      if (!strcmp(dpnd, "TRAIN")) {
	occur_LtoR_2[0] = atof(occurLtoR);
	occur_RtoL_2[0] = atof(occurRtoL);
	prob_LtoR_2[0] = atof(probLtoR);
	prob_RtoL_2[0] = atof(probRtoL);
      }
      else if (!strcmp(dpnd, "GIGA")) {
	occur_LtoR_2[1] = atof(occurLtoR);
	occur_RtoL_2[1] = atof(occurRtoL);
	prob_LtoR_2[1] = atof(probLtoR);
	prob_RtoL_2[1] = atof(probRtoL);
      }
      if (curRule[k]) {
	free(curRule[k]);
	curRule[k] = NULL;
      }
    }
  }

  if (pos_rule_2 != NULL) {
    count = 0;
    rule = NULL;
    rule = strtok(pos_rule_2, ":");
    while (rule) {
      curRule[count] = malloc(strlen(rule) + 1);
      strcpy(curRule[count], rule);
      count++;
      rule = NULL;
      rule = strtok(NULL, ":");
    }

    for (k = 0; k < count; k++) {
      direction = NULL;
      probLtoR = NULL;
      occurLtoR = NULL;
      probRtoL = NULL;
      occurRtoL = NULL;
      dpnd = NULL;
			    
      direction = strtok(curRule[k], "_");
      probLtoR = strtok(NULL, "_");
      occurLtoR = strtok(NULL, "_");
      probRtoL = strtok(NULL, "_");
      occurRtoL = strtok(NULL, "_");
      dpnd = strtok(NULL, "_");
			    
      if (!strcmp(dpnd, "TRAIN")) {
	occur_LtoR_3[0] = atof(occurLtoR);
	occur_RtoL_3[0] = atof(occurRtoL);
	prob_LtoR_3[0] = atof(probLtoR);
	prob_RtoL_3[0] = atof(probRtoL);
      }
      else if (!strcmp(dpnd, "GIGA")) {
	occur_LtoR_3[1] = atof(occurLtoR);
	occur_RtoL_3[1] = atof(occurRtoL);
	prob_LtoR_3[1] = atof(probLtoR);
	prob_RtoL_3[1] = atof(probRtoL);
      }
      if (curRule[k]) {
	free(curRule[k]);
	curRule[k] = NULL;
      }
    }
  }

  if (pos_rule != NULL) {
    count = 0;
    rule = NULL;
    rule = strtok(pos_rule, ":");
    while (rule) {
      curRule[count] = malloc(strlen(rule) + 1);
      strcpy(curRule[count], rule);
      count++;
      rule = NULL;
      rule = strtok(NULL, ":");
    }

    for (k = 0; k < count; k++) {
      direction = NULL;
      probLtoR = NULL;
      occurLtoR = NULL;
      probRtoL = NULL;
      occurRtoL = NULL;
      dpnd = NULL;
			    
      direction = strtok(curRule[k], "_");
      probLtoR = strtok(NULL, "_");
      occurLtoR = strtok(NULL, "_");
      probRtoL = strtok(NULL, "_");
      occurRtoL = strtok(NULL, "_");
      dpnd = strtok(NULL, "_");

      if (!strcmp(dpnd, "TRAIN")) {
	occur_LtoR_4[0] = atof(occurLtoR);
	occur_RtoL_4[0] = atof(occurRtoL);
	prob_LtoR_4[0] = atof(probLtoR);
	prob_RtoL_4[0] = atof(probRtoL);
      }
      else if (!strcmp(dpnd, "GIGA")) {
	occur_LtoR_4[1] = atof(occurLtoR);
	occur_RtoL_4[1] = atof(occurRtoL);
	prob_LtoR_4[1] = atof(probLtoR);
	prob_RtoL_4[1] = atof(probRtoL);
      }
      if (curRule[k]) {
	free(curRule[k]);
	curRule[k] = NULL;
      }
    }
  }

  for (k = 0; k < 2; k++) {
    lamda = 0.0;
    prob[k] = 0.0;
    /* prob_LtoR */
    if (prob_LtoR_1[k] > DOUBLE_MIN) {
      lamda = prob_LtoR_1[k] / (prob_LtoR_1[k] + 1);
      if (prob_LtoR_2[k] > DOUBLE_MIN || prob_LtoR_3[k] > DOUBLE_MIN) {
	prob[k] = (lamda * prob_LtoR_1[k] / occur_LtoR_1[k]) +
	  ((1 - lamda) * (prob_LtoR_2[k] + prob_LtoR_3[k]) / (occur_LtoR_2[k] + occur_LtoR_3[k]));
      }
      else {
	prob[k] = lamda * prob_LtoR_1[k] / occur_LtoR_1[k];
      }
    }
    else if (prob_LtoR_2[k] > DOUBLE_MIN || prob_LtoR_3[k] > DOUBLE_MIN) {
      lamda = (prob_LtoR_2[k] + prob_LtoR_3[k]) /
	(prob_LtoR_2[k] + prob_LtoR_3[k] + 1);

      if (prob_LtoR_4[k] > DOUBLE_MIN) {
	prob[k] = prob_bk_weight_1 * ((lamda) * (prob_LtoR_2[k] + prob_LtoR_3[k]) / (occur_LtoR_2[k] + occur_LtoR_3[k]) + 
				      ((1 - lamda) * (prob_LtoR_4[k] / occur_LtoR_4[k])));
      }
      else {
	prob[k] = prob_bk_weight_1 * ((lamda) * (prob_LtoR_2[k] + prob_LtoR_3[k]) / (occur_LtoR_2[k] + occur_LtoR_3[k]));
      }
    }
    else if (prob_LtoR_4[k] > DOUBLE_MIN) {
      prob[k] = prob_bk_weight_2 * prob_LtoR_4[k] / occur_LtoR_4[k];
    }
  }

  if (prob[0] > DOUBLE_MIN) {
    fprob_LtoR = prob[0] * giga_weight + prob[1] * (1 - giga_weight);
  }
  else {
    fprob_LtoR = prob[1];
  }

  for (k = 0; k < 2; k++) {
    lamda = 0.0;
    prob[k] = 0.0;
    /* prob_RtoL */
    if (prob_RtoL_1[k] > DOUBLE_MIN) {
      lamda = prob_RtoL_1[k] / (prob_RtoL_1[k] + 1);
      if (prob_RtoL_2[k] > DOUBLE_MIN || prob_RtoL_3[k] > DOUBLE_MIN) {
	prob[k] = (lamda * prob_RtoL_1[k] / occur_RtoL_1[k]) +
	  ((1 - lamda) * (prob_RtoL_2[k] + prob_RtoL_3[k]) / (occur_RtoL_2[k] + occur_RtoL_3[k]));
      }
      else {
	prob[k] = lamda * prob_RtoL_1[k] / occur_RtoL_1[k];
      }
    }
    else if (prob_RtoL_2[k] > DOUBLE_MIN || prob_RtoL_3[k] > DOUBLE_MIN) {
      lamda = (prob_RtoL_2[k] + prob_RtoL_3[k]) / 
	(prob_RtoL_2[k] + prob_RtoL_3[k] + 1);

      if (prob_RtoL_4[k] > DOUBLE_MIN) {
	prob[k] = prob_bk_weight_1 * ((lamda) * (prob_RtoL_2[k] + prob_RtoL_3[k]) / (occur_RtoL_2[k] + occur_RtoL_3[k]) + 
				      ((1 - lamda) * (prob_RtoL_4[k] / occur_RtoL_4[k])));
      }
      else {
	prob[k] = prob_bk_weight_1 * ((lamda) * (prob_RtoL_2[k] + prob_RtoL_3[k]) / (occur_RtoL_2[k] + occur_RtoL_3[k]));
      }
    }
    else if (prob_RtoL_4[k] > DOUBLE_MIN) {
      prob[k] = prob_bk_weight_2 * prob_RtoL_4[k] / occur_RtoL_4[k];
    }
  }

  if (prob[0] > DOUBLE_MIN) { 
    fprob_RtoL = prob[0] * giga_weight + prob[1] * (1 - giga_weight);
  }
  else {
    fprob_RtoL = prob[1];
  }

  /* free memory */
  if (pos_rule) {
    free(pos_rule);
    pos_rule = NULL;
  }
  if (lex_rule) {
    free(lex_rule);
    lex_rule = NULL;
  }
  if (pos_rule_1) {
    free(pos_rule_1);
    pos_rule_1 = NULL;
  }
  if (pos_rule_2) {
    free(pos_rule_2);
    pos_rule_2 = NULL;
  }
}

/* calculate dpnd and dis_comma probability for model with pos-tagging */
/*==================================================================*/
void get_prob_wpos(SENTENCE_DATA *sp, int left, int right, int distance, int comma, char *pos_left, char *pos_right)
     /*==================================================================*/
{
  int i, j, k;
  char *pos_rule;
  char *probLtoR, *probRtoL, *occurLtoR, *occurRtoL, *dpnd, *direction, *rule;
  char *curRule[CHI_DPND_TYPE_MAX];
  int count;
  double prob[2];
  BNST_DATA *k_ptr, *u_ptr;
    
  double      prob_LtoR[2];
  double      occur_LtoR[2];
  double      prob_RtoL[2];
  double      occur_RtoL[2];
  double      lamda; /* parameter of each dpnd type */

  /* initialization */
  k_ptr = sp->bnst_data + left;
  u_ptr = sp->bnst_data + right;

  pos_rule = NULL;

  prob_LtoR[0] = 0;
  prob_LtoR[1] = 0;
  occur_LtoR[0] = 0;
  occur_LtoR[1] = 0;
  prob_RtoL[0] = 0;
  prob_RtoL[1] = 0;
  occur_RtoL[0] = 0;
  occur_RtoL[1] = 0;
  lamda = 0;
  fprob_LtoR = 0.0;
  fprob_RtoL = 0.0;

  /* read rule from DB for Chinese */
  /* for each pair, [0] store TRAIN, [1] store GIGA */
  pos_rule = get_chi_dpnd_rule("XX", pos_left, "XX", pos_right, distance, comma);

  if (pos_rule != NULL) {
    count = 0;
    rule = NULL;
    rule = strtok(pos_rule, ":");
    while (rule) {
      curRule[count] = malloc(strlen(rule) + 1);
      strcpy(curRule[count], rule);
      count++;
      rule = NULL;
      rule = strtok(NULL, ":");
    }

    for (k = 0; k < count; k++) {
      direction = NULL;
      probLtoR = NULL;
      occurLtoR = NULL;
      probRtoL = NULL;
      occurRtoL = NULL;
      dpnd = NULL;
			    
      direction = strtok(curRule[k], "_");
      probLtoR = strtok(NULL, "_");
      occurLtoR = strtok(NULL, "_");
      probRtoL = strtok(NULL, "_");
      occurRtoL = strtok(NULL, "_");
      dpnd = strtok(NULL, "_");

      if (!strcmp(dpnd, "TRAIN")) {
	occur_LtoR[0] = atof(occurLtoR);
	occur_RtoL[0] = atof(occurRtoL);
	prob_LtoR[0] = atof(probLtoR);
	prob_RtoL[0] = atof(probRtoL);
      }
      else if (!strcmp(dpnd, "GIGA")) {
	occur_LtoR[1] = atof(occurLtoR);
	occur_RtoL[1] = atof(occurRtoL);
	prob_LtoR[1] = atof(probLtoR);
	prob_RtoL[1] = atof(probRtoL);
      }
      if (curRule[k]) {
	free(curRule[k]);
	curRule[k] = NULL;
      }
    }

    for (k = 0; k < 2; k++) {
      lamda = 0.0;
      prob[k] = 0.0;
      /* prob_LtoR */
      if (prob_LtoR[k] > DOUBLE_MIN) {
	prob[k] = prob_bk_weight_2 * prob_LtoR[k] / occur_LtoR[k];
      }
    }

    if (prob[0] > DOUBLE_MIN) {
      fprob_LtoR = prob[0] * giga_weight + prob[1] * (1 - giga_weight);
    }
    else {
      fprob_LtoR = prob[1];
    }

    for (k = 0; k < 2; k++) {
      lamda = 0.0;
      prob[k] = 0.0;
      /* prob_RtoL */
      if (prob_RtoL[k] > DOUBLE_MIN) {
	prob[k] = prob_bk_weight_2 * prob_RtoL[k] / occur_RtoL[k];
      }
    }

    if (prob[0] > DOUBLE_MIN) { 
      fprob_RtoL = prob[0] * giga_weight + prob[1] * (1 - giga_weight);
    }
    else {
      fprob_RtoL = prob[1];
    }
  }
  /* free memory */
  if (pos_rule) {
    free(pos_rule);
    pos_rule = NULL;
  }
}

/* calculate dpnd and dis_comma probability for model with pos-tagging */
/*==================================================================*/
void get_lex_prob_wpos(SENTENCE_DATA *sp, int left, int right, int distance, int comma, char *pos_left, char *pos_right)
/*==================================================================*/
{
  int i, j, k;
  char *lex_rule;
  char *probLtoR, *probRtoL, *occurLtoR, *occurRtoL, *dpnd, *direction, *rule;
  char *curRule[CHI_DPND_TYPE_MAX];
  int count;
  double prob[2];
  BNST_DATA *k_ptr, *u_ptr;
    
  double      prob_LtoR[2];
  double      occur_LtoR[2];
  double      prob_RtoL[2];
  double      occur_RtoL[2];
  double      lamda; /* parameter of each dpnd type */

  /* initialization */
  k_ptr = sp->bnst_data + left;
  u_ptr = sp->bnst_data + right;

  lex_rule = NULL;

  prob_LtoR[0] = 0;
  prob_LtoR[1] = 0;
  occur_LtoR[0] = 0;
  occur_LtoR[1] = 0;
  prob_RtoL[0] = 0;
  prob_RtoL[1] = 0;
  occur_RtoL[0] = 0;
  occur_RtoL[1] = 0;
  lamda = 0;
  fprob_LtoR = 0.0;
  fprob_RtoL = 0.0;

  /* read rule from DB for Chinese */
  /* for each pair, [0] store TRAIN, [1] store GIGA */
  lex_rule = get_chi_dpnd_rule(k_ptr->head_ptr->Goi, pos_left, k_ptr->head_ptr->Goi, pos_right, distance, comma);

  if (lex_rule != NULL) {
    count = 0;
    rule = NULL;
    rule = strtok(lex_rule, ":");
    while (rule) {
      curRule[count] = malloc(strlen(rule) + 1);
      strcpy(curRule[count], rule);
      count++;
      rule = NULL;
      rule = strtok(NULL, ":");
    }

    for (k = 0; k < count; k++) {
      direction = NULL;
      probLtoR = NULL;
      occurLtoR = NULL;
      probRtoL = NULL;
      occurRtoL = NULL;
      dpnd = NULL;
			    
      direction = strtok(curRule[k], "_");
      probLtoR = strtok(NULL, "_");
      occurLtoR = strtok(NULL, "_");
      probRtoL = strtok(NULL, "_");
      occurRtoL = strtok(NULL, "_");
      dpnd = strtok(NULL, "_");

      if (!strcmp(dpnd, "TRAIN")) {
	occur_LtoR[0] = atof(occurLtoR);
	occur_RtoL[0] = atof(occurRtoL);
	prob_LtoR[0] = atof(probLtoR);
	prob_RtoL[0] = atof(probRtoL);
      }
      else if (!strcmp(dpnd, "GIGA")) {
	occur_LtoR[1] = atof(occurLtoR);
	occur_RtoL[1] = atof(occurRtoL);
	prob_LtoR[1] = atof(probLtoR);
	prob_RtoL[1] = atof(probRtoL);
      }
      if (curRule[k]) {
	free(curRule[k]);
	curRule[k] = NULL;
      }
    }
  }

  for (k = 0; k < 2; k++) {
    lamda = 0.0;
    prob[k] = 0.0;
    /* prob_LtoR */
    if (prob_LtoR[k] > DOUBLE_MIN) {
      prob[k] = prob_bk_weight_2 * prob_LtoR[k] / occur_LtoR[k];
    }
  }

  if (prob[0] > DOUBLE_MIN) {
    fprob_LtoR = prob[0] * giga_weight + prob[1] * (1 - giga_weight);
  }
  else {
    fprob_LtoR = prob[1];
  }

  for (k = 0; k < 2; k++) {
    lamda = 0.0;
    prob[k] = 0.0;
    /* prob_RtoL */
    if (prob_RtoL[k] > DOUBLE_MIN) {
      prob[k] = prob_bk_weight_2 * prob_RtoL[k] / occur_RtoL[k];
    }
  }

  if (prob[0] > DOUBLE_MIN) { 
    fprob_RtoL = prob[0] * giga_weight + prob[1] * (1 - giga_weight);
  }
  else {
    fprob_RtoL = prob[1];
  }

  /* free memory */
  if (lex_rule) {
    free(lex_rule);
    lex_rule = NULL;
  }
}

/* calculate root probability */
/*==================================================================*/
double get_root_prob(SENTENCE_DATA *sp, int root)
/*==================================================================*/
{
  int i, j, k;
  char *lex_rule, *pos_rule;
  char *probLtoR, *probRtoL, *occurLtoR, *occurRtoL, *dpnd, *direction, *rule;
  char *curRule[CHI_DPND_TYPE_MAX];
  int count;
  double prob[2], root_prob;
  double lex_root_prob[2], pos_root_prob[2], lex_root_occur[2], pos_root_occur[2], lamda_root;
  BNST_DATA *k_ptr = sp->bnst_data + root;

  /* get rule for root */
  /* initialization */
  lex_rule = NULL;
  pos_rule = NULL;
  lex_root_prob[0] = 0;
  lex_root_prob[1] = 0;
  lex_root_occur[0] = 0;
  lex_root_occur[1] = 0;
  pos_root_prob[0] = 0;
  pos_root_prob[1] = 0;
  pos_root_occur[0] = 0;
  pos_root_occur[1] = 0;
  prob[0] = 0.0;
  prob[1] = 0.0;
	
  /* get root rule */
  lex_rule = get_chi_dpnd_rule(k_ptr->head_ptr->Goi, k_ptr->head_ptr->Pos, "ROOT", "", 0, 0);
  pos_rule = get_chi_dpnd_rule("XX", k_ptr->head_ptr->Pos, "ROOT", "", 0, 0);

  if (lex_rule != NULL) {
    count = 0;
    rule = NULL;
    rule = strtok(lex_rule, ":");
    while (rule) {
      curRule[count] = malloc(strlen(rule) + 1);
      strcpy(curRule[count], rule);
      count++;
      rule = NULL;
      rule = strtok(NULL, ":");
    }

    for (k = 0; k < count; k++) {
      probLtoR = NULL;
      occurLtoR = NULL;
      dpnd = NULL;
			    
      probLtoR = strtok(curRule[k], "_");
      occurLtoR = strtok(NULL, "_");
      dpnd = strtok(NULL, "_");

      if (!strcmp(dpnd, "TRAIN")) {
	lex_root_prob[0] = atof(probLtoR);
	lex_root_occur[0] = atof(occurLtoR);
      }
      else if (!strcmp(dpnd, "GIGA")) {
	lex_root_prob[1] = atof(probLtoR);
	lex_root_occur[1] = atof(occurLtoR);
      }
      if (curRule[k]) {
	free(curRule[k]);
	curRule[k] = NULL;
      }
    }
  }

  if (pos_rule != NULL) {
    count = 0;
    rule = NULL;
    rule = strtok(pos_rule, ":");
    while (rule) {
      curRule[count] = malloc(strlen(rule) + 1);
      strcpy(curRule[count], rule);
      count++;
      rule = NULL;
      rule = strtok(NULL, ":");
    }

    for (k = 0; k < count; k++) {
      probLtoR = NULL;
      occurLtoR = NULL;
      dpnd = NULL;
			    
      probLtoR = strtok(curRule[k], "_");
      occurLtoR = strtok(NULL, "_");
      dpnd = strtok(NULL, "_");

      if (!strcmp(dpnd, "TRAIN")) {
	pos_root_prob[0] = atof(probLtoR);
	pos_root_occur[0] = atof(occurLtoR);
      }
      else if (!strcmp(dpnd, "GIGA")) {
	pos_root_prob[1] = atof(probLtoR);
	pos_root_occur[1] = atof(occurLtoR);
      }
      if (curRule[k]) {
	free(curRule[k]);
	curRule[k] = NULL;
      }
    }
  }

  /* calculate root prob */
  lamda_root = 0.5;
  for (k = 0; k < 1; k++) {
    if (lex_root_prob[k] > DOUBLE_MIN && pos_root_prob[k] > DOUBLE_MIN) {
      prob[k] = (lamda_root * lex_root_prob[k] / lex_root_occur[k]) + ((1 - lamda_root) * (pos_root_prob[k] / pos_root_occur[k]));
    }
    else if (pos_root_prob[k] > DOUBLE_MIN) {
      prob[k] = (1 - lamda_root) * (pos_root_prob[k] / pos_root_occur[k]);
    }
    else {
      prob[k] = 0;
    }
  }

  root_prob = prob[0];

  /* free memory */
  if (lex_rule) {
    free(lex_rule);
    lex_rule = NULL;
  }
  if (pos_rule) {
    free(pos_rule);
    pos_rule = NULL;
  }

  return root_prob;
}

/* calculate root probability for model with pos-tagging */
/*==================================================================*/
double get_root_prob_wpos(SENTENCE_DATA *sp, int root, char *root_pos)
     /*==================================================================*/
{
  int i, j, k;
  char *pos_rule;
  char *probLtoR, *probRtoL, *occurLtoR, *occurRtoL, *dpnd, *direction, *rule;
  char *curRule[CHI_DPND_TYPE_MAX];
  int count;
  double prob[2], root_prob;
  double pos_root_prob[2], pos_root_occur[2];

  /* get rule for root */
  /* initialization */
  pos_rule = NULL;
  pos_root_prob[0] = 0;
  pos_root_prob[1] = 0;
  pos_root_occur[0] = 0;
  pos_root_occur[1] = 0;
  prob[0] = 0.0;
  prob[1] = 0.0;
	
  /* get root rule */
  pos_rule = get_chi_dpnd_rule("XX", root_pos, "ROOT", "", 0, 0);

  if (pos_rule != NULL) {
    count = 0;
    rule = NULL;
    rule = strtok(pos_rule, ":");
    while (rule) {
      curRule[count] = malloc(strlen(rule) + 1);
      strcpy(curRule[count], rule);
      count++;
      rule = NULL;
      rule = strtok(NULL, ":");
    }

    for (k = 0; k < count; k++) {
      probLtoR = NULL;
      occurLtoR = NULL;
      dpnd = NULL;
			    
      probLtoR = strtok(curRule[k], "_");
      occurLtoR = strtok(NULL, "_");
      dpnd = strtok(NULL, "_");

      if (!strcmp(dpnd, "TRAIN")) {
	pos_root_prob[0] = atof(probLtoR);
	pos_root_occur[0] = atof(occurLtoR);
      }
      else if (!strcmp(dpnd, "GIGA")) {
	pos_root_prob[1] = atof(probLtoR);
	pos_root_occur[1] = atof(occurLtoR);
      }
      if (curRule[k]) {
	free(curRule[k]);
	curRule[k] = NULL;
      }
    }
  }

  /* calculate root prob */
  for (k = 0; k < 1; k++) {
    if (pos_root_prob[k] > DOUBLE_MIN) {
      prob[k] = pos_root_prob[k] / pos_root_occur[k];
    }
    else {
      prob[k] = 0;
    }
  }

  root_prob = prob[0];

  /* free memory */
  if (pos_rule) {
    free(pos_rule);
    pos_rule = NULL;
  }

  return root_prob;
}

/* calculate dpnd probability & root probability for Chinese probabilistic model */
/*==================================================================*/
    void calc_chi_dpnd_matrix_forProbModel(SENTENCE_DATA *sp)
/*==================================================================*/
{
  int i, j, k;
  int comma, distance;
  double total_word_prob;
  double total_LtoR, total_RtoL;

  for (i = 0; i < sp->Bnst_num; i++) {
    /* get root prob */
    Chi_root_prob_matrix[i].prob[0] = get_root_prob(sp, i);
    /* get dpnd rule for word pair */
    for (j = i + 1; j < sp->Bnst_num; j++) {
      /* get comma and distance */
      comma = 0;
      for (k = i + 1; k < j; k++) {
	if (check_feature((sp->bnst_data+k)->f, "PU") &&
	    (!strcmp((sp->bnst_data+k)->head_ptr->Goi, ",") ||
	     !strcmp((sp->bnst_data+k)->head_ptr->Goi, "：") ||
	     !strcmp((sp->bnst_data+k)->head_ptr->Goi, ":") ||
	     !strcmp((sp->bnst_data+k)->head_ptr->Goi, "；") ||
	     !strcmp((sp->bnst_data+k)->head_ptr->Goi, "，"))) {
	  comma = 1;
	  break;
	}
      }
      if (j == i + 1) {
	distance = 1;
      }
      else {
	distance = 2;
      }

      /* get dis_comma prob */
      fprob_LtoR = 0.0;
      fprob_RtoL = 0.0;
      get_prob(sp, i, j, distance, comma);
      Chi_dpnd_matrix[i][j].prob_dis_comma_LtoR[0] = fprob_LtoR;
      Chi_dpnd_matrix[i][j].prob_dis_comma_RtoL[0] = fprob_RtoL;

      total_LtoR = 0.0;
      total_RtoL = 0.0;

      /* get dpnd prob */
      fprob_LtoR = 0.0;
      fprob_RtoL = 0.0;
      get_prob(sp, i, j, 0, 0);
      Chi_dpnd_matrix[i][j].prob_LtoR[0] = fprob_LtoR;
      Chi_dpnd_matrix[i][j].prob_RtoL[0] = fprob_RtoL;

      /* direction */
      Chi_dpnd_matrix[i][j].direction[0] = 0;
      Chi_dpnd_matrix[i][j].count = 0;
      if (Chi_dpnd_matrix[i][j].prob_LtoR[0] > DOUBLE_MIN && Chi_dpnd_matrix[i][j].prob_RtoL[0] > DOUBLE_MIN &&
	  Chi_dpnd_matrix[i][j].prob_dis_comma_LtoR[0] > DOUBLE_MIN && Chi_dpnd_matrix[i][j].prob_dis_comma_RtoL[0] > DOUBLE_MIN) {
	Chi_dpnd_matrix[i][j].direction[0] = 'B';
	Chi_dpnd_matrix[i][j].count = 1;
      }
      else if (Chi_dpnd_matrix[i][j].prob_LtoR[0] > DOUBLE_MIN &&
	       Chi_dpnd_matrix[i][j].prob_dis_comma_LtoR[0] > DOUBLE_MIN) {
	Chi_dpnd_matrix[i][j].direction[0] = 'R';
	Chi_dpnd_matrix[i][j].count = 1;
      }
      else if (Chi_dpnd_matrix[i][j].prob_RtoL[0] > DOUBLE_MIN &&
	       Chi_dpnd_matrix[i][j].prob_dis_comma_RtoL[0] > DOUBLE_MIN) {
	Chi_dpnd_matrix[i][j].direction[0] = 'L';
	Chi_dpnd_matrix[i][j].count = 1;
      }
		
      if (Chi_dpnd_matrix[i][j].count > 0) {
	Dpnd_matrix[i][j] = 'O';
      }
    }
  }

  /* normalize prob_dpnd */
  /* p(wi|wr) = F(wi|wr)/(+F(wj|wr)) */
  for (i = 0; i < sp->Bnst_num; i++) {
    total_word_prob = 0.0;
    for (j = 0; j < sp->Bnst_num; j++) {
      if (j == i) {
	continue;
      }
      else if (j < i) {
	if (Chi_dpnd_matrix[j][i].prob_LtoR[0] > DOUBLE_MIN) {
	  total_word_prob += Chi_dpnd_matrix[j][i].prob_LtoR[0];
	}
      }
      else if (j > i) {
	if (Chi_dpnd_matrix[i][j].prob_RtoL[0] > DOUBLE_MIN) {
	  total_word_prob += Chi_dpnd_matrix[i][j].prob_RtoL[0];
	}
      }
    }
    if (total_word_prob < DOUBLE_MIN) {
      continue;
    }
    for (j = 0; j < sp->Bnst_num; j++) {
      if (j == i) {
	continue;
      }
      else if (j < i) {
	Chi_dpnd_matrix[j][i].prob_LtoR[0] /= total_word_prob;
      }
      else if (j > i) {
	Chi_dpnd_matrix[i][j].prob_RtoL[0] /= total_word_prob;
      }
    }
  }
}

/* calculate dpnd probability & root probability & pos probability for Chinese probabilistic model with pos-tagging */
/*==================================================================*/
void calc_chi_dpnd_matrix_wpos(SENTENCE_DATA *sp)
     /*==================================================================*/
{
  int i, j, k, m, n;
  int comma, distance;
  double total_word_prob;
  double total_LtoR, total_RtoL;

  for (i = 0; i < sp->Bnst_num; i++) {
    Chi_dpnd_matrix[i][i].count = 0;
    for (m = 0; m < CHI_POS_NBEST && m < Chi_pos_matrix[i].pos_max; m++) {
      Chi_dpnd_matrix[i][i].left_pos_index[Chi_dpnd_matrix[i][i].count] = Chi_pos_matrix[i].pos_index[m];
      Chi_dpnd_matrix[i][i].right_pos_index[Chi_dpnd_matrix[i][i].count] = Chi_pos_matrix[i].pos_index[m];
      Chi_dpnd_matrix[i][i].count++;
    }
    /* get dpnd rule for word pair */
    for (j = i + 1; j < sp->Bnst_num; j++) {
      Chi_dpnd_matrix[i][j].count = 0;
      /* get comma and distance */
      comma = 0;
      for (k = i + 1; k < j; k++) {
	if ((!strcmp((sp->bnst_data+k)->head_ptr->Goi, ",") ||
	     !strcmp((sp->bnst_data+k)->head_ptr->Goi, "：") ||
	     !strcmp((sp->bnst_data+k)->head_ptr->Goi, ":") ||
	     !strcmp((sp->bnst_data+k)->head_ptr->Goi, "；") ||
	     !strcmp((sp->bnst_data+k)->head_ptr->Goi, "，"))) {
	  comma = 1;
	  break;
	}
      }
      if (j == i + 1) {
	distance = 1;
      }
      else {
	distance = 2;
      }

      for (m = 0; m < CHI_POS_NBEST && m < Chi_pos_matrix[i].pos_max; m++) {
	/* get root prob */
	Chi_root_prob_matrix[i].prob[m] = get_root_prob_wpos(sp, i, Chi_pos_matrix[i].pos[m]);
	Chi_root_prob_matrix[i].pos_index[m] = m;

	for (n = 0; n < CHI_POS_NBEST && n < Chi_pos_matrix[j].pos_max; n++) {
	  /* get dpnd prob */
	  fprob_LtoR = 0.0;
	  fprob_RtoL = 0.0;
	  get_prob_wpos(sp, i, j, 0, 0, Chi_pos_matrix[i].pos[m], Chi_pos_matrix[j].pos[n]);
	  //	  get_lex_prob_wpos(sp, i, j, distance, comma, Chi_pos_matrix[i].pos[m], Chi_pos_matrix[j].pos[n]);
	  if (fprob_LtoR > DOUBLE_MIN || fprob_RtoL > DOUBLE_MIN) {
	    Chi_dpnd_matrix[i][j].prob_LtoR[Chi_dpnd_matrix[i][j].count] = fprob_LtoR;
	    Chi_dpnd_matrix[i][j].prob_RtoL[Chi_dpnd_matrix[i][j].count] = fprob_RtoL;

	    total_LtoR = 0.0;
	    total_RtoL = 0.0;

	    /* get dis_comma prob */
	    fprob_LtoR = 0.0;
	    fprob_RtoL = 0.0;
	    get_prob_wpos(sp, i, j, distance, comma, Chi_pos_matrix[i].pos[m], Chi_pos_matrix[j].pos[n]);
	    //	    get_lex_prob_wpos(sp, i, j, 0, 0, Chi_pos_matrix[i].pos[m], Chi_pos_matrix[j].pos[n]);
	    Chi_dpnd_matrix[i][j].prob_dis_comma_LtoR[Chi_dpnd_matrix[i][j].count] = fprob_LtoR;
	    Chi_dpnd_matrix[i][j].prob_dis_comma_RtoL[Chi_dpnd_matrix[i][j].count] = fprob_RtoL;

	    total_LtoR = 0.0;
	    total_RtoL = 0.0;

/* 	    /\* get lex_dis_comma prob *\/ */
/* 	    fprob_LtoR = 0.0; */
/* 	    fprob_RtoL = 0.0; */
/* 	    get_lex_prob_wpos(sp, i, j, distance, comma, Chi_pos_matrix[i].pos[m], Chi_pos_matrix[j].pos[n]); */
/* 	    Chi_dpnd_matrix[i][j].lex_prob_dis_comma_LtoR[Chi_dpnd_matrix[i][j].count] = fprob_LtoR; */
/* 	    Chi_dpnd_matrix[i][j].lex_prob_dis_comma_RtoL[Chi_dpnd_matrix[i][j].count] = fprob_RtoL; */

/* 	    total_LtoR = 0.0; */
/* 	    total_RtoL = 0.0; */

/* 	    /\* get dpnd prob *\/ */
/* 	    fprob_LtoR = 0.0; */
/* 	    fprob_RtoL = 0.0; */
/* 	    get_lex_prob_wpos(sp, i, j, 0, 0, Chi_pos_matrix[i].pos[m], Chi_pos_matrix[j].pos[n]); */
/* 	    Chi_dpnd_matrix[i][j].lex_prob_LtoR[Chi_dpnd_matrix[i][j].count] = fprob_LtoR; */
/* 	    Chi_dpnd_matrix[i][j].lex_prob_RtoL[Chi_dpnd_matrix[i][j].count] = fprob_RtoL; */

	    Chi_dpnd_matrix[i][j].left_pos_index[Chi_dpnd_matrix[i][j].count] = Chi_pos_matrix[i].pos_index[m];
	    Chi_dpnd_matrix[i][j].right_pos_index[Chi_dpnd_matrix[i][j].count] = Chi_pos_matrix[j].pos_index[n];

	    /* direction */
	    if (Chi_dpnd_matrix[i][j].prob_LtoR[Chi_dpnd_matrix[i][j].count] > DOUBLE_MIN && Chi_dpnd_matrix[i][j].prob_RtoL[Chi_dpnd_matrix[i][j].count] > DOUBLE_MIN &&
		Chi_dpnd_matrix[i][j].prob_dis_comma_LtoR[Chi_dpnd_matrix[i][j].count] > DOUBLE_MIN && Chi_dpnd_matrix[i][j].prob_dis_comma_RtoL[Chi_dpnd_matrix[i][j].count] > DOUBLE_MIN) {
	      Chi_dpnd_matrix[i][j].direction[Chi_dpnd_matrix[i][j].count] = 'B';
	    }
	    else if (Chi_dpnd_matrix[i][j].prob_LtoR[Chi_dpnd_matrix[i][j].count] > DOUBLE_MIN &&
		     Chi_dpnd_matrix[i][j].prob_dis_comma_LtoR[Chi_dpnd_matrix[i][j].count] > DOUBLE_MIN) {
	      Chi_dpnd_matrix[i][j].direction[Chi_dpnd_matrix[i][j].count] = 'R';
	    }
	    else if (Chi_dpnd_matrix[i][j].prob_RtoL[Chi_dpnd_matrix[i][j].count] > DOUBLE_MIN &&
		     Chi_dpnd_matrix[i][j].prob_dis_comma_RtoL[Chi_dpnd_matrix[i][j].count] > DOUBLE_MIN) {
	      Chi_dpnd_matrix[i][j].direction[Chi_dpnd_matrix[i][j].count] = 'L';
	    }

	    Chi_dpnd_matrix[i][j].count++;
		
	    if (Chi_dpnd_matrix[i][j].count > 0) {
	      Dpnd_matrix[i][j] = 'O';
	    }
	  }
	}
      }
    }
  }
}

/* get pos rule for Chinese */
/*==================================================================*/
char* get_chi_pos_rule(char *word, char *pos)
/*==================================================================*/
{
  char *key;
  
  if (OptChiPos && CHIPosExist == FALSE) {
    return NULL;
  }
  
  key = malloc_db_buf(strlen(word) + strlen(pos) + 2);
  sprintf(key, "%s_%s", pos, word);

  return db_get(chi_pos_db, key);
}

/* calculate pos probability for Chinese probabilistic model */
/*==================================================================*/
void calc_chi_pos_matrix(SENTENCE_DATA *sp)
/*==================================================================*/
{
  int i, j, k, l;
  char *pos_rule;
  char *prob, *occur, *dpnd, *rule;
  char *curRule[CHI_DPND_TYPE_MAX];
  int count;
  double pos_prob[2];
  double fprob;
    
  double      tmp_prob[2];
  double      tmp_occur[2];

  for (i = 0; i < sp->Bnst_num; i++) {
    /* initialization */
    for (j = 0; j < CHI_POS_MAX; j++) {
      Chi_pos_matrix[i].prob[j] = 0.0;
      Chi_pos_matrix[i].prob_pos_index[j] = 0.0;
      Chi_pos_matrix[i].pos_index[j] = -1;
      Chi_pos_matrix[i].pos[j] = (char *)malloc(sizeof(char) * (CHI_POS_LEN_MAX + 1));
    }
    Chi_pos_matrix[i].pos_max = 0;

    for (j = 0; j < CHI_POS_MAX; j++) {
      /* initialization */
      pos_rule = NULL;

      tmp_prob[0] = 0;
      tmp_prob[1] = 0;
      tmp_occur[0] = 0;
      tmp_occur[1] = 0;

      /* read rule from DB for Chinese */
      /* for each pair, [0] store TRAIN, [1] store GIGA */
      pos_rule = get_chi_pos_rule((sp->bnst_data + i)->head_ptr->Goi, Chi_word_pos[j]);

      if (pos_rule != NULL) {
	count = 0;
	rule = NULL;
	rule = strtok(pos_rule, ":");
	while (rule) {
	  curRule[count] = malloc(strlen(rule) + 1);
	  strcpy(curRule[count], rule);
	  count++;
	  rule = NULL;
	  rule = strtok(NULL, ":");
	}

	for (k = 0; k < count; k++) {
	  prob = NULL;
	  occur = NULL;
	  dpnd = NULL;
			    
	  prob = strtok(curRule[k], "_");
	  occur = strtok(NULL, "_");
	  dpnd = strtok(NULL, "_");

	  if (!strcmp(dpnd, "TRAIN")) {
	    tmp_occur[0] = atof(occur);
	    tmp_prob[0] = atof(prob);
	  }
	  else if (!strcmp(dpnd, "GIGA")) {
	    tmp_occur[0] = atof(occur);
	    tmp_prob[0] = atof(prob);
	  }
	  if (curRule[k]) {
	    free(curRule[k]);
	    curRule[k] = NULL;
	  }
	}

	for (k = 0; k < 2; k++) {
	  pos_prob[k] = 0.0;
	  if (tmp_prob[k] > DOUBLE_MIN) {
	    pos_prob[k] = tmp_prob[k] / tmp_occur[k];
	  }
	}

	if (pos_prob[0] > DOUBLE_MIN) {
	  fprob = pos_prob[0] * giga_weight + pos_prob[1] * (1 - giga_weight);
	}
	else {
	  fprob = pos_prob[1];
	}
	
	Chi_pos_matrix[i].prob_pos_index[j] = fprob;

	if (Chi_pos_matrix[i].pos_max == 0) {
	    Chi_pos_matrix[i].prob[0] = fprob;
	    Chi_pos_matrix[i].pos_index[0] = j;
	    strcpy(Chi_pos_matrix[i].pos[0], Chi_word_pos[j]);
	    Chi_pos_matrix[i].pos_max++;
	}
	else {
	  for (k = 0; k < Chi_pos_matrix[i].pos_max; k++) {
	    if (fprob > Chi_pos_matrix[i].prob[k]) {
	      for (l = Chi_pos_matrix[i].pos_max; l > k; l--) {
		Chi_pos_matrix[i].prob[l] = Chi_pos_matrix[i].prob[l - 1];
		Chi_pos_matrix[i].pos_index[l] = Chi_pos_matrix[i].pos_index[l - 1];
		strcpy(Chi_pos_matrix[i].pos[l], Chi_pos_matrix[i].pos[l - 1]);
	      }
	      Chi_pos_matrix[i].prob[k] = fprob;
	      Chi_pos_matrix[i].pos_index[k] = j;
	      strcpy(Chi_pos_matrix[i].pos[k], Chi_word_pos[j]);
	      Chi_pos_matrix[i].pos_max++;
	      break;
	    }
	  }
	}
      }

      /* free memory */
      if (pos_rule) {
	free(pos_rule);
	pos_rule = NULL;
      }
    }
  }
}

/*==================================================================*/
           int relax_dpnd_matrix(SENTENCE_DATA *sp)
/*==================================================================*/
{
  /* 係り先がない場合の緩和

  括弧によるマスクは優先し，その制限内で末尾に係れるように変更

  ○ Ａ‥‥「‥‥‥‥‥」‥‥Ｂ (文末)
  ○ ‥‥‥「Ａ‥‥‥Ｂ」‥‥‥ (括弧終)
  ○ ‥‥‥「Ａ‥Ｂ．‥」‥‥‥ (係:文末)
  × Ａ‥‥‥‥Ｂ「‥‥‥‥Ｃ」 (Ｂに係り得るとはしない．
  Ｃとの関係は解析で対処)
  */

  int i, j, ok_flag, relax_flag, last_possibility;
  
  relax_flag = FALSE;

  for (i = 0; i < sp->Bnst_num - 1  ; i++) {
    ok_flag = FALSE;
    last_possibility = i;
    for (j = i + 1; j < sp->Bnst_num ; j++) {
      if (Quote_matrix[i][j]) {
	if (Dpnd_matrix[i][j] > 0) {
	  ok_flag = TRUE;
	  break;
	} else if (check_feature(sp->bnst_data[j].f, "係:文末")) {
	  last_possibility = j;
	  break;
	} else {
	  last_possibility = j;
	}
      }
    }

    if (ok_flag == FALSE) {
      if (check_feature(sp->bnst_data[last_possibility].f, "文末") ||
	  check_feature(sp->bnst_data[last_possibility].f, "係:文末") ||
	  check_feature(sp->bnst_data[last_possibility].f, "括弧終")) {
	Dpnd_matrix[i][last_possibility] = 'R';
	relax_flag = TRUE;
      }
    }
  }

  return relax_flag;
}

/*==================================================================*/
int check_uncertain_d_condition(SENTENCE_DATA *sp, DPND *dp, int gvnr)
     /*==================================================================*/
{
  /* 後方チ(ェック)の d の係り受けを許す条件

  ・ 次の可能な係り先(D)が３つ以上後ろ ( d - - D など )
  ・ 係り元とdの後ろが同じ格	例) 日本で最初に京都で行われた
  ・ d(係り先)とdの後ろが同じ格	例) 東京で計画中に京都に変更された

  ※ 「dに読点がある」ことでdを係り先とするのは不適切
  例) 「うすい板を木目が直角になるように、何枚もはり合わせたもの。」
  */

  int i, next_D;
  char *dpnd_cp, *gvnr_cp, *next_cp;

  next_D = 0;
  for (i = gvnr + 1; i < sp->Bnst_num ; i++) {
    if (Mask_matrix[dp->pos][i] &&
	Quote_matrix[dp->pos][i] &&
	dp->mask[i] &&
	Dpnd_matrix[dp->pos][i] == 'D') {
      next_D = i;
      break;
    }
  }
  dpnd_cp = check_feature(sp->bnst_data[dp->pos].f, "係");
  gvnr_cp = check_feature(sp->bnst_data[gvnr].f, "係");
  if (gvnr < sp->Bnst_num-1) {
    next_cp = check_feature(sp->bnst_data[gvnr+1].f, "係");	
  }
  else {
    next_cp = NULL;
  }

  if (next_D == 0 ||
      gvnr + 2 < next_D ||
      (gvnr + 2 == next_D && gvnr < sp->Bnst_num-1 &&
       check_feature(sp->bnst_data[gvnr+1].f, "体言") &&
       ((dpnd_cp && next_cp && !strcmp(dpnd_cp, next_cp)) ||
	(gvnr_cp && next_cp && !strcmp(gvnr_cp, next_cp))))) {
    /* fprintf(stderr, "%d -> %d OK\n", i, j); */
    return 1;
  } else {
    return 0;
  }
}

/*==================================================================*/
int compare_dpnd(SENTENCE_DATA *sp, TOTAL_MGR *new_mgr, TOTAL_MGR *best_mgr)
     /*==================================================================*/
{
  int i;

  return FALSE;

  if (Possibility == 1 || new_mgr->dflt < best_mgr->dflt) {
    return TRUE;
  } else {
    for (i = sp->Bnst_num - 2; i >= 0; i--) {
      if (new_mgr->dpnd.dflt[i] < best_mgr->dpnd.dflt[i]) 
	return TRUE;
      else if (new_mgr->dpnd.dflt[i] > best_mgr->dpnd.dflt[i]) 
	return FALSE;
    }
  }

  fprintf(stderr, ";; Error in compare_dpnd !!\n");
  exit(1);
}


/*==================================================================*/
       void dpnd_info_to_bnst(SENTENCE_DATA *sp, DPND *dp)
/*==================================================================*/
{
  /* 係り受けに関する種々の情報を DPND から BNST_DATA にコピー */

  int		i;
  BNST_DATA	*b_ptr;

  for (i = 0, b_ptr = sp->bnst_data; i < sp->Bnst_num; i++, b_ptr++) {
    if (Language != CHINESE && (dp->type[i] == 'd' || dp->type[i] == 'R')) {
      b_ptr->dpnd_head = dp->head[i];
      b_ptr->dpnd_type = 'D';	/* relaxした場合もDに */
    } else {
      b_ptr->dpnd_head = dp->head[i];
      b_ptr->dpnd_type = dp->type[i];
    }
    b_ptr->dpnd_dflt = dp->dflt[i];
  }
}

/*==================================================================*/
       void dpnd_info_to_tag_raw(SENTENCE_DATA *sp, DPND *dp)
/*==================================================================*/
{
  /* 係り受けに関する種々の情報を DPND から TAG_DATA にコピー */

  int		i, j, last_b, offset, check_ac, rep_length;
  char	*cp, *strp, buf[SMALL_DATA_LEN];
  TAG_DATA	*t_ptr, *ht_ptr;

  for (i = 0, t_ptr = sp->tag_data; i < sp->Tag_num; i++, t_ptr++) {
    /* もっとも近い文節行を記憶 */
    if (t_ptr->bnum >= 0) {
      last_b = t_ptr->bnum;
    }

    /* 文末 */
    if (i == sp->Tag_num - 1) {
      t_ptr->dpnd_head = -1;
      t_ptr->dpnd_type = 'D';
    }
    /* 隣にかける */
    else if (t_ptr->inum != 0) {
      t_ptr->dpnd_head = t_ptr->num + 1;
      t_ptr->dpnd_type = 'D';
    }
    /* 文節内最後のタグ単位 (inum == 0) */
    else {
      if ((!check_feature((sp->bnst_data + last_b)->f, "タグ単位受無視")) &&
	  (cp = check_feature((sp->bnst_data + dp->head[last_b])->f, "タグ単位受"))) {
	  offset = atoi(cp + strlen("タグ単位受:"));
	if (offset > 0 || (sp->bnst_data + dp->head[last_b])->tag_num <= -1 * offset) {
	  offset = 0;
	}
      }
      else {
	offset = 0;
      }

      /* ＡのＢＣなどがあった場合は、Ｃの格フレームにＡが存在せず、
	 かつ、Ｂの格フレームにＡが存在した場合は、ＡがＢにかかると考える */
      if ((!check_feature((sp->bnst_data + last_b)->f, "タグ単位受無視")) &&
	  check_feature(t_ptr->f, "係:ノ格") && dp->head[last_b] - last_b == 1) {

	if (OptCaseFlag & OPT_CASE_USE_REP_CF) {
	  strp = get_mrph_rep(t_ptr->head_ptr);
	  rep_length = get_mrph_rep_length(strp);
	}
	else {
	  strp = t_ptr->head_ptr->Goi2;
	  rep_length = strlen(strp);
	}

	/* 「ＡのＣ」をチェック */
	ht_ptr = (sp->bnst_data + dp->head[last_b])->tag_ptr + 
	  (sp->bnst_data + dp->head[last_b])->tag_num - 1 + offset;	
	if (ht_ptr->cf_ptr) {
	  check_ac = check_examples(strp, rep_length,
				 ht_ptr->cf_ptr->ex_list[0],
				 ht_ptr->cf_ptr->ex_num[0]);
	}
	else {
	  check_ac = -1;
	}
	if (OptDisplay == OPT_DEBUG) {
	    fprintf(Outfp, "☆直前タグ受判定AC: %6s の %6s: %d\n",
		    t_ptr->head_ptr->Goi2, ht_ptr->head_ptr->Goi2, check_ac);	    
	}

	/* Ｂが複数タグから成る場合のためのループ */
	for (j = 0, ht_ptr = (sp->bnst_data + dp->head[last_b])->tag_ptr; 
	     j < (sp->bnst_data + dp->head[last_b])->tag_num - 1 + offset;
	     j++, ht_ptr++) {
		    
	  if (OptCaseFlag & OPT_CASE_USE_REP_CF) {
	    strp = get_mrph_rep(t_ptr->head_ptr);
	    rep_length = get_mrph_rep_length(strp);
	  }
	  else {
	    strp = t_ptr->head_ptr->Goi2;
	    rep_length = strlen(strp);
	  }

	  /* 「ＡのＢ」をチェック */
	  if (OptDisplay == OPT_DEBUG && ht_ptr->cf_ptr) {
	      fprintf(Outfp, "☆直前タグ受判定AB: %6s の %6s: %d\n",
		      t_ptr->head_ptr->Goi2, ht_ptr->head_ptr->Goi2,
		      check_examples(strp, rep_length,
				     ht_ptr->cf_ptr->ex_list[0],
				     ht_ptr->cf_ptr->ex_num[0]));
	  }

	  if (check_ac == -1 && ht_ptr->cf_ptr &&
	      check_examples(strp, rep_length,
			     ht_ptr->cf_ptr->ex_list[0],
			     ht_ptr->cf_ptr->ex_num[0]) != -1) {
	      
	      offset = j - ((sp->bnst_data + dp->head[last_b])->tag_num - 1);
	      sprintf(buf, "直前タグ受:%d", offset);
	      assign_cfeature(&((sp->bnst_data + dp->head[last_b])->f), buf, FALSE);
	      assign_cfeature(&(ht_ptr->f), buf, FALSE);
	      break;
	  }
	}		 
      }

      if (dp->head[last_b] == -1) {
	t_ptr->dpnd_head = -1;
      }
      else {
	t_ptr->dpnd_head = ((sp->bnst_data + dp->head[last_b])->tag_ptr + 
			    (sp->bnst_data + dp->head[last_b])->tag_num - 1 + offset)->num;
      }

      if (Language != CHINESE && (dp->type[last_b] == 'd' || dp->type[last_b] == 'R')) {
	t_ptr->dpnd_type = 'D';
      }
      else {
	t_ptr->dpnd_type = dp->type[last_b];
      }
    }
  }
}

/*==================================================================*/
         void dpnd_info_to_tag(SENTENCE_DATA *sp, DPND *dp)
/*==================================================================*/
{
  if (OptInput == OPT_RAW || 
      (OptInput & OPT_INPUT_BNST) ||
      OptUseNCF) {
    dpnd_info_to_tag_raw(sp, dp);
  }
  else {
    dpnd_info_to_tag_pm(sp);
  }
}

/*==================================================================*/
      int check_semantic_head(MRPH_DATA *m_ptr, TAG_DATA *t_ptr)
/*==================================================================*/
{
    char *cp;

    if ((cp = check_feature(t_ptr->f, "用言"))) { /* 用言の場合のみ対象 */
	if (!strcmp(cp, "用言:判") && !check_feature(t_ptr->f, "体言止")) { /* 判定詞がある場合はその判定詞 */
	    if (m_ptr->num > t_ptr->head_ptr->num && /* 判定詞はheadより後 (括弧文で例外があるためチェック) */
		!strcmp(Class[m_ptr->Hinshi][0].id, "判定詞")) {
		return TRUE;
	    }
	}
	else { /* 判定詞以外は(準)?内容語 */
	    if (m_ptr->num == t_ptr->head_ptr->num) {
		return TRUE;
	    }
	}
    }

    return FALSE;
}

/*==================================================================*/
	    void dpnd_info_to_mrph_raw(SENTENCE_DATA *sp)
/*==================================================================*/
{
    int i, j, last_t, offset, head, proj_table[MRPH_MAX];
    int appear_content_word_flag = FALSE, last_content_m, this_is_content_m_flag;
    MRPH_DATA *m_ptr, *head_ptr, *last_content_m_ptr = NULL;

    /* initialize proj_table */
    memset(proj_table, 0, sizeof(int) * MRPH_MAX);

    for (i = 0, m_ptr = sp->mrph_data; i < sp->Mrph_num; i++, m_ptr++) {
	/* もっとも近い基本句行を記憶 */
	if (m_ptr->tnum >= 0) {
	    last_t = m_ptr->tnum;
	    appear_content_word_flag = FALSE;
            last_content_m_ptr = NULL;
	}
	/* この形態素がsemantic headかどうかをチェック */
	if (appear_content_word_flag == FALSE && check_semantic_head(m_ptr, sp->tag_data + last_t)) {
	    appear_content_word_flag = TRUE;
	    last_content_m = m_ptr->num;
            last_content_m_ptr = m_ptr;
            this_is_content_m_flag = TRUE;
	}
        else
            this_is_content_m_flag = FALSE;

	/* 隣にかける */
	if (m_ptr->inum != 0 && ((OptSemanticHead && appear_content_word_flag == FALSE) || /* semantic head時 */
                                 (!OptSemanticHead && !this_is_content_m_flag && !check_feature(m_ptr->f, "Ｔ句内要素")))) { /* syntactic headかつ句内要素ではない */
	    m_ptr->dpnd_head = m_ptr->num + 1;
	    m_ptr->dpnd_type = 'D';
            if (last_content_m_ptr) { /* semantic headからこの形態素にかける */
                last_content_m_ptr->dpnd_head = m_ptr->num;
                last_content_m_ptr->dpnd_type = 'D';
                last_content_m_ptr = NULL;
            }
	}
        /* 基本句内の内容語(前側にある)にかける */
	else if (appear_content_word_flag == TRUE && !this_is_content_m_flag && 
                 (OptSemanticHead || check_feature(m_ptr->f, "Ｔ句内要素"))) { /* semantic head時もしくは句内要素 */
	    m_ptr->dpnd_head = last_content_m;
	    m_ptr->dpnd_type = 'D';
	}
	/* 基本句内最後の形態素(inum == 0) もしくは semantic head */
	else {
            if (last_content_m_ptr && !this_is_content_m_flag) { /* semantic headからこの形態素にかける */
                last_content_m_ptr->dpnd_head = m_ptr->num;
                last_content_m_ptr->dpnd_type = 'D';
                last_content_m_ptr = NULL;
            }

            /* 文末 */
            if (i == sp->Mrph_num - 1) {
                m_ptr->dpnd_head = -1;
                m_ptr->dpnd_type = 'D';
            }
            else {
                offset = 0;
                head = (sp->tag_data + last_t)->dpnd_head;
                m_ptr->dpnd_type = (sp->tag_data + last_t)->dpnd_type;

                if (head == -1) {
                    m_ptr->dpnd_head = -1;
                }
                else {
                    /* m_ptr->dpnd_head = (sp->tag_data + head)->head_ptr->num; */
                    head_ptr = find_head_mrph_from_dpnd_bnst((BNST_DATA *)(sp->tag_data + last_t), 
                                                             (BNST_DATA *)(sp->tag_data + head));
                    /* check projectivity */
                    if (proj_table[i] && proj_table[i] < head_ptr->num) {
                        if (OptDisplay == OPT_DEBUG) {
                            fprintf(stderr, ";; violation of projectivity in mrph tree (%s: modified %dth mrph: %d -> %d)\n", 
                                    sp->KNPSID ? sp->KNPSID + 5: "?", i, head_ptr->num, proj_table[i]);
                        }
                        m_ptr->dpnd_head = proj_table[i];
                    }
                    else {
                        m_ptr->dpnd_head = head_ptr->num;

                        /* update proj_table */
                        for (j = i + 1; j < m_ptr->dpnd_head; j++) {
                            if (!proj_table[j] || proj_table[j] > m_ptr->dpnd_head) {
                                proj_table[j] = m_ptr->dpnd_head;
                            }
                        }
                    }
                }
            }
	}
    }
}

/*==================================================================*/
	      void dpnd_info_to_mrph(SENTENCE_DATA *sp)
/*==================================================================*/
{
    if (OptInput == OPT_RAW || 
	(OptInput & OPT_INPUT_BNST) ||
	OptUseNCF) {
	dpnd_info_to_mrph_raw(sp);
    }
    else {
	/* dpnd_info_to_mrph_pm(sp); */
    }
}

/*==================================================================*/
void copy_para_info(SENTENCE_DATA *sp, BNST_DATA *dst, BNST_DATA *src)
/*==================================================================*/
{
  dst->para_num = src->para_num;
  dst->para_key_type = src->para_key_type;
  dst->para_top_p = src->para_top_p;
  dst->para_type = src->para_type;
  dst->to_para_p = src->to_para_p;
}

/*==================================================================*/
       void tag_bnst_postprocess(SENTENCE_DATA *sp, int flag)
/*==================================================================*/
{
  /* タグ単位・文節を後処理して、機能的なタグ単位をマージ
     flag == 0: num, dpnd_head の番号の付け替えはしない */

  int	i, j, count = -1, t_table[TAG_MAX], b_table[BNST_MAX], merge_to;
  TAG_DATA *t_ptr;
  BNST_DATA *b_ptr;
  char *cp;

  /* タグ後処理用ルールの適用 
     FEATUREの伝搬はこの中で行う */
  assign_general_feature(sp->tag_data, sp->Tag_num, PostProcessTagRuleType, FALSE, FALSE);

  /* マージするタグ・文節の処理 */
  for (i = 0, t_ptr = sp->tag_data; i < sp->Tag_num; i++, t_ptr++) {
    /* もとのnum, mrph_numを保存 */
    t_ptr->preserve_mrph_num = t_ptr->mrph_num;
    if (t_ptr->bnum >= 0) { /* 文節区切りでもあるとき */
      t_ptr->b_ptr->preserve_mrph_num = t_ptr->b_ptr->mrph_num;
    }

    if (check_feature(t_ptr->f, "Ｔマージ←")) {
      for (merge_to = i - 1; merge_to >= 0; merge_to--) {
	if ((sp->tag_data + merge_to)->num >= 0) {
	  break;
	}
      }
      t_ptr->num = -1; /* 無効なタグ単位である印をつけておく */
      (sp->tag_data + merge_to)->mrph_num += t_ptr->mrph_num;
      (sp->tag_data + merge_to)->dpnd_head = t_ptr->dpnd_head;
      (sp->tag_data + merge_to)->dpnd_type = t_ptr->dpnd_type;
      for (j = 0; j < t_ptr->mrph_num; j++) {
	(sp->tag_data + merge_to)->length += strlen((t_ptr->mrph_ptr + j)->Goi2);
      }

      assign_cfeature(&((sp->tag_data + merge_to)->f), "後処理-基本句マージ", FALSE);
      /* <文節始>や<タグ単位始>のfeature消去はしない */

      if (t_ptr->bnum >= 0) { /* 文節区切りでもあるとき */
	for (merge_to = -1; t_ptr->b_ptr->num + merge_to >= 0; merge_to--) {
	  if ((t_ptr->b_ptr + merge_to)->num >= 0) {
	    break;
	  }
	}
	copy_para_info(sp, t_ptr->b_ptr + merge_to, t_ptr->b_ptr);
	t_ptr->b_ptr->num = -1;
	(t_ptr->b_ptr + merge_to)->mrph_num += t_ptr->b_ptr->mrph_num;
	(t_ptr->b_ptr + merge_to)->dpnd_head = t_ptr->b_ptr->dpnd_head;
	(t_ptr->b_ptr + merge_to)->dpnd_type = t_ptr->b_ptr->dpnd_type;
	for (j = 0; j < t_ptr->mrph_num; j++) {
	  (t_ptr->b_ptr + merge_to)->length += strlen((t_ptr->b_ptr->mrph_ptr + j)->Goi2);
	}
      }
    }
    else {
      count++;
    }
    t_table[i] = count;
  }

  if (flag == 0) {
    return;
  }

  count = -1;
  for (i = 0, b_ptr = sp->bnst_data; i < sp->Bnst_num; i++, b_ptr++) {
    if (b_ptr->num != -1) {
      count++;
    }
    b_table[i] = count;
  }

  /* タグ単位番号の更新 */
  for (i = 0, t_ptr = sp->tag_data; i < sp->Tag_num; i++, t_ptr++) {
    if (t_ptr->num != -1) { /* numの更新 (★どこかで tag_data + num をするとだめ) */
      t_ptr->num = t_table[i];
      if (t_ptr->dpnd_head != -1) {
	t_ptr->dpnd_head = t_table[t_ptr->dpnd_head];
      }
    }
    if (t_ptr->bnum >= 0) { /* bnumの更新 (★どこかで bnst_data + bnum をするとだめ) */
      t_ptr->bnum = b_table[t_ptr->bnum];
    }
  }

  /* 文節番号の更新 */
  for (i = 0, b_ptr = sp->bnst_data; i < sp->Bnst_num; i++, b_ptr++) {
    if (b_ptr->num != -1) {
      b_ptr->num = b_table[i];
      if (b_ptr->dpnd_head != -1) {
	b_ptr->dpnd_head = b_table[b_ptr->dpnd_head];
      }
    }
  }
}

/*==================================================================*/
           void undo_tag_bnst_postprocess(SENTENCE_DATA *sp)
/*==================================================================*/
{
  int i, b_count = 0;
  TAG_DATA *t_ptr;

  /* nbestオプションなどでprint_result()が複数回呼ばれるときのために
     変更したnum, mrph_num, lengthを元に戻しておく */

  for (i = 0, t_ptr = sp->tag_data; i < sp->Tag_num; i++, t_ptr++) {
    t_ptr->num = i;
    t_ptr->mrph_num = t_ptr->preserve_mrph_num;
    calc_bnst_length(sp, (BNST_DATA *)t_ptr);

    if (t_ptr->bnum >= 0) { /* 文節区切りでもあるとき */
      t_ptr->b_ptr->num = b_count++;
      t_ptr->bnum = t_ptr->b_ptr->num;
      t_ptr->b_ptr->mrph_num = t_ptr->b_ptr->preserve_mrph_num;
      calc_bnst_length(sp, t_ptr->b_ptr);
    }
  }    
}

/*==================================================================*/
              void para_postprocess(SENTENCE_DATA *sp)
/*==================================================================*/
{
  int i;

  for (i = 0; i < sp->Bnst_num; i++) {
    if (check_feature((sp->bnst_data + i)->f, "用言") &&
	!check_feature((sp->bnst_data + i)->f, "〜とみられる") &&
	(sp->bnst_data + i)->para_num != -1 &&
	sp->para_data[(sp->bnst_data + i)->para_num].status != 'x') {
	    
      assign_cfeature(&((sp->bnst_data + i)->f), "提題受:30", FALSE);
      assign_cfeature(&(((sp->bnst_data + i)->tag_ptr + 
			 (sp->bnst_data + i)->tag_num - 1)->f), "提題受:30", FALSE);
    }
  }
}

/*==================================================================*/
   void dpnd_evaluation(SENTENCE_DATA *sp, DPND dpnd, int eos_flag)
/*==================================================================*/
{
  int i, j, k, one_score, score, rentai, vacant_slot_num;
  int topic_score;
  int scase_check[SCASE_CODE_SIZE], ha_check, un_count, pred_p;
  char *cp, *cp2, *buffer;
  BNST_DATA *g_ptr, *d_ptr;

  /* 依存構造だけを評価する場合の関数
     (各文節について，そこに係っている文節の評価点を計算)

     評価基準
     ========
     0. 係り先のdefault位置との差をペナルティに(kakari_uke.rule)

     1. 「〜は」(提題,係:未格)の係り先は優先されるものがある
     (bnst_etc.ruleで指定，並列のキーは並列解析後プログラムで指定)

     2. 「〜は」は一述語に一つ係ることを優先(時間,数量は別)

     3. すべての格要素は同一表層格が一述語に一つ係ることを優先(ガガは別)

     4. 未格，連体修飾先はガ,ヲ,ニ格の余っているスロット数だけ点数付与
  */

  score = 0;
  for (i = 1; i < sp->Bnst_num; i++) {
    g_ptr = sp->bnst_data + i;

    one_score = 0;
    for (k = 0; k < SCASE_CODE_SIZE; k++) scase_check[k] = 0;
    ha_check = 0;
    un_count = 0;

    if (check_feature(g_ptr->f, "用言") ||
	check_feature(g_ptr->f, "準用言")) {
      pred_p = 1;
    } else {
      pred_p = 0;
    }

    for (j = i-1; j >= 0; j--) {
      d_ptr = sp->bnst_data + j;

      if (dpnd.head[j] == i) {

	/* 係り先のDEFAULTの位置との差をペナルティに
	   ※ 提題はC,B'を求めて遠くに係ることがあるが，それが
	   他の係り先に影響しないよう,ペナルティに差をつける */

	if (check_feature(d_ptr->f, "提題")) {
	  one_score -= dpnd.dflt[j];
	} else {
	  one_score -= dpnd.dflt[j] * 2;
	}
	    
	/* 読点をもつものが隣にかかることを防ぐ */

	if (j + 1 == i && check_feature(d_ptr->f, "読点")) {
	  one_score -= 5;
	}

	if (pred_p &&
	    (cp = check_feature(d_ptr->f, "係")) != NULL) {
		    
	  /* 未格 提題(「〜は」)の扱い */

	  if (check_feature(d_ptr->f, "提題") &&
	      !strcmp(cp, "係:未格")) {

	    /* 文末, 「〜が」など, 並列末, C, B'に係ることを優先 */

	    if ((cp2 = check_feature(g_ptr->f, "提題受")) 
		!= NULL) {
	      sscanf(cp2, "%*[^:]:%d", &topic_score);
	      one_score += topic_score;
	    }
	    /* else {one_score -= 15;} */

	    /* 一つめの提題にだけ点を与える (時間,数量は別)
	       → 複数の提題が同一述語に係ることを防ぐ */

	    if (check_feature(d_ptr->f, "時間") ||
		check_feature(d_ptr->f, "数量")) {
	      one_score += 10;
	    } else if (ha_check == 0){
	      one_score += 10;
	      ha_check = 1;
	    }
	  }

	  k = case2num(cp+3);

	  /* 格要素一般の扱い */

	  /* 未格 : 数えておき，後で空スロットを調べる (時間,数量は別) */

	  if (!strcmp(cp, "係:未格")) {
	    if (check_feature(d_ptr->f, "時間") ||
		check_feature(d_ptr->f, "数量")) {
	      one_score += 10;
	    } else {
	      un_count++;
	    }
	  }

	  /* ノ格 : 体言以外なら break 
	     → それより前の格要素には点を与えない．
	     → ノ格がかかればそれより前の格はかからない

	     ※ 「体言」というのは判定詞のこと，ただし
	     文末などでは用言:動となっていることも
	     あるので，「体言」でチェック */

	  else if (!strcmp(cp, "係:ノ格")) {
	    if (!check_feature(g_ptr->f, "体言")) {
	      one_score += 10;
	      break;
	    }
	  } 

	  /* ガ格 : ガガ構文があるので少し複雑 */

	  else if (!strcmp(cp, "係:ガ格")) {
	    if (g_ptr->SCASE_code[case2num("ガ格")] &&
		scase_check[case2num("ガ格")] == 0) {
	      one_score += 10;
	      scase_check[case2num("ガ格")] = 1;
	    } 
	    else if (g_ptr->SCASE_code[case2num("ガ２")] &&
		     scase_check[case2num("ガ２")] == 0) {
	      one_score += 10;
	      scase_check[case2num("ガ格")] = 1;
	    }
	  }

	  /* 他の格 : 各格1つは点数をあたえる
	     ※ ニ格の場合，時間とそれ以外は区別する方がいいかも？ */
	  else if (k != -1) {
	    if (scase_check[k] == 0) {
	      scase_check[k] = 1;
	      one_score += 10;
	    } 
	  }

	  /* 「〜するのは〜だ」にボーナス 01/01/11
	     ほとんどの場合改善．

	     改善例)
	     「抗議したのも 任官を 拒否される 理由の 一つらしい」

	     「使うのは 恐ろしい ことだ。」
	     「円満決着に なるかどうかは 微妙な ところだ。」
	     ※ これらの例は「こと/ところだ」に係ると扱う

	     「他人に 教えるのが 好きになる やり方です」
	     ※ この例は曖昧だが，文脈上正しい

	     副作用例)
	     「だれが ＭＶＰか 分からない 試合でしょう」
	     「〜 殴るなど した 疑い。」
	     「ビザを 取るのも 大変な 時代。」
	     「波が 高まるのは 避けられそうにない 雲行きだ。」
	     「あまり 役立つとは 思われない 論理だ。」
	     「どう 折り合うかが 問題視されてきた 法だ。」
	     「認められるかどうかが 争われた 裁判で」

	     ※問題※
	     「あの戦争」が〜 のような場合も用言とみなされるのが問題
	  */

	  if (check_feature(d_ptr->f, "用言") &&
	      (check_feature(d_ptr->f, "係:未格") ||
	       check_feature(d_ptr->f, "係:ガ格")) &&
	      check_feature(g_ptr->f, "用言:判")) {
	    one_score += 3;
	  }
	}
      }
    }

    /* 用言の場合，最終的に未格,ガ格,ヲ格,ニ格,連体修飾に対して
       ガ格,ヲ格,ニ格のスロット分だけ点数を与える */

    if (pred_p) {

      /* 連体修飾の場合，係先が
	 ・形式名詞,副詞的名詞
	 ・「予定」,「見込み」など
	 でなければ一つの格要素と考える */

      if (check_feature(g_ptr->f, "係:連格")) {
	if (check_feature(sp->bnst_data[dpnd.head[i]].f, "外の関係") || 
	    check_feature(sp->bnst_data[dpnd.head[i]].f, "ルール外の関係")) {
	  rentai = 0;
	  one_score += 10;	/* 外の関係ならここで加点 */
	} else {
	  rentai = 1;	/* それ以外なら後で空きスロットをチェック */
	}
      } else {
	rentai = 0;
      }

      /* 空いているガ格,ヲ格,ニ格,ガ２ */

      vacant_slot_num = 0;
      if ((g_ptr->SCASE_code[case2num("ガ格")]
	   - scase_check[case2num("ガ格")]) == 1) {
	vacant_slot_num ++;
      }
      if ((g_ptr->SCASE_code[case2num("ヲ格")]
	   - scase_check[case2num("ヲ格")]) == 1) {
	vacant_slot_num ++;
      }
      if ((g_ptr->SCASE_code[case2num("ニ格")]
	   - scase_check[case2num("ニ格")]) == 1 &&
	  rentai == 1 &&
	  check_feature(g_ptr->f, "用言:動")) {
	vacant_slot_num ++;
	/* ニ格は動詞で連体修飾の場合だけ考慮，つまり連体
	   修飾に割り当てるだけで，未格のスロットとはしない */
      }
      if ((g_ptr->SCASE_code[case2num("ガ２")]
	   - scase_check[case2num("ガ２")]) == 1) {
	vacant_slot_num ++;
      }

      /* 空きスロット分だけ連体修飾，未格にスコアを与える */

      if ((rentai + un_count) <= vacant_slot_num) 
	one_score += (rentai + un_count) * 10;
      else 
	one_score += vacant_slot_num * 10;
    }

    score += one_score;

    if (OptDisplay == OPT_DEBUG) {
      if (i == 1) 
	fprintf(Outfp, "Score:    ");
      if (pred_p) {
	fprintf(Outfp, "%2d*", one_score);
      } else {
	fprintf(Outfp, "%2d ", one_score);
      }
    }
  }

  if (OptDisplay == OPT_DEBUG) {
    fprintf(Outfp, "=%d\n", score);
  }

  if (OptDisplay == OPT_DEBUG || OptNbest == TRUE) {
    dpnd_info_to_bnst(sp, &dpnd);
    if (!(OptExpress & OPT_NOTAG)) {
      dpnd_info_to_tag(sp, &dpnd); 
    }
    if (make_dpnd_tree(sp)) {
      if (!(OptExpress & OPT_NOTAG)) {
	bnst_to_tag_tree(sp); /* タグ単位の木へ */
      }

      if (OptNbest == TRUE) {
	  sp->score = score;
	  print_result(sp, 0, eos_flag);
      }
      else {
	  print_kakari(sp, OptExpress & OPT_NOTAG ? OPT_BNSTTREE : OPT_TREE, eos_flag);
      }
    }
  }

  if (score > sp->Best_mgr->score) {
    sp->Best_mgr->dpnd = dpnd;
    sp->Best_mgr->score = score;
    sp->Best_mgr->ID = dpndID;
    Possibility++;
  }

  dpndID++;
}

/*==================================================================*/
  void count_dpnd_candidates(SENTENCE_DATA *sp, DPND *dpnd, int pos)
/*==================================================================*/
{
  int i, count = 0, d_possibility = 1;
  BNST_DATA *b_ptr = sp->bnst_data + pos;

  if (pos == -1) {
    return;
  }

  if (pos < sp->Bnst_num - 2) {
    for (i = pos + 2; i < dpnd->head[pos + 1]; i++) {
      dpnd->mask[i] = 0;
    }
  }

  for (i = pos + 1; i < sp->Bnst_num; i++) {
    if (Quote_matrix[pos][i] &&
	dpnd->mask[i]) {

      if (d_possibility && Dpnd_matrix[pos][i] == 'd') {
	if (check_uncertain_d_condition(sp, dpnd, i)) {
	  dpnd->check[pos].pos[count] = i;
	  count++;
	}
	d_possibility = 0;
      }
      else if (Dpnd_matrix[pos][i] && 
	       Dpnd_matrix[pos][i] != 'd') {
	dpnd->check[pos].pos[count] = i;
	count++;
	d_possibility = 0;
      }

      /* バリアのチェック */
      if (count && 
	  b_ptr->dpnd_rule->barrier.fp[0] && 
	  feature_pattern_match(&(b_ptr->dpnd_rule->barrier), 
				sp->bnst_data[i].f, 
				b_ptr, sp->bnst_data + i) == TRUE) {
	break;
      }
    }
  }

  if (count) {
    dpnd->check[pos].num = count;	/* 候補数 */
    dpnd->check[pos].def = b_ptr->dpnd_rule->preference == -1 ? count : b_ptr->dpnd_rule->preference;	/* デフォルトの位置 */
  }

  count_dpnd_candidates(sp, dpnd, pos - 1);
}

/*==================================================================*/
    void call_count_dpnd_candidates(SENTENCE_DATA *sp, DPND *dpnd)
/*==================================================================*/
{
  int i;

  for (i = 0; i < sp->Bnst_num; i++) {
    dpnd->mask[i] = 1;
    memset(&(dpnd->check[i]), 0, sizeof(CHECK_DATA));
    dpnd->check[i].num = -1;
  }

  count_dpnd_candidates(sp, dpnd, sp->Bnst_num - 1);
}

/*==================================================================*/
     void decide_dpnd(SENTENCE_DATA *sp, DPND dpnd, int eos_flag)
/*==================================================================*/
{
  int i, count, possibilities[BNST_MAX], default_pos, d_possibility;
  int MaskFlag = 0;
  char *cp;
  BNST_DATA *b_ptr;
    
  if (OptDisplay == OPT_DEBUG) {
    if (dpnd.pos == sp->Bnst_num - 1) {
      fprintf(Outfp, "------");
      for (i = 0; i < sp->Bnst_num; i++)
	fprintf(Outfp, "-%02d", i);
      fputc('\n', Outfp);
    }
    fprintf(Outfp, "In %2d:", dpnd.pos);
    for (i = 0; i < sp->Bnst_num; i++)
      fprintf(Outfp, " %2d", dpnd.head[i]);
    fputc('\n', Outfp);
  }

  dpnd.pos --;

  /* 文頭まで解析が終わったら評価関数をよぶ */

  if (dpnd.pos == -1) {
    /* 無格従属: 前の文節の係り受けに従う場合 */
    for (i = 0; i < sp->Bnst_num -1; i++)
      if (dpnd.head[i] < 0) {
	/* ありえない係り受け */
	if (i >= dpnd.head[i + dpnd.head[i]]) {
	  return;
	}
	dpnd.head[i] = dpnd.head[i + dpnd.head[i]];
	dpnd.check[i].pos[0] = dpnd.head[i];
      }

    if (OptAnalysis == OPT_DPND ||
	OptAnalysis == OPT_CASE2) {
	dpnd_evaluation(sp, dpnd, eos_flag);
    } 
    else if (OptAnalysis == OPT_CASE) {
	call_case_analysis(sp, dpnd, eos_flag);
    }
    return;
  }

  b_ptr = sp->bnst_data + dpnd.pos;
  dpnd.f[dpnd.pos] = b_ptr->f;

  /* (前の係りによる)非交差条件の設定 (dpnd.mask が 0 なら係れない) */

  if (dpnd.pos < sp->Bnst_num - 2)
    for (i = dpnd.pos + 2; i < dpnd.head[dpnd.pos + 1]; i++)
      dpnd.mask[i] = 0;
    
  /* 並列構造のキー文節, 部分並列の文節<I>
     (すでに行われた並列構造解析の結果をマークするだけ) */

  for (i = dpnd.pos + 1; i < sp->Bnst_num; i++) {
    if (Mask_matrix[dpnd.pos][i] == 2) {
      dpnd.head[dpnd.pos] = i;
      dpnd.type[dpnd.pos] = 'P';
      /* チェック用 */
      /* 並列の場合は一意に決まっているので、候補を挙げるのは意味がない */
      dpnd.check[dpnd.pos].num = 1;
      dpnd.check[dpnd.pos].pos[0] = i;
      decide_dpnd(sp, dpnd, eos_flag);
      return;
    } else if (Mask_matrix[dpnd.pos][i] == 3) {
      dpnd.head[dpnd.pos] = i;
      dpnd.type[dpnd.pos] = 'I';

      dpnd.check[dpnd.pos].num = 1;
      dpnd.check[dpnd.pos].pos[0] = i;
      decide_dpnd(sp, dpnd, eos_flag);
      return;
    }
  }

  /* 前の文節の係り受けに従う場合  例) 「〜大統領は一日，〜」 */

  if ((cp = check_feature(b_ptr->f, "係:無格従属")) != NULL) {
    sscanf(cp, "%*[^:]:%*[^:]:%d", &(dpnd.head[dpnd.pos]));
    dpnd.type[dpnd.pos] = 'D';
    dpnd.dflt[dpnd.pos] = 0;
    dpnd.check[dpnd.pos].num = 1;
    decide_dpnd(sp, dpnd, eos_flag);
    return;
  }

  /* 通常の係り受け解析 */

  /* 係り先の候補を調べる */
    
  count = 0;
  d_possibility = 1;
  for (i = dpnd.pos + 1; i < sp->Bnst_num; i++) {
    if (Mask_matrix[dpnd.pos][i] &&
	Quote_matrix[dpnd.pos][i] &&
	dpnd.mask[i]) {

      if (d_possibility && Dpnd_matrix[dpnd.pos][i] == 'd') {
	if (check_uncertain_d_condition(sp, &dpnd, i)) {
	  possibilities[count] = i;
	  count++;
	}
	d_possibility = 0;
      }
      else if (Dpnd_matrix[dpnd.pos][i] && 
	       Dpnd_matrix[dpnd.pos][i] != 'd') {
	possibilities[count] = i;
	count++;
	d_possibility = 0;
      }

      /* バリアのチェック */
      if (count &&
	  b_ptr->dpnd_rule->barrier.fp[0] &&
	  feature_pattern_match(&(b_ptr->dpnd_rule->barrier), 
				sp->bnst_data[i].f,
				b_ptr, sp->bnst_data + i) == TRUE)
	break;
    }
    else {
      MaskFlag = 1;
    }
  }

  /* 実際に候補をつくっていく(この関数の再帰的呼び出し) */

  if (count) {

    /* preference は一番近く:1, 二番目:2, 最後:-1
       default_pos は一番近く:1, 二番目:2, 最後:count に変更 */

    default_pos = (b_ptr->dpnd_rule->preference == -1) ?
      count: b_ptr->dpnd_rule->preference;

    dpnd.check[dpnd.pos].num = count;	/* 候補数 */
    dpnd.check[dpnd.pos].def = default_pos;	/* デフォルトの位置 */
    for (i = 0; i < count; i++) {
      dpnd.check[dpnd.pos].pos[i] = possibilities[i];
    }

    /* 一意に決定する場合 */

    if (b_ptr->dpnd_rule->barrier.fp[0] == NULL || 
	b_ptr->dpnd_rule->decide) {
      if (default_pos <= count) {
	dpnd.head[dpnd.pos] = possibilities[default_pos - 1];
      } else {
	dpnd.head[dpnd.pos] = possibilities[count - 1];
	/* default_pos が 2 なのに，countが 1 しかない場合 */
      }
      dpnd.type[dpnd.pos] = Dpnd_matrix[dpnd.pos][dpnd.head[dpnd.pos]];
      dpnd.dflt[dpnd.pos] = 0;
      decide_dpnd(sp, dpnd, eos_flag);
    } 

    /* すべての可能性をつくり出す場合 */
    /* 節間の係り受けの場合は一意に決めるべき */

    else {
      for (i = 0; i < count; i++) {
	dpnd.head[dpnd.pos] = possibilities[i];
	dpnd.type[dpnd.pos] = Dpnd_matrix[dpnd.pos][dpnd.head[dpnd.pos]];
	dpnd.dflt[dpnd.pos] = abs(default_pos - 1 - i);
	decide_dpnd(sp, dpnd, eos_flag);
      }
    }
  } 

  /* 係り先がない場合
     文末が並列にマスクされていなければ，文末に係るとする */

  else {
    if (Mask_matrix[dpnd.pos][sp->Bnst_num - 1]) {
      dpnd.head[dpnd.pos] = sp->Bnst_num - 1;
      dpnd.type[dpnd.pos] = 'D';
      dpnd.dflt[dpnd.pos] = 10;
      dpnd.check[dpnd.pos].num = 1;
      dpnd.check[dpnd.pos].pos[0] = sp->Bnst_num - 1;
      decide_dpnd(sp, dpnd, eos_flag);
    }
  }
}

/*==================================================================*/
           void when_no_dpnd_struct(SENTENCE_DATA *sp)
/*==================================================================*/
{
  int i;

  sp->Best_mgr->dpnd.head[sp->Bnst_num - 1] = -1;
  sp->Best_mgr->dpnd.type[sp->Bnst_num - 1] = 'D';

  for (i = sp->Bnst_num - 2; i >= 0; i--) {
    sp->Best_mgr->dpnd.head[i] = i + 1;
    sp->Best_mgr->dpnd.type[i] = 'D';
    sp->Best_mgr->dpnd.check[i].num = 1;
    sp->Best_mgr->dpnd.check[i].pos[0] = i + 1;
  }

  sp->Best_mgr->score = 0;
}

/*==================================================================*/
	int after_decide_dpnd(SENTENCE_DATA *sp, int eos_flag)
/*==================================================================*/
{
  int i, j;
  TAG_DATA *check_b_ptr;
    
  /* 解析済: 構造は与えられたもの1つのみ */
  if (OptInput & OPT_PARSED) {
    Possibility = 1;
  }

  if (Possibility != 0) {
    /* 依存構造決定後 格解析を行う場合 */
    if (OptAnalysis == OPT_CASE2) {
      sp->Best_mgr->score = -10000;
      if (call_case_analysis(sp, sp->Best_mgr->dpnd, eos_flag) == FALSE) {
	return FALSE;
      }
    }

    /* 依存構造・格構造決定後の処理 */

    /* 格解析結果の情報をfeatureへ */
    if (OptAnalysis == OPT_CASE || OptAnalysis == OPT_CASE2) {
      /* 格解析結果を用言基本句featureへ */
      for (i = 0; i < sp->Best_mgr->pred_num; i++) {
	assign_nil_assigned_components(sp, &(sp->Best_mgr->cpm[i])); /* 未対応格要素の処理 */

	/* assign_case_component_feature(sp, &(sp->Best_mgr->cpm[i]), FALSE); <格要素-??>feature */

	/* 格フレームの意味情報を用言基本句featureへ */
	for (j = 0; j < sp->Best_mgr->cpm[i].cmm[0].cf_ptr->element_num; j++) {
	  append_cf_feature(&(sp->Best_mgr->cpm[i].pred_b_ptr->f), 
			    &(sp->Best_mgr->cpm[i]), sp->Best_mgr->cpm[i].cmm[0].cf_ptr, j);
	}
      }
    }

    /* 木を作成 */
    dpnd_info_to_bnst(sp, &(sp->Best_mgr->dpnd));
    if (make_dpnd_tree(sp) == FALSE) {
      return FALSE;
    }
    bnst_to_tag_tree(sp); /* タグ単位の木へ */

    if (OptAnalysis == OPT_CASE || OptAnalysis == OPT_CASE2) {
      /* 格解析の結果を用言文節へ */
      for (i = 0; i < sp->Best_mgr->pred_num; i++) {
	sp->Best_mgr->cpm[i].pred_b_ptr->cpm_ptr = &(sp->Best_mgr->cpm[i]);
	/* ※ 暫定的
	   並列のときに make_dpnd_tree() を呼び出すと cpm_ptr がなくなるので、
	   ここでコピーしておく */
	check_b_ptr = sp->Best_mgr->cpm[i].pred_b_ptr;
	while (check_b_ptr->parent && check_b_ptr->parent->para_top_p == TRUE && 
	       check_b_ptr->parent->cpm_ptr == NULL) {
	  check_b_ptr->parent->cpm_ptr = &(sp->Best_mgr->cpm[i]);
	  check_b_ptr = check_b_ptr->parent;
	}

	/* 各格要素の親用言を設定
	   ※ 文脈解析のときに格フレームを決定してなくても格解析は行っているので
	   これは成功する */
	for (j = 0; j < sp->Best_mgr->cpm[i].cf.element_num; j++) {
	  /* 省略解析の結果 or 連体修飾は除く */
	  if (sp->Best_mgr->cpm[i].elem_b_num[j] <= -2 || 
	      sp->Best_mgr->cpm[i].elem_b_ptr[j]->num > sp->Best_mgr->cpm[i].pred_b_ptr->num) {
	    continue;
	  }
	  sp->Best_mgr->cpm[i].elem_b_ptr[j]->pred_b_ptr = sp->Best_mgr->cpm[i].pred_b_ptr;
	}

	/* 格フレームがある場合 */
	if (sp->Best_mgr->cpm[i].result_num != 0 && 
	    sp->Best_mgr->cpm[i].cmm[0].cf_ptr->cf_address != -1 && 
	    (((OptCaseFlag & OPT_CASE_USE_PROBABILITY) && 
	      sp->Best_mgr->cpm[i].cmm[0].score != CASE_MATCH_FAILURE_PROB) || 
	     (!(OptCaseFlag & OPT_CASE_USE_PROBABILITY) && 
	      sp->Best_mgr->cpm[i].cmm[0].score != CASE_MATCH_FAILURE_SCORE))) {
	  /* 文脈解析のときは格フレーム決定している用言についてのみ */
	  if (!OptEllipsis || sp->Best_mgr->cpm[i].decided == CF_DECIDED) {
	    if (OptCaseFlag & OPT_CASE_ASSIGN_GA_SUBJ) {
	      assign_ga_subject(sp, &(sp->Best_mgr->cpm[i]));
	    }
	    fix_sm_place(sp, &(sp->Best_mgr->cpm[i]));

	    if (OptUseSmfix == TRUE) {
	      specify_sm_from_cf(sp, &(sp->Best_mgr->cpm[i]));
	    }

	    /* マッチした用例をfeatureに出力 *
	       record_match_ex(sp, &(sp->Best_mgr->cpm[i])); */

	    /* 直前格のマッチスコアをfeatureに出力 *
	       record_closest_cc_match(sp, &(sp->Best_mgr->cpm[i])); */

	    /* 格解析の結果を用いて形態素曖昧性を解消 */
	    verb_lexical_disambiguation_by_case_analysis(&(sp->Best_mgr->cpm[i]));
	    noun_lexical_disambiguation_by_case_analysis(&(sp->Best_mgr->cpm[i]));
	  }
	  else if (sp->Best_mgr->cpm[i].decided == CF_CAND_DECIDED) {
	    if (OptCaseFlag & OPT_CASE_ASSIGN_GA_SUBJ) {
	      assign_ga_subject(sp, &(sp->Best_mgr->cpm[i]));
	    }
	  }

	  if (sp->Best_mgr->cpm[i].decided == CF_DECIDED) {
	    assign_cfeature(&(sp->Best_mgr->cpm[i].pred_b_ptr->f), "格フレーム決定", FALSE);
	  }
	}
      }
    }
    return TRUE;
  }
  else { 
    return FALSE;
  }
}

/*==================================================================*/
     int detect_dpnd_case_struct(SENTENCE_DATA *sp, int eos_flag)
/*==================================================================*/
{
  int i;
  DPND dpnd;

  sp->Best_mgr->score = -10000; /* スコアは「より大きい」時に入れ換えるので，
				   初期値は十分小さくしておく */
  sp->Best_mgr->dflt = 0;
  sp->Best_mgr->ID = -1;
  Possibility = 0;
  dpndID = 0;

  /* 係り状態の初期化 */

  for (i = 0; i < sp->Bnst_num; i++) {
    dpnd.head[i] = -1;
    dpnd.type[i] = 'D';
    dpnd.dflt[i] = 0;
    dpnd.mask[i] = 1;
    memset(&(dpnd.check[i]), 0, sizeof(CHECK_DATA));
    dpnd.check[i].num = -1;
    dpnd.f[i] = NULL;
  }
  dpnd.pos = sp->Bnst_num - 1;

  /* 格解析キャッシュの初期化 */
  if (OptAnalysis == OPT_CASE) {
    InitCPMcache();
  }

  /* 依存構造解析 --> 格構造解析 */

  decide_dpnd(sp, dpnd, eos_flag);

  /* 格解析キャッシュの初期化 */
  if (OptAnalysis == OPT_CASE) {
    ClearCPMcache();
  }

  return after_decide_dpnd(sp, eos_flag);
}

/*==================================================================*/
            void check_candidates(SENTENCE_DATA *sp)
/*==================================================================*/
{
    int i, j, b2t_table[BNST_MAX];
    TOTAL_MGR *tm = sp->Best_mgr;
    char b_buffer[DATA_LEN], t_buffer[DATA_LEN], tmp_b_buffer[SMALL_DATA_LEN], tmp_t_buffer[SMALL_DATA_LEN], *cp;

    for (i = 0; i < sp->Bnst_num; i++) { /* 文節番号->基本句番号のテーブル */
	b2t_table[(sp->bnst_data + i)->num] = ((sp->bnst_data + i)->tag_ptr + (sp->bnst_data + i)->tag_num - 1)->num;
    }

    /* 各文節ごとにチェック用の feature を与える */
    for (i = 0; i < sp->Bnst_num; i++)
	if (tm->dpnd.check[i].num != -1) {
	    /* 係り側 -> 係り先 */
	    sprintf(b_buffer, "候補");
	    sprintf(t_buffer, "候補");
	    for (j = 0; j < tm->dpnd.check[i].num; j++) {
		/* 候補たち */
		sprintf(tmp_b_buffer, ":%d", tm->dpnd.check[i].pos[j]);
		sprintf(tmp_t_buffer, ":%d", b2t_table[tm->dpnd.check[i].pos[j]]);
		if (strlen(t_buffer)+strlen(tmp_t_buffer) >= DATA_LEN) {
		    fprintf(stderr, ";; Too long string <%s> (%d) in check_candidates. (%s)\n", 
			    t_buffer, tm->dpnd.check[i].num, sp->KNPSID ? sp->KNPSID+5 : "?");
		    return;
		}
		strcat(b_buffer, tmp_b_buffer);
		strcat(t_buffer, tmp_t_buffer);
	    }
	    assign_cfeature(&((sp->bnst_data + i)->f), b_buffer, FALSE);
	    assign_cfeature(&(((sp->bnst_data + i)->tag_ptr + (sp->bnst_data + i)->tag_num - 1)->f), t_buffer, FALSE);
	}
}

/*==================================================================*/
              void memo_by_program(SENTENCE_DATA *sp)
/*==================================================================*/
{
  /*
   *  プログラムによるメモへの書き込み
   */

  /* 緩和をメモに記録する場合
     int i;

     for (i = 0; i < sp->Bnst_num - 1; i++) {
     if (sp->Best_mgr->dpnd.type[i] == 'd') {
     strcat(PM_Memo, " 緩和d");
     sprintf(PM_Memo+strlen(PM_Memo), "(%d)", i);
     } else if (sp->Best_mgr->dpnd.type[i] == 'R') {
     strcat(PM_Memo, " 緩和R");
     sprintf(PM_Memo+strlen(PM_Memo), "(%d)", i);
     }
     }
  */

  /* 遠い係り受けをメモに記録する場合

  for (i = 0; i < sp->Bnst_num - 1; i++) {
  if (sp->Best_mgr->dpnd.head[i] > i + 3 &&
  !check_feature(sp->bnst_data[i].f, "ハ") &&
  !check_feature(sp->bnst_data[i].f, "読点") &&
  !check_feature(sp->bnst_data[i].f, "用言") &&
  !check_feature(sp->bnst_data[i].f, "係:ガ格") &&
  !check_feature(sp->bnst_data[i].f, "用言:無") &&
  !check_feature(sp->bnst_data[i].f, "並キ") &&
  !check_feature(sp->bnst_data[i+1].f, "括弧始")) {
  strcat(PM_Memo, " 遠係");
  sprintf(PM_Memo+strlen(PM_Memo), "(%d)", i);
  }
  }
  */
}

/* get gigaword pa count for Chinese, for cell (i,j), i is the position of argument, j is the position of predicate */
/*==================================================================*/
            void calc_gigaword_pa_matrix(SENTENCE_DATA *sp)
/*==================================================================*/
{
  int i, j, dis;
    
  for (i = 0; i < sp->Bnst_num; i++) {
    for (j = 0; j < sp->Bnst_num; j++) {
      Chi_pa_matrix[i][j] = 0;
    }
  }

  for (i = 0; i < sp->Bnst_num; i++) {
    for (j = i + 1; j < sp->Bnst_num; j++) {
      if (check_feature(sp->bnst_data[i].f, "PU") || check_feature(sp->bnst_data[j].f, "PU")) {
	Chi_pa_matrix[i][j] = 0;
	Chi_pa_matrix[j][i] = 0;
      }
      else {
	if (j == i + 1) {
	  dis = 1;
	}
	else {
	  dis = 2;
	}

	Chi_pa_matrix[i][j] = get_chi_pa(sp->bnst_data+i, sp->bnst_data+j, dis);
	Chi_pa_matrix[j][i] = get_chi_pa(sp->bnst_data+j, sp->bnst_data+i, dis);
      }
    }
  }
}

/*==================================================================*/
double get_case_prob(SENTENCE_DATA *sp, int head, int left_arg_num, int right_arg_num)
/*==================================================================*/
{
  int i, j, k;
  double case_prob = 0.0;
  char *lex_key, *bk_key; 
  char *left_arg_key, *right_arg_key;
  char *tmp_key;
  int left_arg_len = 0, right_arg_len = 0;
  char *lex_rule, *bk_rule; 
  double lex_occur[2], lex_total[2], bk_occur[2], bk_total[2];
  double lamda;
  double prob[2];
  char *rule, *occur, *total, *type;
  char *curRule[2];
  int count;
  double lamda_ctb, lamda_giga;
    
  //printf("\nhead:%d left:", head);
  if (OptChiGenerative && CHICaseExist == FALSE) {
    return case_prob;
  }

  if (left_arg_num >= CHI_ARG_NUM_MAX || right_arg_num >= CHI_ARG_NUM_MAX) {
    fprintf(stderr, ";;; number of arguments exceeded maximum\n");
    return case_prob;
  }

  if (left_arg_num > 0) {
    for (i = 0; i < left_arg_num; i++) {
      if (left_arg[i] == -1) {
	break;
      }
      left_arg_len += (strlen((sp->bnst_data+left_arg[i])->head_ptr->Type) + 1);
    }
  }
  else {
    left_arg_len = 4;
  }

  left_arg_key = (char *)malloc(sizeof(char) * (left_arg_len + 1));

  if (left_arg_num > 0) {
    for (i = 0; i < left_arg_num; i++) {
      if (left_arg[i] == -1) {
	break;
      }
      //printf("%d,", left_arg[i]);
      if (i == left_arg_num - 1) {
	tmp_key = (char *)malloc(sizeof(char) * strlen((sp->bnst_data+left_arg[i])->head_ptr->Type + 2));
	sprintf(tmp_key, "%s", (sp->bnst_data+left_arg[i])->head_ptr->Type);
      }
      else {
	tmp_key = (char *)malloc(sizeof(char) * strlen((sp->bnst_data+left_arg[i])->head_ptr->Type + 1));
	sprintf(tmp_key, "%s_", (sp->bnst_data+left_arg[i])->head_ptr->Type);
      }
      if (i == 0) {
	strcpy(left_arg_key, tmp_key);
      }
      else {
	strcat(left_arg_key, tmp_key);
      }
      if (tmp_key) {
	free(tmp_key);
	tmp_key = NULL;
      }
    }
  }
  else {
    strcpy(left_arg_key, "NULL");
  }
    
  //printf(" right:");
  if (right_arg_num > 0) {
    for (i = right_arg_num - 1; i >= 0; i--) {
      if (right_arg[i] == -1) {
	break;
      }
      right_arg_len += (strlen((sp->bnst_data+right_arg[i])->head_ptr->Type) + 1);
    }
  }
  else {
    right_arg_len = 4;
  }

  right_arg_key = (char *)malloc(sizeof(char) * (right_arg_len + 1));

  if (right_arg_num > 0) {
    for (i = right_arg_num - 1; i >= 0; i--) {
      if (right_arg[i] == -1) {
	break;
      }
      //printf("%d,", right_arg[i]);
      if (i == 0) {
	tmp_key = (char *)malloc(sizeof(char) * strlen((sp->bnst_data+right_arg[i])->head_ptr->Type + 2));
	sprintf(tmp_key, "%s", (sp->bnst_data+right_arg[i])->head_ptr->Type);
      }
      else {
	tmp_key = (char *)malloc(sizeof(char) * strlen((sp->bnst_data+right_arg[i])->head_ptr->Type + 1));
	sprintf(tmp_key, "%s_", (sp->bnst_data+right_arg[i])->head_ptr->Type);
      }
      if (i == right_arg_num - 1) {
	strcpy(right_arg_key, tmp_key);
      }
      else {
	strcat(right_arg_key, tmp_key);
      }
      if (tmp_key) {
	free(tmp_key);
	tmp_key = NULL;
      }
    }
  }
  else {
    strcpy(right_arg_key, "NULL");
  }

  /* get lex rule */
  lex_key = (char *)malloc(sizeof(char) * (left_arg_len + right_arg_len + strlen((sp->bnst_data+head)->head_ptr->Goi) + strlen((sp->bnst_data+head)->head_ptr->Type) + 9));
  sprintf(lex_key, "(%s_%s)_(%s)_(%s)", (sp->bnst_data+head)->head_ptr->Type, (sp->bnst_data+head)->head_ptr->Goi, left_arg_key, right_arg_key);
  lex_rule = db_get(chi_case_db, lex_key);

  //printf("\nlex_key: %s", lex_key);
  //printf("\nlex_rule: %s", lex_rule);

  /* get bk rule */
  bk_key = (char *)malloc(sizeof(char) * (left_arg_len + right_arg_len + strlen((sp->bnst_data+head)->head_ptr->Type) + 11));
  sprintf(bk_key, "(%s_XX)_(%s)_(%s)", (sp->bnst_data+head)->head_ptr->Type, left_arg_key, right_arg_key);
  bk_rule = db_get(chi_case_db, bk_key);

  //printf("\nbk_key: %s", bk_key);
  //printf("\nbk_rule: %s\n", bk_rule);

  for (k = 0; k < 2; k++) {
    lex_occur[k] = 0;
    lex_total[k] = 0;
    bk_occur[k] = 0;
    bk_total[k] = 0;
  }

  if (lex_rule != NULL) {
    count = 0;
    rule = NULL;
    rule = strtok(lex_rule, ":");
    while (rule) {
      curRule[count] = (char *)malloc(sizeof(char) * (strlen(rule) + 1));
      strcpy(curRule[count], rule);
      count++;
      rule = NULL;
      rule = strtok(NULL, ":");
    }

    for (k = 0; k < count; k++) {
      occur = NULL;
      total = NULL;
      type = NULL;
			    
      occur = strtok(curRule[k], "_");
      total = strtok(NULL, "_");
      type = strtok(NULL, "_");

      if (!strcmp(type, "TRAIN")) {
	lex_occur[0] = atof(occur);
	lex_total[0] = atof(total);
      }
      else if (!strcmp(type, "GIGA")) {
	lex_occur[1] = atof(occur);
	lex_total[1] = atof(total);
      }
      if (curRule[k]) {
	free(curRule[k]);
	curRule[k] = NULL;
      }
    }
  }

  if (bk_rule != NULL) {
    count = 0;
    rule = NULL;
    rule = strtok(bk_rule, ":");
    while (rule) {
      curRule[count] = (char *)malloc(sizeof(char) * (strlen(rule) + 1));
      strcpy(curRule[count], rule);
      count++;
      rule = NULL;
      rule = strtok(NULL, ":");
    }

    for (k = 0; k < count; k++) {
      occur = NULL;
      total = NULL;
      type = NULL;
			    
      occur = strtok(curRule[k], "_");
      total = strtok(NULL, "_");
      type = strtok(NULL, "_");

      if (!strcmp(type, "TRAIN")) {
	bk_occur[0] = atof(occur);
	bk_total[0] = atof(total);
      }
      else if (!strcmp(type, "GIGA")) {
	bk_occur[1] = atof(occur);
	bk_total[1] = atof(total);
      }
      if (curRule[k]) {
	free(curRule[k]);
	curRule[k] = NULL;
      }
    }
  }
    
  for (k = 0; k < 2; k++) {
    prob[k] = 0.0;
    if (lex_total[k] > DOUBLE_MIN) {
      lamda = lex_occur[k] / (lex_occur[k] + 1);
      prob[k] = lamda * (lex_occur[k] / lex_total[k]);
      prob[k] += (1 - lamda) * (bk_occur[k] / bk_total[k]);
    }
    else if (bk_total[k] > DOUBLE_MIN) {
      lamda = bk_occur[k] / (bk_occur[k] + 1);
      prob[k] = lamda * bk_occur[k] / bk_total[k];
    }
  }


  if (prob[0] > DOUBLE_MIN) {
    if (prob[1] > DOUBLE_MIN) {
      //	lamda = log(lex_occur[0] + bk_occur[0]) / (log(lex_occur[0] + bk_occur[0]) + 1);
      lamda_ctb = (lex_occur[0] + bk_occur[0]) / (lex_occur[0] + bk_occur[0] + 1);
      lamda_giga = (lex_occur[1] + bk_occur[1]) / (lex_occur[1] + bk_occur[1] + 1);
      lamda = lamda_ctb < lamda_giga ? lamda_ctb/lamda_giga : lamda_giga/lamda_ctb;
      case_prob = prob[0] * lamda + prob[1] * (1 - lamda);
    }
    else {
      case_prob = prob[0];
    }
  }
  else {
    if (prob[1] > DOUBLE_MIN) {
      //	lamda = (lex_occur[1] + bk_occur[1]) / (lex_occur[1] + bk_occur[1] + 1);
      lamda = log(lex_occur[1] + bk_occur[1]) / (log(lex_occur[1] + bk_occur[1]) + 1);
      case_prob = prob[1] * lamda;
    }
    else {
      case_prob = 0;
    }
  }

  //printf("prob_ctb: %f, prob_giga: %f, lamda: %f", prob[0], prob[1], lamda);
  //printf("prob: %f\n\n", case_prob);

  if (left_arg_key) {
    free(left_arg_key);
    left_arg_key = NULL;
  }
  if (right_arg_key) {
    free(right_arg_key);
    right_arg_key = NULL;
  }
  if (lex_key) {
    free(lex_key);
    lex_key = NULL;
  }
  if (bk_key) {
    free(bk_key);
    bk_key = NULL;
  }
  if (lex_rule) {
    free(lex_rule);
    lex_rule = NULL;
  }
  if (bk_rule) {
    free(bk_rule);
    bk_rule = NULL;
  }

  return case_prob;
}

/*==================================================================*/
double get_case_prob_wpos(SENTENCE_DATA *sp, int head, int left_arg_num, int right_arg_num, int pos_index_pre)
     /*==================================================================*/
{
  int i, j, k;
  double case_prob = 0.0;
  char *bk_key; 
  char *left_arg_key, *right_arg_key;
  char *tmp_key;
  int left_arg_len = 0, right_arg_len = 0;
  char *bk_rule; 
  double bk_occur[2], bk_total[2];
  double lamda;
  double prob[2];
  char *rule, *occur, *total, *type;
  char *curRule[2];
  int count;
  double lamda_ctb, lamda_giga;
    
  //printf("\nhead:%d left:", head);
  if (OptChiGenerative && CHICaseExist == FALSE) {
    return case_prob;
  }

  if (left_arg_num >= CHI_ARG_NUM_MAX || right_arg_num >= CHI_ARG_NUM_MAX) {
    fprintf(stderr, ";;; number of arguments exceeded maximum\n");
    return case_prob;
  }

  if (left_arg_num > 0) {
    for (i = 0; i < left_arg_num; i++) {
      if (left_arg[i] == -1) {
	break;
      }
      left_arg_len += (strlen(Chi_word_type[left_arg[i]]) + 1);
    }
  }
  else {
    left_arg_len = 4;
  }

  left_arg_key = (char *)malloc(sizeof(char) * (left_arg_len + 1));

  if (left_arg_num > 0) {
    for (i = 0; i < left_arg_num; i++) {
      if (left_arg[i] == -1) {
	break;
      }
      //printf("%d,", left_arg[i]);
      if (i == left_arg_num - 1) {
	tmp_key = (char *)malloc(sizeof(char) * strlen(Chi_word_type[left_arg[i]] + 2));
	sprintf(tmp_key, "%s", Chi_word_type[left_arg[i]]);
      }
      else {
	tmp_key = (char *)malloc(sizeof(char) * strlen(Chi_word_type[left_arg[i]] + 1));
	sprintf(tmp_key, "%s_", Chi_word_type[left_arg[i]]);
      }
      if (i == 0) {
	strcpy(left_arg_key, tmp_key);
      }
      else {
	strcat(left_arg_key, tmp_key);
      }
      if (tmp_key) {
	free(tmp_key);
	tmp_key = NULL;
      }
    }
  }
  else {
    strcpy(left_arg_key, "NULL");
  }
    
  //printf(" right:");
  if (right_arg_num > 0) {
    for (i = right_arg_num - 1; i >= 0; i--) {
      if (right_arg[i] == -1) {
	break;
      }
      right_arg_len += (strlen(Chi_word_type[right_arg[i]]) + 1);
    }
  }
  else {
    right_arg_len = 4;
  }

  right_arg_key = (char *)malloc(sizeof(char) * (right_arg_len + 1));

  if (right_arg_num > 0) {
    for (i = right_arg_num - 1; i >= 0; i--) {
      if (right_arg[i] == -1) {
	break;
      }
      //printf("%d,", right_arg[i]);
      if (i == 0) {
	tmp_key = (char *)malloc(sizeof(char) * strlen(Chi_word_type[right_arg[i]] + 2));
	sprintf(tmp_key, "%s", Chi_word_type[right_arg[i]]);
      }
      else {
	tmp_key = (char *)malloc(sizeof(char) * strlen(Chi_word_type[right_arg[i]] + 1));
	sprintf(tmp_key, "%s_", Chi_word_type[right_arg[i]]);
      }
      if (i == right_arg_num - 1) {
	strcpy(right_arg_key, tmp_key);
      }
      else {
	strcat(right_arg_key, tmp_key);
      }
      if (tmp_key) {
	free(tmp_key);
	tmp_key = NULL;
      }
    }
  }
  else {
    strcpy(right_arg_key, "NULL");
  }

  /* get bk rule */
  bk_key = (char *)malloc(sizeof(char) * (left_arg_len + right_arg_len + strlen(Chi_word_type[pos_index_pre]) + 11));
  sprintf(bk_key, "(%s_XX)_(%s)_(%s)", Chi_word_type[pos_index_pre], left_arg_key, right_arg_key);
  bk_rule = db_get(chi_case_db, bk_key);

  //printf("\nbk_key: %s", bk_key);
  //printf("\nbk_rule: %s\n", bk_rule);

  for (k = 0; k < 2; k++) {
    bk_occur[k] = 0;
    bk_total[k] = 0;
  }

  if (bk_rule != NULL) {
    count = 0;
    rule = NULL;
    rule = strtok(bk_rule, ":");
    while (rule) {
      curRule[count] = (char *)malloc(sizeof(char) * (strlen(rule) + 1));
      strcpy(curRule[count], rule);
      count++;
      rule = NULL;
      rule = strtok(NULL, ":");
    }

    for (k = 0; k < count; k++) {
      occur = NULL;
      total = NULL;
      type = NULL;
			    
      occur = strtok(curRule[k], "_");
      total = strtok(NULL, "_");
      type = strtok(NULL, "_");

      if (!strcmp(type, "TRAIN")) {
	bk_occur[0] = atof(occur);
	bk_total[0] = atof(total);
      }
      else if (!strcmp(type, "GIGA")) {
	bk_occur[1] = atof(occur);
	bk_total[1] = atof(total);
      }
      if (curRule[k]) {
	free(curRule[k]);
	curRule[k] = NULL;
      }
    }
  }
    
  for (k = 0; k < 2; k++) {
    prob[k] = 0.0;
    if (bk_total[k] > DOUBLE_MIN) {
      lamda = bk_occur[k] / (bk_occur[k] + 1);
      prob[k] = lamda * bk_occur[k] / bk_total[k];
    }
  }


  if (prob[0] > DOUBLE_MIN) {
    if (prob[1] > DOUBLE_MIN) {
      lamda_ctb = bk_occur[0] / (bk_occur[0] + 1);
      lamda_giga = bk_occur[1] / (bk_occur[1] + 1);
      lamda = lamda_ctb < lamda_giga ? lamda_ctb/lamda_giga : lamda_giga/lamda_ctb;
      case_prob = prob[0] * lamda + prob[1] * (1 - lamda);
    }
    else {
      case_prob = prob[0];
    }
  }
  else {
    if (prob[1] > DOUBLE_MIN) {
      lamda = log(bk_occur[1]) / (log(bk_occur[1]) + 1);
      case_prob = prob[1] * lamda;
    }
    else {
      case_prob = 0;
    }
  }

  //printf("prob_ctb: %f, prob_giga: %f, lamda: %f", prob[0], prob[1], lamda);
  //printf("prob: %f\n\n", case_prob);

  if (left_arg_key) {
    free(left_arg_key);
    left_arg_key = NULL;
  }
  if (right_arg_key) {
    free(right_arg_key);
    right_arg_key = NULL;
  }
  if (bk_key) {
    free(bk_key);
    bk_key = NULL;
  }
  if (bk_rule) {
    free(bk_rule);
    bk_rule = NULL;
  }

  return case_prob;
}

/*==================================================================*/
double get_lex_case_prob_wpos(SENTENCE_DATA *sp, int head, int left_arg_num, int right_arg_num, int pos_index_pre)
     /*==================================================================*/
{
  int i, j, k;
  double case_prob = 0.0;
  char *lex_key; 
  char *left_arg_key, *right_arg_key;
  char *tmp_key;
  int left_arg_len = 0, right_arg_len = 0;
  char *lex_rule;
  double lex_occur[2], lex_total[2];
  double lamda;
  double prob[2];
  char *rule, *occur, *total, *type;
  char *curRule[2];
  int count;
  double lamda_ctb, lamda_giga;
    
  //printf("\nhead:%d left:", head);
  if (OptChiGenerative && CHICaseExist == FALSE) {
    return case_prob;
  }

  if (left_arg_num >= CHI_ARG_NUM_MAX || right_arg_num >= CHI_ARG_NUM_MAX) {
    fprintf(stderr, ";;; number of arguments exceeded maximum\n");
    return case_prob;
  }

  if (left_arg_num > 0) {
    for (i = 0; i < left_arg_num; i++) {
      if (left_arg[i] == -1) {
	break;
      }
      left_arg_len += (strlen(Chi_word_type[left_arg[i]]) + 1);
    }
  }
  else {
    left_arg_len = 4;
  }

  left_arg_key = (char *)malloc(sizeof(char) * (left_arg_len + 1));

  if (left_arg_num > 0) {
    for (i = 0; i < left_arg_num; i++) {
      if (left_arg[i] == -1) {
	break;
      }
      //printf("%d,", left_arg[i]);
      if (i == left_arg_num - 1) {
	tmp_key = (char *)malloc(sizeof(char) * strlen(Chi_word_type[left_arg[i]] + 2));
	sprintf(tmp_key, "%s", Chi_word_type[left_arg[i]]);
      }
      else {
	tmp_key = (char *)malloc(sizeof(char) * strlen(Chi_word_type[left_arg[i]] + 1));
	sprintf(tmp_key, "%s_", Chi_word_type[left_arg[i]]);
      }
      if (i == 0) {
	strcpy(left_arg_key, tmp_key);
      }
      else {
	strcat(left_arg_key, tmp_key);
      }
      if (tmp_key) {
	free(tmp_key);
	tmp_key = NULL;
      }
    }
  }
  else {
    strcpy(left_arg_key, "NULL");
  }
    
  //printf(" right:");
  if (right_arg_num > 0) {
    for (i = right_arg_num - 1; i >= 0; i--) {
      if (right_arg[i] == -1) {
	break;
      }
      right_arg_len += (strlen(Chi_word_type[right_arg[i]]) + 1);
    }
  }
  else {
    right_arg_len = 4;
  }

  right_arg_key = (char *)malloc(sizeof(char) * (right_arg_len + 1));

  if (right_arg_num > 0) {
    for (i = right_arg_num - 1; i >= 0; i--) {
      if (right_arg[i] == -1) {
	break;
      }
      //printf("%d,", right_arg[i]);
      if (i == 0) {
	tmp_key = (char *)malloc(sizeof(char) * strlen(Chi_word_type[right_arg[i]] + 2));
	sprintf(tmp_key, "%s", Chi_word_type[right_arg[i]]);
      }
      else {
	tmp_key = (char *)malloc(sizeof(char) * strlen(Chi_word_type[right_arg[i]] + 1));
	sprintf(tmp_key, "%s_", Chi_word_type[right_arg[i]]);
      }
      if (i == right_arg_num - 1) {
	strcpy(right_arg_key, tmp_key);
      }
      else {
	strcat(right_arg_key, tmp_key);
      }
      if (tmp_key) {
	free(tmp_key);
	tmp_key = NULL;
      }
    }
  }
  else {
    strcpy(right_arg_key, "NULL");
  }

  /* get lex rule */
  lex_key = (char *)malloc(sizeof(char) * (left_arg_len + right_arg_len + strlen((sp->bnst_data+head)->head_ptr->Goi) + strlen(Chi_word_type[pos_index_pre]) + 9));
  sprintf(lex_key, "(%s_%s)_(%s)_(%s)", Chi_word_type[pos_index_pre], (sp->bnst_data+head)->head_ptr->Goi, left_arg_key, right_arg_key);
  lex_rule = db_get(chi_case_db, lex_key);

  //printf("\nlex_key: %s", lex_key);
  //printf("\nlex_rule: %s", lex_rule);

  for (k = 0; k < 2; k++) {
    lex_occur[k] = 0;
    lex_total[k] = 0;
  }

  if (lex_rule != NULL) {
    count = 0;
    rule = NULL;
    rule = strtok(lex_rule, ":");
    while (rule) {
      curRule[count] = (char *)malloc(sizeof(char) * (strlen(rule) + 1));
      strcpy(curRule[count], rule);
      count++;
      rule = NULL;
      rule = strtok(NULL, ":");
    }

    for (k = 0; k < count; k++) {
      occur = NULL;
      total = NULL;
      type = NULL;
			    
      occur = strtok(curRule[k], "_");
      total = strtok(NULL, "_");
      type = strtok(NULL, "_");

      if (!strcmp(type, "TRAIN")) {
	lex_occur[0] = atof(occur);
	lex_total[0] = atof(total);
      }
      else if (!strcmp(type, "GIGA")) {
	lex_occur[1] = atof(occur);
	lex_total[1] = atof(total);
      }
      if (curRule[k]) {
	free(curRule[k]);
	curRule[k] = NULL;
      }
    }
  }

  for (k = 0; k < 2; k++) {
    prob[k] = 0.0;
    if (lex_total[k] > DOUBLE_MIN) {
      lamda = lex_occur[k] / (lex_occur[k] + 1);
      prob[k] = lamda * (lex_occur[k] / lex_total[k]);
      prob[k] += (1 - lamda) * (lex_occur[k] / lex_total[k]);
    }
  }

  if (prob[0] > DOUBLE_MIN) {
    if (prob[1] > DOUBLE_MIN) {
      lamda_ctb = lex_occur[0] / (lex_occur[0] + 1);
      lamda_giga = lex_occur[1] / (lex_occur[1] + 1);
      lamda = lamda_ctb < lamda_giga ? lamda_ctb/lamda_giga : lamda_giga/lamda_ctb;
      case_prob = prob[0] * lamda + prob[1] * (1 - lamda);
    }
    else {
      case_prob = prob[0];
    }
  }
  else {
    if (prob[1] > DOUBLE_MIN) {
      lamda = log(lex_occur[1]) / (log(lex_occur[1]) + 1);
      case_prob = prob[1] * lamda;
    }
    else {
      case_prob = 0;
    }
  }

  //printf("prob_ctb: %f, prob_giga: %f, lamda: %f", prob[0], prob[1], lamda);
  //printf("prob: %f\n\n", case_prob);

  if (left_arg_key) {
    free(left_arg_key);
    left_arg_key = NULL;
  }
  if (right_arg_key) {
    free(right_arg_key);
    right_arg_key = NULL;
  }
  if (lex_key) {
    free(lex_key);
    lex_key = NULL;
  }
  if (lex_rule) {
    free(lex_rule);
    lex_rule = NULL;
  }

  return case_prob;
}

/*====================================================================
                                  END
====================================================================*/
