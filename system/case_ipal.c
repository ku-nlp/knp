/*====================================================================

		       格構造解析: 格フレーム側

                                               S.Kurohashi 93. 5.31

    $Id$
====================================================================*/
#include "knp.h"

FILE *cf_fp;
DBM_FILE cf_db;
FILE *cf_noun_fp;
DBM_FILE cf_noun_db;
DBM_FILE cf_sim_db;

CASE_FRAME 	*Case_frame_array = NULL; 	/* 格フレーム */
int 	   	Case_frame_num;			/* 格フレーム数 */
int 	   	MAX_Case_frame_num = 0;		/* 最大格フレーム数 */

CF_FRAME CF_frame;
int MAX_cf_frame_length = 0;
unsigned char *cf_str_buf;

int	CFExist;
int	CFNounExist;
int	CFSimExist;
int	PrintDeletedSM = 0;

/*==================================================================*/
	   void init_cf_structure(CASE_FRAME *p, int size)
/*==================================================================*/
{
    memset(p, 0, sizeof(CASE_FRAME)*size);
}

/*==================================================================*/
			  void realloc_cf()
/*==================================================================*/
{
    Case_frame_array = (CASE_FRAME *)realloc_data(Case_frame_array, 
						  sizeof(CASE_FRAME)*(MAX_Case_frame_num+ALLOCATION_STEP), 
						  "realloc_cf");
    init_cf_structure(Case_frame_array+MAX_Case_frame_num, ALLOCATION_STEP);
    MAX_Case_frame_num += ALLOCATION_STEP;
}

/*==================================================================*/
			    void init_cf()
/*==================================================================*/
{
    char *index_db_filename, *data_filename;

    if (DICT[CF_DATA]) {
	data_filename = check_dict_filename(DICT[CF_DATA], TRUE);
    }
    else {
	data_filename = check_dict_filename(CF_DAT_NAME, FALSE);
    }

    if (DICT[CF_INDEX_DB]) {
	index_db_filename = check_dict_filename(DICT[CF_INDEX_DB], TRUE);
    }
    else {
	index_db_filename = check_dict_filename(CF_DB_NAME, FALSE);
    }

    if (OptDisplay == OPT_DEBUG) {
	fprintf(Outfp, "Opening %s ... ", data_filename);
    }

    if ((cf_fp = fopen(data_filename, "rb")) == NULL) {
	if (OptDisplay == OPT_DEBUG) {
	    fputs("failed.\n", Outfp);
	}
#ifdef DEBUG
	fprintf(stderr, ";; Cannot open CF DATA <%s>.\n", data_filename);
#endif
	CFExist = FALSE;
    }
    else if ((cf_db = DB_open(index_db_filename, O_RDONLY, 0)) == NULL) {
	if (OptDisplay == OPT_DEBUG) {
	    fprintf(Outfp, "done.\nOpening %s ... failed.\n", index_db_filename);
	}
	fprintf(stderr, ";; Cannot open CF INDEX Database <%s>.\n", index_db_filename);
	/* 格フレーム DATA は読めるのに、DB が読めないときは終わる */
	exit(1);
    } 
    else {
	if (OptDisplay == OPT_DEBUG) {
	    fprintf(Outfp, "done.\nOpening %s ... done.\n", index_db_filename);
	}
	CFExist = TRUE;
    }

    free(data_filename);
    free(index_db_filename);

    /* 格フレーム類似度DB */

    if (DICT[CF_SIM_DB]) {
	index_db_filename = check_dict_filename(DICT[CF_SIM_DB], TRUE);
    }
    else {
	index_db_filename = check_dict_filename(CF_SIM_DB_NAME, FALSE);
    }

    if (OptDisplay == OPT_DEBUG) {
	fprintf(Outfp, "Opening %s ... ", index_db_filename);
    }

    if ((cf_sim_db = DB_open(index_db_filename, O_RDONLY, 0)) == NULL) {
	if (OptDisplay == OPT_DEBUG) {
	    fputs("failed.\n", Outfp);
	}
	CFSimExist = FALSE;
    } 
    else {
	if (OptDisplay == OPT_DEBUG) {
	    fputs("done.\n", Outfp);
	}
	CFSimExist = TRUE;
    }

    free(index_db_filename);
}

/*==================================================================*/
			 void init_noun_cf()
/*==================================================================*/
{
    char *index_db_filename, *data_filename;

    if (DICT[CF_NOUN_DATA]) {
	data_filename = check_dict_filename(DICT[CF_NOUN_DATA], TRUE);
    }
    else {
	data_filename = check_dict_filename(CF_NOUN_DAT_NAME, FALSE);
    }

    if (DICT[CF_NOUN_INDEX_DB]) {
	index_db_filename = check_dict_filename(DICT[CF_NOUN_INDEX_DB], TRUE);
    }
    else {
	index_db_filename = check_dict_filename(CF_NOUN_DB_NAME, FALSE);
    }

    if (OptDisplay == OPT_DEBUG) {
	fprintf(Outfp, "Opening %s ... ", data_filename);
    }

    if ((cf_noun_fp = fopen(data_filename, "rb")) == NULL) {
	if (OptDisplay == OPT_DEBUG) {
	    fputs("failed.\n", Outfp);
	}
#ifdef DEBUG
	fprintf(stderr, ";; Cannot open CF(noun) DATA <%s>.\n", data_filename);
#endif
	CFNounExist = FALSE;
    }
    else if ((cf_noun_db = DB_open(index_db_filename, O_RDONLY, 0)) == NULL) {
	if (OptDisplay == OPT_DEBUG) {
	    fprintf(Outfp, "done.\nOpening %s ... failed.\n", index_db_filename);
	}
	fprintf(stderr, ";; Cannot open CF(noun) INDEX Database <%s>.\n", index_db_filename);
	/* 格フレーム DATA は読めるのに、DB が読めないときは終わる */
	exit(1);
    } 
    else {
	if (OptDisplay == OPT_DEBUG) {
	    fprintf(Outfp, "done.\nOpening %s ... done.\n", index_db_filename);
	}
	CFNounExist = TRUE;
    }

    free(data_filename);
    free(index_db_filename);
}

/*==================================================================*/
		 void clear_mgr_cf(SENTENCE_DATA *sp)
/*==================================================================*/
{
    int i, j;

    for (i = 0; i < CPM_MAX; i++) {
	for (j = 0; j < CF_ELEMENT_MAX; j++) {
	    free(sp->Best_mgr->cpm[i].cf.ex[j]);
	    sp->Best_mgr->cpm[i].cf.ex[j] = NULL;

	    free(sp->Best_mgr->cpm[i].cf.sm[j]);
	    sp->Best_mgr->cpm[i].cf.sm[j] = NULL;

	    free(sp->Best_mgr->cpm[i].cf.ex_list[j][0]);
	    free(sp->Best_mgr->cpm[i].cf.ex_list[j]);
	    free(sp->Best_mgr->cpm[i].cf.ex_freq[j]);
	}
    }
}

/*==================================================================*/
		   void init_mgr_cf(TOTAL_MGR *tmp)
/*==================================================================*/
{
    int i;

    for (i = 0; i < CPM_MAX; i++) {
	init_case_frame(&tmp->cpm[i].cf);
    }
}

/*==================================================================*/
	    void init_case_analysis_cpm(SENTENCE_DATA *sp)
/*==================================================================*/
{
    if (OptAnalysis == OPT_CASE || 
	OptAnalysis == OPT_CASE2) {

	/* 格フレーム領域確保 */
	Case_frame_array = (CASE_FRAME *)malloc_data(sizeof(CASE_FRAME)*ALL_CASE_FRAME_MAX, 
						     "init_case_analysis_cpm");
	MAX_Case_frame_num = ALL_CASE_FRAME_MAX;
	init_cf_structure(Case_frame_array, MAX_Case_frame_num);

	/* Best_mgrのcpm領域確保 */
	init_mgr_cf(sp->Best_mgr);

	/* 名詞-意味素HASHの初期化 */
	memset(smlist, 0, sizeof(SMLIST)*TBLSIZE);
    }
}

