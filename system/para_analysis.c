/*====================================================================

			     並列構造解析

                                               S.Kurohashi 91. 6.25
                                               S.Kurohashi 93. 5.31

    $Id$
====================================================================*/
#include "knp.h"

#define PENALTY		1 /* 2 */
#define BONUS   	6
#define MINUS   	7
#define PENA_MAX   	1000
#define ENOUGH_MINUS	-100.0
#define START_HERE	-1

int 	score_matrix[BNST_MAX][BNST_MAX];
int 	prepos_matrix[BNST_MAX][BNST_MAX];
int 	maxpos_array[BNST_MAX];
int 	maxsco_array[BNST_MAX];
int 	penalty_table[BNST_MAX];

float	norm[] = {
    1.00,  1.00,  1.59,  2.08,  2.52,  2.92,  3.30,  3.66,  4.00,  4.33,
    4.64,  4.95,  5.24,  5.53,  5.81,  6.08,  6.35,  6.61,  6.87,  7.12,
    7.37,  7.61,  7.85,  8.09,  8.32,  8.55,  8.78,  9.00,  9.22,  9.44,
    9.65,  9.87, 10.08, 10.29, 10.50, 10.70, 10.90, 11.10, 11.30, 11.50,
   11.70, 11.89, 12.08, 12.27, 12.46, 12.65, 12.84, 13.02, 13.21, 13.39};

extern QUOTE_DATA quote_data;

/*==================================================================*/
	void mask_quote_scope(SENTENCE_DATA *sp, int L_B_pos)
/*==================================================================*/
{
    int i, j, k, l, start, end;

    /* 括弧がある場合に並列構造の範囲に制限を設ける */

    for (k = 0; quote_data.in_num[k] >= 0; k++) {

	int start = quote_data.in_num[k];
	int end = quote_data.out_num[k];

	/* 後に括弧がある場合 */

	if (L_B_pos < start) {
	    for (i = 0; i < start; i++)
		for (j = start; j < end; j++)
		    restrict_matrix[i][j] = 0;
	} 

	/* 前に括弧がある場合 (キーが括弧内の末尾の場合も) */

	else if (end <= L_B_pos) {
	    for (i = start + 1; i <= end; i++)
		for (j = end + 1; j < sp->Bnst_num; j++)
		    restrict_matrix[i][j] = 0;
	}

	/* キーが括弧の中にある場合 */

	else {
	    for (i = 0; i <= end; i++)
		for (j = start; j < sp->Bnst_num; j++)
		    if (i < start || end < j)
			restrict_matrix[i][j] = 0;

	    /* 括弧の中に句点がある場合 */
	    for (l = start; l < end; l++)
		if (check_feature(sp->bnst_data[l].f, "係:文末"))
		    for (i = start; i <= l; i++)
			for (j = l + 1; j <= end; j++)
			    restrict_matrix[i][j] = 0;
	}
    }

    if (k && OptDisplay == OPT_DEBUG)
	print_matrix(sp, PRINT_RSTQ, L_B_pos);
}

/*==================================================================*/
	int bnst_match(SENTENCE_DATA *sp, int pos1, int pos2)
/*==================================================================*/
{
    int flag1, flag2;
    char *cp1, *cp2;
    BNST_DATA *ptr1 = &(sp->bnst_data[pos1]);
    BNST_DATA *ptr2 = &(sp->bnst_data[pos2]);

    /*
      パスのスコア計算において区切りペナルティをcancelする条件
    	・係が同じ
	・用言であるかどうかが同じ
	・読点があるかないかが同じ
	
       ※ 条件は少し緩くしてある．問題があれば強める 
    */

    cp1 = (char *)check_feature(ptr1->f, "係");
    cp2 = (char *)check_feature(ptr2->f, "係");
    if (strcmp(cp1, cp2)) return 0;
	
    flag1 = check_feature(ptr1->f, "用言") ? 1 : 0;
    flag2 = check_feature(ptr2->f, "用言") ? 1 : 0;
    if (flag1 != flag2) return 0;

    if (check_feature(ptr1->f, "用言")) {
	cp1 = (char *)check_feature(ptr1->f, "ID");
	cp2 = (char *)check_feature(ptr2->f, "ID");
	if (!cp1 || !cp2 || strcmp(cp1, cp2)) return 0;
    }
    
    flag1 = check_feature(ptr1->f, "読点") ? 1 : 0;
    flag2 = check_feature(ptr2->f, "読点") ? 1 : 0;
    if (flag1 != flag2) return 0;

    return 1;
}

