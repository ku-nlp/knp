/*====================================================================

			格構造解析: マッチング

                                               S.Kurohashi 93. 5.31

    $Id$
====================================================================*/
#include "knp.h"

float 	Current_max_score;	/* 得点 */
int 	Current_pure_score[MAX_MATCH_MAX];	/* 正規化する前の得点 */
float	Current_sufficiency;	/* 埋まりぐあい */
int 	Current_max_m_e;	/* 要素数 */
int 	Current_max_m_p;	/* 要素の位置 */
int 	Current_max_c_e;	/* 交差数 */

int 	Current_max_num;
LIST 	Current_max_list1[MAX_MATCH_MAX];
LIST 	Current_max_list2[MAX_MATCH_MAX];


int 	SM_match_score[] = {0, 10, 10, 10, 10, 10, 10, 10, 10, 
			    10, 10, 10, 10};
  				/*{0, 5, 8, 10, 12};*/	/* ＳＭ対応スコア */
int     SM_match_unknown = 10;			 	/* データ未知     */

/* int 	EX_match_score[] = {0, 0, 5, 7, 8, 9, 10, 11}; */
/* int 	EX_match_score[] = {0, 0, 0, 1, 3, 5, 10, 11}; */
int 	EX_match_score[]  = {0, 0, 0, 1, 3, 5, 8, 11};
int 	EX_match_score2[] = {0, 0, 0, 1, 2, 4, 7, 11};
							/* 用例対応スコア */
int     EX_match_unknown = 6;				/* データ未知     */
int     EX_match_sentence = 10;				/* 格要素 -- 文   */
int     EX_match_tim = 0;				/* 格要素 -- 時間:時間格 */
int     EX_match_tim2 = 12;				/* 格要素 -- 時間:その他の格 */
int     EX_match_tim3 = 8;				/* 格要素 -- 時間:格選択時 */
int     EX_match_qua = 9; /* 10; */			/* 格要素 -- 数量 */
int	EX_match_exact = 12;
int	EX_match_subject = 8;
int	EX_match_modification = 0;
int	EX_match_demonstrative = 0;

int	SOTO_THRESHOLD = 8;
int	CASE_ASSIGN_THRESHOLD = 0;

/*==================================================================*/
	    void print_assign(LIST *list, CASE_FRAME *cf)
/*==================================================================*/
{
    /* 対応リストの表示 */

    int i;
    for (i = 0; i < cf->element_num; i++) {
	if (list->flag[i] == NIL_ASSIGNED)
	  fprintf(Outfp, "  X");
	else
	  fprintf(Outfp, "%3d", list->flag[i]+1);
    }
    fprintf(Outfp, "\n");
}

/*==================================================================*/
	     int comp_sm(char *cpp, char *cpd, int start)
/*==================================================================*/
{
    /* start からチェックする
       普通は 1 
       品詞ごとチェックするときは 0 */

    int i;

    for (i = start; i < SM_CODE_SIZE; i++) {
	if (cpp[i] == '*')
	    return i;
	else if (cpd[i] == '*')
	    return 0;
	else if (cpp[i] != cpd[i])
	    return 0;
    }
    return SM_CODE_SIZE;
}

/*==================================================================*/
	 int _sm_match_score(char *cpp, char *cpd, int flag)
