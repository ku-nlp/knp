/*====================================================================

		       格構造解析: 格フレーム側

                                               S.Kurohashi 93. 5.31

    $Id$
====================================================================*/
#include "knp.h"

FILE *ipal_fp;
DBM_FILE ipal_db;

CASE_FRAME 	*Case_frame_array = NULL; 	/* 格フレーム */
int 	   	Case_frame_num;			/* 格フレーム数 */
int 	   	MAX_Case_frame_num = 0;		/* 最大格フレーム数 */

IPAL_FRAME Ipal_frame;
int MAX_ipal_frame_length = 0;
unsigned char *ipal_str_buf;

int	IPALExist;
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
	data_filename = (char *)check_dict_filename(DICT[CF_DATA], TRUE);
    }
    else {
	data_filename = (char *)check_dict_filename(IPAL_DAT_NAME, FALSE);
    }

    if (DICT[CF_INDEX_DB]) {
	index_db_filename = (char *)check_dict_filename(DICT[CF_INDEX_DB], TRUE);
    }
    else {
	index_db_filename = (char *)check_dict_filename(IPAL_DB_NAME, FALSE);
    }

    if ((ipal_fp = fopen(data_filename, "rb")) == NULL) {
#ifdef DEBUG
	fprintf(stderr, "Cannot open CF DATA <%s>.\n", data_filename);
#endif
	IPALExist = FALSE;
    }
    else if ((ipal_db = DBM_open(index_db_filename, O_RDONLY, 0)) == NULL) {
	fprintf(stderr, "Cannot open CF INDEX Database <%s>.\n", index_db_filename);
	/* 格フレーム DATA は読めるのに、DB が読めないときは終わる */
	exit(1);
    } 
    else {
	IPALExist = TRUE;
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
	    if (Thesaurus == USE_BGH) {
		free(sp->Best_mgr->cpm[i].cf.ex[j]);
		sp->Best_mgr->cpm[i].cf.ex[j] = NULL;
	    }
	    else if (Thesaurus == USE_NTT) {
		free(sp->Best_mgr->cpm[i].cf.ex2[j]);
		sp->Best_mgr->cpm[i].cf.ex2[j] = NULL;
	    }
	    free(sp->Best_mgr->cpm[i].cf.sm[j]);
	    sp->Best_mgr->cpm[i].cf.sm[j] = NULL;

	    free(sp->Best_mgr->cpm[i].cf.ex_list[j][0]);
	    free(sp->Best_mgr->cpm[i].cf.ex_list[j]);
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
		   void init_cf2(SENTENCE_DATA *sp)
/*==================================================================*/
{
    if (OptAnalysis == OPT_CASE || 
	OptAnalysis == OPT_CASE2) {

	/* 格フレーム領域確保 */
	Case_frame_array = (CASE_FRAME *)malloc_data(sizeof(CASE_FRAME)*ALL_CASE_FRAME_MAX, "init_cf");
	MAX_Case_frame_num = ALL_CASE_FRAME_MAX;
	init_cf_structure(Case_frame_array, MAX_Case_frame_num);

	/* Best_mgrのcpm領域確保 */
	init_mgr_cf(sp->Best_mgr);
    }
}

/*==================================================================*/
			   void close_cf()
/*==================================================================*/
{
    if (IPALExist == TRUE) {
	fclose(ipal_fp);
	DBM_close(ipal_db);
    }
}

/*==================================================================*/
	     char *get_ipal_address(unsigned char *word)
/*==================================================================*/
{
    if (IPALExist == FALSE)
	return NULL;

    return db_get(ipal_db, word);
}

/*==================================================================*/
	  IPAL_FRAME *get_ipal_frame(int address, int size)
/*==================================================================*/
{
    if (size > MAX_ipal_frame_length) {
	MAX_ipal_frame_length += ALLOCATION_STEP*((size-MAX_ipal_frame_length)/ALLOCATION_STEP+1);
	Ipal_frame.DATA = 
	    (unsigned char *)realloc_data(Ipal_frame.DATA, 
					  sizeof(unsigned char)*MAX_ipal_frame_length, 
					  "get_ipal_frame");
	ipal_str_buf = 
	    (unsigned char *)realloc_data(ipal_str_buf, 
					  sizeof(unsigned char)*MAX_ipal_frame_length, 
					  "get_ipal_frame");
    }

    fseek(ipal_fp, (long)address, 0);
    if (fread(&Ipal_frame, sizeof(IPAL_FRAME_INDEX), 1, ipal_fp) < 1) {
	fprintf(stderr, "Error in fread.\n");
	exit(1);
    }
    if (fread(Ipal_frame.DATA, size-sizeof(IPAL_FRAME_INDEX), 1, ipal_fp) < 1) {
	fprintf(stderr, "Error in fread.\n");
	exit(1);
    }
    return &Ipal_frame;
}

/*==================================================================*/
unsigned char *extract_ipal_str(unsigned char *dat, unsigned char *ret)
/*==================================================================*/
{
    if (*dat == '\0' || !strcmp(dat, "＊")) 
      return NULL;

    while (1) {
	if (*dat == '\0') {
	    *ret = '\0';
	    return dat;
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
int _make_ipal_cframe_pp(CASE_FRAME *c_ptr, unsigned char *cp, int num)
/*==================================================================*/
{
    /* 助詞の読みだし */

    unsigned char *point;
    int pp_num = 0;

    if (!strcmp(cp+strlen(cp)-2, "＠")) {
	c_ptr->adjacent[num] = TRUE;
	*(cp+strlen(cp)-2) = '\0';
    }

    if (!strcmp(cp+strlen(cp)-2, "＊")) {
	c_ptr->oblig[num] = FALSE;
    }
    else {
	/* 未決定 (ひとつめの格を見て決める) */
	c_ptr->oblig[num] = END_M;
    }

    point = cp; 
    while ((point = extract_ipal_str(point, ipal_str_buf))) {
	if (pp_num == 0 && c_ptr->oblig[num] == END_M) {
	    if (str_eq(ipal_str_buf, "ガ") || 
		str_eq(ipal_str_buf, "ヲ") || 
		str_eq(ipal_str_buf, "ニ") || 
		str_eq(ipal_str_buf, "ヘ") || 
		str_eq(ipal_str_buf, "ヨリ")) {
		c_ptr->oblig[num] = TRUE;
	    }
	    else {
		c_ptr->oblig[num] = FALSE;
	    }
	}
	c_ptr->pp[num][pp_num++] = pp_kstr_to_code(ipal_str_buf);
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
    int sm_num = 0, length = 0, mlength, sm_delete_sm_max = 0;
    char buf[SM_ELEMENT_MAX*SM_CODE_SIZE], *sm_delete_sm = NULL, *temp;

    if (*cp == '\0') {
	return;
    }

    point = cp;
    buf[0] = '\0';
    while ((point = extract_ipal_str(point, ipal_str_buf))) {
	/* 使ってはいけない意味素 */
        if (ipal_str_buf[0] == '-') {
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
		/* 
		if (!c_ptr->semantics[num] && (flag & STOREtoCF)) {
		    if (PrintDeletedSM) {
			c_ptr->semantics[num] = strdup(cp);
		    }
		    else {
			* 禁止コードをひとつ分もどす *
			if (*point == '\0') {
			    loc = point-SM_CODE_SIZE-1;
			}
			else {
			    loc = point-SM_CODE_SIZE-3;
			}
			c_ptr->semantics[num] = (char *)malloc_data(
			    sizeof(char)*(loc-cp+11), "_make_ipal_cframe_sm");
			strncpy(c_ptr->semantics[num], cp, loc-cp);
			c_ptr->semantics[num][loc-cp] = '\0';
			strcat(c_ptr->semantics[num], "意味素削除");
		    }
		} */
	    }
	    else if (c_ptr->sm_delete_num[num] >= c_ptr->sm_delete_size[num]) {
		c_ptr->sm_delete[num] = (char *)realloc_data(c_ptr->sm_delete[num], 
		    sizeof(char)*(c_ptr->sm_delete_size[num] <<= 1)*SM_CODE_SIZE+1, 
		    "_make_ipal_cframe_sm");
	    }

	    /* codeが書いてあるとき */
	    if (ipal_str_buf[1] == '1') {
		strcat(c_ptr->sm_delete[num], &ipal_str_buf[1]);

		if (PrintDeletedSM) {
		    temp = code2sm(&ipal_str_buf[1]);
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
		strcat(c_ptr->sm_delete[num], sm2code(&ipal_str_buf[1]));
	    }
	    c_ptr->sm_delete_num[num]++;
	    continue;
	}

	sm_num++;
	if (sm_num >= SM_ELEMENT_MAX){
	    fprintf(stderr, ";;; Not enough sm_num !!\n");
	    break;
	}

	if (!strncmp(ipal_str_buf, "数量", 4)) {
	    strcat(buf, sm2code("数量"));
	}
	else if (!strncmp(ipal_str_buf, "主体準", 6)) {
	    strcat(buf, sm2code("主体"));
	    c_ptr->etcflag |= CF_GA_SEMI_SUBJECT;
	}
	else {
	    strcat(buf, sm2code(ipal_str_buf));
	}

	if ((flag & STOREtoCF) && sm_num <= EX_PRINT_NUM) {
	    if (*point) {
		length = point-cp-2;
	    }
	    else {
		length = point-cp;
	    }
	}
    }

    if (buf[0]) {
	c_ptr->sm[num] = strdup(buf);
    }

    if (flag & STOREtoCF) {
	if (sm_num <= EX_PRINT_NUM) {
	    mlength = length+1;
	    if (sm_delete_sm) mlength += strlen(sm_delete_sm)+2; /* ／の分 */
	    c_ptr->semantics[num] = (char *)malloc_data(sizeof(char)*mlength, 
							"_make_ipal_cframe_sm");
	    if (length) strncpy(c_ptr->semantics[num], cp, length);
	    *(c_ptr->semantics[num]+length) = '\0';
	    if (sm_delete_sm) {
		if (length) strcat(c_ptr->semantics[num], "／");
		strcat(c_ptr->semantics[num], sm_delete_sm);
	    }
	}
	else {
	    /* "...\0" の 4 つ分増やす */
	    mlength = length+4;
	    if (sm_delete_sm) mlength += strlen(sm_delete_sm)+2; /* ／の分 */
	    c_ptr->semantics[num] = (char *)malloc_data(sizeof(char)*mlength, 
							"_make_ipal_cframe_sm");
	    if (length) strncpy(c_ptr->semantics[num], cp, length);
	    *(c_ptr->semantics[num]+length) = '\0';
	    if (sm_delete_sm) {
		if (length) strcat(c_ptr->semantics[num], "／");
		strcat(c_ptr->semantics[num], sm_delete_sm);
	    }
	    strcat(c_ptr->semantics[num], "...");
	}
    }
    if (sm_delete_sm) free(sm_delete_sm);
}

/*==================================================================*/
void _make_ipal_cframe_ex(CASE_FRAME *c_ptr, unsigned char *cp, int num, int flag)
/*==================================================================*/
{
    /* 例の読みだし */

    unsigned char *point, *point2;
    int max, count = 0, length = 0, thesaurus = USE_NTT;
    char *code, **destination, *buf;

    if (*cp == '\0') {
	return;
    }

    /* 引くリソースによって関数などをセット */
    if (flag & USE_BGH) {
	thesaurus = USE_BGH;
	destination = &c_ptr->ex[num];
	max = EX_ELEMENT_MAX*BGH_CODE_SIZE;
    }
    else if (flag & USE_NTT) {
	thesaurus = USE_NTT;
	destination = &c_ptr->ex2[num];
	max = SM_ELEMENT_MAX*SM_CODE_SIZE;
    }

    /* 最大値やめないといけませんな */
    buf = (char *)malloc_data(sizeof(char)*max, "_make_ipal_cframe_ex");

    point = cp;
    *buf = '\0';
    while ((point = extract_ipal_str(point, ipal_str_buf))) {
	point2 = ipal_str_buf;

	/* 「ＡのＢ」の「Ｂ」だけを処理
	for (i = strlen(point2) - 2; i > 2; i -= 2) {
	    if (!strncmp(point2+i-2, "の", 2)) {
		point2 += i;
		break;
	    }
	}
	*/

	if (*point2 != '\0') {
	    code = get_str_code(point2, thesaurus);
	    if (code) {
		if (strlen(buf) + strlen(code) >= max) {
		    fprintf(stderr, "Too many EX <%s> (%2dth).\n", ipal_str_buf, count);
		    free(code);
		    break;
		}
		strcat(buf, code);
		free(code);
	    }

	    if (c_ptr->ex_size[num] == 0) {
		c_ptr->ex_size[num] = 10;	/* 初期確保数 */
		c_ptr->ex_list[num] = (char **)malloc_data(sizeof(char *)*c_ptr->ex_size[num], 
							    "_make_ipal_cframe_ex");
	    }
	    else if (c_ptr->ex_num[num] >= c_ptr->ex_size[num]) {
		c_ptr->ex_list[num] = (char **)realloc_data(c_ptr->ex_list[num], 
							    sizeof(char *)*(c_ptr->ex_size[num] <<= 1), 
							    "_make_ipal_cframe_ex");
	    }
	    c_ptr->ex_list[num][c_ptr->ex_num[num]++] = strdup(point2);
	}

	count++;
	if (flag & STOREtoCF && count == EX_PRINT_NUM) {
	    length = point-cp-2;
	}
    }

    *destination = strdup(buf);
    free(buf);

    if (flag & STOREtoCF) {
	if (*cp) {
	    if (count <= EX_PRINT_NUM) {
		c_ptr->examples[num] = strdup(cp);
	    }
	    else {
		/* "...\0" の 4 つ分増やす */
		c_ptr->examples[num] = (char *)malloc_data(sizeof(char)*(length+4), 
							   "_make_ipal_cframe_ex");
		strncpy(c_ptr->examples[num], cp, length);
		*(c_ptr->examples[num]+length) = '\0';
		strcat(c_ptr->examples[num], "...");
	    }
	}
	else {
	    c_ptr->examples[num] = NULL;
	}
    }
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
    while ((point = extract_ipal_str(point, ipal_str_buf)))
	if (!strcmp(ipal_str_buf, "Ａ")) return TRUE;
    return FALSE;
}

/*==================================================================*/
  void _make_ipal_cframe(IPAL_FRAME *i_ptr, CASE_FRAME *cf_ptr, int address, int size)
/*==================================================================*/
{
    int i, j = 0, ga_p = FALSE, c1, c2, count = 0;
    char ast_cap[32], *token, *cp, *buf;

    cf_ptr->ipal_address = address;
    cf_ptr->ipal_size = size;
    cf_ptr->concatenated_flag = 0;
    strcpy(cf_ptr->ipal_id, i_ptr->DATA+i_ptr->id); 
    strcpy(cf_ptr->imi, i_ptr->DATA+i_ptr->imi);
    cf_ptr->etcflag = CF_NORMAL;
    cf_ptr->feature[0] = '\0';

    token = strtok(i_ptr->DATA+i_ptr->jyutugoso, " ");
    while (token) {
	/* ORの格フレームは「述語素」に「和フレーム」と書いてある */
	if (str_eq(token, "和フレーム")) {
	    cf_ptr->etcflag |= CF_SUM;
	}
	else {
	    /* 格フレームが変化したフラグ */
	    if (str_eq(token, "★変化")) {
		cf_ptr->etcflag |= CF_CHANGE;
	    }
	    else if ((cp = strchr(token, ':')) != NULL) {
		buf = strdup(token);
		cp = buf+(cp-token);
		*cp = '\0';
		c1 = pp_kstr_to_code(buf);
		c2 = pp_kstr_to_code(cp+1);
		free(buf);

		if (c1 == END_M || c2 == END_M) {
		    fprintf(stderr, ";; Can't understand <%s>\n", token);
		}
		/* 溢れチェック */
		else if (count >= CF_ELEMENT_MAX) {
		    break;
		}
		else {
		    /* 数が小さい格を前に入れる */
		    if (c1 > c2) {
			cf_ptr->samecase[count][0] = c2;
			cf_ptr->samecase[count][1] = c1;
		    }
		    else {
			cf_ptr->samecase[count][0] = c1;
			cf_ptr->samecase[count][1] = c2;
		    }
		    count++;
		}
	    }

	    /* 溢れチェック */
	    if (strlen(cf_ptr->feature)+strlen(token)-1 >= SMALL_DATA_LEN) {
		break;
	    }
	    if (cf_ptr->feature[0] != '\0') {
		strcat(cf_ptr->feature, " ");
	    }
	    strcat(cf_ptr->feature, token);
	}
	token = strtok(NULL, " ");
    }

    cf_ptr->samecase[count][0] = END_M;
    cf_ptr->samecase[count][1] = END_M;

    /* 格要素の追加 */

    if (cf_ptr->voice == FRAME_PASSIVE_I ||
	cf_ptr->voice == FRAME_CAUSATIVE_WO_NI ||
	cf_ptr->voice == FRAME_CAUSATIVE_WO ||
	cf_ptr->voice == FRAME_CAUSATIVE_NI) {
	_make_ipal_cframe_pp(cf_ptr, "ガ", j);
	/* _make_ipal_cframe_sm(cf_ptr, "ＤＩＶ／ＨＵＭ", j, Thesaurus); 現在無効 */
	_make_ipal_cframe_sm(cf_ptr, "主体準", j, USE_NTT_WITH_STORE);
	_make_ipal_cframe_ex(cf_ptr, "彼", j, Thesaurus);
	j++;
    }

    /* 各格要素の処理 */

    for (i = 0; i < CASE_MAX_NUM && j < CASE_MAX_NUM && *(i_ptr->DATA+i_ptr->kaku_keishiki[i]); i++, j++) { 
	cf_ptr->adjacent[j] = FALSE;
	if (_make_ipal_cframe_pp(cf_ptr, i_ptr->DATA+i_ptr->kaku_keishiki[i], j) == FALSE) {
	    j--;
	    continue;
	}
	_make_ipal_cframe_sm(cf_ptr, i_ptr->DATA+i_ptr->imisosei[i], j, USE_NTT_WITH_STORE);
	if (Thesaurus == USE_BGH) {
	    _make_ipal_cframe_ex(cf_ptr, i_ptr->DATA+i_ptr->meishiku[i], j, USE_BGH_WITH_STORE);
	}
	else if (Thesaurus == USE_NTT) {
	    _make_ipal_cframe_ex(cf_ptr, i_ptr->DATA+i_ptr->meishiku[i], j, USE_NTT_WITH_STORE);
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
		      _make_ipal_cframe_pp(cf_ptr, "ヲ／ニ", j);
		    else if (cf_ptr->voice == FRAME_CAUSATIVE_WO)
		      _make_ipal_cframe_pp(cf_ptr, "ヲ", j);
		    else if (cf_ptr->voice == FRAME_CAUSATIVE_NI)
		      _make_ipal_cframe_pp(cf_ptr, "ニ", j);
		} else {
		    _make_ipal_cframe_pp(cf_ptr, "ヲ", j); /* ガ・ガ構文 */
		}
	    }
	}
	else if (cf_ptr->voice == FRAME_PASSIVE_I ||	/* 受身 */
		 cf_ptr->voice == FRAME_PASSIVE_1 ||
		 cf_ptr->voice == FRAME_PASSIVE_2) {
	    /* 間接 ガ→ニ，直接 ガ→ニ／ニヨッテ／．． */
	    if (!strcmp(i_ptr->DATA+i_ptr->kaku_keishiki[i], "ガ")) {
		if (cf_ptr->voice == FRAME_PASSIVE_I)
		  _make_ipal_cframe_pp(cf_ptr, "ニ", j);
		else if (cf_ptr->voice == FRAME_PASSIVE_1)
		  _make_ipal_cframe_pp(cf_ptr, 
				       i_ptr->DATA+i_ptr->tyoku_ukemi1, j);
		else if (cf_ptr->voice == FRAME_PASSIVE_2)
		  _make_ipal_cframe_pp(cf_ptr, 
				       i_ptr->DATA+i_ptr->tyoku_ukemi2, j);
	    }
	    /* 直接 ニ／ニヨッテ／．．→ガ */
	    else if ((cf_ptr->voice == FRAME_PASSIVE_1 && 
		      (!strcmp(i_ptr->DATA+i_ptr->kaku_keishiki[i], 
			       i_ptr->DATA+i_ptr->tyoku_noudou1) ||
		       (sprintf(ast_cap, "%s＊", 
				i_ptr->DATA+i_ptr->tyoku_noudou1) &&
			!strcmp(i_ptr->DATA+i_ptr->kaku_keishiki[i], 
				ast_cap)))) ||
		     (cf_ptr->voice == FRAME_PASSIVE_2 && 
		      (!strcmp(i_ptr->DATA+i_ptr->kaku_keishiki[i], 
			       i_ptr->DATA+i_ptr->tyoku_noudou2) ||
		       (sprintf(ast_cap, "%s＊", 
				i_ptr->DATA+i_ptr->tyoku_noudou2) &&
			!strcmp(i_ptr->DATA+i_ptr->kaku_keishiki[i], 
				ast_cap))))) {
		_make_ipal_cframe_pp(cf_ptr, "ガ", j);
	    }
	}
	else if (cf_ptr->voice == FRAME_POSSIBLE) {	/* 可能 */
	    if (!strcmp(i_ptr->DATA+i_ptr->kaku_keishiki[i], "ヲ")) {
		 _make_ipal_cframe_pp(cf_ptr, "ガ／ヲ", j);
	     }
	}
	else if (cf_ptr->voice == FRAME_SPONTANE) {	/* 自発 */
	    if (!strcmp(i_ptr->DATA+i_ptr->kaku_keishiki[i], "ヲ")) {
		 _make_ipal_cframe_pp(cf_ptr, "ガ", j);
	     }
	}
    }
    cf_ptr->element_num = j;
}

/*==================================================================*/
		     void f_num_inc(int *f_num_p)
/*==================================================================*/
{
    (*f_num_p)++;
    if ((Case_frame_num + *f_num_p) >= MAX_Case_frame_num) {
	realloc_cf();
	realloc_cmm();
    }
}

/*==================================================================*/
int _make_ipal_cframe_subcontract(BNST_DATA *b_ptr, int start, 
				  char *verb, int voice)
/*==================================================================*/
{
    IPAL_FRAME *i_ptr;
    CASE_FRAME *cf_ptr;
    int f_num = 0, address, break_flag = 0, size, match, c;
    char *pre_pos, *cp, *address_str, *vtype = NULL;

    if (!verb)
    	return f_num;

    cf_ptr = Case_frame_array+start;

    address_str = get_ipal_address(verb);

    /* なければ */
    if (!address_str)
	return f_num;

    if ((vtype = check_feature(b_ptr->f, "用言"))) {
	vtype += 5;
    }
    else if ((vtype = check_feature(b_ptr->f, "準用言"))) {
	;
    }

    for (cp = pre_pos = address_str; ; cp++) {
	if (*cp == '/' || *cp == '\0') {
	    if (*cp == '\0')
		break_flag = 1;
	    else 
		*cp = '\0';
	    
	    /* IPALデータの読みだし */
	    match = sscanf(pre_pos, "%d:%d", &address, &size);
	    if (match != 2) {
		fprintf(stderr, "CaseFrame Dictionary Index error (it seems version 1.).\n");
		exit(1);
	    }

	    i_ptr = get_ipal_frame(address, size);
	    pre_pos = cp + 1;

	    /* 用言のタイプがマッチしなければ (準用言なら通過) */
	    if (vtype && strncmp(i_ptr->DATA+i_ptr->id, vtype, 2)) {
		if (break_flag)
		    break;
		else
		    continue;
	    }

	    (cf_ptr+f_num)->entry = strdup(verb);

	    /* 能動態 */
	    if (voice == 0) {
		(cf_ptr+f_num)->voice = FRAME_ACTIVE;
		_make_ipal_cframe(i_ptr, cf_ptr+f_num, address, size);
		/* 格フレーム受身 */
		if (b_ptr->voice) {
		    /* ニ格がないとき */
		    if ((c = check_cf_case(cf_ptr+f_num, "ニ")) < 0) {
			_make_ipal_cframe_pp(cf_ptr+f_num, "ニ", (cf_ptr+f_num)->element_num);
			_make_ipal_cframe_sm(cf_ptr+f_num, "主体準", (cf_ptr+f_num)->element_num, USE_NTT_WITH_STORE);
			(cf_ptr+f_num)->element_num++;
		    }
		    /* ニ格はあるけど<主体>がないとき */
		    else if (sm_match_check(sm2code("主体"), (cf_ptr+f_num)->sm[c]) == FALSE) {
			_make_ipal_cframe_sm(cf_ptr+f_num, "主体準", c, USE_NTT_WITH_STORE);
		    }
		    (cf_ptr+f_num)->voice = FRAME_PASSIVE_1;
		}
		f_num_inc(&f_num);
		cf_ptr = Case_frame_array+start;
	    }

	    /* 使役 */
	    if (voice == VOICE_SHIEKI ||
		voice == VOICE_MORAU) {
		if (!strcmp(i_ptr->DATA+i_ptr->sase, "ヲ使役，ニ使役"))
		  (cf_ptr+f_num)->voice = FRAME_CAUSATIVE_WO_NI;
		else if (!strcmp(i_ptr->DATA+i_ptr->sase, "ヲ使役"))
		  (cf_ptr+f_num)->voice = FRAME_CAUSATIVE_WO;
		else if (!strcmp(i_ptr->DATA+i_ptr->sase, "ニ使役"))
		  (cf_ptr+f_num)->voice = FRAME_CAUSATIVE_NI;
		
		_make_ipal_cframe(i_ptr, cf_ptr+f_num, address, size);
		f_num_inc(&f_num);
		cf_ptr = Case_frame_array+start;
	    }
	    
	    /* 受身 */
	    if (voice == VOICE_UKEMI ||
		voice == VOICE_MORAU) {
		/* 直接受身１ */
		if (*(i_ptr->DATA+i_ptr->tyoku_noudou1)) {
		    (cf_ptr+f_num)->voice = FRAME_PASSIVE_1;
		    _make_ipal_cframe(i_ptr, cf_ptr+f_num, address, size);
		    f_num_inc(&f_num);
		    cf_ptr = Case_frame_array+start;
		}
		/* 直接受身２ */
		if (*(i_ptr->DATA+i_ptr->tyoku_noudou2)) {
		    (cf_ptr+f_num)->voice = FRAME_PASSIVE_2;
		    _make_ipal_cframe(i_ptr, cf_ptr+f_num, address, size);
		    f_num_inc(&f_num);
		    cf_ptr = Case_frame_array+start;
		}
		/* 間接受身 */
		if (str_part_eq(i_ptr->DATA+i_ptr->rare, "間受")) {
		    (cf_ptr+f_num)->voice = FRAME_PASSIVE_I;
		    _make_ipal_cframe(i_ptr, cf_ptr+f_num, address, size);
		    f_num_inc(&f_num);
		    cf_ptr = Case_frame_array+start;
		}
	    }
	    /* 可能，尊敬，自発 */
	    if (voice == VOICE_UKEMI) {
		if (str_part_eq(i_ptr->DATA+i_ptr->rare, "可能")) {
		    (cf_ptr+f_num)->voice = FRAME_POSSIBLE;
		    _make_ipal_cframe(i_ptr, cf_ptr+f_num, address, size);
		    f_num_inc(&f_num);
		    cf_ptr = Case_frame_array+start;
		}
		if (str_part_eq(i_ptr->DATA+i_ptr->rare, "尊敬")) {
		    (cf_ptr+f_num)->voice = FRAME_POLITE;
		    _make_ipal_cframe(i_ptr, cf_ptr+f_num, address, size);
		    f_num_inc(&f_num);
		    cf_ptr = Case_frame_array+start;
		}
		if (str_part_eq(i_ptr->DATA+i_ptr->rare, "自発")) {
		    (cf_ptr+f_num)->voice = FRAME_SPONTANE;
		    _make_ipal_cframe(i_ptr, cf_ptr+f_num, address, size);
		    f_num_inc(&f_num);
		    cf_ptr = Case_frame_array+start;
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
int make_ipal_cframe_subcontract(BNST_DATA *b_ptr, int start, char *verb)
/*==================================================================*/
{
    int f_num = 0, plus_num;

    if (check_feature(b_ptr->f, "サ変止")) {
	/* 能動態 
	   b_ptr->voice == 0 で行う */
	b_ptr->voice = 0;
	f_num = _make_ipal_cframe_subcontract(b_ptr, start, verb, 0);
	b_ptr->voice = VOICE_UNKNOWN;
    }

    /* 受身の格フレーム */
    if (b_ptr->voice == VOICE_UKEMI ||
	b_ptr->voice == VOICE_MORAU || 
	b_ptr->voice == VOICE_UNKNOWN) {
	strcat(verb, ":A");
	plus_num = _make_ipal_cframe_subcontract(b_ptr, start+f_num, verb, 0);
	if (plus_num != 0) {
	    return f_num+plus_num;
	}
	*(verb+strlen(verb)-2) = '\0';	/* みつからなかったらもとにもどす */
    }

    if (b_ptr->voice == VOICE_UNKNOWN) {
	/* 受身 */
	f_num += _make_ipal_cframe_subcontract(b_ptr, start+f_num, verb, VOICE_UKEMI);
	return f_num;
    }
    return _make_ipal_cframe_subcontract(b_ptr, start, verb, b_ptr->voice);
}

/*==================================================================*/
 int make_ipal_cframe(SENTENCE_DATA *sp, BNST_DATA *b_ptr, int start)
/*==================================================================*/
{
    int f_num = 0, i;
    char *verb, buffer[3][WORD_LEN_MAX];

    /* 自立語末尾語を用いて格フレーム辞書を引く */

    if (!b_ptr->jiritu_ptr) {
	return f_num;
    }

    /* 「（〜を）〜に」 のときは 「する」 で探す */
    if (check_feature(b_ptr->f, "ID:（〜を）〜に")) {
	sprintf(buffer[0], "する");
	verb = buffer[0];
    }
    /* 「〜化」, 「〜的だ」 で 名詞+接尾辞(自立語) の形のもの
       (準用言ではない … 「〜年」などの時間を除く) */
    /* ★ そのうち ルール化する */
    else if (b_ptr->jiritu_num > 1 && L_Jiritu_M(b_ptr)->Hinshi == 14 && 
	     !check_feature(b_ptr->f, "準用言")) {
	assign_cfeature(&(b_ptr->f), "名詞+接尾辞");
	sprintf(buffer[0], "%s%s", (b_ptr->jiritu_ptr+b_ptr->jiritu_num-2)->Goi, L_Jiritu_M(b_ptr)->Goi);
	verb = buffer[0];

	/* 「形容詞+する」 */
	if (check_feature(b_ptr->f, "形容詞スル")) {
	    strcat(verb, "する");
	}

	f_num = make_ipal_cframe_subcontract(b_ptr, start, verb);
	if (f_num != 0) {
	    Case_frame_num += f_num;
	    for (i = 0; i < f_num; i++) {
		(Case_frame_array+start+i)->concatenated_flag = 0;
	    }
	    return f_num;
	}
	strcpy(buffer[0], L_Jiritu_M(b_ptr)->Goi);
    }
    else {
	strcpy(buffer[0], L_Jiritu_M(b_ptr)->Goi);
	verb = buffer[0];
    }

    /* 「形容詞+する」 */
    if (check_feature(b_ptr->f, "形容詞スル")) {
	strcat(verb, "する");
    }

    f_num = make_ipal_cframe_subcontract(b_ptr, start, verb);
    Case_frame_num += f_num;
    for (i = 0; i < f_num; i++) {
	(Case_frame_array+start+i)->concatenated_flag = 0;
    }

    return f_num;
}

/*==================================================================*/
	 int make_default_cframe(BNST_DATA *b_ptr, int start)
/*==================================================================*/
{
    int i, num = 0, f_num = 0;
    CASE_FRAME *cf_ptr;

    cf_ptr = Case_frame_array+start;

    if (MAX_ipal_frame_length == 0) {
	ipal_str_buf = 
	    (unsigned char *)realloc_data(ipal_str_buf, 
					  sizeof(unsigned char)*ALLOCATION_STEP, 
					  "make_default_cframe");
    }

    if (check_feature(b_ptr->f, "用言:判")) {
	_make_ipal_cframe_pp(cf_ptr, "ガ＊", num);
	_make_ipal_cframe_sm(cf_ptr, "主体", num++, USE_NTT_WITH_STORE);
	cf_ptr->ipal_address = 1;
	strcpy(cf_ptr->ipal_id, "判");
    }
    else {
	_make_ipal_cframe_pp(cf_ptr, "ガ＊", num++);
	_make_ipal_cframe_pp(cf_ptr, "ヲ＊", num++);
	_make_ipal_cframe_pp(cf_ptr, "ニ＊", num++);
	_make_ipal_cframe_pp(cf_ptr, "ヘ＊", num++);
	_make_ipal_cframe_pp(cf_ptr, "ヨリ＊", num++);
	cf_ptr->ipal_address = -1;
	cf_ptr->ipal_id[0] = '\0';
    }

    cf_ptr->element_num = num;
    cf_ptr->etcflag = CF_NORMAL;

    for (i = 0; i < num; i++) {
	cf_ptr->pp[i][1] = END_M;
    }

    f_num_inc(&f_num);
    Case_frame_num++;
    b_ptr->cf_num = 1;
    return 1;
}

/*==================================================================*/
      void make_case_frames(SENTENCE_DATA *sp, BNST_DATA *b_ptr)
/*==================================================================*/
{
    if ((b_ptr->cf_num = make_ipal_cframe(sp, b_ptr, Case_frame_num)) == 0) {
	make_default_cframe(b_ptr, Case_frame_num);
    }
}

/*==================================================================*/
	      void set_pred_caseframe(SENTENCE_DATA *sp)
/*==================================================================*/
{
    int i, start[BNST_MAX];
    BNST_DATA  *b_ptr;

    Case_frame_num = 0;

    for (i = 0, b_ptr = sp->bnst_data; i < sp->Bnst_num; i++, b_ptr++) {
	/* 正解コーパスを入力したときに自立語がない場合がある */
	if (b_ptr->jiritu_ptr != NULL && 
	    (check_feature(b_ptr->f, "用言") ||
	     /* check_feature(b_ptr->f, "準用言") || */
	     check_feature(b_ptr->f, "サ変名詞格解析"))) {

	    /* 以下の2つの処理はfeatureレベルで起動している */
	    /* set_pred_voice(b_ptr); ヴォイス */
	    /* get_scase_code(b_ptr); 表層格 */

	    start[i] = Case_frame_num;
	    make_case_frames(sp, b_ptr);
	}
	else {
	    start[i] = -1;
	}
    }

    /* 格フレームを文節へリンクする */
    for (i = 0; i < sp->Bnst_num; i++) {
	if (start[i] >= 0) {
	    (sp->bnst_data+i)->cf_ptr = Case_frame_array+start[i];
	}
	else {
	    (sp->bnst_data+i)->cf_ptr = NULL;
	    (sp->bnst_data+i)->cf_num = 0;
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
	    if (Thesaurus == USE_BGH) {
		if ((Case_frame_array+i)->ex[j]) {
		    free((Case_frame_array+i)->ex[j]);
		    (Case_frame_array+i)->ex[j] = NULL;
		}
	    }
	    else if (Thesaurus == USE_NTT) {
		if ((Case_frame_array+i)->ex2[j]) {
		    free((Case_frame_array+i)->ex2[j]);
		    (Case_frame_array+i)->ex2[j] = NULL;
		}
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
	    if ((Case_frame_array+i)->ex_list[j]) {
		for (k = 0; k < (Case_frame_array+i)->ex_num[j]; k++) {
		    if ((Case_frame_array+i)->ex_list[j][k]) {
			free((Case_frame_array+i)->ex_list[j][k]);
		    }
		}
		free((Case_frame_array+i)->ex_list[j]);
		(Case_frame_array+i)->ex_list[j] = NULL;
		(Case_frame_array+i)->ex_size[j] = 0;
		(Case_frame_array+i)->ex_num[j] = 0;
	    }
	    if ((Case_frame_array+i)->examples[j]) {
		free((Case_frame_array+i)->examples[j]);
		(Case_frame_array+i)->examples[j] = NULL;
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
    }
}

/*==================================================================*/
	       void MakeInternalBnst(SENTENCE_DATA *sp)
/*==================================================================*/
{
    int i, j, suffix_num;
    BNST_DATA *bp;

    /* 最後の自立語がサ変名詞のときに、それより前の名詞を
       仮の文節として考える */

    for (i = 0; i < sp->Bnst_num; i++) {
	bp = sp->bnst_data+i;
	bp->internal_num = 0;

	/* 自立語末尾がサ変名詞でない場合 */
	if (bp->jiritu_ptr == NULL || !check_feature(L_Jiritu_M(bp)->f, "サ変") || 
	    check_feature(bp->f, "用言:判")) {
	    continue;
	}

	/* 複合名詞であり、Head の直前が数詞でない
	   (「６１連勝」などを除く) */
	if (bp->jiritu_num > 1 && 
	    !((bp->jiritu_ptr+bp->jiritu_num-2)->Bunrui == 7 && 
	      (bp->jiritu_ptr+bp->jiritu_num-2)->Hinshi == 6)) {
	    suffix_num = 0;
	    for (j = 2; j <= bp->jiritu_num; j++) {
		/* 名詞 or 副詞であれば */
		if ((bp->jiritu_ptr+bp->jiritu_num-j)->Hinshi == 6 || 
		    (bp->jiritu_ptr+bp->jiritu_num-j)->Hinshi == 8) {
		    if (bp->internal_num == 0) {
			bp->internal = (BNST_DATA *)malloc_data(sizeof(BNST_DATA), "MakeInternalBnst");
			bp->internal_max = 1;
		    }
		    else if (bp->internal_num >= bp->internal_max) {
			bp->internal = (BNST_DATA *)realloc_data(bp->internal, 
								 sizeof(BNST_DATA)*(bp->internal_max <<= 1), 
								 "MakeInternalBnst");
		    }
		    assign_cfeature(&(bp->f), "内部文節");
		    memset(bp->internal+bp->internal_num, 0, sizeof(BNST_DATA));
		    (bp->internal+bp->internal_num)->num = -1;
		    (bp->internal+bp->internal_num)->mrph_num = suffix_num+1;
		    (bp->internal+bp->internal_num)->mrph_ptr = bp->jiritu_ptr+bp->jiritu_num-j;
		    (bp->internal+bp->internal_num)->jiritu_num = suffix_num+1;
		    (bp->internal+bp->internal_num)->jiritu_ptr = bp->jiritu_ptr+bp->jiritu_num-j;
		    strcpy((bp->internal+bp->internal_num)->Jiritu_Go, 
			   (bp->internal+bp->internal_num)->jiritu_ptr->Goi);

		    /* 文節ルールを適用する */
		    _assign_general_feature(bp->internal+bp->internal_num, 1, BnstRuleType);

		    get_bnst_code(bp->internal+bp->internal_num, USE_BGH);
		    get_bnst_code(bp->internal+bp->internal_num, USE_NTT);
		    assign_sm_aux_feature(bp->internal+bp->internal_num);
		    assign_cfeature(&((bp->internal+bp->internal_num)->f), "係:文節内");
		    (bp->internal+bp->internal_num)->parent = bp;
		    bp->internal_num++;
		    break;	/* とりあえず、ひとつできれば終わる */
		}
		/* 接尾辞は保存しておいて、名詞が来たときにくっつける */
		else if ((bp->jiritu_ptr+bp->jiritu_num-j)->Hinshi == 14) {
		    suffix_num++;
		}
	    }
	}
    }
}

/*==================================================================*/
	     int check_cf_case(CASE_FRAME *cfp, char *pp)
/*==================================================================*/
{
    int i;
    for (i = 0; i < cfp->element_num; i++) {
	if (MatchPP(cfp->pp[i][0], pp)) {
	    return i;
	}
    }
    return -1;
}

/*====================================================================
                               END
====================================================================*/