/*==================================================================*/
int calc_static_level_penalty(SENTENCE_DATA *sp, int L_B_pos, int pos)
/*==================================================================*/
{
    int minus_score = 0;
    int level1 = sp->bnst_data[L_B_pos].sp_level;
    int level2 = sp->bnst_data[pos].sp_level;

    if (level1 <= level2)
	minus_score = MINUS * (level2 - level1 + 1);

    return minus_score;
}
/*==================================================================*/
int calc_dynamic_level_penalty(SENTENCE_DATA *sp, int L_B_pos, int pos1, int pos2)
/*==================================================================*/
{
    if (sp->bnst_data[pos1].sp_level == sp->bnst_data[pos2].sp_level &&
	bnst_match(sp, pos1, pos2) &&
	!bnst_match(sp, pos1, L_B_pos))
	return 0;
    else if (check_feature(sp->bnst_data[pos1].f, "提題") &&
	     check_feature(sp->bnst_data[pos2].f, "提題"))
	return 0;
			/* 「〜は」の場合は読点の有無,レベルを無視 */
    else
	return(penalty_table[pos1] + penalty_table[pos2]);
}

/*==================================================================*/
     int plus_bonus_score(SENTENCE_DATA *sp, int R_pos, int type)
/*==================================================================*/
{
    BNST_DATA *b_ptr;

    b_ptr = &sp->bnst_data[R_pos];

    if (type == PARA_KEY_I) { 
	return 0;
    } else if (type == PARA_KEY_N) {
	if (check_feature(b_ptr->f, "名並終点"))
	    return BONUS;
	else return 0;
    } else if (type == PARA_KEY_P) {
	if (check_feature(b_ptr->f, "述並終点"))
	    return BONUS;
	else return 0;
    } else {
	return 0;
    }
}

/*==================================================================*/
   void dp_search_scope(SENTENCE_DATA *sp, int L_B_pos, int R_pos)
/*==================================================================*/
{
    int i, j, current_max, score_upward, score_sideway;
    
    /* ＤＰマッチング */

    for (j = R_pos; j > L_B_pos; j--)  {

	/* 最右列の処理 */
	
	if (j == R_pos) {
	    score_matrix[L_B_pos][R_pos] = match_matrix[L_B_pos][R_pos];
	    prepos_matrix[L_B_pos][R_pos] = START_HERE;
	    for (i=L_B_pos-1; i>=0; i--)
	      score_matrix[i][R_pos] = - PENA_MAX;
	}
	
	else {

	    /* 最下行の処理 */

	    score_sideway = score_matrix[L_B_pos][j+1] 
	      		    - PENALTY - penalty_table[j];
	    score_matrix[L_B_pos][j] = score_sideway;
	    prepos_matrix[L_B_pos][j] = L_B_pos;

	    /* 他の行の処理:下からと左からのスコアを比較 */

	    for (i=L_B_pos-1; i>=0; i--) {
		score_upward = match_matrix[i][j] + maxsco_array[i+1]
		  - calc_dynamic_level_penalty(sp, L_B_pos, i, j);
		score_sideway = score_matrix[i][j+1] 
		  - PENALTY - penalty_table[j];
		
		if (score_upward >= score_sideway) {
		    score_matrix[i][j] = score_upward;
		    prepos_matrix[i][j] = maxpos_array[i+1];
		} else {
		    score_matrix[i][j] = score_sideway;
		    prepos_matrix[i][j] = i;
		}
	    }
	}

	/* 次の列のために最大値，最大位置を計算 */

	current_max = score_matrix[L_B_pos][j];
	maxpos_array[L_B_pos] = L_B_pos;
	maxsco_array[L_B_pos] = score_matrix[L_B_pos][j];

	for (i=L_B_pos-1; i>=0; i--) {

	    current_max -= (PENALTY + penalty_table[i]);
	    if (current_max <= score_matrix[i][j]) {
		current_max = score_matrix[i][j];
		maxpos_array[i] = i;
		maxsco_array[i] = current_max;
	    } else {
		maxpos_array[i] = maxpos_array[i+1];
		maxsco_array[i] = current_max;
	    }
	}
    }
}