/*==================================================================*/
{
    /*
      NTTの意味素の一桁目は品詞情報
      格フレーム <-----> データ
       x(補文)   <-----> xだけOK
       1(名詞)   <-----> x以外OK
                         (名詞以外のものはget_smの時点で排除 99/01/13)
    */

    /* 
       flag == SM_EXPAND_NE    : 固有名詞意味属性を一般名詞意味属性に変換する
       flag == SM_NO_EXPAND_NE : 固有名詞意味属性を一般名詞意味属性に変換しない
       flag == SM_CHECK_FULL   : コードの一文字目からチェックする
     */

    int current_score, score = 0;
    char *cp;

    if (flag == SM_CHECK_FULL)
	return comp_sm(cpp, cpd, 0);

    if (cpp[0] == 'x') {
	if (cpd[0] == 'x')
	    return SM_CODE_SIZE;
	else
	    return 0;
    } else {
	if (cpd[0] == 'x')
	    return 0;
    }

    /* 意味マーカのマッチング度の計算

       ・パターンが先に* --- マッチ
       ・データが先に* --- マッチしない
       ・最後まで一致 --- マッチ

         マッチ : マッチする階層の深さを返す
	 マッチしないとき : 0を返す
    */

    /* データが固有名詞のとき */
    if (cpd[0] == '2') {
	if (flag == SM_EXPAND_NE && cpp[0] != '2') {
	    if (SMP2SMGExist == FALSE) {
		fprintf(stderr, ";;; Cannot open smp2smg table!\n");
		return 0;
	    }
	    else {
		char *start;
		start = _smp2smg(cpd);
		if (start == NULL) {
		    return score;
		}
		for (cp = start; *cp; cp+=SM_CODE_SIZE) {
		    if (*cp == '/') {
			cp++;
		    }
		    else if (cp != start) {
			fprintf(stderr, ";;; Invalid delimiter! <%c> (%s)\n", 
				*cp, start);
		    }

		    /* 副作用フラグがある意味素変換は行わない */
		    if (!strncmp(cp+SM_CODE_SIZE, " side-effect", 12)) {
			cp += 12; /* " side-effect" の分進める */
			continue;
		    }

		    current_score = comp_sm(cpp, cp, 1);
		    if (current_score > score) {
			score = current_score;
		    }
		}
		free(start);
		return score;
	    }
	}
	else if (flag == SM_NO_EXPAND_NE && cpp[0] == '2')
	    return comp_sm(cpp, cpd, 1);
	else
	    return 0;
    }
    /* 両方とも一般名詞のとき */
    else if (cpp[0] != '2')
	return comp_sm(cpp, cpd, 1);
    else
	return 0;
}

/*==================================================================*/
     int _cf_match_element(char *d, char *p, int start, int len)
/*==================================================================*/
{
    int i, j;
    char *code;

    if (Thesaurus == USE_BGH) {
	for (i = 0; *(d+i); i += BGH_CODE_SIZE) {
	    if (!strncmp(d+i+start, p+start, len)) {
		return TRUE;
	    }
	}
    }
    else if (Thesaurus == USE_NTT) {
	for (i = 0; *(d+i); i += SM_CODE_SIZE) {
	    /* 固有名詞体系 */
	    if (*(d+i) == '2') {
		/* 一般体系にマッピング 
		   ※ side-effect を無視する */
		code = smp2smg(d+i, TRUE);
		if (code == NULL) {
		    continue;
		}
		for (j = 0; *(code+j); j += SM_CODE_SIZE) {
		    if (!strncmp(code+j+start, p+start, len)) {
			free(code);
			return TRUE;
		    }
		}
		free(code);
	    }
	    else {
		if (!strncmp(d+i+start, p+start, len)) {
		    return TRUE;
		}
	    }
	}
    }
    return FALSE;
}

/*==================================================================*/
	int cf_match_element(char *d, char *target, int flag)
/*==================================================================*/
{
    char *code;

    /* flag == TRUE  : その意味素と exact match
       flag == FALSE : その意味素以下にあれば match */

    if (d == NULL) {
	return FALSE;
    }

    code = sm2code(target);

    if (Thesaurus == USE_BGH) {
	return _cf_match_element(d, code, 0, BGH_CODE_SIZE);
    }
    else if (Thesaurus == USE_NTT) {
	if (flag == TRUE) {
	    return _cf_match_element(d, code, 0, SM_CODE_SIZE);
	}
	else {
	    /* ※ コードが 2 文字以上ある必要がある */
	    return _cf_match_element(d, code, 1, sm_code_depth(code));
	}
    }
}

/*==================================================================*/
 int cf_match_both_element(char *d, char *p, char *target, int flag)
/*==================================================================*/
{
    int len;
    char *code;

    /* 両方に target が存在するかチェック */

    /* flag == TRUE  : その意味素と exact match
       flag == FALSE : その意味素以下にあれば match */

    if (p == NULL) {
	return FALSE;
    }

    code = sm2code(target);

    if (Thesaurus == USE_BGH) {
	if (_cf_match_element(d, code, 0, BGH_CODE_SIZE) == TRUE && 
	    _cf_match_element(p, code, 0, BGH_CODE_SIZE) == TRUE) {
	    return TRUE;
	}
	else {
	    return FALSE;
	}
    }
    else if (Thesaurus == USE_NTT) {
	if (flag == TRUE) {
	    if (_cf_match_element(d, code, 0, SM_CODE_SIZE) == TRUE && 
		_cf_match_element(p, code, 0, SM_CODE_SIZE) == TRUE) {
		return TRUE;
	    }
	    else {
		return FALSE;
	    }
	}
	else {
	    /* ※ コードが 2 文字以上ある必要がある
	       1文字目(品詞)を無視して、与えられたコード以下にあるかどうかチェック */
	    len = sm_code_depth(code);
	    if (_cf_match_element(d, code, 1, len) == TRUE && 
		_cf_match_element(p, code, 1, len) == TRUE) {
		return TRUE;
	    }
	    else {
		return FALSE;
	    }
	}
    }
}

