/*====================================================================

			 NTT  検索プログラム

                                               S.Kurohashi 91. 6.25
                                               S.Kurohashi 93. 5.31

    $Id$
====================================================================*/
#include "knp.h"

DBM_FILE	sm_db;
DBM_FILE	sm2code_db;
DBM_FILE	code2sm_db;
DBM_FILE	smp2smg_db;
int		SMExist;
int		SM2CODEExist;
int		CODE2SMExist;
int		SMP2SMGExist;

char  		cont_str[DBM_CON_MAX];

/*==================================================================*/
			   void init_ntt()
/*==================================================================*/
{
    char *filename;

    /***  データベースオープン  ***/
    
    /* 単語 <=> 意味素コード */

    // ファイル名を指定する
    if (DICT[SM_DB]) {   
	filename = check_dict_filename(DICT[SM_DB], TRUE);  // .knprc で定義されているとき   → SM_ADD_DB は const.h で定義されている
	                                                    //                                  DICT[SM_ADD_DB] は configfile.c で指定されている
    }
    else {
	filename = check_dict_filename(SM_DB_NAME, FALSE);  // .knprc で定義されていないとき → path.h の default値(SM_DB_NAME) を使う
    }

    if (OptDisplay == OPT_DEBUG) {
	fprintf(Outfp, "Opening %s ... ", filename);
    }

    if ((sm_db = DB_open(filename, O_RDONLY, 0)) == NULL) {
	if (OptDisplay == OPT_DEBUG) {
	    fputs("failed.\n", Outfp);
	}
	SMExist = FALSE;
#ifdef DEBUG
	fprintf(stderr, ";; Cannot open NTT word dictionary <%s>.\n", filename);
#endif
    }
    else {
	if (OptDisplay == OPT_DEBUG) {
	    fputs("done.\n", Outfp);
	}
	SMExist = TRUE;
    }
    free(filename);

    
    /* 意味素 => 意味素コード */
    if (Thesaurus == USE_NTT) {
	if (DICT[SM2CODE_DB]) {
	    filename = check_dict_filename(DICT[SM2CODE_DB], TRUE);
	}
	else {
	    filename = check_dict_filename(SM2CODE_DB_NAME, FALSE);
	}

	if (OptDisplay == OPT_DEBUG) {
	    fprintf(Outfp, "Opening %s ... ", filename);
	}

	if ((sm2code_db = DB_open(filename, O_RDONLY, 0)) == NULL) {
	    if (OptDisplay == OPT_DEBUG) {
		fputs("failed.\n", Outfp);
	    }
	    SM2CODEExist = FALSE;
#ifdef DEBUG
	    fprintf(stderr, ";; Cannot open NTT sm dictionary <%s>.\n", filename);
#endif
	}
	else {
	    if (OptDisplay == OPT_DEBUG) {
		fputs("done.\n", Outfp);
	    }
	    SM2CODEExist = TRUE;
	}
	free(filename);
    }

    /* 意味素コード => 意味素 */
    if (DICT[CODE2SM_DB]) {
	filename = check_dict_filename(DICT[CODE2SM_DB], TRUE);
    }
    else {
	filename = check_dict_filename(CODE2SM_DB_NAME, FALSE);
    }

    if (OptDisplay == OPT_DEBUG) {
	fprintf(Outfp, "Opening %s ... ", filename);
    }

    if ((code2sm_db = DB_open(filename, O_RDONLY, 0)) == NULL) {
	if (OptDisplay == OPT_DEBUG) {
	    fputs("failed.\n", Outfp);
	}
	CODE2SMExist = FALSE;
#ifdef DEBUG
	fprintf(stderr, ";; Cannot open NTT code2sm dictionary <%s>.\n", filename);
#endif
    }
    else {
	if (OptDisplay == OPT_DEBUG) {
	    fputs("done.\n", Outfp);
	}
	CODE2SMExist = TRUE;
    }
    free(filename);

    /* 固有名詞体系 <=> 一般名詞体系 */
    if (DICT[SMP2SMG_DB]) {
	filename = check_dict_filename(DICT[SMP2SMG_DB], TRUE);
    }
    else {
	filename = check_dict_filename(SMP2SMG_DB_NAME, FALSE);
    }

    if (OptDisplay == OPT_DEBUG) {
	fprintf(Outfp, "Opening %s ... ", filename);
    }

    if ((smp2smg_db = DB_open(filename, O_RDONLY, 0)) == NULL) {
	if (OptDisplay == OPT_DEBUG) {
	    fputs("failed.\n", Outfp);
	}
	SMP2SMGExist = FALSE;
#ifdef DEBUG
	fprintf(stderr, ";; Cannot open NTT smp smg table <%s>.\n", filename);
#endif
    }
    else {
	if (OptDisplay == OPT_DEBUG) {
	    fputs("done.\n", Outfp);
	}
	SMP2SMGExist = TRUE;
    }
    free(filename);
}