/*==================================================================*/
void _detect_para_scope(SENTENCE_DATA *sp, PARA_DATA *ptr, int R_pos)
/*==================================================================*/
{
    int i, j, flag, nth;
    int L_B_pos = ptr->L_B;
    int ending_bonus_score;
    int max_pos = -1;
    float current_score, sim_threshold, new_threshold,
	max_score = ENOUGH_MINUS, pure_score = 0;
    char *cp;
    FEATURE *fp;

    /*							 */
    /* スタート位置(R_pos)からの解析を本当に行うかどうか */
    /*							 */

    /* 類似度が0なら中止 */

    if (match_matrix[L_B_pos][R_pos] == 0) return;

    /* restrict_matrixで可能性がない場合は中止 */

    flag = FALSE;
    for (i = 0; i <= L_B_pos; i++) {
	if (restrict_matrix[i][R_pos]) {
	    flag = TRUE; break;
	}
    }
    if (flag == FALSE) return;

    /* 「〜，それを」という並列は中止 */

    if (L_B_pos + 1 == R_pos &&	
	check_feature(sp->bnst_data[R_pos].f, "指示詞"))
	return;

    /* ルールによる制限(類似スコアの閾値を取得) */

    /* 条件がなければ閾値は0.0に */
    if ((ptr->f_pattern).fp[0] == NULL) {
	sim_threshold = 0.0;
    } 
    /* 条件があれば，マッチするものの中で最低の閾値に */
    else {
	sim_threshold = 100.0;
	nth = 0;
	while (fp = (ptr->f_pattern).fp[nth]) {
	    if (feature_AND_match(fp, sp->bnst_data[R_pos].f,
				  sp->bnst_data + L_B_pos,
				  sp->bnst_data + R_pos) == TRUE) {
		if (cp = (char *)check_feature(fp, "&ST")) {
		    sscanf(cp, "&ST:%f", &new_threshold);
		} else {
		    new_threshold = 0.0;
		}
		if (new_threshold < sim_threshold )
		    sim_threshold = new_threshold;
	    }
	    nth++;
	}
	if (sim_threshold == 100.0) return;
    }

    /* if (feature_pattern_match(&(ptr->f_pattern), 
       sp->bnst_data[R_pos].f,
       sp->bnst_data + L_B_pos,
       sp->bnst_data + R_pos) == FALSE) */

    /*		    */
    /* DP MATCHING  */
    /*		    */

    dp_search_scope(sp, L_B_pos, R_pos);


    /* 最大パスの検出 */

    ending_bonus_score = plus_bonus_score(sp, R_pos, ptr->type);
    for (i = L_B_pos; i >= 0; i--) {
	current_score = 
	    (float)(score_matrix[i][L_B_pos+1] + ending_bonus_score)
	    / norm[R_pos - i + 1];
	if (restrict_matrix[i][R_pos] && 
	    max_score < current_score) {
	    max_score = current_score;
	    pure_score = (float)score_matrix[i][L_B_pos+1]
		/ norm[R_pos - i + 1];
	    /* pure_score は末尾表現のボーナスを除いた値 */
	    max_pos = i;
	}
    }

    /* ▼ (a...)(b)という並列は扱えない．括弧の制限などでこうならざる
       をえない場合は，並列とは認めないことにする (暫定的) */

    if (L_B_pos + 1 == R_pos && max_pos != L_B_pos) {
	max_pos = i;
	max_score = -100;
	return;
    }

    /*
      閾値を越えて，まだstatusが x なら n に
      閾値を越えて，statusが n なら スコア比較
      閾値を越えなくての，参考のためスコアを記憶
    */
    flag = FALSE;
    if (sim_threshold <= pure_score &&
	ptr->status == 'x') {
	ptr->status = 'n';
	flag = TRUE;
    }
    else if (sim_threshold <= pure_score &&
	     ptr->status == 'n' &&
	     ptr->max_score < max_score) {
	flag = TRUE;
    }
    else if (ptr->status == 'x' &&
	     ptr->max_score < max_score) {
	flag = TRUE;
    }	

    if (flag == TRUE) {
	ptr->max_score = max_score;
	ptr->pure_score = pure_score;
	ptr->max_path[0] = max_pos;
	for (j = 0;; j++) {
	    ptr->max_path[j+1] = prepos_matrix[ptr->max_path[j]][j+L_B_pos+1];
	    if (ptr->max_path[j+1] == START_HERE) {
		ptr->R = j + L_B_pos + 1;
		break;
	    }
	}
    }
}

/*==================================================================*/
int detect_para_scope(SENTENCE_DATA *sp, int para_num, int restrict_p)
/*==================================================================*/
{
    int i, j, k;
    PARA_DATA *para_ptr = &(sp->para_data[para_num]);
    int L_B_pos = para_ptr->L_B;

    /* 
       restrict_p
         TRUE : 前の並列解析の失敗によって特定のキーだけを処理する場合
	 FALSE : はじめにすべてのキーを処理する場合
	 
       restrict_matrix
         括弧による制限と前の並列構造解析による制限(restrict_pの場合)
	 (restrict_p==FALSEの場合ここで初期化)
    */

    para_ptr->status = 'x';
    para_ptr->max_score = ENOUGH_MINUS;
    para_ptr->pure_score = ENOUGH_MINUS;
    para_ptr->manager_ptr = NULL;

    if (restrict_p == FALSE)
	for (i = 0; i < sp->Bnst_num; i++)
	    for (j = i + 1; j < sp->Bnst_num; j++)
		restrict_matrix[i][j] = 1;

    mask_quote_scope(sp, L_B_pos);

    for (k = 0; k < sp->Bnst_num; k++) {
	penalty_table[k] = (k == L_B_pos) ? 
	  0 : calc_static_level_penalty(sp, L_B_pos, k);
    }

    for (j = L_B_pos+1; j < sp->Bnst_num; j++)
	_detect_para_scope(sp, para_ptr, j);

    if (para_ptr->status == 'x') {
	;
	/*
	fprintf(Outfp, ";; Cannot find proper CS for the key <");
	print_bnst(sp->bnst_data + ptr->L_B, NULL);
	fprintf(Outfp, ">.\n");
	*/
    } else if (para_ptr->status == 'n' &&
	       para_ptr->pure_score > 4.0) {
	para_ptr->status = 's';
    }
    
    return TRUE;	/* 解析結果statusがxでも,一応TUREを返す */
}