/*==================================================================*/
       int elmnt_match_score_each_sm(int as1, CASE_FRAME *cfd,
				     int as2, CASE_FRAME *cfp, int *pos)
/*==================================================================*/
{
    /* 意味素 : 格要素 -- 補文 */
    if (cf_match_both_element(cfd->sm[as1], cfp->sm[as2], "補文", TRUE)) {
	return EX_match_sentence;
    }
    /* 意味素 : 格要素 -- 時間 */
    else if (cf_match_both_element(cfd->sm[as1], cfp->sm[as2], "時間", TRUE)) {
	/* 格フレーム側が時間格の場合はスコアを低く */
	if (MatchPP(cfp->pp[as2][0], "時間")) {
	    return EX_match_tim;
	}
	/* 格フレーム:時間格以外, 入力側:格選択時
	   格が曖昧なときは
	   1. <時間>時間格 : <時間>時間格 (score == 0)
	   2. 「用例」普通の格 : 「用例」普通の格
	   3. <時間>普通の格 : <時間>普通の格 (here) */
	else if (cfd->pp[as1][1] != END_M) {
	    return EX_match_tim3;
	}
	else {
	    return EX_match_tim2;
	}
    }
    /* 意味素 : 格要素 -- 数量 */
    else if (cf_match_both_element(cfd->sm[as1], cfp->sm[as2], "数量", TRUE)) {
	return EX_match_qua;
    }
    /* 意味素 : 格要素 -- 主体 (ガ格のみ) */
    else if (((cfp->voice == FRAME_ACTIVE && 
	       MatchPP(cfp->pp[as2][0], "ガ")) || 
	      (cfp->voice == FRAME_PASSIVE_1 && 
	       MatchPP(cfp->pp[as2][0], "ニ"))) && 
	     cf_match_both_element(cfd->sm[as1], cfp->sm[as2], "主体", FALSE)) {
	*pos = MATCH_SUBJECT;
	return EX_match_subject;
    }
    return -100;
}

/*==================================================================*/
int cf_match_exactly(TAG_DATA *d, char **ex_list, int ex_num, int *pos)
/*==================================================================*/
{
    if (!check_feature(d->f, "形副名詞")) {
	*pos = check_examples(d->head_ptr->Goi, ex_list, ex_num);
	if (*pos >= 0) {
	    return 1;
	}
    }
    return 0;
}

/*==================================================================*/
	   int elmnt_match_score(int as1, CASE_FRAME *cfd, 
				 int as2, CASE_FRAME *cfp, 
				 int flag, int *pos, int *score)