/*==================================================================*/
			   void close_cf()
/*==================================================================*/
{
    if (CFExist == TRUE) {
	fclose(cf_fp);
	DB_close(cf_db);
    }
}

/*==================================================================*/
			 void close_noun_cf()
/*==================================================================*/
{
    if (CFNounExist == TRUE) {
	fclose(cf_noun_fp);
	DB_close(cf_noun_db);
    }
}

/*==================================================================*/
	char *get_ipal_address(unsigned char *word, int flag)
/*==================================================================*/
{
    if (flag == CF_PRED) {
	if (CFExist == FALSE) {
	    return NULL;
	}

	return db_get(cf_db, word);
    }
    else {
	if (CFNounExist == FALSE) {
	    return NULL;
	}

	return db_get(cf_noun_db, word);
    }
}

/*==================================================================*/
      CF_FRAME *get_ipal_frame(int address, int size, int flag)
/*==================================================================*/
{
    int i, c1, c2, count = 0;
    char *cp;
    FILE *fp;

    if (flag == CF_PRED) {
	fp = cf_fp;
    }
    else {
	fp = cf_noun_fp;
    }

    if (size > MAX_cf_frame_length) {
	MAX_cf_frame_length += ALLOCATION_STEP*((size-MAX_cf_frame_length)/ALLOCATION_STEP+1);
	CF_frame.DATA = 
	    (unsigned char *)realloc_data(CF_frame.DATA, 
					  sizeof(unsigned char)*MAX_cf_frame_length, 
					  "get_ipal_frame");
	cf_str_buf = 
	    (unsigned char *)realloc_data(cf_str_buf, 
					  sizeof(unsigned char)*MAX_cf_frame_length, 
					  "get_ipal_frame");
    }

    fseek(fp, (long)address, 0);
    if (fread(CF_frame.DATA, size, 1, fp) < 1) {
	fprintf(stderr, ";; Error in fread.\n");
	exit(1);
    }

    /* 読み, 表記, 素性を設定 */
    CF_frame.casenum = 0;
    for (i = 0; i < size-1; i++) {
	if (*(CF_frame.DATA+i) == '\0') {
	    if (count == 0)
		CF_frame.yomi = CF_frame.DATA+i+1;
	    else if (count == 1)
		CF_frame.hyoki = CF_frame.DATA+i+1;
	    else if (count == 2)
		CF_frame.feature = CF_frame.DATA+i+1;
	    else {
		if (count%3 == 0) {
		    CF_frame.cs[count/3-1].kaku_keishiki = CF_frame.DATA+i+1;
		    CF_frame.casenum++;
		    if (CF_frame.casenum > CASE_MAX_NUM) {
			fprintf(stderr, ";; # of cases is more than MAX (%d).\n", CASE_MAX_NUM);
		    }
		}
		else if (count%3 == 1)
		    CF_frame.cs[count/3-1].meishiku = CF_frame.DATA+i+1;
		else if (count%3 == 2)
		    CF_frame.cs[count/3-1].imisosei = CF_frame.DATA+i+1;
	    }
	    count++;
	}
    }

    count = 0;
    CF_frame.voice = 0;
    CF_frame.etcflag = CF_NORMAL;
    if (*CF_frame.feature) {
	char *string, *token, *buf;
	string = strdup(CF_frame.feature);
	token = strtok(string, " ");
	while (token) {
	    if (!strcmp(token, "和フレーム")) {
		CF_frame.etcflag |= CF_SUM;
	    }
	    else if (!strcmp(token, "格フレーム変化")) {
		CF_frame.etcflag |= CF_CHANGE;
	    }
	    else if (!strcmp(token, "ヲ使役")) {
		CF_frame.voice |= CF_CAUSATIVE_WO;
	    }
	    else if (!strcmp(token, "ニ使役")) {
		CF_frame.voice |= CF_CAUSATIVE_NI;
	    }
	    else if (!strcmp(token, "直受1")) {
		CF_frame.voice |= CF_PASSIVE_1;
	    }
	    else if (!strcmp(token, "直受2")) {
		CF_frame.voice |= CF_PASSIVE_2;
	    }
	    else if (!strcmp(token, "間受")) {
		CF_frame.voice |= CF_PASSIVE_I;
	    }
	    else if (!strcmp(token, "可能")) {
		CF_frame.voice |= CF_POSSIBLE;
	    }
	    else if (!strcmp(token, "尊敬")) {
		CF_frame.voice |= CF_POLITE;
	    }
	    else if (!strcmp(token, "自発")) {
		CF_frame.voice |= CF_SPONTANE;
		;
	    }
	    /* merged cases */
	    else if ((cp = strstr(token, "＝")) != NULL) {
		buf = strdup(token);
		cp = buf+(cp-token);
		*cp = '\0';
		/* if (!strncmp(buf+strlen(buf)-2, "格", 2)) *(buf+strlen(buf)-2) = '\0';
		if (!strncmp(cp+strlen(cp+2), "格", 2)) *(cp+strlen(cp+2)) = '\0'; */
		c1 = pp_kstr_to_code(buf);
		c2 = pp_kstr_to_code(cp+2);
		free(buf);

		if (c1 == END_M || c2 == END_M) {
		    fprintf(stderr, ";; Can't understand <%s> as merged cases\n", token);
		}
		/* 溢れチェック */
		else if (count >= CF_ELEMENT_MAX - 1) {
		    break;
		}
		else {
		    /* 数が小さい格を前に入れる */
		    if (c1 > c2) {
			CF_frame.samecase[count][0] = c2;
			CF_frame.samecase[count][1] = c1;
		    }
		    else {
			CF_frame.samecase[count][0] = c1;
			CF_frame.samecase[count][1] = c2;
		    }
		    count++;
		}
	    }
	    token = strtok(NULL, " ");
	}
	free(string);
    }

    CF_frame.samecase[count][0] = END_M;
    CF_frame.samecase[count][1] = END_M;

    if (cp = strchr(CF_frame.DATA, ':')) {
	strncpy(CF_frame.pred_type, cp+1, 2);
	CF_frame.pred_type[2] = '\0';
    }
    else {
	CF_frame.pred_type[0] = '\0';
    }

    return &CF_frame;
}

/*==================================================================*/
unsigned char *extract_ipal_str(unsigned char *dat, unsigned char *ret, int flag)
/*==================================================================*/
{
    int freq;

    /* flag == TRUE: 頻度付きで返す */

    if (*dat == '\0' || !strcmp(dat, "＊")) {
	return NULL;
    }

    while (1) {
	if (*dat == '\0') {
	    *ret = '\0';
	    return dat;
	}
	/* 頻度が記述してある場合 */
	else if (*dat == ':') {
	    if (flag) {
		*ret++ = *dat;
	    }
	    /* flag == FALSE: 頻度を返さない */
	    else {
		*ret++ = '\0';	/* ':' -> '\0' */
	    }
	    dat++;
	}
	/* 空白でも切る */
	else if (*dat == ' ') {
	    *ret = '\0';
	    return dat+1;
	}
	else if (*dat < 0x80) {
	    *ret++ = *dat++;
	}
	else if (!strncmp(dat, "／", 2) || 
		 !strncmp(dat, "，", 2) ||
		 !strncmp(dat, "、", 2) ||
		 !strncmp(dat, "＊", 2)) {
	    *ret = '\0';
	    return dat+2;
	}
	else {
	    *ret++ = *dat++;
	    *ret++ = *dat++;
	}
    }
}