/*==================================================================*/
			   void close_ntt()
/*==================================================================*/
{
    if (SMExist == TRUE)
	DB_close(sm_db);

    if (SM2CODEExist == TRUE)
	DB_close(sm2code_db);

    if (SMP2SMGExist == TRUE)
	DB_close(smp2smg_db);
}

/*==================================================================*/
		   int ne_check_all_sm(char *code)
/*==================================================================*/
{
    int i;

    /* すべての意味属性が固有名詞なら TRUE */

    for (i = 0; *(code+i); i+=SM_CODE_SIZE) {
	if (*(code+i) != '2') {
	    return FALSE;
	}
    }
    return TRUE;
}

/*==================================================================*/
                 char *_get_ntt(char *cp, char *arg)
/*==================================================================*/
{

    /* データベースから取り出した code を処理する */
    int i, j, pos;
    char *code;

    code = db_get(sm_db, cp);
    if (code) {

	/* 溢れたら、縮める */
	if (strlen(code) > SM_CODE_SIZE*SM_ELEMENT_MAX) {
#ifdef DEBUG
    fprintf(stderr, "Too long SM content <%s>.\n", code);
#endif
    code[SM_CODE_SIZE*SM_ELEMENT_MAX] = '\0';
	}
	
	pos = 0;
	
	/* すべての意味属性が固有名詞のとき */
	if (ne_check_all_sm(code) == TRUE) {
	    for (i = 0; code[i]; i+=SM_CODE_SIZE) {
		if (code[i] == '2' && 
		    strncmp(code+i, "2001030", 7)) { /* 大字 ではない */
		    strncpy(code+pos, code+i, SM_CODE_SIZE);
		    pos += SM_CODE_SIZE;
		}
	    }
	}
	else {
	    /* 意味素を付与する品詞 */
	    for (i = 0; code[i]; i+=SM_CODE_SIZE) {
		if ((*arg && code[i] == *arg) ||	/* 指定された品詞 */
		    code[i] == '3' ||	/* 名 */
		    code[i] == '4' ||	/* 名(形式) */
		    code[i] == '5' ||	/* 名(形動) */
		    code[i] == '6' ||	/* 名(転生) */
		    code[i] == '7' ||	/* サ変 */
		    code[i] == '9' ||	/* 時詞 */
		    code[i] == 'a') {	/* 代名 */
		    strncpy(code+pos, code+i, SM_CODE_SIZE);
		    pos += SM_CODE_SIZE;
		}
	    }
	}
	code[pos] = '\0';
    }
    return code;
}

/*==================================================================*/
		       char *sm2code(char *cp)
/*==================================================================*/
{
    char *code;

    /* sm と code は 1:1 対応 
       -> cont_str は溢れない */

    if (SM2CODEExist == FALSE) {
	cont_str[0] = '\0';
	return cont_str;
    }

    code = db_get(sm2code_db, cp);
    if (code) {
	strcpy(cont_str, code);
	free(code);
    }
    else {
	cont_str[0] = '\0';
    }
    return cont_str;
}

/*==================================================================*/
		       char *code2sm(char *cp)
/*==================================================================*/
{
    char *sm;

    /* sm と code は 1:1 対応 
       -> cont_str は溢れない */

    if (CODE2SMExist == FALSE) {
	cont_str[0] = '\0';
	return cont_str;
    }

    sm = db_get(code2sm_db, cp);
    if (sm) {
	strcpy(cont_str, sm);
	free(sm);
    }
    else {
	cont_str[0] = '\0';
    }
    return cont_str;
}

/*==================================================================*/
		       char *_smp2smg(char *cp)