/*==================================================================*/
{
    /* 意味マーカのマッチング度の計算 */

    int i, j, k, ex_score = 0;
    char *exd, *exp;
    int *match_score;

    *score = -100;
    exd = cfd->ex[as1];
    exp = cfp->ex[as2];
    match_score = EX_match_score;

    if (flag == SEMANTIC_MARKER) {
	int tmp_score;

	if (SMExist == FALSE || 
	    cfd->sm[as1][0] == '\0'|| 
	    cfp->sm[as2] == NULL || 
	    cfp->sm[as2][0] == '\0') {
	    *score = SM_match_unknown;
	    return TRUE;
	}

	for (j = 0; cfp->sm[as2][j]; j+=SM_CODE_SIZE) {
	    /* 具体的な用例が書いてある場合 */
	    if (!strncmp(cfp->sm[as2]+j, sm2code("→"), SM_CODE_SIZE)) {
		tmp_score = (int)calc_similarity(exd, exp, 0);
		if (tmp_score == 1) {
		    *score = 10;
		    return TRUE;
		}
	    }
	    else {
		/* 選択制限によるマッチ (NTTシソーラスがある場合) */
		for (i = 0; cfd->sm[as1][i]; i+=SM_CODE_SIZE) {
		    tmp_score = 
			SM_match_score[_sm_match_score(cfp->sm[as2]+j,
						       cfd->sm[as1]+i, SM_NO_EXPAND_NE)];
		    if (tmp_score > *score) *score = tmp_score;
		}
	    }
	}
	return TRUE;
    }

    else if (flag == EXAMPLE) {
	int subject_match;

	/* 修飾格のとき */
	if (MatchPP(cfd->pp[as1][0], "修飾")) {
	    *score = EX_match_modification;
	    return TRUE;
	}

	/* 指示詞のとき */
	if (check_feature(cfd->pred_b_ptr->cpm_ptr->elem_b_ptr[as1]->f, "省略解析対象指示詞")) {
	    *score = EX_match_demonstrative;
	    return TRUE;
	}

	/* 能動ガ格, 受身ニ格で<主体>のとき */
	/* if (((cfp->voice == FRAME_ACTIVE && 
	      MatchPP(cfp->pp[as2][0], "ガ")) || 
	     (cfp->voice == FRAME_PASSIVE_1 && 
	     MatchPP(cfp->pp[as2][0], "ニ"))) && */
	if (!MatchPP(cfp->pp[as2][0], "ヲ") && 
	    cf_match_both_element(cfd->sm[as1], cfp->sm[as2], "主体", FALSE) || 
	    (cfd->ex[as1][0] == '\0' && /* ガ格で意味素なしのとき固有名詞だと思う */
	     cf_match_element(cfp->sm[as2], "主体", TRUE))) {
	    subject_match = 1;
	}
	else {
	    subject_match = 0;
	}

	/* exact match */
	if (subject_match == 0 && 
	    cf_match_exactly(cfd->pred_b_ptr->cpm_ptr->elem_b_ptr[as1], 
			     cfp->ex_list[as2], cfp->ex_num[as2], pos)) {
	    if (MatchPP(cfp->pp[as2][0], "外の関係")) {
		*score = EX_match_score[7]; /* 最大値 */
	    }
	    else {
		*score = EX_match_exact;
	    }
	    return TRUE;
	}

	/* 外の関係のときシソーラス使わない */
	if (MatchPP(cfp->pp[as2][0], "外の関係")) {
	    *score = 0;
	    return FALSE;
	}

	/* <主体>共通スコア */
	if (subject_match) {
	    *pos = MATCH_SUBJECT;
	    *score = EX_match_subject;
	    return TRUE;
	}
	else {
	    *score = elmnt_match_score_each_sm(as1, cfd, as2, cfp, pos);
	}

	/* 入力側の用例の意味属性がない場合 */
	if (*exd == '\0') {
	    if (*cfd->sm[as1] == '\0') {
		*score = EX_match_unknown;
	    }
	    else if (score < 0) {
		*score = 0;
	    }
	}
	/* 格フレームの用例の意味属性がない場合 */
	else if (exp == NULL || *exp == '\0') {
	    /* 格フレーム側の意味属性がないとき */
	    if (cfp->sm[as2] == NULL) {
		*score = 0; /* EX_match_unknown; */
	    }
	    /* 意味属性はあるが、match しないとき */
	    else if (score < 0) {
		/* 格フレームに用例自体がないとき */
		if (!MatchPP(cfp->pp[as2][0], "ガ") && cfp->ex_num[as2] == 0) {
		    *score = 0;
		    /* return FALSE; */
		}
		*score = 0;
	    }
	}
	else {
	    float rawscore;

	    rawscore = CalcSmWordsSimilarity(exd, cfp->ex_list[as2], cfp->ex_num[as2], pos, 
					     cfp->sm_delete[as2], 0);
	    /* 用例のマッチング */
	    if (Thesaurus == USE_NTT && 
		sm_check_match_max(exd, exp, 0, sm2code("抽象"))) { /* <抽象>のマッチを低く */
		ex_score = EX_match_score2[(int)(rawscore*7)];
	    }
	    else {
		ex_score = *(match_score+(int)(rawscore*7));
	    }
	}

	/* 大きい方をかえす */
	if (ex_score > *score) {
	    *score = ex_score;
	}

	/* 用例, 意味素のマッチが不成功 */
	if (*score > CASE_ASSIGN_THRESHOLD) {
	    return TRUE;
	}
	else {
	    return FALSE;
	}
    }
    return FALSE;
}

