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
	  int mask_quote_scope(int L_B_pos, int restrict_p)
/*==================================================================*/
{
    int i, j, k, start, end;

    /* 
       括弧がある場合に並列構造の範囲に制限を設ける

       <注意> restrict_tableについては制限していない．結果的に
       matrixで制限されるが, 本来はきっちりと処理する必要がある．
       restrict_tableは廃止してもいいかも ？？
    */

    if (restrict_p == NULL) {
	for (i = 0; i < Bnst_num; i++)
	    for (j = i + 1; j < Bnst_num; j++)
		restrict_matrix[i][j] = 1;
	for (j = L_B_pos + 1; j < Bnst_num; j++)
	    restrict_table[j] = 1;
    }

    for (k = 0; quote_data.in_num[k] >= 0; k++) {

	int start = quote_data.in_num[k];
	int end = quote_data.out_num[k];

	restrict_p = TRUE;

	/* 後に括弧がある場合 */

	if (L_B_pos < start) {
	    for (i = 0; i < start; i++)
		for (j = start; j < end; j++)
		    restrict_matrix[i][j] = 0;
	} 

	/* 前に括弧がある場合 (キーが括弧内の末尾の場合も) */

	else if (end <= L_B_pos) {
	    for (i = start + 1; i <= end; i++)
		for (j = end + 1; j < Bnst_num; j++)
		    restrict_matrix[i][j] = 0;
	}

	/* キーが括弧の中にある場合 */

	else {
	    for (i = 0; i <= end; i++)
		for (j = start; j < Bnst_num; j++)
		    if (i < start || end < j)
			restrict_matrix[i][j] = 0;
	}
    }

    if (k && OptDisplay == OPT_DEBUG)
	print_matrix(PRINT_RSTQ, L_B_pos);
}

/*==================================================================*/
		int bnst_match(int pos1, int pos2)
/*==================================================================*/
{
    int flag1, flag2;
    char *cp1, *cp2;
    BNST_DATA *ptr1 = &(bnst_data[pos1]);
    BNST_DATA *ptr2 = &(bnst_data[pos2]);

    /*
      パスのスコア計算において区切りペナルティをcancelする条件
    	・係が同じ
	・用言:強であるかどうかが同じ
	・読点があるかないかが同じ
	
       ※ 条件は少し緩くしてある．問題があれば強める 
    */

    cp1 = (char *)check_feature(ptr1->f, "係");
    cp2 = (char *)check_feature(ptr2->f, "係");
    if (strcmp(cp1, cp2)) return 0;
	

    flag1 = check_feature(ptr1->f, "用言:強") ? 1 : 0;
    flag2 = check_feature(ptr2->f, "用言:強") ? 1 : 0;
    if (flag1 != flag2) return 0;
    
    flag1 = check_feature(ptr1->f, "読点") ? 1 : 0;
    flag2 = check_feature(ptr2->f, "読点") ? 1 : 0;
    if (flag1 != flag2) return 0;

    return 0;
}

/*==================================================================*/
	 int calc_static_level_penalty(int L_B_pos, int pos)
/*==================================================================*/
{
    int minus_score = 0;
    int level1 = bnst_data[L_B_pos].sp_level;
    int level2 = bnst_data[pos].sp_level;

    if (level1 <= level2)
      minus_score = MINUS * (level2 - level1 + 1);

    return minus_score;
}
/*==================================================================*/
   int calc_dynamic_level_penalty(int L_B_pos, int pos1, int pos2)
/*==================================================================*/
{
    if (bnst_data[pos1].sp_level == bnst_data[pos2].sp_level &&
	bnst_match(pos1, pos2) &&
	!bnst_match(pos1, L_B_pos))
	return 0;
    else if (check_feature(bnst_data[pos1].f, "提題") &&
	     check_feature(bnst_data[pos2].f, "提題"))
	return 0;
			/* 「〜は」の場合は読点の有無,レベルを無視 */
    else
	return(penalty_table[pos1] + penalty_table[pos2]);
}

/*==================================================================*/
	      int plus_bonus_score(int R_pos, int type)