/*==================================================================*/
	    void detect_all_para_scope(SENTENCE_DATA *sp)
/*==================================================================*/
{
    int i;

    for (i = 0; i < Para_num; i++) 
	detect_para_scope(sp, i, FALSE);
}

/*==================================================================*/
		int check_para_key(SENTENCE_DATA *sp)
/*==================================================================*/
{
    int i;
    char *cp, type[16], condition[256];

    for (i = 0; i < sp->Bnst_num; i++) {

	if ((cp = (char *)check_feature(sp->bnst_data[i].f, "並キ")) != NULL) {

	    sp->bnst_data[i].para_num = Para_num;

	    type[0] = NULL;
	    condition[0] = NULL;
	    sscanf(cp, "%*[^:]:%[^:]:%s", type, condition);

	    sp->para_data[Para_num].para_char = 'a'+ Para_num;
	    sp->para_data[Para_num].L_B = i;

	    if (!strcmp(type, "名")) {
		sp->bnst_data[i].para_key_type = PARA_KEY_N	;
	    } else if (!strcmp(type, "述")) {
		sp->bnst_data[i].para_key_type = PARA_KEY_P;
	    } else if (!strcmp(type, "？")) {
		sp->bnst_data[i].para_key_type = PARA_KEY_A;
	    }
	    sp->para_data[Para_num].type = sp->bnst_data[i].para_key_type;
	    string2feature_pattern(&(sp->para_data[Para_num].f_pattern),condition);
	    
	    Para_num ++;
	    if (Para_num >= PARA_MAX) {
		fprintf(stderr, "Too many para (%s)!\n", Comment);
		return CONTINUE;
	    }
	}
	else {
	    sp->bnst_data[i].para_num = -1;
	}
    }

    if (Para_num == 0) return 0;

    for (i = 0; i < sp->Bnst_num; i++) {

	if ((cp = (char *)check_feature(sp->bnst_data[i].f, "区切")) != NULL) {
	    if (check_feature(sp->bnst_data[i].f, "読点")) {
		sscanf(cp, "%*[^:]:%*d-%d", &(sp->bnst_data[i].sp_level));
	    } else {
		sscanf(cp, "%*[^:]:%d-%*d", &(sp->bnst_data[i].sp_level));
	    }
	} else {
	    sp->bnst_data[i].sp_level = 0;
	}
    }

    return Para_num;
}


/*==================================================================*/
       int farthest_child(SENTENCE_DATA *sp, BNST_DATA *b_ptr)
/*==================================================================*/
{
    /* 一番遠い子供の文節番号を返す
       (今のところこの関数は使っていない) */

    int i;
    BNST_DATA	*loop_ptr = b_ptr;
    
    while (loop_ptr->child[0]) {
	for (i = 0; loop_ptr->child[i]; i++);
	loop_ptr = loop_ptr->child[i-1];
    }
    
    return (loop_ptr - sp->bnst_data);
}

/*==================================================================*/
		void para_recovery(SENTENCE_DATA *sp)
/*==================================================================*/
{
    /* 並列構造の情報の再現 */

    int		i, j;
    BNST_DATA	*b_ptr;

    Para_num = 0;
    Para_M_num = 0;

    for (i = 0, b_ptr = sp->bnst_data; i < sp->Bnst_num; i++, b_ptr++) {
	if (b_ptr->dpnd_type == 'P') {
	    sp->para_data[Para_num].L_B = i;
	    sp->para_data[Para_num].R = b_ptr->dpnd_head;
	    for (j = i - 1; 
		 j >= 0 && 
		     (sp->bnst_data[j].dpnd_head < i ||
		      (sp->bnst_data[j].dpnd_head == i &&
		       sp->bnst_data[j].dpnd_type != 'P'));
		 j--);
	    sp->para_data[Para_num].max_path[0] = j + 1;
	    sp->para_data[Para_num].status = 'n';
	    Para_num++;
	}
    }
    detect_para_relation(sp);
}

/*====================================================================
                               END
====================================================================*/
