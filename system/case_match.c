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

int 	EX_match_score[] = {0, 0, 5, 7, 8, 9, 10, 11};
/* int 	EX_match_score[] = {0, 0, 0, 1, 3, 5, 10, 11}; */
							/* 用例対応スコア */
int     EX_match_unknown = 8; /* 10; */			/* データ未知     */
int     EX_match_sentence = 10;				/* 格要素 -- 文   */
int     EX_match_tim = 0;				/* 格要素 -- 時間:時間格 */
int     EX_match_tim2 = 12;				/* 格要素 -- 時間:その他の格 */
int     EX_match_tim3 = 8;				/* 格要素 -- 時間:格選択時 */
int     EX_match_qua = 9; /* 10; */			/* 格要素 -- 数量 */
int	EX_match_exact = 12;
int	EX_match_subject = 8;
int	EX_match_modification = 0;

int	Thesaurus = USE_NTT;

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

    int i, current_score, score = 0;
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
	      int _ex_match_score(char *cp1, char *cp2)
/*==================================================================*/
{
    /* 例の分類語彙表コードのマッチング度の計算 */

    int match = 0;

    /* 単位の項目は無視 */
    if (!strncmp(cp1, "11960", 5) || !strncmp(cp2, "11960", 5))
	return 0;
    
    /* 比較 */
    match = bgh_code_match(cp1, cp2);

    /* 代名詞の項目は類似度を押さえる */
    if ((!strncmp(cp1, "12000", 5) || !strncmp(cp2, "12000", 5)) &&
	match > 3)
	return 3;
    
    return match;
}

/*==================================================================*/
     int _cf_match_element(char *d, char *p, int start, int len)