/*==================================================================*/
	 int count_pat_element(CASE_FRAME *cfp, LIST *list2)
/*==================================================================*/
{
    int i, pat_element = 0;
    for (i = 0; i < cfp->element_num; i++) {
	if (!(cfp->oblig[i] == FALSE && list2->flag[i] == UNASSIGNED)) {
	    pat_element++;
	}
    }
    return pat_element;
}

/*==================================================================*/
	 int check_same_case(int dp, int pp, CASE_FRAME *cf)
/*==================================================================*/
{
    int i, p1, p2;

    if (dp < pp) {
	p1 = dp;
	p2 = pp;
    }
    else {
	p1 = pp;
	p2 = dp;
    }

    for (i = 0; cf->samecase[i][0] != END_M; i++) {
	if (cf->samecase[i][0] == p1 && 
	    cf->samecase[i][1] == p2) {
	    return 1;
	}
    }
    return 0;
}

/*==================================================================*/
		int check_case(CASE_FRAME *cf, int c)
/*==================================================================*/
{
    int i, j;

    for (i = 0; i < cf->element_num; i++) {
	for (j = 0; cf->pp[i][j] != END_M; j++) {
	    if (cf->pp[i][j] == c) {
		return i;
	    }
	}
    }
    return -1;
}

/*==================================================================*/
int check_adjacent_assigned(CASE_FRAME *cfd, CASE_FRAME *cfp, LIST *list1)
/*==================================================================*/
{
    int i;
	       
    for (i = 0; i < cfd->element_num; i++) {
	if (cfd->adjacent[i] == TRUE && 
	    list1->flag[i] != NIL_ASSIGNED && 
	    cfp->adjacent[list1->flag[i]] == TRUE) {
	    return TRUE;
	}
    }
    return FALSE;
}

/*==================================================================*/
	    void eval_assign(CASE_FRAME *cfd, LIST *list1,
			     CASE_FRAME *cfp, LIST *list2,
			     int score, int closest)