/*==================================================================*/
int _make_ipal_cframe_pp(CASE_FRAME *c_ptr, unsigned char *cp, int num, int flag)
/*==================================================================*/
{
    /* 助詞の読みだし */

    unsigned char *point;
    int pp_num = 0;

    /* 直前格 */
    if (*(cp+strlen(cp)-1) == '*') {
	c_ptr->adjacent[num] = TRUE;
	*(cp+strlen(cp)-1) = '\0';
    }
    else if (!strcmp(cp+strlen(cp)-2, "＠")) {
	c_ptr->adjacent[num] = TRUE;
	*(cp+strlen(cp)-2) = '\0';
    }

    /* 任意格 */
    if (!strcmp(cp+strlen(cp)-2, "＊")) {
	c_ptr->oblig[num] = FALSE;
    }
    else {
	/* 未決定 (ひとつめの格を見て決める) */
	c_ptr->oblig[num] = END_M;
    }

    point = cp; 
    while ((point = extract_ipal_str(point, cf_str_buf, FALSE))) {
	if (pp_num == 0 && c_ptr->oblig[num] == END_M) {
	    if (str_eq(cf_str_buf, "ガ") || 
		str_eq(cf_str_buf, "ヲ") || 
		str_eq(cf_str_buf, "ニ") || 
		str_eq(cf_str_buf, "ヘ") || 
		str_eq(cf_str_buf, "ヨリ")) {
		c_ptr->oblig[num] = TRUE;
	    }
	    else {
		c_ptr->oblig[num] = FALSE;
	    }
	}

	if (flag == CF_PRED) {
	    c_ptr->pp[num][pp_num] = pp_kstr_to_code(cf_str_buf);
	    if (c_ptr->pp[num][pp_num] == END_M) {
		fprintf(stderr, ";; Unknown case (%s) in PP!\n", cf_str_buf);
		exit(1);
	    }
	}
	else {
	    c_ptr->pp[num][pp_num] = 0;
	    c_ptr->pp_str[num] = strdup(cf_str_buf);
	    c_ptr->oblig[num] = TRUE;
	}

	pp_num++;
    }

    c_ptr->pp[num][pp_num] = END_M;
    return TRUE;
}
    
/*==================================================================*/
void _make_ipal_cframe_sm(CASE_FRAME *c_ptr, unsigned char *cp, int num, int flag)
/*==================================================================*/
{
    /* 意味マーカの読みだし */

    unsigned char *point;
    int size, sm_num = 0, sm_print_num = 0, mlength, sm_delete_sm_max = 0, sm_specify_sm_max = 0;
    char buf[SM_ELEMENT_MAX*SM_CODE_SIZE], *sm_delete_sm = NULL, *sm_specify_sm = NULL, *temp, *str;

    if (*cp == '\0') {
	return;
    }

    if (flag & USE_BGH) {
	size = BGH_CODE_SIZE;
    }
    else if (flag & USE_NTT) {
	size = SM_CODE_SIZE;
    }

    str = strdup(cp);
    *str = '\0';
    point = cp;
    buf[0] = '\0';
    while ((point = extract_ipal_str(point, cf_str_buf, FALSE))) {
	/* 意味素制限 (NTT) */
        if (cf_str_buf[0] == '+') {
	    if (Thesaurus == USE_BGH) continue;
	    if (c_ptr->sm_specify[num] == NULL) {
		c_ptr->sm_specify_size[num] = SM_ELEMENT_MAX;
		c_ptr->sm_specify[num] = (char *)malloc_data(
		    sizeof(char)*c_ptr->sm_specify_size[num]*SM_CODE_SIZE+1, 
		    "_make_ipal_cframe_sm");
		*c_ptr->sm_specify[num] = '\0';

		sm_specify_sm_max = sizeof(char)*ALLOCATION_STEP;
		sm_specify_sm = (char *)malloc_data(sm_specify_sm_max, 
						   "_make_ipal_cframe_sm");
		strcpy(sm_specify_sm, "意味素制限:");
	    }
	    else if (c_ptr->sm_specify_num[num] >= c_ptr->sm_specify_size[num]) {
		c_ptr->sm_specify[num] = (char *)realloc_data(c_ptr->sm_specify[num], 
		    sizeof(char)*(c_ptr->sm_specify_size[num] <<= 1)*SM_CODE_SIZE+1, 
		    "_make_ipal_cframe_sm");
	    }

	    if (cf_str_buf[1] == '1') {
		strcat(c_ptr->sm_specify[num], &cf_str_buf[1]);
		temp = code2sm(&cf_str_buf[1]);
		/* -1 ではないのは '/' の分 */
		if (strlen(sm_specify_sm)+strlen(temp) > sm_specify_sm_max-2) {
		    sm_specify_sm = (char *)realloc_data(sm_specify_sm, 
							 sm_specify_sm_max <<= 1, 
							 "_make_ipal_cframe_sm");
		}
		strcat(sm_specify_sm, "/");
		strcat(sm_specify_sm, temp);
	    }
	    else {
		strcat(c_ptr->sm_specify[num], sm2code(&cf_str_buf[1]));
	    }
	    c_ptr->sm_specify_num[num]++;
	    continue;
	}
	/* 使ってはいけない意味素 (NTT) */
        else if (cf_str_buf[0] == '-') {
	    if (Thesaurus == USE_BGH) continue;
	    if (c_ptr->sm_delete[num] == NULL) {
		c_ptr->sm_delete_size[num] = SM_ELEMENT_MAX;
		c_ptr->sm_delete[num] = (char *)malloc_data(
		    sizeof(char)*c_ptr->sm_delete_size[num]*SM_CODE_SIZE+1, 
		    "_make_ipal_cframe_sm");
		*c_ptr->sm_delete[num] = '\0';

		if (PrintDeletedSM) {
		    sm_delete_sm_max = sizeof(char)*ALLOCATION_STEP;
		    sm_delete_sm = (char *)malloc_data(sm_delete_sm_max, 
						       "_make_ipal_cframe_sm");
		    strcpy(sm_delete_sm, "意味素削除:");
		}
		else {
		    sm_delete_sm = strdup("意味素削除");
		}
	    }
	    else if (c_ptr->sm_delete_num[num] >= c_ptr->sm_delete_size[num]) {
		c_ptr->sm_delete[num] = (char *)realloc_data(c_ptr->sm_delete[num], 
		    sizeof(char)*(c_ptr->sm_delete_size[num] <<= 1)*SM_CODE_SIZE+1, 
		    "_make_ipal_cframe_sm");
	    }

	    /* NTT codeが書いてあるとき */
	    if (cf_str_buf[1] == '1') {
		strcat(c_ptr->sm_delete[num], &cf_str_buf[1]);

		if (PrintDeletedSM) {
		    temp = code2sm(&cf_str_buf[1]);
		    /* -1 ではないのは '/' の分 */
		    if (strlen(sm_delete_sm)+strlen(temp) > sm_delete_sm_max-2) {
			sm_delete_sm = (char *)realloc_data(sm_delete_sm, 
							    sm_delete_sm_max <<= 1, 
							    "_make_ipal_cframe_sm");
		    }
		    strcat(sm_delete_sm, "/");
		    strcat(sm_delete_sm, temp);
		}
	    }
	    else {
		/* NULL: core dump */
		strcat(c_ptr->sm_delete[num], sm2code(&cf_str_buf[1]));
	    }
	    c_ptr->sm_delete_num[num]++;
	    continue;
	}

	/* 普通の意味素 */

	sm_num++;
	sm_print_num++;
	if (sm_num >= SM_ELEMENT_MAX){
	    fprintf(stderr, ";;; Not enough sm_num !!\n");
	    break;
	}

	if (!strncmp(cf_str_buf, "数量", 4)) {
	    /* 前回も<数量>のときは入れない */
	    if (sm_num > 1 && !strncmp(&buf[size*(sm_num-2)], sm2code("数量"), size)) {
		sm_num--;
	    }
	    else {
		strcat(buf, sm2code("数量"));
	    }
	}
	else if (!strncmp(cf_str_buf, "主体準", 6)) {
	    strcat(buf, sm2code("主体"));
	    c_ptr->etcflag |= CF_GA_SEMI_SUBJECT;
	}
	else {
	    strcat(buf, sm2code(cf_str_buf));
	}

	if ((flag & STOREtoCF) && 
	    (EX_PRINT_NUM < 0 || sm_print_num <= EX_PRINT_NUM)) {
	    if (str[0])	strcat(str, "/");
	    strcat(str, cf_str_buf);
	}
    }

    if (buf[0]) {
	c_ptr->sm[num] = strdup(buf);
    }

    if (flag & STOREtoCF) {
	if (EX_PRINT_NUM < 0 || sm_print_num <= EX_PRINT_NUM) {
	    mlength = strlen(str)+1;
	    if (sm_delete_sm) mlength += strlen(sm_delete_sm)+1; /* "/"の分 */
	    if (sm_specify_sm) mlength += strlen(sm_specify_sm)+1; /* "/"の分 */
	    c_ptr->semantics[num] = (char *)malloc_data(sizeof(char)*mlength, 
							"_make_ipal_cframe_sm");
	    strcpy(c_ptr->semantics[num], str);
	    if (sm_delete_sm) {
		if (str[0]) strcat(c_ptr->semantics[num], "/");
		strcat(c_ptr->semantics[num], sm_delete_sm);
	    }
	    if (sm_specify_sm) {
		if (str[0] || sm_delete_sm) strcat(c_ptr->semantics[num], "/");
		strcat(c_ptr->semantics[num], sm_specify_sm);
	    }
	}
	else {
	    /* "...\0" の 4 つ分増やす */
	    mlength = strlen(str)+4;
	    if (sm_delete_sm) mlength += strlen(sm_delete_sm)+2; /* "/"の分 */
	    if (sm_specify_sm) mlength += strlen(sm_specify_sm)+2; /* "/"の分 */
	    c_ptr->semantics[num] = (char *)malloc_data(sizeof(char)*mlength, 
							"_make_ipal_cframe_sm");
	    strcpy(c_ptr->semantics[num], str);
	    if (sm_delete_sm) {
		if (str[0]) strcat(c_ptr->semantics[num], "/");
		strcat(c_ptr->semantics[num], sm_delete_sm);
	    }
	    if (sm_specify_sm) {
		if (str[0] || sm_delete_sm) strcat(c_ptr->semantics[num], "/");
		strcat(c_ptr->semantics[num], sm_specify_sm);
	    }
	    strcat(c_ptr->semantics[num], "...");
	}
    }
    free(str);
    if (sm_delete_sm) free(sm_delete_sm);
    if (sm_specify_sm) free(sm_specify_sm);
}