/*==================================================================*/
{
    int i, j;
    char *code;

    for (i = 0; *(d+i); i += SM_CODE_SIZE) {
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

    if (flag == TRUE) {
	return _cf_match_element(d, code, 0, SM_CODE_SIZE);
    }
    else {
	/* ※ コードが 2 文字以上ある必要がある */
	return _cf_match_element(d, code, 1, sm_code_depth(code));
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
	/* ※ コードが 2 文字以上ある必要がある */
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
    else if (MatchPP(cfp->pp[as2][0], "ガ") && 
	     cf_match_both_element(cfd->sm[as1], cfp->sm[as2], "主体", FALSE)) {
	*pos = MATCH_SUBJECT;
	return EX_match_subject;
    }
    return -100;
}

/*==================================================================*/
int cf_match_exactly(BNST_DATA *d, char **ex_list, int ex_num, int *pos)
/*==================================================================*/
{
    if (!check_feature(d->f, "形副名詞") && 
	d->jiritu_ptr != NULL) {
	if (d->jiritu_num > 1 && 
	    check_feature((d->jiritu_ptr+d->jiritu_num-1)->f, "Ｔ固有末尾")) {
	    *pos = check_examples((d->jiritu_ptr+d->jiritu_num-2)->Goi, ex_list, ex_num);
	}
	else {
	    *pos = check_examples(L_Jiritu_M(d)->Goi, ex_list, ex_num);
	}
	if (*pos >= 0) {
	    return 1;
	}
    }
    return 0;
}

/*==================================================================*/
	   int elmnt_match_score(int as1, CASE_FRAME *cfd,
				 int as2, CASE_FRAME *cfp, int flag, int *pos)
/*==================================================================*/
{
    /* 意味マーカのマッチング度の計算 */

    int i, j, k, tmp_score, score = -100, ex_score = -100;
    char *exd, *exp;
    int *match_score;

    if (flag == SEMANTIC_MARKER) {
	
	if (cfd->sm[as1][0] == '\0'|| cfp->sm[as2] == NULL || cfp->sm[as2][0] == '\0') 
	    return SM_match_unknown;

	for (j = 0; cfp->sm[as2][j]; j+=SM_CODE_SIZE) {
	    if (!strncmp(cfp->sm[as2]+j, (char *)sm2code("→"), SM_CODE_SIZE)) {

		for (k = 0; cfp->ex[as2][k]; k+=BGH_CODE_SIZE) {
		    for (i = 0; cfd->ex[as1][i]; i+=BGH_CODE_SIZE) {
			tmp_score = 
			    EX_match_score[_ex_match_score(cfp->ex[as2]+k, 
							   cfd->ex[as1]+i)];
			if (tmp_score == 11) {
			    return 10;
			}
		    }
		}
	    }
	    else {
		for (i = 0; cfd->sm[as1][i]; i+=SM_CODE_SIZE) {
		    tmp_score = 
			SM_match_score[_sm_match_score(cfp->sm[as2]+j,
						       cfd->sm[as1]+i, SM_NO_EXPAND_NE)];
		    if (tmp_score > score) score = tmp_score;
		}
	    }
	}
	return score;
    }

    else if (flag == EXAMPLE) {
	int ga_subject;

	/* 修飾格のとき */
	if (MatchPP(cfd->pp[as1][0], "修飾")) {
	    return EX_match_modification;
	}

	if (MatchPP(cfp->pp[as2][0], "ガ") && 
	    cf_match_both_element(cfd->sm[as1], cfp->sm[as2], "主体", FALSE)) {
	    ga_subject = 1;
	}
	else {
	    ga_subject = 0;
	}

	/* exact match */
	if (ga_subject == 0 && 
	    cfp->concatenated_flag == 0 && 
	    cf_match_exactly(cfd->pred_b_ptr->cpm_ptr->elem_b_ptr[as1], 
			     cfp->ex_list[as2], cfp->ex_num[as2], pos)) {
	    return EX_match_exact;
	}

	if (Thesaurus == USE_BGH) {
	    exd = cfd->ex[as1];
	    exp = cfp->ex[as2];
	    match_score = EX_match_score;
	}
	else if (Thesaurus == USE_NTT) {
	    exd = cfd->ex2[as1];
	    exp = cfp->ex2[as2];
	    match_score = EX_match_score;
	}

	/* ガ格<主体>共通スコア */
	if (ga_subject) {
	    *pos = MATCH_SUBJECT;
	    return EX_match_subject;
	}
	else {
	    score = elmnt_match_score_each_sm(as1, cfd, as2, cfp, pos);
	}

	/* 用例がどちらか一方でもなかったら */
	if (*exd == '\0') {
	    if (*cfd->sm[as1] == '\0') {
		score = EX_match_unknown;
	    }
	    else if (score < 0) {
		score = 0;
	    }
	}
	else if (exp == NULL || *exp == '\0') {
	    /* 格フレーム側の用例の意味属性がないとき */
	    if (cfp->sm[as2] == NULL) {
		score = EX_match_unknown;
	    }
	    /* 意味属性はあるが、match しないとき */
	    else if (score < 0) {
		score = 0;
	    }
	}
	else {
	    float rawscore;

	    rawscore = CalcSmWordsSimilarity(exd, cfp->ex_list[as2], cfp->ex_num[as2], pos, 
					     cfp->sm_delete[as2], 0);
	    /* rawscore = CalcWordsSimilarity(cfd->ex_list[as1][0], cfp->ex_list[as2], cfp->ex_num[as2], pos); */
	    /* rawscore = CalcSimilarity(exd, exp); */
	    /* 用例のマッチング */
	    if (Thesaurus == USE_BGH) {
		ex_score = *(match_score+(int)rawscore);
	    }
	    else if (Thesaurus == USE_NTT) {
		ex_score = *(match_score+(int)(rawscore*7));
	    }
	}

	/* 大きい方をかえす */
	if (ex_score > score) {
	    return ex_score;
	}
	else {
	    return score;
	}
    }
    return 0;
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
	    void eval_assign(CASE_FRAME *cfd, LIST *list1,
			     CASE_FRAME *cfp, LIST *list2,
			     int score)
/*==================================================================*/
{
    /* フレームのマッチング度の計算(格明示部分を除く) */

    int i, j;
    int local_m_e = 0;
    int local_m_p = 0;
    int local_c_e = 0;
    int pat_element, dat_element = 0;
    int cf_element = 0;
    float local_score;

    local_score = score;

    /* ★ experimental (類似度が高い直前格にボーナス)
    for (i = 0; i < cfd->element_num; i++) {
	if (cfd->adjacent[i] == TRUE && 
	    list1->flag[i] != NIL_ASSIGNED && 
	    cfp->adjacent[list1->flag[i]] == TRUE && 
	    list1->score[i] > 10) {
	    local_score += 2;
	}
    } */

    /* 要素数，要素の位置，交差数 */
    for (i = 0; i < cfd->element_num; i++) {
	if (list1->flag[i] != NIL_ASSIGNED) {
	    local_m_e ++;
	    local_m_p += i;
	    for (j = i+1; j < cfd->element_num; j++)
	      if (list1->flag[j] != NIL_ASSIGNED &&
		  list1->flag[j] < list1->flag[i]) 
		local_c_e --;
	}
    }

    /* 文中の要素数(任意でマッチしていない要素以外) */
    /* ※ 埋め込み文の被修飾語は任意扱い */
    for (i = 0; i < cfd->element_num; i++)
	if (!(cfd->oblig[i] == FALSE && list1->flag[i] == NIL_ASSIGNED))
	    dat_element++;

    /* 格フレーム中の要素数(任意でマッチしていない要素以外) */
    pat_element = count_pat_element(cfp, list2);

    /* 格フレーム中の要素数 */
    for (i = 0; i < cfp->element_num; i++) {
	if (list2->flag[i] != UNASSIGNED) {
	    cf_element++;
	}
    }

    if (local_m_e < dat_element) {
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

	local_score = local_score / sqrt((double)pat_element);

	/* corpus based case analysis 00/01/04 */
	/* local_score /= 10;	* 正規化しない,最大11に */
    }

    /* corpus based case analysis 00/01/04 */
    /* 任意格に加点 */
    /* 並列の expand を行ったときのスコアを考慮する必要がある */
    /* local_score += (cfd->element_num - dat_element) * OPTIONAL_CASE_SCORE; */

    if (local_score > Current_max_score || 
	(local_score == Current_max_score &&
	 local_m_e > Current_max_m_e) ||
	(local_score == Current_max_score &&
	 local_m_e == Current_max_m_e &&
	 local_m_p > Current_max_m_p) ||
	(local_score == Current_max_score &&
	 local_m_e == Current_max_m_e &&
	 local_m_p == Current_max_m_p &&
	 local_c_e > Current_max_c_e)) {
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
	    
/*==================================================================*/
	       void assign_list(CASE_FRAME *cfd, LIST list1,
				CASE_FRAME *cfp, LIST list2,
				int score, int flag)
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

    int ec_match_flag;	/* 明示されている格助詞
			     対応有,意味マーカ一致:    1
			     対応有,意味マーカ不一致: -1
			     対応無:                   0 */

    int target = -1;	/* データ側の処理対象の格要素 */
    int target_pp = 0;
    int elmnt_score, multi_pp = 0;
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
    
    /* 明示されている格助詞のチェック */
    for (i = 0; i < cfd->element_num; i++) {
	if (list1.flag[i] == UNASSIGNED) {
	    for (j = 0; cfd->pp[i][j] != END_M; j++) {
		if (cfd->pp[i][j] >= 0) {
		    target = i;
		    target_pp = j;
		    break;
		}
	    }
	    if (target >= 0) {
		break;
	    }
	}
    }

    /* 明示されている格助詞の処理 */
    if (target >= 0) {
	for (i = 0; cfd->pp[target][i] != END_M; i++) {
	    if (cfd->pp[target][i] < 0) {
		multi_pp = 1;
		break;
	    }
	}
	ec_match_flag = 0;
	for (i = 0; i < cfp->element_num; i++) {
	    if (list2.flag[i] == UNASSIGNED) {
		for (target_pp = 0; cfd->pp[target][target_pp] != END_M; target_pp++) {
		    if (cfd->pp[target][target_pp] < 0) {
			continue;
		    }
		    for (j = 0; cfp->pp[i][j] >= 0; j++) {
			if (cfd->pp[target][target_pp] == cfp->pp[i][j] ||
			    (cfd->pp[target][target_pp] == pp_hstr_to_code("によって") &&
			     cfp->pp[i][j] == pp_kstr_to_code("デ"))) {

			    pos = MATCH_NONE;
			    elmnt_score = 
				elmnt_match_score(target, cfd, i, cfp, flag, &pos);

			    if (flag == EXAMPLE || 
				(flag == SEMANTIC_MARKER && elmnt_score != 0)) {

				/* 対応付けをして，残りの格要素の処理に進む */

				ec_match_flag = 1;
				if (cfd->weight[target]) {
				    elmnt_score /= cfd->weight[target];
				}
				list1.flag[target] = i;
				list2.flag[i] = target;
				list1.score[target] = elmnt_score;
				list2.score[i] = elmnt_score;
				list2.pos[i] = pos;
				assign_list(cfd, list1, cfp, list2, 
					    score + elmnt_score, flag);
				list2.flag[i] = UNASSIGNED;
			    } 
			
			    else {
				/* flag == SEMANTIC_MARKER && elmnt_score == 0
				   すなわち，格助詞の対応する格スロットがあるのに
				   意味マーカ不一致の場合も，処理を進める */

				if (ec_match_flag == 0) ec_match_flag = -1;

				list1.flag[target] = i;
				list2.flag[i] = target;
				list1.score[target] = elmnt_score;
				list2.score[i] = elmnt_score;
				list2.pos[i] = pos;
				/* 対応付けをして，残りの格要素の処理に進む */
				assign_list(cfd, list1, cfp, list2, 
					    score + elmnt_score, flag);
				list2.flag[i] = UNASSIGNED;
			    }
			    break;
			}
		    }
		}
	    }
	}

	list1.flag[target] = NIL_ASSIGNED;
		/* target番目の格要素には対応付けを行わないマーク */

	/* 任意格の場合
	   ※ 同じ表層格が格フレームにある場合，対応付けをすることは
	      すでに上で試されている
	 */
	if (cfd->oblig[target] == FALSE) {
	    assign_list(cfd, list1, cfp, list2, score, flag);
	}

	/* 必須格で対応無(表層格の一致するものがない)の場合
	   ※ この場合eval_assignでscoreはマイナスになるはずなので,
	      この呼出は不必要か？
	 */
	else if (ec_match_flag == 0) {
	    assign_list(cfd, list1, cfp, list2, score, flag);
	}

	/* 必須格で対応有の場合
	      後ろに同じ格助詞があれば対応付けをしない可能性も試す
	   ※ この場合もeval_assignでscoreはマイナスになるはずなので,
	      この呼出は不必要か？
	 */
	else {
	    for (i = target + 1; i < cfd->element_num; i++) {
		if ( cfd->pp[target][target_pp] == cfd->pp[i][0]) {
		    assign_list(cfd, list1, cfp, list2, score, flag);
		    break;
		}
	    }
	}
	/* multi_pp のときは list1.flag[target] を UNASSIGNED にして、
	   明示格を先にチェックする場合と非明示格を先にチェックする場合
	   の両方をチェックする */
	if (multi_pp) {
	    list1.flag[target] = UNASSIGNED;
	    target = -1;
	}
	else {
	    return;
	}
    }

    /* 明示されていない格助詞のチェック */
    for (i = 0; i < cfd->element_num; i++) {
	if (list1.flag[i] == UNASSIGNED) {
	    for (j = 0; cfd->pp[i][j] != END_M; j++) {
		if (cfd->pp[i][j] < 0) {
		    target = i;
		    target_pp = j;
		    break;
		}
	    }
	    if (target >= 0) {
		break;
	    }
	}
    }

    /* 明示されていない格助詞の処理 */
    if (target >= 0) {
	int renkaku, mikaku, verb, gaflag = 0, sotoflag = 0, soto_decide;

	if (cfd->pp[target][target_pp] == -2) {
	    renkaku = 1;
	}
	else {
	    renkaku = 0;
	}
	if (cfd->pp[target][target_pp] == -1) {
	    mikaku = 1;
	}
	else {
	    mikaku = 0;
	}
	if (cfd->ipal_id[0] && str_eq(cfd->ipal_id, "動")) {
	    verb = 1;
	}
	else {
	    verb = 0;
	}

	/* すでにガ格に割り当てられているか (ガガ解析用) */
	if (OptCaseFlag & OPT_CASE_GAGA) {
	    for (i = 0; i < cfp->element_num; i++) {
		if (list2.flag[i] != UNASSIGNED && 
		    cfp->pp[i][1] == END_M && 
		    cfp->pp[i][0] == pp_kstr_to_code("ガ")) {
		    gaflag = 1;
		    break;
		}
	    }
	}

	/* 外の関係解析 */
	if (OptCaseFlag & OPT_CASE_SOTO) {
	    sotoflag = 1;
	}

	for (i = 0; i < cfp->element_num; i++) {

	    /* "〜は" --> "が，を，に" と 対応可
	       埋込文 --> 何とでも対応可 */

	    if (list2.flag[i] == UNASSIGNED &&
		((mikaku &&
		  cfp->pp[i][1] == END_M &&
		  (cfp->pp[i][0] == pp_kstr_to_code("ガ") ||
		   cfp->pp[i][0] == pp_kstr_to_code("ヲ") || 
		   (gaflag && cfp->pp[i][0] == pp_kstr_to_code("ガ２"))
		   )) ||
		 (renkaku &&
		  cfp->pp[i][1] == END_M &&
		  (cfp->pp[i][0] == pp_kstr_to_code("ガ") ||
		   cfp->pp[i][0] == pp_kstr_to_code("ヲ") ||
		   (sotoflag && cfp->pp[i][0] == pp_kstr_to_code("外の関係")) ||
		   (verb && cfp->voice == FRAME_ACTIVE && cfp->pp[i][0] == pp_kstr_to_code("ニ")))))) {
		pos = MATCH_NONE;
		elmnt_score = elmnt_match_score(target, cfd, i, cfp, flag, &pos);
		if (elmnt_score != 0 || flag == EXAMPLE) {
		    if (cfd->weight[target]) {
			elmnt_score /= cfd->weight[target];
		    }
		    list1.flag[target] = i;
		    list2.flag[i] = target;
		    list1.score[target] = elmnt_score;
		    list2.score[i] = elmnt_score;
		    list2.pos[i] = pos;
		    assign_list(cfd, list1, cfp, list2, score + elmnt_score, flag);
		    list2.flag[i] = UNASSIGNED;
		}
	    }
	}

	list1.flag[target] = NIL_ASSIGNED;
	elmnt_score = 0;
	soto_decide = 0;

	/* 外の関係だと推定してボーナスを与える */
	if (renkaku && verb) {
	    /* 外の関係スコア == SOTO_ADD_SCORE + OPTIONAL_CASE_SCORE */
	    if (OptDisc == OPT_DISC) {
		elmnt_score = 0;
	    }
	    else {
		elmnt_score = SOTO_SCORE;
	    }
	    if (cfd->weight[target]) {
		elmnt_score /= cfd->weight[target];
	    }
	    /* elmnt_score -= OPTIONAL_CASE_SCORE; * 任意格ボーナスの分 */

	    /* 格フレームに「外の関係」格を追加 
	       下で cfp->element_num を戻しているので、
	       同じ格が複数あるかどうかをチェックしていない */
	    if (cfp->element_num < CF_ELEMENT_MAX) {
		_make_ipal_cframe_pp(cfp, "外の関係", cfp->element_num);
		list1.flag[target] = cfp->element_num;
		list1.score[target] = elmnt_score;
		list2.flag[cfp->element_num] = target;
		list2.score[cfp->element_num] = elmnt_score;
		cfp->element_num++;
		soto_decide = 1;
	    }
	}
	else if (mikaku) {
	    /* elmnt_score -= OPTIONAL_CASE_SCORE; * 任意格ボーナスの分 */
	}
	assign_list(cfd, list1, cfp, list2, score + elmnt_score, flag);

	/* cfp->element_num は global に影響するので元に戻しておく */
	if (soto_decide) {
	    cfp->element_num--;
	}
	return;
    }
    else if (multi_pp == 1) {
	return;
    }

    /* 評価 : すべての対応付けが終った場合 */
    eval_assign(cfd, &list1, cfp, &list2, score);
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
/*    for (i = 0; i < cmm_ptr->cf_ptr->element_num; i++) { */
    for (i = 0; i < CF_ELEMENT_MAX; i++) {
	assign_p_list.flag[i] = UNASSIGNED;
	assign_p_list.score[i] = -1;
	assign_p_list.pos[i] = -1;
    }

    /* 処理 */

    assign_list(cfd, assign_d_list, cmm_ptr->cf_ptr, assign_p_list, 0, flag);
					    	/* flag 例 or 意味コード */

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
    if (closest > -1) {
	/* 直前格要素の割り当てがあることが条件 */
	if (Current_max_list1[0].flag[closest] != NIL_ASSIGNED) {
	    cmm_ptr->score = (float)Current_max_list1[0].score[closest];
	}
	else {
	    cmm_ptr->score = -1;
	    return 0;
	}
    }
    else {
	cmm_ptr->score = Current_max_score;
    }

    /* tentative */
    if (cmm_ptr->cf_ptr->concatenated_flag == 1) {
	cmm_ptr->score += 1;
    }

#ifdef CASE_DEBUG
    print_crrspnd(cfd, cmm_ptr);
#endif
    return 1;
}

/*====================================================================
                               END
====================================================================*/