/*==================================================================*/
{
    char *code, key[SM_CODE_SIZE+1];

    /* 値は長くても 52 bytes ぐらい */

    if (SMP2SMGExist == FALSE) {
	cont_str[0] = '\0';
	return cont_str;
    }

    strncpy(key, cp, SM_CODE_SIZE);
    key[SM_CODE_SIZE] = '\0';

    code = db_get(smp2smg_db, key);
    return code;
}

/*==================================================================*/
		  char *smp2smg(char *cpd, int flag)
/*==================================================================*/
{
    char *cp, *start;
    int storep = 0, inc, use = 1;

    if (SMP2SMGExist == FALSE) {
	fprintf(stderr, ";;; Cannot open smp2smg table!\n");
	return NULL;
    }

    start = _smp2smg(cpd);

    if (start == NULL) {
	return NULL;
    }

    for (cp = start; *cp; cp+=SM_CODE_SIZE) {
	use = 1;
	if (*(cp+SM_CODE_SIZE) == '/') {
	    inc = 1;
	}
	else if (!strncmp(cp+SM_CODE_SIZE, " side-effect", 12)) {
	    if (*(cp+SM_CODE_SIZE+12) == '/') {
		inc = 13;		
	    }
	    /* 今回で終わり */
	    else {
		inc = 0;
	    }
	    /* flag == FALSE の場合 side-effect を使わない */
	    if (flag == FALSE) {
		use = 0;
	    }
	}
	else if (*(cp+SM_CODE_SIZE) != '\0') {
	    fprintf(stderr, ";;; Invalid delimiter! <%c> (%s)\n", 
		    *(cp+SM_CODE_SIZE), "smp2smg");
	    inc = 1;
	}
	/* 今回で終わり '\0' */
	else {
	    inc = 0;
	}

	if (use) {
	    strncpy(start+storep, cp, SM_CODE_SIZE);
	    storep+=SM_CODE_SIZE;
	}
	if (inc) {
	    cp += inc;
	}
	else {
	    break;
	}
    }

    if (storep) {
	*(start+storep) = '\0';
	return start;
    }
    free(start);
    return NULL;
}

/*==================================================================*/
		   void merge_smp2smg(BNST_DATA *bp)
/*==================================================================*/
{
    int i;
    char *p;

    /* smp2smg の結果をくっつける */

    if (bp->SM_code[0] == '\0') {
	return;
    }

    for (i = 0; i < bp->SM_num; i++) {
	if (bp->SM_code[i*SM_CODE_SIZE] == '2') {
	    p = smp2smg(&(bp->SM_code[i*SM_CODE_SIZE]), FALSE);
	    if (p) {
		/* 溢れた場合 */
		if ((strlen(bp->SM_code)+strlen(p))/SM_CODE_SIZE > SM_ELEMENT_MAX) {
		    return;
		}
		strcat(bp->SM_code, p);
		free(p);
	    }
	}
    }
    bp->SM_num = strlen(bp->SM_code)/SM_CODE_SIZE;
}

/*==================================================================*/
		     int sm_code_depth(char *cp)
/*==================================================================*/
{
    int i;

    /* 意味素コードの深さを返す関数 (0 .. SM_CODE_SIZE-1) */

    for (i = 1; i < SM_CODE_SIZE; i++) {
	if (*(cp+i) == '*') {
	    return i-1;
	}
    }
    return SM_CODE_SIZE-1;
}

/*==================================================================*/
	      float _ntt_code_match(char *c1, char *c2)
/*==================================================================*/
{
    int i, d1, d2, min;

    if ((*c1 == '2' && *c2 != '2') || 
	(*c1 != '2' && *c2 == '2')) {
	return 0;
    }

    d1 = sm_code_depth(c1);
    d2 = sm_code_depth(c2);

    if (d1 + d2 == 0) {
	return 0;
    }

    min = d1 < d2 ? d1 : d2;

    if (min == 0) {
	return 0;
    }

    for (i = 1; i <= min; i++) {
	if (*(c1+i) != *(c2+i)) {
	    return (float)2*(i-1)/(d1+d2);
	}
    }
    return (float)2*min/(d1+d2);
}

/*==================================================================*/
	  float ntt_code_match(char *c1, char *c2, int flag)