/*==================================================================*/
		  int split_freq(unsigned char *cp)
/*==================================================================*/
{
    int freq;

    while (1) {
	if (*cp == ':') {
	    sscanf(cp + 1, "%d", &freq);
	    *cp = '\0';
	    return freq;
	}
	else if (*cp == '\0') {
	    return 1;
	}
	cp++;
    }
    return 0;
}

/*==================================================================*/
void _make_ipal_cframe_ex(CASE_FRAME *c_ptr, unsigned char *cp, int num, 
			  int flag, int fflag)
/*==================================================================*/
{
    /* 例の読みだし */

    /* fflag: 頻度1を使うかどうか
              格が外の関係のときだけ使う */

    unsigned char *point, *point2;
    int max, count = 0, thesaurus = USE_NTT, freq, over_flag = 0;
    char *code, **destination, *buf;

    if (*cp == '\0') {
	return;
    }

    /* 引くリソースによって関数などをセット */
    destination = &c_ptr->ex[num];
    if (flag & USE_BGH) {
	thesaurus = USE_BGH;
	max = EX_ELEMENT_MAX*BGH_CODE_SIZE;
    }
    else if (flag & USE_NTT) {
	thesaurus = USE_NTT;
	max = SM_ELEMENT_MAX*SM_CODE_SIZE;
    }

    /* 最大値やめないといけません */
    buf = (char *)malloc_data(sizeof(char)*max, "_make_ipal_cframe_ex");

    point = cp;
    *buf = '\0';
    while ((point = extract_ipal_str(point, cf_str_buf, TRUE))) {
	point2 = cf_str_buf;

	/* 頻度の抽出 */
	freq = split_freq(point2);

	/* fflag == TRUE: 頻度1を削除 */
	if (fflag && freq < 2) {
	    continue;
	}

	/* 「ＡのＢ」の「Ｂ」だけを処理
	for (i = strlen(point2) - 2; i > 2; i -= 2) {
	    if (!strncmp(point2+i-2, "の", 2)) {
		point2 += i;
		break;
	    }
	}
	*/

	if (*point2 != '\0') {
	    if (!over_flag) {
		code = get_str_code(point2, thesaurus);
		if (code) {
		    if (strlen(buf) + strlen(code) >= max) {
			/* fprintf(stderr, "Too many EX <%s> (%2dth).\n", cf_str_buf, count); */
			free(code);
			over_flag = 1;
		    }
		    else {
			strcat(buf, code);
			free(code);
		    }
		}
	    }

	    if (c_ptr->ex_size[num] == 0) {
		c_ptr->ex_size[num] = 10;	/* 初期確保数 */
		c_ptr->ex_list[num] = (char **)malloc_data(sizeof(char *)*c_ptr->ex_size[num], 
							   "_make_ipal_cframe_ex");
		c_ptr->ex_freq[num] = (int *)malloc_data(sizeof(int)*c_ptr->ex_size[num], 
							 "_make_ipal_cframe_ex");
	    }
	    else if (c_ptr->ex_num[num] >= c_ptr->ex_size[num]) {
		c_ptr->ex_list[num] = (char **)realloc_data(c_ptr->ex_list[num], 
							    sizeof(char *)*(c_ptr->ex_size[num] <<= 1), 
							    "_make_ipal_cframe_ex");
		c_ptr->ex_freq[num] = (int *)realloc_data(c_ptr->ex_freq[num], 
							  sizeof(int)*c_ptr->ex_size[num], 
							  "_make_ipal_cframe_ex");
	    }
	    c_ptr->ex_list[num][c_ptr->ex_num[num]] = strdup(point2);
	    c_ptr->ex_freq[num][c_ptr->ex_num[num]++] = freq;


	    count++;
	}
    }

    *destination = strdup(buf);
    free(buf);
}

/*==================================================================*/
       int check_examples(char *cp, char **ex_list, int ex_num)
/*==================================================================*/
{
    int i;

    if (!ex_list) {
	return -1;
    }

    for (i = 0; i < ex_num; i++) {
	if (str_eq(cp, *(ex_list+i))) {
	    return i;
	}
    }
    return -1;
}

/*==================================================================*/
		int check_agentive(unsigned char *cp)
/*==================================================================*/
{
    unsigned char *point;

    point = cp;
    while ((point = extract_ipal_str(point, cf_str_buf, FALSE)))
	if (!strcmp(cf_str_buf, "Ａ")) return TRUE;
    return FALSE;
}

/*==================================================================*/
     void _make_ipal_cframe(CF_FRAME *i_ptr, CASE_FRAME *cf_ptr,
			    int address, int size, char *verb, int flag)