/*==================================================================*/
{
    /* フレームのマッチング度の計算(格明示部分を除く) */

    int i, j;
    int local_m_e = 0;
    int local_m_p = 0;
    int local_c_e = 0;
    int pat_element, dat_element = 0;
    int cf_element = 0, lastpp;
    int unassigned_ga = 0;
    float local_score;

    local_score = score;

    /* 要素数，要素の位置，交差数 */
    for (i = 0; i < cfd->element_num; i++) {
	if (list1->flag[i] != NIL_ASSIGNED) {
	    local_m_e++;
	    local_m_p += i;
	    for (j = i+1; j < cfd->element_num; j++) {
		if (list1->flag[j] != NIL_ASSIGNED &&
		    list1->flag[j] < list1->flag[i]) {
		    local_c_e--;
		}
	    }
	}
    }

    /* 文中の要素数(任意でマッチしていない要素以外) */
    /* ※ 埋め込み文の被修飾語は任意扱い */
    for (i = 0; i < cfd->element_num; i++) {
	if (!(cfd->oblig[i] == FALSE && list1->flag[i] == NIL_ASSIGNED)) {
	    dat_element++;
	}
    }

    /* 格フレーム中の要素数(任意でマッチしていない要素以外) */
    pat_element = count_pat_element(cfp, list2);

    /* 格フレーム中の要素数 */
    for (i = 0; i < cfp->element_num; i++) {
	if (list2->flag[i] != UNASSIGNED) {
	    cf_element++;
	    lastpp = cfp->pp[i][0];
	}
	/* 割り当てのないガ格がある */
	else if (MatchPP(cfp->pp[i][0], "ガ")) {
	    unassigned_ga = 1;
	}
    }


#ifdef CASE_DEBUG
    fprintf(Outfp, "dat %s score=%.3f m_e=%d dat=%d pat=%d ", 
	    cfp->cf_id, local_score, local_m_e, dat_element, pat_element);
    for (i = 0; i < cfd->element_num; i++)
	fprintf(Outfp, "%d ", list1->flag[i]);
    fputc('\n', Outfp);
#endif

    if ((local_m_e < dat_element) || 
	/* (入力側)必須格の直前格のマッチを条件とする */
	(closest > -1 && cfd->oblig[closest] == TRUE && list1->flag[closest] == NIL_ASSIGNED) || 
	/* 外の関係だけのマッチを避ける */
	(!OptEllipsis && 
	 cf_element == 1 && MatchPP(lastpp, "外の関係"))) {
	local_score = -1;
    }
    else if (dat_element == 0 || pat_element == 0 || local_m_e == 0) {
	local_score = 0;
    }
    else {
	/* local_score = local_score * sqrt((double)local_m_e)
	   / sqrt((double)dat_element * pat_element);*/

	/* local_score = local_score * local_m_e
	   / (dat_element * sqrt((double)pat_element)); */

	/* 同じ格フレームでの対応付けに影響 */
	local_score = local_score / sqrt((double)pat_element);

	/* corpus based case analysis 00/01/04 */
	/* local_score /= 10;	* 正規化しない,最大11に */
    }

    /* corpus based case analysis 00/01/04 */
    /* 任意格に加点 */
    /* 並列の expand を行ったときのスコアを考慮する必要がある */
    /* local_score += (cfd->element_num - dat_element) * OPTIONAL_CASE_SCORE; */

    if (0 && OptEllipsis) {
	if (local_score > Current_max_score) {
	    Current_max_list1[0] = *list1;
	    Current_max_list2[0] = *list2;
	    Current_max_score = local_score;
	    Current_pure_score[0] = score;
	    Current_sufficiency = (float)cf_element/cfp->element_num;
	    Current_max_m_e = local_m_e;
	    Current_max_m_p = local_m_p;
	    Current_max_c_e = local_c_e;
	    Current_max_num = 1;
	}
	else if (local_score == Current_max_score && 
		 Current_max_num < MAX_MATCH_MAX) {
	    Current_max_list1[Current_max_num] = *list1;
	    Current_max_list2[Current_max_num] = *list2;
	    Current_pure_score[Current_max_num] = score;
	    Current_max_num++;
	}
    }
    else {
	if (local_score > Current_max_score || 
	    (local_score == Current_max_score &&
	     local_m_e > Current_max_m_e) ||
	    (local_score == Current_max_score &&
	     local_m_e == Current_max_m_e &&
	     local_m_p > Current_max_m_p) ||
	    (local_score == Current_max_score &&
	     local_m_e == Current_max_m_e &&
	     local_m_p == Current_max_m_p &&
	     local_c_e > Current_max_c_e) || 
	    (local_score == Current_max_score &&
	     local_m_e == Current_max_m_e &&
	     local_m_p == Current_max_m_p &&
	     local_c_e == Current_max_c_e && 
	     unassigned_ga == 0)) {
	    Current_max_list1[0] = *list1;
	    Current_max_list2[0] = *list2;
	    Current_max_score = local_score;
	    Current_pure_score[0] = score;
	    Current_sufficiency = (float)cf_element/cfp->element_num;
	    Current_max_m_e = local_m_e;
	    Current_max_m_p = local_m_p;
	    Current_max_c_e = local_c_e;
	    Current_max_num = 1;
	}
	else if (local_score == Current_max_score &&
		 local_m_e == Current_max_m_e &&
		 local_m_p == Current_max_m_p &&
		 local_c_e == Current_max_c_e &&
		 Current_max_num < MAX_MATCH_MAX) {
	    Current_max_list1[Current_max_num] = *list1;
	    Current_max_list2[Current_max_num] = *list2;
	    Current_pure_score[Current_max_num] = score;
	    Current_max_num++;
	}
    }
}

int assign_list(CASE_FRAME *cfd, LIST list1,
		CASE_FRAME *cfp, LIST list2,
		int score, int flag, int closest);

/*==================================================================*/
	    int _assign_list(CASE_FRAME *cfd, LIST list1,
			     CASE_FRAME *cfp, LIST list2,
			     int score, int flag, int assign_flag, int closest)