/*==================================================================*/
{
    if (flag == SM_EXPAND_NE) {
	float score, maxscore = 0;
	char *cp1, *cp2;
	int i, j;
	int f1 = 0, f2 = 0, c1num = 1, c2num = 1;

	if (*c1 == '2') {
	    c1 = smp2smg(c1, FALSE);
	    if (!c1) {
		return 0;
	    }
	    f1 = 1;
	    c1num = strlen(c1)/SM_CODE_SIZE;
	}
	if (*c2 == '2') {
	    c2 = smp2smg(c2, FALSE);
	    if (!c2) {
		if (f1 == 1) {
		    free(c1);
		}
		return 0;
	    }
	    f2 = 1;
	    c2num = strlen(c2)/SM_CODE_SIZE;
	}

	for (cp1 = c1, i = 0; i < c1num; cp1+=SM_CODE_SIZE, i++) {
	    for (cp2 = c2, j = 0; j < c2num; cp2+=SM_CODE_SIZE, j++) {
		score = _ntt_code_match(cp1, cp2);
		if (score > maxscore) {
		    maxscore = score;
		}
	    }
	}
	if (f1 == 1) {
	    free(c1);
	}
	if (f2 == 1) {
	    free(c2);
	}
	return maxscore;
    }
    else if (flag == SM_EXPAND_NE_DATA) {
	float score, maxscore = 0;
	char *cp2;
	int i;
	int f2 = 0, c2num = 1;

	/* PATTERN: 固有名詞 */
	if (*c1 == '2') {
	    return _ntt_code_match(c1, c2);
	}

	/* PATTERN: 普通名詞 */

	if (*c2 == '2') {
	    c2 = smp2smg(c2, FALSE);
	    if (!c2) {
		return 0;
	    }
	    f2 = 1;
	    c2num = strlen(c2)/SM_CODE_SIZE;
	}

	for (cp2 = c2, i = 0; i < c2num; cp2+=SM_CODE_SIZE, i++) {
	    score = _ntt_code_match(c1, cp2);
	    if (score > maxscore) {
		maxscore = score;
	    }
	}
	if (f2 == 1) {
	    free(c2);
	}
	return maxscore;
    }
    else {
	return _ntt_code_match(c1, c2);
    }
}

/*==================================================================*/
	       int sm_match_check(char *pat, char *codes)
/*==================================================================*/
{
    int i;

    if (codes == NULL) {
	return FALSE;
    }

    for (i = 0; *(codes+i); i += SM_CODE_SIZE) {
	if (_sm_match_score(pat, codes+i, SM_NO_EXPAND_NE) > 0) {
	    return TRUE;
	}
    }
    return FALSE;
}

/*==================================================================*/
		int assign_sm(BNST_DATA *bp, char *cp)
/*==================================================================*/
{
    char *code;
    code = sm2code(cp);

    /* すでにその意味属性をもっているとき */
    if (sm_match_check(code, bp->SM_code) == TRUE) {
	return FALSE;
    }

    /* ★溢れる?★ */
    strcat(bp->SM_code, code);
    bp->SM_num++;
    return TRUE;
}

/*==================================================================*/
 int sm_check_match_max(char *exd, char *exp, int expand, char *target)
/*==================================================================*/
{
    int i, j, step = SM_CODE_SIZE, flag;
    float score = 0, tempscore;

    /* どちらかに用例のコードがないとき */
    if (!(exd && exp && *exd && *exp)) {
	return FALSE;
    }

    if (expand != SM_NO_EXPAND_NE) {
	expand = SM_EXPAND_NE_DATA;
    }

    /* 最大マッチスコアを求める */
    for (j = 0; exp[j]; j+=step) {
	for (i = 0; exd[i]; i+=step) {
	    tempscore = ntt_code_match(exp+j, exd+i, expand);
	    if (tempscore > score) {
		score = tempscore;
		/* 両方 target 意味素に属す */
		if (sm_match_check(target, exd) && sm_match_check(target, exp)) {
		    flag = TRUE;
		}
		else {
		    flag = FALSE;
		}
	    }
	}
    }
    return flag;
}

/*==================================================================*/
	       int sm_fix(BNST_DATA *bp, char *targets)