/*==================================================================*/
{
    int i, j = 0, ga_p = FALSE, c1, c2, count = 0;
    char ast_cap[32], *token, *cp, *buf;

    cf_ptr->type = flag;
    cf_ptr->cf_address = address;
    cf_ptr->cf_size = size;
    strcpy(cf_ptr->cf_id, i_ptr->DATA); 
    cf_ptr->etcflag = i_ptr->etcflag;
    cf_ptr->entry = strdup(verb);
    strcpy(cf_ptr->pred_type, i_ptr->pred_type);
    for (i = 0; i_ptr->samecase[i][0] != END_M; i++) {
	cf_ptr->samecase[i][0] = i_ptr->samecase[i][0];
	cf_ptr->samecase[i][1] = i_ptr->samecase[i][1];
    }
    cf_ptr->samecase[i][0] = END_M;
    cf_ptr->samecase[i][1] = END_M;

    if (*(i_ptr->feature)) {
	cf_ptr->feature = strdup(i_ptr->feature);
    }
    else {
	cf_ptr->feature = NULL;
    }
    cf_ptr->cf_similarity = 0;


    /* 格要素の追加 */

    if (cf_ptr->voice == FRAME_PASSIVE_I ||
	cf_ptr->voice == FRAME_CAUSATIVE_WO_NI ||
	cf_ptr->voice == FRAME_CAUSATIVE_WO ||
	cf_ptr->voice == FRAME_CAUSATIVE_NI) {
	_make_ipal_cframe_pp(cf_ptr, "ガ", j, flag);
	_make_ipal_cframe_sm(cf_ptr, "主体準", j, 
			     Thesaurus == USE_NTT ? USE_NTT_WITH_STORE : USE_BGH_WITH_STORE);
	_make_ipal_cframe_ex(cf_ptr, "彼", j, Thesaurus, FALSE);
	j++;
    }
    else if (cf_ptr->voice == FRAME_CAUSATIVE_PASSIVE) {
	_make_ipal_cframe_pp(cf_ptr, "ニ", j, flag);
	_make_ipal_cframe_sm(cf_ptr, "主体準", j, 
			     Thesaurus == USE_NTT ? USE_NTT_WITH_STORE : USE_BGH_WITH_STORE);
	_make_ipal_cframe_ex(cf_ptr, "彼", j, Thesaurus, FALSE);
	j++;
    }

    /* 各格要素の処理 */

    for (i = 0; i < i_ptr->casenum && j < CASE_MAX_NUM; i++, j++) { 
	cf_ptr->adjacent[j] = FALSE;
	if (_make_ipal_cframe_pp(cf_ptr, i_ptr->cs[i].kaku_keishiki, j, flag) == FALSE) {
	    j--;
	    continue;
	}
	if (Thesaurus == USE_BGH) {
	    _make_ipal_cframe_ex(cf_ptr, i_ptr->cs[i].meishiku, j, USE_BGH_WITH_STORE, 
				 (OptCaseFlag & OPT_CASE_USE_EX_ALL) ? 0 : !MatchPP(cf_ptr->pp[j][0], "外の関係"));
	    _make_ipal_cframe_sm(cf_ptr, i_ptr->cs[i].imisosei, j, USE_BGH_WITH_STORE);
	}
	else if (Thesaurus == USE_NTT) {
	    _make_ipal_cframe_ex(cf_ptr, i_ptr->cs[i].meishiku, j, USE_NTT_WITH_STORE, 
				 (OptCaseFlag & OPT_CASE_USE_EX_ALL) ? 0 : !MatchPP(cf_ptr->pp[j][0], "外の関係"));
	    _make_ipal_cframe_sm(cf_ptr, i_ptr->cs[i].imisosei, j, USE_NTT_WITH_STORE);
	}

	/* 能動 : Agentive ガ格を任意的とする場合
	if (cf_ptr->voice == FRAME_ACTIVE &&
	    i == 0 && 
	    cf_ptr->pp[i][0] == pp_kstr_to_code("ガ") &&
	    check_agentive(i_ptr->DATA+i_ptr->jyutugoso) == TRUE)
	  cf_ptr->oblig[i] = FALSE;
	*/

	if (cf_ptr->voice == FRAME_CAUSATIVE_WO_NI ||	/* 使役 */
	    cf_ptr->voice == FRAME_CAUSATIVE_WO ||
	    cf_ptr->voice == FRAME_CAUSATIVE_NI) {
	    /* ガ → ヲ，ニ */
	    if (cf_ptr->pp[j][0] == pp_kstr_to_code("ガ")) {
		if (ga_p == FALSE) {
		    ga_p = TRUE;
		    if (cf_ptr->voice == FRAME_CAUSATIVE_WO_NI)
		      _make_ipal_cframe_pp(cf_ptr, "ヲ／ニ", j, flag);
		    else if (cf_ptr->voice == FRAME_CAUSATIVE_WO)
		      _make_ipal_cframe_pp(cf_ptr, "ヲ", j, flag);
		    else if (cf_ptr->voice == FRAME_CAUSATIVE_NI)
		      _make_ipal_cframe_pp(cf_ptr, "ニ", j, flag);
		} else {
		    _make_ipal_cframe_pp(cf_ptr, "ヲ", j, flag); /* ガ・ガ構文 */
		}
	    }
	}
	else if (cf_ptr->voice == FRAME_PASSIVE_I ||	/* 受身 */
		 cf_ptr->voice == FRAME_PASSIVE_1 ||
		 cf_ptr->voice == FRAME_PASSIVE_2) {
	    /* 間接 ガ→ニ，直接 ガ→ニ／ニヨッテ／．． */
	    if (!strcmp(i_ptr->cs[i].kaku_keishiki, "ガ")) {
		if (cf_ptr->voice == FRAME_PASSIVE_I)
		  _make_ipal_cframe_pp(cf_ptr, "ニ", j, flag);
		else if (cf_ptr->voice == FRAME_PASSIVE_1)
		  _make_ipal_cframe_pp(cf_ptr, 
				       "ニ／ニヨル／カラ", j, flag);
		else if (cf_ptr->voice == FRAME_PASSIVE_2)
		  _make_ipal_cframe_pp(cf_ptr, 
				       "ニ／ニヨル／カラ", j, flag);
	    }
	    /* 直接 ニ／ニヨッテ／．．→ガ */
	    else if ((cf_ptr->voice == FRAME_PASSIVE_1 && 
		      (!strcmp(i_ptr->cs[i].kaku_keishiki, "ヲ") ||
		       (sprintf(ast_cap, "ヲ＊") &&
			!strcmp(i_ptr->cs[i].kaku_keishiki, ast_cap)))) ||
		     (cf_ptr->voice == FRAME_PASSIVE_2 && 
		      (!strcmp(i_ptr->cs[i].kaku_keishiki, "ニ") ||
		       (sprintf(ast_cap, "ニ＊") &&
			!strcmp(i_ptr->cs[i].kaku_keishiki, ast_cap))))) {
		_make_ipal_cframe_pp(cf_ptr, "ガ", j, flag);
	    }
	}
	else if (cf_ptr->voice == FRAME_POSSIBLE) {	/* 可能 */
	    if (!strcmp(i_ptr->cs[i].kaku_keishiki, "ヲ")) {
		_make_ipal_cframe_pp(cf_ptr, "ガ／ヲ", j, flag);
	     }
	}
	else if (cf_ptr->voice == FRAME_SPONTANE) {	/* 自発 */
	    if (!strcmp(i_ptr->cs[i].kaku_keishiki, "ヲ")) {
		_make_ipal_cframe_pp(cf_ptr, "ガ", j, flag);
	     }
	}
    }
    cf_ptr->element_num = j;
}

/*==================================================================*/
TAG_DATA *get_quasi_closest_case_component(SENTENCE_DATA *sp, TAG_DATA *t_ptr)
/*==================================================================*/
{
    TAG_DATA *tp;

    if (t_ptr->num < 1 || t_ptr->inum != 0) {
	return NULL;
    }

    if (check_feature(t_ptr->f, "ID:（〜を）〜に")) {
	return t_ptr;
    }

    /* 文節単位では考えていない */
    tp = sp->tag_data+t_ptr->num - 1;

    if (tp->inum != 0) {
	return NULL;
    }

    if (!check_feature(tp->f, "体言")) {
	return NULL;
    }

    if (check_feature(tp->f, "指示詞") || 
	(tp->SM_code[0] == '\0' && 
	 check_feature(tp->f, "係:ガ格"))) {
	return NULL;
    }

    if (check_feature(tp->f, "係:ヲ格") || 
	check_feature(tp->f, "係:ニ格") || 
	(!cf_match_element(tp->SM_code, "主体", FALSE) && 
	(check_feature(tp->f, "係:ガ格") || 
	 check_feature(tp->f, "係:カラ格") || 
	 check_feature(tp->f, "係:ヘ格") || 
	 check_feature(tp->f, "係:ヨリ格") || 
	 check_feature(tp->f, "係:ト格") || 
	 check_feature(tp->f, "係:マデ格")))) {
	return tp;
    }
    return NULL;
}

/*==================================================================*/
		   char *feature2case(TAG_DATA *tp)
