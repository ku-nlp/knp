/*====================================================================

			格構造解析: マッチング

                                               S.Kurohashi 93. 5.31

    $Id$
====================================================================*/
#include "knp.h"

int 	Current_ec_score;	/* 明示されている格の得点 */
int 	Current_max_score;	/* 明示されていない格の得点 */
int 	Current_max_m_e;	/* 要素数 */
int 	Current_max_m_p;	/* 要素の位置 */
int 	Current_max_c_e;	/* 交差数 */

int 	Current_max_num;
LIST 	Current_max_list1[MAX_MATCH_MAX];
LIST 	Current_max_list2[MAX_MATCH_MAX];


int 	SM_match_score[] = {0, 100, 100, 100, 100, 100, 100, 100, 100, 
			    100, 100, 100, 100};
  				/*{0, 5, 8, 10, 12};*/	/* ＳＭ対応スコア */
int     SM_match_unknown = 10;	/* 2 */		 	/* データ未知     */

int 	EX_match_score[] = {0, 0, 50, 70, 80, 90, 100, 110}; 
							/* 用例対応スコア */
int     EX_match_unknown = 40; /* 20 */			/* データ未知     */
int     EX_match_sentence = 100;			/* 格要素 -- 文   */
int     EX_match_tim = 100;				/* 格要素 -- 時間 */
int     EX_match_qua = 100;				/* 格要素 -- 数量 */

/*==================================================================*/
	    void print_assign(LIST *list, CASE_FRAME *cf)
/*==================================================================*/
{
    /* 対応リストの表示 */

    int i;
    for (i = 0; i < cf->element_num; i++) {
	if (list->flag[i] == NIL_ASSIGNED)
	  fprintf(stdout, "  X");
	else
	  fprintf(stdout, "%3d", list->flag[i]+1);
    }
    fprintf(stdout, "\n");
}

/*==================================================================*/
	      int _sm_match_score(char *cpp, char *cpd)