/*==================================================================*/
{
    BNST_DATA *b_ptr;

    b_ptr = &bnst_data[R_pos];

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
void dp_search_scope(int L_B_pos, int R_pos, int type, int restrict_p)
/*==================================================================*/
{
    int i, j, current_max, score_upward, score_sideway, score_start;
    
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

	    if (0 &&
		restrict_p == FALSE &&
		match_matrix[L_B_pos][j] && 
		(score_start = 
		   match_matrix[L_B_pos][j] + plus_bonus_score(j, type))
		     >= score_sideway) {

		/* その位置からのスタートを考慮する場合(高速化)
		   スコアを範囲の広さで正規化する場合は使えない
		   さらに,
		   plus_bonus_score(j, type)は最後に計算すること
		   にしたので，この高速化は実質的にはもう使えない
		   */

		score_matrix[L_B_pos][j] = score_start;
		prepos_matrix[L_B_pos][j] = START_HERE;
	    } else {
		score_matrix[L_B_pos][j] = score_sideway;
		prepos_matrix[L_B_pos][j] = L_B_pos;
	    }

	    for (i=L_B_pos-1; i>=0; i--) {

		/* 下からのスコアと左からのスコアを比較 */

		score_upward = match_matrix[i][j] + maxsco_array[i+1]
		  - calc_dynamic_level_penalty(L_B_pos, i, j);
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
  void _detect_para_scope(PARA_DATA *ptr, int R_pos, int restrict_p)
/*==================================================================*/
{
    int i, j, flag;
    int L_B_pos = ptr->L_B;
    int ending_bonus_score;
    int max_pos = -1;
    float current_score, max_score = ENOUGH_MINUS, pure_score;
    FEATURE *fp;

    /* 制限がある場合,まったく可能性のないスタート位置(R_pos)は排除 */

    if (restrict_p) { 
	flag = FALSE;
	for (i = 0; i <= L_B_pos; i++)
	    if (restrict_matrix[i][R_pos]) flag = TRUE;
	if (flag == FALSE) return;
    }

    /* 末尾の文節に制限がある場合 */
    
    if (feature_pattern_match(&(ptr->f_pattern), 
			      bnst_data[R_pos].f,
			      bnst_data + L_B_pos,
			      bnst_data + R_pos) == FALSE) return;  

    /* 「〜，それを」という誤った並列を排除 */

    if (L_B_pos + 1 == R_pos &&	
	check_feature(bnst_data[R_pos].f, "指示詞"))
	return;

    /* DP MATCHING (R_posで終るものについて) */

    dp_search_scope(L_B_pos, R_pos, ptr->type, restrict_p);
    ending_bonus_score = plus_bonus_score(R_pos, ptr->type);

    /* 最大パスの検出 (R_posで終るものについて) */

    for (i = L_B_pos; i >= 0; i--) {
	current_score = 
	    (float)(score_matrix[i][L_B_pos+1] + ending_bonus_score)
	    / norm[R_pos - i + 1];
	if (!(restrict_p == TRUE && restrict_matrix[i][R_pos] == 0) &&
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

    /* 他のR_posの最大パスと比較 */

    if (ptr->max_score < max_score) {
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
	       int check_para_strength(PARA_DATA *ptr)
/*==================================================================*/
{
    if (ptr->pure_score < ptr->threshold) {
	ptr->status = 'x';
	return FALSE;
    }
    if (ptr->pure_score > 4.0) {
	ptr->status = 's';
	return TRUE;
    }
    else if (ptr->pure_score > 0.0) {
	ptr->status = 'n';
	return TRUE;
    }
    else {
	ptr->status = 'x';
	/*
	fprintf(stdout, ";; Cannot find proper CS for the key <");
	print_bnst(bnst_data + ptr->L_B, NULL);
	fprintf(stdout, ">.\n");
	*/
	return FALSE;
    }
}

/*==================================================================*/
	 int detect_para_scope(int para_num, int restrict_p)
/*==================================================================*/
{
    int i, j, k;
    PARA_DATA *para_ptr = &(para_data[para_num]);
    int L_B_pos = para_ptr->L_B;

    para_ptr->max_score = ENOUGH_MINUS;
    para_ptr->pure_score = ENOUGH_MINUS;
    para_ptr->manager_ptr = NULL;
  
    if (mask_quote_scope(L_B_pos, restrict_p)) 
	restrict_p = TRUE;

    for (k = 0; k < Bnst_num; k++) {
	penalty_table[k] = (k == L_B_pos) ? 
	  0 : calc_static_level_penalty(L_B_pos, k);
    }

    for (j = L_B_pos+1; j < Bnst_num; j++)
	if (match_matrix[L_B_pos][j] != 0)
	    _detect_para_scope(para_ptr, j, restrict_p);

    check_para_strength(para_ptr);
	    
    return TRUE;	/* 解析結果statusがxでも,一応TUREを返す */
}

/*==================================================================*/
		     void detect_all_para_scope()
/*==================================================================*/
{
    int i;

    for (i = 0; i < Para_num; i++) 
	detect_para_scope(i, FALSE);
}

/*==================================================================*/
			 int check_para_key()
/*==================================================================*/
{
    int i;
    float threshold;
    char *cp, type[16], condition[256];

    for (i = 0; i < Bnst_num; i++) {

	if ((cp = (char *)check_feature(bnst_data[i].f, "並キ")) != NULL) {

	    bnst_data[i].para_num = Para_num;

	    type[0] = NULL;
	    threshold = 0.0;
	    condition[0] = NULL;
	    sscanf(cp, "%*[^:]:%[^:]:%f:%s", type, &threshold, condition);

	    para_data[Para_num].para_char = 'a'+ Para_num;
	    para_data[Para_num].L_B = i;

	    if (!strcmp(type, "名")) {
		bnst_data[i].para_key_type = PARA_KEY_N	;
	    } else if (!strcmp(type, "述")) {
		bnst_data[i].para_key_type = PARA_KEY_P;
	    } else if (!strcmp(type, "？")) {
		bnst_data[i].para_key_type = PARA_KEY_A;
	    }
	    para_data[Para_num].type = bnst_data[i].para_key_type;

	    para_data[Para_num].threshold = threshold;

	    string2feature_pattern(&(para_data[Para_num].f_pattern),condition);
	    
	    Para_num ++;
	    if (Para_num >= PARA_MAX) {
		fprintf(stderr, "Too many para (%s)!\n", Comment);
		return CONTINUE;
	    }
	}
	else {
	    bnst_data[i].para_num = -1;
	}
    }

    if (Para_num == 0) return 0;

    for (i = 0; i < Bnst_num; i++) {

	if ((cp = (char *)check_feature(bnst_data[i].f, "区切")) != NULL) {
	    if (check_feature(bnst_data[i].f, "読点")) {
		sscanf(cp, "%*[^:]:%*d-%d", &(bnst_data[i].sp_level));
	    } else {
		sscanf(cp, "%*[^:]:%d-%*d", &(bnst_data[i].sp_level));
	    }
	} else {
	    bnst_data[i].sp_level = 0;
	}
    }

    return Para_num;
}

/*====================================================================
                               END
====================================================================*/