/*==================================================================*/
{
    /* 
       文中の格要素と格フレームの格要素の対応付け

       ・この関数の一回の呼び出しで処理するのは文中の格要素一つ

       ・明示されている格要素(ガ格,ヲ格など)があれば，それを処理,
         なければ明示されていない格要素(未格,埋込文など)を処理

       ・list.flag[i]にはi番目の格要素の対応付けの状況を保持

	  UNASSINGED ---- 対応付けまだ
	  NIL_ASSINGED -- 対応付けしないことを決定
          j(その他)------ 相手のj番目と対応付け

       ・明示されている格助詞の必須格で，対応する格スロットがあるのに
         意味マーカ不一致の場合，格フレームが文に対して不適当というよ
	 りは意味マーカの指定が硬すぎるので，一応対応付けを行って処理
	 を進める．
    */

    int target = -1;	/* データ側の処理対象の格要素 */
    int target_pp = 0;
    int elmnt_score, gaflag = 0, sotoflag = 0, toflag = 0, match_result;
    int i, j, pos;

#ifdef CASE_DEBUG
    fprintf(Outfp, "dat");
    for (i = 0; i < cfd->element_num; i++)
	fprintf(Outfp, "%d ", list1.flag[i]);
    fputc('\n', Outfp);
    fprintf(Outfp, "pat");
    for (i = 0; i < cfp->element_num; i++)
	fprintf(Outfp, "%d ", list2.flag[i]);
    fputc('\n', Outfp);
#endif 
    
    /* まだ割り当てのない格助詞のチェック */
    for (i = 0; i < cfd->element_num; i++) {
	if (list1.flag[i] == UNASSIGNED && 
	    /* cfd->pp[i][0] >= 0 && (未 == -3) */
	    ((assign_flag == TRUE && cfd->pred_b_ptr->cpm_ptr->elem_b_num[i] != -1) || 
	     (assign_flag == FALSE && cfd->pred_b_ptr->cpm_ptr->elem_b_num[i] == -1))) {
	    target = i;
	    break;
	}
    }

    if (target >= 0) {
	/* すでにガ格に割り当てがあるかどうか (ガ２割り当て可能かどうか) */
	for (i = 0; i < cfp->element_num; i++) {
	    if (list2.flag[i] != UNASSIGNED && 
		MatchPP2(cfp->pp[i], "ガ")) {
		gaflag = 1;
		break;
	    }
	}

	/* <主体>かどうか (外の関係割り当て可能かどうか) */
	if (!cf_match_element(cfd->sm[target], "主体", FALSE)) {
	    sotoflag = 1;
	}

	/* すでに補文ト格に割り当てがあるかどうか (ヲ格割り当て可能かどうか) */
	for (i = 0; i < cfp->element_num; i++) {
	    if (list2.flag[i] != UNASSIGNED && 
		MatchPP2(cfp->pp[i], "ト")) {
		/* cf_match_element(cfp->sm[i], "補文", TRUE)) { */
		toflag = 1;
		break;
	    }
	}

	/* 格フレームの格ループ */
	for (i = 0; i < cfp->element_num; i++) {
	    /* 格フレームの空いている格 */
	    if (list2.flag[i] == UNASSIGNED) {
		/* 解釈されうる格のループ */
		for (target_pp = 0; cfd->pp[target][target_pp] != END_M; target_pp++) {
		    for (j = 0; cfp->pp[i][j] >= 0; j++) { /* 自動構築格フレームには複数の格はない */
			if ((cfd->pp[target][target_pp] == cfp->pp[i][j] && 
			     !((cfp->pp[i][j] == pp_kstr_to_code("外の関係") && !sotoflag) || 
			       (cfp->pp[i][j] == pp_kstr_to_code("ガ２") && !gaflag) || 
			       (cfp->pp[i][j] == pp_kstr_to_code("ヲ") && toflag) || 
			       (cfp->pp[i][j] == pp_kstr_to_code("ノ") && 
				check_adjacent_assigned(cfd, cfp, &list1) == FALSE))) || 
			    (cfd->pp[target][target_pp] == pp_kstr_to_code("未") && 
			     check_same_case(cfd->sp[target], cfp->pp[i][j], cfp))) {
			    pos = MATCH_NONE;
			    match_result = 
				elmnt_match_score(target, cfd, i, cfp, flag, &pos, &elmnt_score);

			    if (!(cfp->pp[i][j] == pp_kstr_to_code("外の関係") || 
				  cfp->pp[i][j] == pp_kstr_to_code("ノ")) || 
				elmnt_score >= SOTO_THRESHOLD) {
				if ((flag == EXAMPLE) || 
				/* if ((flag == EXAMPLE && 
				   (match_result == TRUE || assign_flag == TRUE)) || */
				    (flag == SEMANTIC_MARKER && elmnt_score >= 0)) {
				    /* 対応付けをして，残りの格要素の処理に進む
				       ※ flag == SEMANTIC_MARKER && elmnt_score == 0
				       すなわち，格助詞の対応する格スロットがあるのに
				       意味マーカ不一致の場合も，処理を進める */

				    if (cfd->weight[target]) {
					elmnt_score /= cfd->weight[target];
				    }
				    list1.flag[target] = i;
				    list2.flag[i] = target;
				    list1.score[target] = elmnt_score;
				    list2.score[i] = elmnt_score;
				    list2.pos[i] = pos;
				    assign_list(cfd, list1, cfp, list2, 
						score + elmnt_score, flag, closest);
				    list2.flag[i] = UNASSIGNED;
				    list2.pos[i] = MATCH_NONE;
				}
			    }
			    break;
			}
		    }
		}
	    }
	}

	/* target番目の格要素には対応付けを行わないマーク */
	list1.flag[target] = NIL_ASSIGNED;

	/* 任意格とし対応付けを行わない場合
	   ※ 同じ表層格が格フレームにある場合，対応付けをすることは
	      すでに上で試されている
	      if (cfd->oblig[target] == FALSE) */
	/* 必須格で対応無(表層格の一致するものがない)の場合
	       => eval_assignで不許可
	   必須格で対応有の場合
	       => 後ろに同じ格助詞があれば対応付けをしない可能性も試す? */

	assign_list(cfd, list1, cfp, list2, score, flag, closest);
	return FALSE;
    }
    return TRUE;
}