/*==================================================================*/
{
    int i, j, pos = 0;
    char *codes;

    if (bp->SM_code[0] == '\0') {
	return FALSE;
    }

    codes = bp->SM_code;

    for (i = 0; *(codes+i); i += SM_CODE_SIZE) {
	for (j = 0; *(targets+j); j += SM_CODE_SIZE) {
	    if (_sm_match_score(targets+j, codes+i, SM_NO_EXPAND_NE) > 0) {
		strncpy(codes+pos, codes+i, SM_CODE_SIZE);
		pos += SM_CODE_SIZE;
		break;
	    }
	}
    }

    /* match しない場合ってどんなとき? */
    if (pos != 0) {
	*(codes+pos) = '\0';
	bp->SM_num = strlen(codes)/SM_CODE_SIZE;
    }
    return TRUE;
}

/*==================================================================*/
	       int sm_all_match(char *c, char *target)
/*==================================================================*/
{
    char *p, flag = 0;

    /* 固有名詞のとき以外で、すべての意味属性が時間であれば TRUE */
    for (p = c;*p; p+=SM_CODE_SIZE) {
	/* 固有名詞のときをのぞく */
	if (*p == '2') {
	    continue;
	}

	/* 意味素のチェック */
	if (!comp_sm(target, p, 1)) {
	    return FALSE;
	}
	else if (!flag) {
	    flag = 1;
	}
    }

    if (flag) {
	return TRUE;
    }
    else {
	return FALSE;
    }
}

/*==================================================================*/
	       void assign_time_feature(BNST_DATA *bp)
/*==================================================================*/
{
    /* <時間> の意味素しかもっていなければ <時間> を与える */

    if (!check_feature(bp->f, "時間") && 
	sm_all_match(bp->SM_code, sm2code("時間"))) {
	assign_cfeature(&(bp->f), "時間判定");
	assign_cfeature(&(bp->f), "時間");
    }
}

/*==================================================================*/
	      void assign_sm_aux_feature(BNST_DATA *bp)
/*==================================================================*/
{
    /* ルールに入れた */

    if (Thesaurus != USE_NTT) {
	return;
    }

    /* <時間>属性を付与する */
    assign_time_feature(bp);

    /* <抽象>属性を付与する */
    if (sm_all_match(bp->SM_code, sm2code("抽象"))) {
	assign_cfeature(&(bp->f), "抽象");
    }
}

/*==================================================================*/
	      int delete_matched_sm(char *sm, char *del)
/*==================================================================*/
{
    int i, j, flag, pos = 0;

    for (i = 0; sm[i]; i += SM_CODE_SIZE) {
	flag = 1;
	/* 固有ではないときチェック */
	if (sm[i] != '2') {
	    for (j = 0; del[j]; j += SM_CODE_SIZE) {
		if (_sm_match_score(sm+i, del+j, SM_NO_EXPAND_NE) > 0) {
		    flag = 0;
		    break;
		}
	    }
	}
	if (flag) {
	    strncpy(sm+pos, sm+i, SM_CODE_SIZE);
	    pos += SM_CODE_SIZE;
	}
    }
    *(sm+pos) = '\0';
    return 1;
}

/*==================================================================*/
	     int delete_specified_sm(char *sm, char *del)
/*==================================================================*/
{
    int i, j, flag, pos = 0;

    for (i = 0; sm[i]; i += SM_CODE_SIZE) {
	flag = 1;
	/* 固有ではないときを対象とする */
	if (sm[i] != '2') {
	    for (j = 0; del[j]; j += SM_CODE_SIZE) {
		if (!strncmp(sm+i+1, del+j+1, SM_CODE_SIZE-1)) {
		    flag = 0;
		    break;
		}
	    }
	}
	if (flag) {
	    strncpy(sm+pos, sm+i, SM_CODE_SIZE);
	    pos += SM_CODE_SIZE;
	}
    }
    *(sm+pos) = '\0';
    return 1;
}

/*==================================================================*/
		void fix_sm_person(SENTENCE_DATA *sp)
/*==================================================================*/
{
    int i;

    if (Thesaurus != USE_NTT) return;

    /* 人名のとき: 
       o 一般名詞体系の<主体>以下の意味素を削除
       o 固有名詞体系の意味素の一般名詞体系へのマッピングを禁止 */

    for (i = 0; i < sp->Bnst_num; i++) {
	if (check_feature((sp->bnst_data+i)->f, "人名")) {
	    /* 固有の意味素だけ残したい */
	    delete_matched_sm((sp->bnst_data+i)->SM_code, "100*********"); /* <主体>の意味素 */
	    assign_cfeature(&((sp->bnst_data+i)->f), "Ｔ固有一般展開禁止");
	}
    }
}