/*==================================================================*/
{
    char *cp, *buffer;

    if (check_feature(tp->f, "ID:（〜を）〜に")) {
	buffer = (char *)malloc_data(3, "feature2case");
	strcpy(buffer, "ニ");
	return buffer;
    }
    else if ((cp = check_feature(tp->f, "係"))) {
	buffer = strdup(cp+3);
	if (!strncmp(buffer+strlen(buffer)-2, "格", 2)) {
	    *(buffer+strlen(buffer)-2) = '\0';
	    if (pp_kstr_to_code(buffer) != END_M) {
		return buffer;
	    }
	}
	free(buffer);
    }
    return NULL;
}

/*==================================================================*/
	       void f_num_inc(int start, int *f_num_p)
/*==================================================================*/
{
    (*f_num_p)++;
    if ((start + *f_num_p) >= MAX_Case_frame_num) {
	realloc_cf();
	realloc_cmm();
    }
}

/*==================================================================*/
int _make_ipal_cframe_subcontract(SENTENCE_DATA *sp, TAG_DATA *t_ptr, int start, 
				  char *verb, int voice, int flag)
/*==================================================================*/
{
    CF_FRAME *i_ptr;
    CASE_FRAME *cf_ptr;
    TAG_DATA *cbp;
    int f_num = 0, address, break_flag = 0, size, match, c;
    char *pre_pos, *cp, *address_str, *vtype = NULL;

    if (!verb)
    	return f_num;

    cf_ptr = Case_frame_array + start;

    /* 直前格要素をくっつけて検索 */
    if (flag == CF_PRED) {
	cbp = get_quasi_closest_case_component(sp, t_ptr);
	if (cbp) {
	    char *buffer, *pp;

	    pp = feature2case(cbp);
	    if (pp) {
		buffer = (char *)malloc_data(strlen(cbp->head_ptr->Goi) + strlen(pp) + strlen(verb) + 3, 
					     "_make_ipal_cframe_subcontract");
		sprintf(buffer, "%s-%s-%s", cbp->head_ptr->Goi, pp, verb);
		address_str = get_ipal_address(buffer, flag);
		free(buffer);
		free(pp);
	    }
	    if (!pp || !address_str) {
		address_str = get_ipal_address(verb, flag);
	    }
	}
	else {
	    address_str = get_ipal_address(verb, flag);
	}
    }
    else {
	address_str = get_ipal_address(verb, flag);
    }

    /* なければ */
    if (!address_str)
	return f_num;

    if ((vtype = check_feature(t_ptr->f, "用言"))) {
	vtype += 5;
    }
    else if ((vtype = check_feature(t_ptr->f, "サ変"))) {
	vtype = "動";
    }
    else if ((vtype = check_feature(t_ptr->f, "名詞的形容詞語幹"))) {
	vtype = "形";
    }
    else if ((vtype = check_feature(t_ptr->f, "準用言"))) {
	;
    }
    else if (flag == CF_NOUN && (vtype = check_feature(t_ptr->f, "体言"))) {
	vtype = "名";
    }

    for (cp = pre_pos = address_str; ; cp++) {
	if (*cp == '/' || *cp == '\0') {
	    if (*cp == '\0')
		break_flag = 1;
	    else 
		*cp = '\0';
	    
	    /* 格フレームの読みだし */
	    match = sscanf(pre_pos, "%d:%d", &address, &size);
	    if (match != 2) {
		fprintf(stderr, ";; CaseFrame Dictionary Index error (it seems version 1.).\n");
		exit(1);
	    }

	    i_ptr = get_ipal_frame(address, size, flag);
	    pre_pos = cp + 1;

	    /* 用言のタイプがマッチしなければ (準用言なら通過) */
	    if (vtype) {
		if (strncmp(vtype, i_ptr->pred_type, 2)) {
		    if (break_flag)
			break;
		    else
			continue;
		}
	    }

	    /* 能動態 or 格フレームに態が含まれる場合 */
	    if (voice == 0) {
		/* CF_NOUNの場合、ここでマッチ */
		(cf_ptr + f_num)->voice = FRAME_ACTIVE;
		_make_ipal_cframe(i_ptr, cf_ptr + f_num, address, size, verb, flag);

		/* 以下 flag == CF_PRED のはず */

		/* 格フレーム使役/格フレーム使役&受身*/
		if (t_ptr->voice & VOICE_SHIEKI || 
		    t_ptr->voice & VOICE_SHIEKI_UKEMI) {
		    /* ニ格がないとき */
		    if ((c = check_cf_case(cf_ptr + f_num, "ニ")) < 0) {
			_make_ipal_cframe_pp(cf_ptr + f_num, "ニ", (cf_ptr + f_num)->element_num, flag);
			_make_ipal_cframe_sm(cf_ptr + f_num, "主体準", (cf_ptr + f_num)->element_num, 
					     Thesaurus == USE_NTT ? USE_NTT_WITH_STORE : USE_BGH_WITH_STORE);
			(cf_ptr+f_num)->element_num++;
		    }
		    /* ニ格はあるけど<主体>がないとき */
		    else if (sm_match_check(sm2code("主体"), (cf_ptr + f_num)->sm[c], SM_NO_EXPAND_NE) == FALSE) {
			_make_ipal_cframe_sm(cf_ptr + f_num, "主体準", c, 
					     Thesaurus == USE_NTT ? USE_NTT_WITH_STORE : USE_BGH_WITH_STORE);
		    }
		    if (t_ptr->voice & VOICE_SHIEKI) {
			(cf_ptr + f_num)->voice = FRAME_CAUSATIVE_NI;
		    }
		    else if (t_ptr->voice & VOICE_SHIEKI_UKEMI) {
			(cf_ptr + f_num)->voice = FRAME_CAUSATIVE_PASSIVE;
		    }
		}
		/* 格フレーム受身 */
		else if (t_ptr->voice & VOICE_UKEMI || 
			 t_ptr->voice & VOICE_UNKNOWN) {
		    /* ニ/ニヨル/カラ格がないとき */
		    if ((c = check_cf_case(cf_ptr + f_num, "ニ")) < 0 && 
			(c = check_cf_case(cf_ptr + f_num, "ニヨル")) < 0 && 
			(c = check_cf_case(cf_ptr + f_num, "カラ")) < 0) {
			_make_ipal_cframe_pp(cf_ptr + f_num, "ニ", (cf_ptr + f_num)->element_num, flag);
			_make_ipal_cframe_sm(cf_ptr + f_num, "主体準", (cf_ptr + f_num)->element_num, 
					     Thesaurus == USE_NTT ? USE_NTT_WITH_STORE : USE_BGH_WITH_STORE);
			(cf_ptr+f_num)->element_num++;
		    }
		    /* ニ/ニヨル/カラ格はあるけど<主体>がないとき */
		    else if (sm_match_check(sm2code("主体"), (cf_ptr + f_num)->sm[c], SM_NO_EXPAND_NE) == FALSE) {
			_make_ipal_cframe_sm(cf_ptr + f_num, "主体準", c, 
					     Thesaurus == USE_NTT ? USE_NTT_WITH_STORE : USE_BGH_WITH_STORE);
		    }
		    (cf_ptr + f_num)->voice = FRAME_PASSIVE_1;
		}
		
		f_num_inc(start, &f_num);
		cf_ptr = Case_frame_array + start;
	    }

	    /* 使役 */
	    if (voice & VOICE_SHIEKI) {
		if (i_ptr->voice & CF_CAUSATIVE_WO && i_ptr->voice & CF_CAUSATIVE_NI)
		  (cf_ptr + f_num)->voice = FRAME_CAUSATIVE_WO_NI;
		else if (i_ptr->voice & CF_CAUSATIVE_WO)
		  (cf_ptr + f_num)->voice = FRAME_CAUSATIVE_WO;
		else if (i_ptr->voice & CF_CAUSATIVE_NI)
		  (cf_ptr + f_num)->voice = FRAME_CAUSATIVE_NI;
		
		_make_ipal_cframe(i_ptr, cf_ptr + f_num, address, size, verb, flag);
		f_num_inc(start, &f_num);
		cf_ptr = Case_frame_array + start;
	    }
	    
	    /* 受身 */
	    if (voice & VOICE_UKEMI) {
		/* 直接受身１ */
		if (i_ptr->voice & CF_PASSIVE_1) {
		    (cf_ptr + f_num)->voice = FRAME_PASSIVE_1;
		    _make_ipal_cframe(i_ptr, cf_ptr + f_num, address, size, verb, flag);
		    f_num_inc(start, &f_num);
		    cf_ptr = Case_frame_array + start;
		}
		/* 直接受身２ */
		if (i_ptr->voice & CF_PASSIVE_2) {
		    (cf_ptr + f_num)->voice = FRAME_PASSIVE_2;
		    _make_ipal_cframe(i_ptr, cf_ptr + f_num, address, size, verb, flag);
		    f_num_inc(start, &f_num);
		    cf_ptr = Case_frame_array + start;
		}
		/* 間接受身 */
		if (i_ptr->voice & CF_PASSIVE_I) {
		    (cf_ptr + f_num)->voice = FRAME_PASSIVE_I;
		    _make_ipal_cframe(i_ptr, cf_ptr + f_num, address, size, verb, flag);
		    f_num_inc(start, &f_num);
		    cf_ptr = Case_frame_array + start;
		}
	    }

	    /* もらう/ほしい */
	    if (voice & VOICE_MORAU || 
		voice & VOICE_HOSHII) {
		/* ニ使役 (間接受身でも同じ */
		if (i_ptr->voice & CF_CAUSATIVE_NI) {
		    (cf_ptr + f_num)->voice = FRAME_CAUSATIVE_NI;
		    _make_ipal_cframe(i_ptr, cf_ptr + f_num, address, size, verb, flag);
		    f_num_inc(start, &f_num);
		    cf_ptr = Case_frame_array + start;
		}
 	    }

	    /* せられる/させられる */
	    if (voice & VOICE_SHIEKI_UKEMI) {
		(cf_ptr + f_num)->voice = FRAME_CAUSATIVE_PASSIVE;
		_make_ipal_cframe(i_ptr, cf_ptr + f_num, address, size, verb, flag);
		f_num_inc(start, &f_num);
		cf_ptr = Case_frame_array + start;
	    } 

	    /* 可能，尊敬，自発 */
	    if (voice & VOICE_UKEMI) {
		if (i_ptr->voice & CF_POSSIBLE) {
		    (cf_ptr + f_num)->voice = FRAME_POSSIBLE;
		    _make_ipal_cframe(i_ptr, cf_ptr + f_num, address, size, verb, flag);
		    f_num_inc(start, &f_num);
		    cf_ptr = Case_frame_array + start;
		}
		if (i_ptr->voice & CF_POLITE) {
		    (cf_ptr + f_num)->voice = FRAME_POLITE;
		    _make_ipal_cframe(i_ptr, cf_ptr + f_num, address, size, verb, flag);
		    f_num_inc(start, &f_num);
		    cf_ptr = Case_frame_array + start;
		}
		if (i_ptr->voice & CF_SPONTANE) {
		    (cf_ptr + f_num)->voice = FRAME_SPONTANE;
		    _make_ipal_cframe(i_ptr, cf_ptr + f_num, address, size, verb, flag);
		    f_num_inc(start, &f_num);
		    cf_ptr = Case_frame_array + start;
		}
	    }
	    if (break_flag)
		break;
	}
    }
    free(address_str);
    return f_num;
}