/*==================================================================*/
	     int assign_list(CASE_FRAME *cfd, LIST list1,
			     CASE_FRAME *cfp, LIST list2,
			     int score, int flag, int closest)
/*==================================================================*/
{
    /* 未格, 連格以外を先に割り当て */

    if (_assign_list(cfd, list1, cfp, list2, score, flag, TRUE, closest) == FALSE) {
	return FALSE;
    }
    if (_assign_list(cfd, list1, cfp, list2, score, flag, FALSE, closest) == FALSE) {
	return FALSE;
    }

    /* 評価 : すべての対応付けが終わった場合 */
    eval_assign(cfd, &list1, cfp, &list2, score, closest);
    return TRUE;
}

/*==================================================================*/
int case_frame_match(CF_PRED_MGR *cpm_ptr, CF_MATCH_MGR *cmm_ptr, int flag, int closest)
/*==================================================================*/
{
    /* 格フレームのマッチング */

    LIST assign_d_list, assign_p_list;
    int i;
    CASE_FRAME *cfd = &(cpm_ptr->cf);

    /* 初期化 */

    Current_max_num = 0;
    Current_max_score = -2;
    Current_sufficiency = 0;
    Current_max_m_e = 0;
    Current_max_m_p = 0;
    Current_max_c_e = 0;

    for (i = 0; i < cfd->element_num; i++) {
	assign_d_list.flag[i] = UNASSIGNED;
	assign_d_list.score[i] = -1;
    }

    /* for (i = 0; i < cmm_ptr->cf_ptr->element_num; i++) { */
    for (i = 0; i < CF_ELEMENT_MAX; i++) {
	assign_p_list.flag[i] = UNASSIGNED;
	assign_p_list.score[i] = -1;
	assign_p_list.pos[i] = -1;
    }

    /* 処理 */

    /* flag: 例 or 意味コード */
    assign_list(cfd, assign_d_list, cmm_ptr->cf_ptr, assign_p_list, 0, flag, closest);

    /* 後処理 */

    if (Current_max_num == MAX_MATCH_MAX) {
	fprintf(stderr, "; Too many case matching result !\n");
    }

    cmm_ptr->sufficiency = Current_sufficiency;
    cmm_ptr->result_num = Current_max_num;
    for (i = 0; i < Current_max_num; i++) {
	cmm_ptr->result_lists_p[i] = Current_max_list2[i];
	cmm_ptr->result_lists_d[i] = Current_max_list1[i];
	cmm_ptr->pure_score[i] = Current_pure_score[i];
    }

    /* 直前格要素のスコアのみを用いるとき */
    if (closest > -1 && Current_max_score >= 0 && 
	Current_max_list1[0].flag[closest] != NIL_ASSIGNED) {
	/* 直前格要素の割り当てがあることが条件 */
	cmm_ptr->score = (float)Current_max_list1[0].score[closest];
    }
    else {
	cmm_ptr->score = Current_max_score;
    }

#ifdef CASE_DEBUG
    print_crrspnd(cfd, cmm_ptr);
#endif
    return 1;
}

/*====================================================================
                               END
====================================================================*/