/*==================================================================*/
{
    /*
      NTTの意味素の一桁目は品詞情報
      格フレーム <-----> データ
       x(補文)   <-----> xだけOK
       1(名詞)   <-----> x以外OK
                         (名詞以外のものはget_smの時点で排除 99/01/13)
    */

    int i;

    if (cpp[0] == 'x') {
	if (cpd[0] == 'x')
	    return 12;
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

    for (i = 1; i < 12; i++) {
	if (cpp[i] == '*') {
	    return i;
	} else if (cpd[i] == '*') {
	    return 0;
	} else if (cpp[i] != cpd[i]) {
	    return 0;
	}
    }
    return 12;
}

/*==================================================================*/
	      int _ex_match_score(char *cp1, char *cp2)
/*==================================================================*/
{
    /* 例の分類語彙表コードのマッチング度の計算 */

    int  match = 0;

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
	   int elmnt_match_score(int as1, CASE_FRAME *cfd,
				 int as2, CASE_FRAME *cfp, int flag)
/*==================================================================*/
{
    /* 意味マーカのマッチング度の計算 */

    int i, j, k, tmp_score, score = -100;

    if (flag == SEMANTIC_MARKER) {
	
	if (cfd->sm[as1][0] == '\0'|| cfp->sm[as2][0] == '\0') 
	    return SM_match_unknown;

	for (j = 0; cfp->sm[as2][j]; j+=12) {
	    if (!strncmp(cfp->sm[as2]+j, sm2code("→"), 12)) {

		for (k = 0; cfp->ex[as2][k]; k+=10) {
		    for (i = 0; cfd->ex[as1][i]; i+=10) {
			tmp_score = 
			    EX_match_score[_ex_match_score(cfp->ex[as2]+k, 
							   cfd->ex[as1]+i)];
			if (tmp_score == 110) {
			    return 100;
			}
		    }
		}
	    }
	    else {
		for (i = 0; cfd->sm[as1][i]; i+=12) {
		    tmp_score = 
			SM_match_score[_sm_match_score(cfp->sm[as2]+j,
						       cfd->sm[as1]+i)];
		    if (tmp_score && (cfp->sm_flag[as2][j/12] == FALSE))
			return -100;
		    if (tmp_score > score) score = tmp_score;
		}
	    }
	}
	return score;
    }

    else if (flag == EXAMPLE) {
	
	/* 特別 : 格要素 -- 文 */
	if (!strcmp(cfd->sm[as1], (char *)sm2code("補文")) &&
	    !strcmp(cfp->sm[as2], (char *)sm2code("補文")))
	    return EX_match_sentence;

	/* 特別 : 格要素 -- 時間 */
	if (!strcmp(cfd->sm[as1], (char *)sm2code("時間"))) {
	    for (i = 0; cfp->sm[as2][i]; i+=12)
		if (!strcmp(cfp->sm[as2]+i, (char *)sm2code("時間")))
		    return EX_match_tim;
	}

	/* 特別 : 格要素 -- 数量 */
	if (!strcmp(cfd->sm[as1], (char *)sm2code("数量")) ||
	    !strcmp(cfd->sm[as1]+12, (char *)sm2code("数量"))) {
	    for (i = 0; cfp->sm[as2][i]; i+=12)
		if (!strcmp(cfp->sm[as2]+i, (char *)sm2code("数量")))
		    return EX_match_tim;
	}
	    
	if (cfd->ex[as1][0] == '\0' || cfp->ex[as2][0] == '\0') 
	    return EX_match_unknown;

	for (j = 0; cfp->ex[as2][j]; j+=10) {
	    for (i = 0; cfd->ex[as1][i]; i+=10) {
		tmp_score = 
		  EX_match_score[_ex_match_score(cfp->ex[as2]+j, 
						 cfd->ex[as1]+i)];
		if (tmp_score > score) score = tmp_score;
	    }
	}
	return score;
    }
    return 0;
}

/*==================================================================*/
	    void eval_assign(CASE_FRAME *cfd, LIST *list1,
			     CASE_FRAME *cfp, LIST *list2,
			     int score)
/*==================================================================*/
{
    /* フレームのマッチング度の計算(格明示部分を除く) */

    int i, j, local_score;
    int local_m_e = 0;
    int local_m_p = 0;
    int local_c_e = 0;
    int pat_element = 0, dat_element = 0;
    
    local_score = score;

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

    /* 文中の要素数(任意(埋め込み文の被修飾語)でマッチしていない要素以外) */
    for (i = 0; i < cfd->element_num; i++)
      if (!(cfd->oblig[i] == FALSE && list1->flag[i] == NIL_ASSIGNED))
	dat_element++;

    /* 格フレーム中の要素数(任意でマッチしていない要素以外) */
    for (i = 0; i < cfp->element_num; i++)
      if (!(cfp->oblig[i] == FALSE && list2->flag[i] == UNASSIGNED))
	pat_element++;

    if (local_m_e < dat_element)
      local_score = -1;
    else if (dat_element == 0 || pat_element == 0 || local_m_e == 0)
      local_score = 0;
    else 
      /* local_score = local_score * sqrt((double)local_m_e)
	/ sqrt((double)dat_element * pat_element);*/
      /* local_score = local_score * local_m_e
	/ (dat_element * sqrt((double)pat_element)); */
      local_score = local_score / sqrt((double)pat_element);

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
    int elmnt_score;
    int i, j;

#ifdef CASE_DEBUG
    fprintf(stdout, "dat");
    for (i = 0; i < cfd->element_num; i++)
      fprintf(stdout, "%d ", list1.flag[i]);
    fputc('\n', stdout);
    fprintf(stdout, "pat");
    for (i = 0; i < cfp->element_num; i++)
      fprintf(stdout, "%d ", list2.flag[i]);
    fputc('\n', stdout);
#endif 
    
    /* 明示されている格助詞のチェック */
    for (i = 0; i < cfd->element_num; i++) {
	if (list1.flag[i] == UNASSIGNED &&
	    cfd->pp[i][0] >= 0) {
	    target = i;
	    break;
	}
    }

    /* 明示されている格助詞の処理 */
    if (target >= 0) {
	ec_match_flag = 0;
	for (i = 0; i < cfp->element_num; i++) {
	    if (list2.flag[i] == UNASSIGNED) {
		for (j = 0; cfp->pp[i][j] >= 0; j++) {
		    if (cfd->pp[target][0] == cfp->pp[i][j] ||
			(cfd->pp[target][0] == pp_hstr_to_code("によって") &&
			 cfp->pp[i][j] == pp_kstr_to_code("デ"))) {

			elmnt_score = 
			  elmnt_match_score(target, cfd, i, cfp, flag);

			if (flag == EXAMPLE || 
			    (flag == SEMANTIC_MARKER && elmnt_score != 0)) {

			    /* 対応付けをして，残りの格要素の処理に進む */

			    ec_match_flag = 1;
			    list1.flag[target] = i;
			    list2.flag[i] = target;
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
		if ( cfd->pp[target][0] == cfd->pp[i][0]) {
		    assign_list(cfd, list1, cfp, list2, score, flag);
		    break;
		}
	    }
	}
	return;
    }

    /* 明示されていない格助詞のチェック */
    for (i = 0; i < cfd->element_num; i++) {
	if (list1.flag[i] == UNASSIGNED) {
	    target = i;
	    break;
	}
    }

    /* 明示されていない格助詞の処理 */
    if (target >= 0) {
	for (i = 0; i < cfp->element_num; i++) {

	    /* "〜は" --> "が，を，に" と 対応可
	       埋込文 --> 何とでも対応可 */

	    if (list2.flag[i] == UNASSIGNED &&
		((cfd->pp[target][0] == -1 &&
		  cfp->pp[i][1] == -1 &&
		  (cfp->pp[i][0] == pp_kstr_to_code("ガ") ||
		   cfp->pp[i][0] == pp_kstr_to_code("ヲ") ||
		   cfp->pp[i][0] == pp_kstr_to_code("ニ"))) ||
		 (cfd->pp[target][0] == -2))) {
		elmnt_score = elmnt_match_score(target, cfd, i, cfp, flag);
		if (elmnt_score != 0 || flag == EXAMPLE) {
		    list1.flag[target] = i;
		    list2.flag[i] = target;
		    assign_list(cfd, list1, cfp, list2, score + elmnt_score, flag);
		    list2.flag[i] = UNASSIGNED;
		}
	    }
	}
	
	list1.flag[target] = NIL_ASSIGNED;
	assign_list(cfd, list1, cfp, list2, score, flag);
	return;
    } 

    /* 評価 : すべての対応付けが終った場合 */
    eval_assign(cfd, &list1, cfp, &list2, score);
}

/*==================================================================*/
void case_frame_match(CASE_FRAME *cfd, CF_MATCH_MGR *cmm_ptr, int flag)
/*==================================================================*/
{
    /* 格フレームのマッチング */

    LIST assign_d_list, assign_p_list;
    int i;

    /* 初期化 */

    Current_max_num = 0;
    Current_max_score = -2;
    Current_max_m_e = 0;
    Current_max_m_p = 0;
    Current_max_c_e = 0;
    for (i = 0; i < cfd->element_num; i++)
      assign_d_list.flag[i] = UNASSIGNED;
    for (i = 0; i < cmm_ptr->cf_ptr->element_num; i++)
      assign_p_list.flag[i] = UNASSIGNED;

    /* 処理 */

    assign_list(cfd, assign_d_list, cmm_ptr->cf_ptr, assign_p_list, 0, flag);
					    	/* flag 例 or 意味コード */

    /* 後処理 */

    if (Current_max_num == MAX_MATCH_MAX)
      fprintf(stderr, "Too many case matching result !\n");

    cmm_ptr->score = Current_max_score;
    cmm_ptr->result_num = Current_max_num;
    for (i = 0; i < Current_max_num; i++)
      cmm_ptr->result_lists_p[i] = Current_max_list2[i];


#ifdef CASE_DEBUG
    print_crrspnd(cfd, cmm_ptr);
#endif
}

/*====================================================================
                               END
====================================================================*/