/*==================================================================*/
int make_ipal_cframe_subcontract(SENTENCE_DATA *sp, TAG_DATA *t_ptr, int start, char *in_verb, int flag)
/*==================================================================*/
{
    int f_num = 0, plus_num;
    char *verb;

    if (flag == CF_NOUN) {
	return _make_ipal_cframe_subcontract(sp, t_ptr, start, in_verb, 0, flag);
    }

    verb = (char *)malloc_data(strlen(in_verb) + 4, "make_ipal_cframe_subcontract");
    strcpy(verb, in_verb);
    free(in_verb);

    if (t_ptr->voice == VOICE_UNKNOWN) {
	t_ptr->voice = 0; /* 能動態でtry */
	f_num = _make_ipal_cframe_subcontract(sp, t_ptr, start, verb, 0, flag);

	/* 今のところ受身の場合を考えない */
	free(verb);
	return f_num;

	t_ptr->voice = VOICE_UNKNOWN;
    }

    /* 受身, 使役の格フレーム */
    if (t_ptr->voice == VOICE_UNKNOWN || 
	t_ptr->voice & VOICE_UKEMI ||
	t_ptr->voice & VOICE_SHIEKI || 
	t_ptr->voice & VOICE_SHIEKI_UKEMI) {
	int suffix = 0;

	if (t_ptr->voice & VOICE_SHIEKI) {
	    strcat(verb, ":C");
	    suffix = 2;
	}
	else if (t_ptr->voice & VOICE_UKEMI || 
		 t_ptr->voice & VOICE_UNKNOWN) {
	    strcat(verb, ":P");
	    suffix = 2;
	}
	else if (t_ptr->voice & VOICE_SHIEKI_UKEMI) {
	    strcat(verb, ":PC");
	    suffix = 3;
	}

	plus_num = _make_ipal_cframe_subcontract(sp, t_ptr, start + f_num, verb, 0, flag);
	if (plus_num != 0) {
	    free(verb);
	    return f_num + plus_num;
	}
	*(verb + strlen(verb) - suffix) = '\0'; /* みつからなかったらもとにもどす */
    }

    if (t_ptr->voice == VOICE_UNKNOWN) {
	f_num += _make_ipal_cframe_subcontract(sp, t_ptr, start + f_num, verb, VOICE_UKEMI, flag); /* 受身 */
    }
    else {
	f_num = _make_ipal_cframe_subcontract(sp, t_ptr, start, verb, t_ptr->voice, flag);
    }
    free(verb);    
    return f_num;
}

/*==================================================================*/
	       char *make_pred_string(TAG_DATA *t_ptr)
/*==================================================================*/
{
    char *buffer;

    /* 用言タイプ, voiceの分(7)も確保しておく */

    /* 「（〜を）〜に」 のときは 「する」 で探す */
    if (check_feature(t_ptr->f, "ID:（〜を）〜に")) {
	buffer = (char *)malloc_data(12, "make_pred_string"); /* 4 + 8 */
	strcpy(buffer, "する");
    }
    /* 「形容詞+なる」など */
    else if (check_feature(t_ptr->f, "Ｔ用言見出→")) {
	buffer = (char *)malloc_data(strlen(t_ptr->head_ptr->Goi2) + strlen((t_ptr->head_ptr + 1)->Goi) + 8, 
				     "make_pred_string");
	strcpy(buffer, t_ptr->head_ptr->Goi2);
	strcat(buffer, (t_ptr->head_ptr + 1)->Goi);
    }
    /* 「形容詞語幹+的だ」など */
    else if (check_feature(t_ptr->f, "Ｔ用言見出←")) {
	buffer = (char *)malloc_data(strlen((t_ptr->head_ptr - 1)->Goi2) + strlen(t_ptr->head_ptr->Goi) + 8, 
				     "make_pred_string");
	strcpy(buffer, (t_ptr->head_ptr - 1)->Goi2);
	strcat(buffer, t_ptr->head_ptr->Goi);
    }
    else {
	buffer = (char *)malloc_data(strlen(t_ptr->head_ptr->Goi) + 8, "make_pred_string");
	strcpy(buffer, t_ptr->head_ptr->Goi);
    }

    return buffer;
}

/*==================================================================*/
int make_ipal_cframe(SENTENCE_DATA *sp, TAG_DATA *t_ptr, int start, int flag)
/*==================================================================*/
{
    int f_num = 0;

    /* 自立語末尾語を用いて格フレーム辞書を引く */

    if (!t_ptr->jiritu_ptr) {
	return f_num;
    }

    f_num = make_ipal_cframe_subcontract(sp, t_ptr, start, make_pred_string(t_ptr), flag);
    Case_frame_num += f_num;

    return f_num;
}

/*==================================================================*/
	 int make_default_cframe(TAG_DATA *t_ptr, int start)