/*==================================================================*/
      void fix_sm_place(SENTENCE_DATA *sp, CF_PRED_MGR *cpm_ptr)
/*==================================================================*/
{
    /* そのうち汎用化する
       現在は <場所> のみ */

    int i, num;

    if (Thesaurus != USE_NTT) return;

    for (i = 0; i < cpm_ptr->cf.element_num; i++) {
	num = cpm_ptr->cmm[0].result_lists_d[0].flag[i];
	/* 省略格要素ではない割り当てがあったとき */
	if (cpm_ptr->elem_b_num[i] > -2 && 
	    num >= 0 && 
	    MatchPP(cpm_ptr->cmm[0].cf_ptr->pp[num][0], "デ") && 
	    cf_match_element(cpm_ptr->cmm[0].cf_ptr->sm[num], "場所", TRUE)) {
	    /* 固有→一般変換しておく */
	    merge_smp2smg((BNST_DATA *)cpm_ptr->elem_b_ptr[i]);
	    /* <場所>のみに限定する */
	    sm_fix((BNST_DATA *)cpm_ptr->elem_b_ptr[i], "101*********20**********");
	    assign_cfeature(&(cpm_ptr->elem_b_ptr[i]->f), "Ｔ固有一般展開禁止");
	    assign_cfeature(&(cpm_ptr->elem_b_ptr[i]->f), "非主体");
	    break;
	}
    }
}

/*==================================================================*/
   void assign_ga_subject(SENTENCE_DATA *sp, CF_PRED_MGR *cpm_ptr)
/*==================================================================*/
{
    int i, num;

    if (Thesaurus != USE_NTT) return;

    for (i = 0; i < cpm_ptr->cf.element_num; i++) {
	num = cpm_ptr->cmm[0].result_lists_d[0].flag[i];
	/* 省略格要素ではない割り当てがあったとき */
	if (cpm_ptr->elem_b_num[i] > -2 && 
	    cpm_ptr->cmm[0].result_lists_d[0].flag[i] >= 0 && 
	    MatchPP(cpm_ptr->cmm[0].cf_ptr->pp[num][0], "ガ")) {
	    /* o すでに主体付与されていない
	       o <数量> ではない (<数量>のとき意味属性がない)
	       o <用言:動>である 
	       o 格フレームが<主体>をもつ, <主体準>ではない
	       o 入力側が意味素がないか、(固有名詞と推定)
	         <抽象物> or <事>という意味素をもつ (つまり、<抽象的関係>だけではない)
	    */
	    if (!check_feature(cpm_ptr->elem_b_ptr[i]->f, "主体付与") && 
		!check_feature(cpm_ptr->elem_b_ptr[i]->f, "数量") && 
		check_feature(cpm_ptr->pred_b_ptr->f, "用言:動") && 
		cf_match_element(cpm_ptr->cmm[0].cf_ptr->sm[num], "主体", TRUE) && 
		(cpm_ptr->elem_b_ptr[i]->SM_num == 0 || 
		 /* (!(cpm_ptr->cmm[0].cf_ptr->etcflag & CF_GA_SEMI_SUBJECT) && ( */
		 sm_match_check(sm2code("具体"), cpm_ptr->elem_b_ptr[i]->SM_code) || 
		 sm_match_check(sm2code("地名"), cpm_ptr->elem_b_ptr[i]->SM_code) || /* 組織名, 人名はすでに主体 */
		 sm_match_check(sm2code("抽象物"), cpm_ptr->elem_b_ptr[i]->SM_code) || 
		 sm_match_check(sm2code("事"), cpm_ptr->elem_b_ptr[i]->SM_code))) {
		assign_sm((BNST_DATA *)cpm_ptr->elem_b_ptr[i], "主体");
		assign_cfeature(&(cpm_ptr->elem_b_ptr[i]->f), "主体付与");
	    }
	    break;
	}
    }
}

/*====================================================================
                               END
====================================================================*/