/*==================================================================*/
{
    int i, num = 0, f_num = 0;
    CASE_FRAME *cf_ptr;

    cf_ptr = Case_frame_array + start;

    if (MAX_cf_frame_length == 0) {
	cf_str_buf = 
	    (unsigned char *)realloc_data(cf_str_buf, 
					  sizeof(unsigned char)*ALLOCATION_STEP, 
					  "make_default_cframe");
    }

    if (check_feature(t_ptr->f, "用言:判")) {
	_make_ipal_cframe_pp(cf_ptr, "ガ＊", num, CF_PRED);
	_make_ipal_cframe_sm(cf_ptr, "主体準", num++, 
			     Thesaurus == USE_NTT ? USE_NTT_WITH_STORE : USE_BGH_WITH_STORE);
	cf_ptr->cf_address = -1;	/* ★ 表示するためにこの値を変える必要がある */
	strcpy(cf_ptr->pred_type, "判");
    }
    else {
	_make_ipal_cframe_pp(cf_ptr, "ガ＊", num++, CF_PRED);
	_make_ipal_cframe_pp(cf_ptr, "ヲ＊", num++, CF_PRED);
	_make_ipal_cframe_pp(cf_ptr, "ニ＊", num++, CF_PRED);
	_make_ipal_cframe_pp(cf_ptr, "ヘ＊", num++, CF_PRED);
	_make_ipal_cframe_pp(cf_ptr, "ヨリ＊", num++, CF_PRED);
	cf_ptr->cf_address = -1;
	cf_ptr->pred_type[0] = '\0';
    }

    cf_ptr->element_num = num;
    cf_ptr->etcflag = CF_NORMAL;

    for (i = 0; i < num; i++) {
	cf_ptr->pp[i][1] = END_M;
    }

    cf_ptr->samecase[0][0] = END_M;
    cf_ptr->samecase[0][1] = END_M;

    f_num_inc(start, &f_num);
    Case_frame_num++;
    t_ptr->cf_num = 1;
    return 1;
}

/*==================================================================*/
  void make_caseframes(SENTENCE_DATA *sp, TAG_DATA *t_ptr, int flag)
/*==================================================================*/
{
    t_ptr->cf_num = make_ipal_cframe(sp, t_ptr, Case_frame_num, flag);

    if (flag == CF_PRED && t_ptr->cf_num == 0) {
	make_default_cframe(t_ptr, Case_frame_num);
    }
}

/*==================================================================*/
		void set_caseframes(SENTENCE_DATA *sp)
/*==================================================================*/
{
    int i, j, start;
    TAG_DATA  *t_ptr;

    Case_frame_num = 0;

    for (i = 0, t_ptr = sp->tag_data; i < sp->Tag_num; i++, t_ptr++) {
	/* 正解コーパスを入力したときに自立語がない場合がある */
	if (t_ptr->jiritu_ptr != NULL && 
	    !check_feature(t_ptr->f, "格解析なし")) {
	    if ((!OptEllipsis || 
		 (OptEllipsis & OPT_ELLIPSIS) || 
		 (OptEllipsis & OPT_DEMO)) && 
		(check_feature(t_ptr->f, "用言") || /* 準用言はとりあえず対象外 */
		 check_feature(t_ptr->f, "非用言格解析"))) { /* サ変名詞, 形容詞語幹 */

		/* 以下の2つの処理はfeatureレベルで起動している
		   set_pred_voice(t_ptr); ヴォイス
		   get_scase_code(t_ptr); 表層格 */

		start = Case_frame_num;
		make_caseframes(sp, t_ptr, CF_PRED);
		t_ptr->e_cf_num = t_ptr->cf_num;
	    }
	    /* 名詞格フレームはとりあえず、サ変名詞以外について */
	    else if ((OptEllipsis & OPT_REL_NOUN) && 
		     check_feature(t_ptr->f, "体言") && 
		     !check_feature(t_ptr->f, "サ変")) {
		start = Case_frame_num;
		make_caseframes(sp, t_ptr, CF_NOUN);
	    }
	}
	else {
	    t_ptr->cf_ptr = NULL;
	    t_ptr->cf_num = 0;
	}
    }

    /* 各タグ単位から格フレームへのリンク付け */
    start = 0;
    for (i = 0, t_ptr = sp->tag_data; i < sp->Tag_num; i++, t_ptr++) {
	if (t_ptr->cf_num) {
	    t_ptr->cf_ptr = Case_frame_array + start;
	    start += t_ptr->cf_num;
	}
    }
}

/*==================================================================*/
		       void clear_cf(int flag)
/*==================================================================*/
{
    int i, j, k, end;

    end = flag ? MAX_Case_frame_num : Case_frame_num;

    for (i = 0; i < end; i++) {
	for (j = 0; j < CF_ELEMENT_MAX; j++) {
	    if ((Case_frame_array+i)->pp_str[j]) {
		free((Case_frame_array+i)->pp_str[j]);
		(Case_frame_array+i)->pp_str[j] = NULL;
	    }
	    if ((Case_frame_array+i)->ex[j]) {
		free((Case_frame_array+i)->ex[j]);
		(Case_frame_array+i)->ex[j] = NULL;
	    }
	    if ((Case_frame_array+i)->sm[j]) {
		free((Case_frame_array+i)->sm[j]);
		(Case_frame_array+i)->sm[j] = NULL;
	    }
	    if ((Case_frame_array+i)->sm_delete[j]) {
		free((Case_frame_array+i)->sm_delete[j]);
		(Case_frame_array+i)->sm_delete[j] = NULL;
		(Case_frame_array+i)->sm_delete_size[j] = 0;
		(Case_frame_array+i)->sm_delete_num[j] = 0;
	    }
	    if ((Case_frame_array+i)->sm_specify[j]) {
		free((Case_frame_array+i)->sm_specify[j]);
		(Case_frame_array+i)->sm_specify[j] = NULL;
		(Case_frame_array+i)->sm_specify_size[j] = 0;
		(Case_frame_array+i)->sm_specify_num[j] = 0;
	    }
	    if ((Case_frame_array+i)->ex_list[j]) {
		for (k = 0; k < (Case_frame_array+i)->ex_num[j]; k++) {
		    if ((Case_frame_array+i)->ex_list[j][k]) {
			free((Case_frame_array+i)->ex_list[j][k]);
		    }
		}
		free((Case_frame_array+i)->ex_list[j]);
		free((Case_frame_array+i)->ex_freq[j]);
		(Case_frame_array+i)->ex_list[j] = NULL;
		(Case_frame_array+i)->ex_size[j] = 0;
		(Case_frame_array+i)->ex_num[j] = 0;
	    }
	    if ((Case_frame_array+i)->semantics[j]) {
		free((Case_frame_array+i)->semantics[j]);
		(Case_frame_array+i)->semantics[j] = NULL;
	    }
	}
	if ((Case_frame_array+i)->entry) {
	    free((Case_frame_array+i)->entry);
	    (Case_frame_array+i)->entry = NULL;
	}
	if ((Case_frame_array+i)->feature) {
	    free((Case_frame_array+i)->feature);
	    (Case_frame_array+i)->feature = NULL;
	}
    }
}

/*==================================================================*/
	     int check_cf_case(CASE_FRAME *cfp, char *pp)
/*==================================================================*/
{
    int i;
    for (i = 0; i < cfp->element_num; i++) {
	if (MatchPP2(cfp->pp[i], pp)) {
	    return i;
	}
    }
    return -1;
}

/*==================================================================*/
	    float get_cfs_similarity(char *cf1, char *cf2)
/*==================================================================*/
{
    char *key, *value;
    float ret;

    if (CFSimExist == FALSE || cf1 == NULL || cf2 == NULL) {
	return 0;
    }

    /* 同じとき */
    if (!strcmp(cf1, cf2)) {
	return 1.0;
    }

    key = (char *)malloc_data(sizeof(char) * (strlen(cf1) + strlen(cf2) + 2), 
			      "get_cfs_similarity");
    sprintf(key, "%s-%s", cf1, cf2);
    value = db_get(cf_sim_db, key);
    if (value) {
	    ret = atof(value);
	    free(value);
    }
    free(key);

    return ret;
}

/*====================================================================
                               END
====================================================================*/
