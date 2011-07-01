/*====================================================================

       並列構造内部の依存構造のチェック，依存可能性行列のマスク

                                               S.Kurohashi 93. 5.25
                                               S.Kurohashi 93. 5.31

    $Id$

====================================================================*/
#include "knp.h"

#define PARA_INCOMP_TH	3.0

int D_check_array[BNST_MAX];
int D_found_array[BNST_MAX];

/*==================================================================*/
	  int check_stop_extend(SENTENCE_DATA *sp, int num)
/*==================================================================*/
{
    if ((check_feature(sp->bnst_data[num].f, "読点") &&
	 !check_feature(sp->bnst_data[num].f, "係:同格連体")) ||
	check_feature(sp->bnst_data[num].f, "提題") ||
	(check_feature(sp->bnst_data[num].f, "係:デ格") &&
	 check_feature(sp->bnst_data[num].f, "ハ")))
	return TRUE;
    else
	return FALSE;
}

/*==================================================================*/
      int para_extend_p(SENTENCE_DATA *sp, PARA_MANAGER *m_ptr)
/*==================================================================*/
{
    /* 並列構造前部を延長する基準 : 強並列 or 用言を含む */

    int i;

    if (sp->para_data[m_ptr->para_data_num[0]].status == 's') 
	return TRUE;

    for (i = m_ptr->start[0]; i <= m_ptr->end[0]; i++)
	if (check_feature(sp->bnst_data[i].f, "用言"))
	    return TRUE;
    return FALSE;
}

/*==================================================================*/
	       int parent_range(PARA_MANAGER *m_ptr)
/*==================================================================*/
{
    /* 親の並列構造がある場合の範囲延長の制限
       		先頭部分に含まれる場合 : 制限なし
		それ以外に含まれる場合 : 直前のキーの位置 */

    int i;

    if (m_ptr->parent == NULL) return 0;
    
    for (i = m_ptr->parent->part_num - 1; i > 0; i--) 
      if (m_ptr->parent->start[i] <= m_ptr->start[0])
	return m_ptr->parent->start[i];
    
    return 0;
}

/*==================================================================*/
    int _check_para_d_struct(SENTENCE_DATA *sp, int str, int end,
			     int extend_p, int limit, int *s_p)
/*==================================================================*/
{
    int i, j, k, found, success_p = TRUE;
    int hikousa_array[BNST_MAX];

    D_found_array[end] = TRUE; /* Iマークを埋める時の便宜 */

    for (k = 0; k <= end; k++) hikousa_array[k] = 1;
	/* 延長も調べるので,この初期化はstrからでなく0から */

    /* 並列構造内部の依存構造を調べる
       (各文節がもっとも近い文節にかかると仮定) */

    for (i = end - 1; i >= str; i--) {
	if (D_check_array[i] == TRUE) {
	    D_found_array[i] = TRUE;
	} else {
	    found = FALSE;
	    for (j = i + 1; j <= end; j++)
		if (Mask_matrix[i][j] &&
		    Quote_matrix[i][j] &&
		    Dpnd_matrix[i][j] &&
		    hikousa_array[j]) {
		    D_found_array[i] = TRUE;
		    for (k = i + 1; k < j; k++) hikousa_array[k] = 0;
		    found = TRUE;
		    break;
		}
	    if (found == FALSE) {
		D_found_array[i] = FALSE;
		/* revise_para_kakariからの呼出(s_p == NULL)は表示なし */
		if (OptDisplay == OPT_DEBUG && s_p) {
		    fprintf(stderr, ";; Cannot find a head for bunsetsu <");
		    print_bnst(sp->bnst_data + i, NULL);
		    fprintf(stderr, ">.\n");
		}
		success_p = FALSE;
		for (k = i + 1; k <= end; k++) hikousa_array[k] = 0;
	    }
	}
    } 

    /* 並列構造前部の延長可能範囲を調べる */
    
    if (extend_p == TRUE && success_p == TRUE) {
	for (i = str - 1;; i--) {
	    if (i < limit || check_stop_extend(sp, i) == TRUE) {
		*s_p = i + 1;
		break;
	    } else if (D_check_array[i] == TRUE) {
		D_found_array[i] = TRUE;
	    } else {
		found = FALSE;
		for (j = i + 1; j <= end; j++)

		    /* 
		       '< end' か '<= end' かで, 並列末尾が延長する文節の
		       係り先となり得るかどうかが変わる．

		       実験の結果,'<= end'とする方が全体の精度はよい．

		       具体例) 950101071-030, 950101169-002, 950101074-019
		    */

		    if (Mask_matrix[i][j] && 
			Quote_matrix[i][j] && 
			Dpnd_matrix[i][j] && 
			hikousa_array[j]) {
			D_found_array[i] = TRUE;
			for (k = i + 1; k < j; k++) hikousa_array[k] = 0;
			found = TRUE;
			break;	/* 96/01/22までなかなった?? */
		    }
		if (found == FALSE) {
		    *s_p = i + 1;
		    break;
		}
	    }
	}
    }

    return success_p;
}

/*==================================================================*/
int check_error_state(SENTENCE_DATA *sp, PARA_MANAGER *m_ptr, int error[])
/*==================================================================*/
{
    /* エラー状態のチェック : 

           修正可能な場合 : 2つのだけの部分からなる並列構造
			    3つ以上の部分からなる並列構造で誤りが先頭
			    3つ以上の部分からなる並列構造で誤りが末尾

	   それ以外の場合は修正断念 (return -1) */

    int i;

    if (m_ptr->part_num == 2) {
	return m_ptr->para_data_num[0];
    } 
    else if (error[0] == TRUE) {
	for (i = 1; i < m_ptr->part_num; i++)
	    if (error[i] == TRUE) {
		if (OptDisplay == OPT_DEBUG) {
		    fprintf(stderr, ";; Cannot revise invalid kakari struct in para!!\n");
		}
		return -1;
	    }
	return m_ptr->para_data_num[0];
    } 
    else if (error[m_ptr->part_num - 1] == TRUE) {
	for (i = 0; i < m_ptr->part_num - 1; i++)
	    if (error[i] == TRUE) {
		if (OptDisplay == OPT_DEBUG) {
		    fprintf(stderr, ";; Cannot revise invalid kakari struct in para!!\n");
		}
		return -1;
	    }
	return m_ptr->para_data_num[m_ptr->para_num-1];
    }
    else
	return -1;
}

/*==================================================================*/
int check_para_d_struct(SENTENCE_DATA *sp, PARA_MANAGER *m_ptr)
/*==================================================================*/
{
    int i, j, k;
    int noun_flag;
    int start_pos;
    int invalid_flag = FALSE;
    int no_more_error, no_more_error_here;
    int error_check[PARA_PART_MAX], error_pos[PARA_PART_MAX];
    int revised_p_num;
    int head_pos;

    for (i = 0; i < m_ptr->child_num; i++) { 	/* 子供の再帰処理 */
	if (check_para_d_struct(sp, m_ptr->child[i]) == FALSE) {
	    if (Language == CHINESE) {
		if (i == m_ptr->child_num - 1) {
		    return FALSE;
		}
	    }
	    else {
		return FALSE;
	    }
	}
    }

    if (Language == CHINESE) {
	/* if the first word in a noun coordination is verb, then change it to be verb coordination */
	if (sp->bnst_data[m_ptr->end[0]].para_key_type == PARA_KEY_N) {
	    for (k = 0; k < m_ptr->para_num; k++) {
		if (!check_feature((sp->bnst_data+m_ptr->start[k])->f, "VV") &&
		    !check_feature((sp->bnst_data+m_ptr->start[k])->f, "VA") &&
		    !check_feature((sp->bnst_data+m_ptr->start[k])->f, "VC") &&
		    !check_feature((sp->bnst_data+m_ptr->start[k])->f, "VE")) {
		    sp->bnst_data[m_ptr->end[0]].para_key_type = PARA_KEY_N;
		    break;
		}
		else {
		    sp->bnst_data[m_ptr->end[0]].para_key_type = PARA_KEY_P;
		}
	    }
	}

	/* if the last word in a noun coordination is not noun, then reduce the scope */
	if (sp->bnst_data[m_ptr->end[0]].para_key_type == PARA_KEY_N) {
	    for (k = m_ptr->end[m_ptr->part_num - 1]; k >= m_ptr->start[m_ptr->part_num - 1]; k--) {
		if (check_feature((sp->bnst_data+k)->f, "NN") || 
		    check_feature((sp->bnst_data+k)->f, "NR") || 
		    check_feature((sp->bnst_data+k)->f, "NT") || 
		    check_feature((sp->bnst_data+k)->f, "M") || 
		    check_feature((sp->bnst_data+k)->f, "PN")) {
		    m_ptr->end[m_ptr->part_num - 1] = k;
		    break;
		}
	    }
	}

	/* if the first word in a noun coordination is verb or DEC, then reduce the scope */
	if (sp->bnst_data[m_ptr->end[0]].para_key_type == PARA_KEY_N) {
	    for (k = m_ptr->start[0]; k <= m_ptr->end[0]; k++) {
		if (!check_feature((sp->bnst_data+k)->f, "VV") && 
		    !check_feature((sp->bnst_data+k)->f, "VC") && 
		    !check_feature((sp->bnst_data+k)->f, "VE") && 
		    !check_feature((sp->bnst_data+k)->f, "VA") && 
		    !check_feature((sp->bnst_data+k)->f, "P") && 
		    !check_feature((sp->bnst_data+k)->f, "DEC")) {
		    m_ptr->start[0] = k;
		    break;
		}
	    }
	}

	/* if there is a verb in noun coordination, and there is no DEC, then reduce the coordination scope */
	if (sp->bnst_data[m_ptr->end[0]].para_key_type == PARA_KEY_N) {
	    for (k = 0; k < m_ptr->part_num; k++) {
		if (exist_chi(sp, m_ptr->start[k], m_ptr->end[k], "verb") != -1 && exist_chi(sp, m_ptr->start[k], m_ptr->end[k], "DEC") == -1) {
		    if (k == 0) {
			m_ptr->start[k] = exist_chi(sp, m_ptr->start[k], m_ptr->end[k], "verb") + 1;
		    }
		}
	    }
	}

	/* if the first word in a noun coordination is DEG, then enlarge the scope */
	if (sp->bnst_data[m_ptr->end[0]].para_key_type == PARA_KEY_N) {
	    if (check_feature((sp->bnst_data + m_ptr->start[0])->f, "DEG")) {
		if (Chi_np_start_matrix[m_ptr->start[0] - 1][m_ptr->start[0] - 1] != -1) {
		    m_ptr->start[0] = Chi_np_start_matrix[m_ptr->start[0] - 1][m_ptr->start[0] - 1];
		}
		else if (check_feature((sp->bnst_data + m_ptr->start[0] - 1)->f, "NN") || 
			 check_feature((sp->bnst_data + m_ptr->start[0] - 1)->f, "NR") ||
			 check_feature((sp->bnst_data + m_ptr->start[0] - 1)->f, "PN")) {
		    m_ptr->start[0]--;
		}
		else {
		    m_ptr->start[0]++;
		}
	    }
	}

	for (k = 0; k < m_ptr->part_num; k++) {
	    /* enlarge the coordination scope if there is a NR before the first NN of this coordination */
	    if (check_feature((sp->bnst_data + m_ptr->start[0])->f, "NN") && m_ptr->start[0] != 0 && check_feature((sp->bnst_data + m_ptr->start[0] - 1)->f, "NR")) {
		m_ptr->start[0]--;
	    }

	    // check if the scope of coordination conflict with the scope of baseNP
	    if (((Chi_np_start_matrix[m_ptr->start[k]][m_ptr->start[k]] != -1 && Chi_np_start_matrix[m_ptr->start[k]][m_ptr->start[k]] < m_ptr->start[k]) && 
		 (Chi_np_end_matrix[m_ptr->start[k]][m_ptr->start[k]] != -1 && Chi_np_end_matrix[m_ptr->start[k]][m_ptr->start[k]] >= m_ptr->start[k] && Chi_np_end_matrix[m_ptr->start[k]][m_ptr->start[k]] <= m_ptr->end[k]))) {
		/* enlarge the coordination scope if it is the first conjunctive structure */
		if (k == 0) {
		    m_ptr->start[k] = Chi_np_start_matrix[m_ptr->start[k]][m_ptr->start[k]];
		}
	    }
	    else if ((Chi_np_start_matrix[m_ptr->end[k]][m_ptr->end[k]] != -1 && Chi_np_start_matrix[m_ptr->end[k]][m_ptr->end[k]] >= m_ptr->start[k]) &&
		     (Chi_np_end_matrix[m_ptr->end[k]][m_ptr->end[k]] != -1 && Chi_np_end_matrix[m_ptr->end[k]][m_ptr->end[k]] > m_ptr->end[k])) {
		/* enlarge the coordination scope if it is the last conjunctive structure */
		if (k == m_ptr->part_num - 1) {
		    m_ptr->end[k] = Chi_np_end_matrix[m_ptr->end[k]][m_ptr->end[k]];
		}
	    }

	    // check if the scope of coordination conflict with the scope of quote
	    if (((Chi_quote_start_matrix[m_ptr->start[k]][m_ptr->start[k]] != -1 && Chi_quote_start_matrix[m_ptr->start[k]][m_ptr->start[k]] < m_ptr->start[k]) && 
		 (Chi_quote_end_matrix[m_ptr->start[k]][m_ptr->start[k]] != -1 && Chi_quote_end_matrix[m_ptr->start[k]][m_ptr->start[k]] >= m_ptr->start[k] && Chi_quote_end_matrix[m_ptr->start[k]][m_ptr->start[k]] <= m_ptr->end[k]))) {
		/* enlarge the coordination scope if it is the first conjunctive structure */
		if (k == 0) {
		    m_ptr->start[k] = Chi_quote_start_matrix[m_ptr->start[k]][m_ptr->start[k]];
		}
	    }
	    else if ((Chi_quote_start_matrix[m_ptr->end[k]][m_ptr->end[k]] != -1 && Chi_quote_start_matrix[m_ptr->end[k]][m_ptr->end[k]] >= m_ptr->start[k]) &&
		     (Chi_quote_end_matrix[m_ptr->end[k]][m_ptr->end[k]] != -1 && Chi_quote_end_matrix[m_ptr->end[k]][m_ptr->end[k]] > m_ptr->end[k])) {
		/* enlarge the coordination scope if it is the last conjunctive structure */
		if (k == m_ptr->part_num - 1) {
		    m_ptr->end[k] = Chi_quote_end_matrix[m_ptr->end[k]][m_ptr->end[k]];
		}
	    }
	}
	if (sp->bnst_data[m_ptr->end[0]].para_key_type == PARA_KEY_P) {
	    /* change the scope of verb coordination for Chinese, i.e. change [V][CC V] to be [V CC][V] */
	    for (k = 0; k < m_ptr->part_num - 1; k++) {
		(sp->bnst_data + m_ptr->start[k + 1])->para_num = (sp->bnst_data + m_ptr->end[k])->para_num;
		(sp->bnst_data + m_ptr->end[k])->para_num = -1;
		m_ptr->end[k] = m_ptr->start[k + 1];
		m_ptr->start[k + 1]++;
	    }
	}
    }

    
	/* 体言文節の働きが曖昧なものが，並列構造解析で明確になる場合の処理
       ----------------------------------------------------------------
       例) 「風間さんは雑誌の編集者、恵美子さんは車メーカーのコンパニオン。」
       		「編集者」が判定詞省略であることがわかる

       例) 「発電能力で約四十倍、電力量では約六十倍も増やし、…」
           「今年は四月に統一地方選挙、七月に参院選挙があります。」
       		「約四十倍」,「統一地方選挙」がサ変省略でないことがわかる
       
       → 「a1 a2 a3, b1 b2 b3」において，すべて体言の場合，(a1 a3),(a2 a3)
       の係り受けを(a1 b3),(a2 b3)の係り受けで上書きする．
       ただし，隣にだけ係るような係り受けの場合に副作用があるので，(a2 a3)
       の上書きは，(a2 a3)と(a2 b3)が異なる場合のみとする．

       例) 「映画は二月クランクイン、十月公開の予定。」


       ※ この方法では前のconjunctの係り受けを修正することしかできず，
       次の例は正しく扱えない．

       例) 「前回は７戦だったが、今回は九番勝負で先に５勝した…」

       本来的には,新たに正しいfeatureを与えて係り受けの解析をやり直す
       必要がある．その場合には，
	・片方が判定詞 → もう一方も判定詞
       	・片方がサ変 → サ変を取り消す
       という処理を行えばよいだろう．そうすれば上の例も正しく解析される
       ようになる．
    */

    if (m_ptr->status == 's') {
	noun_flag = 1; 
	for (k = 0; k < m_ptr->part_num; k++)
	    if (!check_feature(sp->bnst_data[m_ptr->end[k]].f, "体言"))
		noun_flag = 0;
	if (noun_flag) {
	    for (k = 0; k < m_ptr->part_num - 1; k++)
	        for (i = m_ptr->start[k]; i < m_ptr->end[k]; i ++)
		    if (!(i == m_ptr->end[k] - 1 &&
			  Dpnd_matrix[i][m_ptr->end[k]] ==     
			  Dpnd_matrix[m_ptr->end[m_ptr->part_num - 1] - 1][m_ptr->end[m_ptr->part_num - 1]]))
		      Dpnd_matrix[i][m_ptr->end[k]] =     
			Dpnd_matrix[i][m_ptr->end[m_ptr->part_num - 1]];
	}
    }

    /* 依存構造解析可能性のチェック */

    start_pos = m_ptr->start[0];
    for (k = 0; k < m_ptr->part_num; k++) {
	if (_check_para_d_struct(sp, m_ptr->start[k], m_ptr->end[k],
				 (k == 0) ? para_extend_p(sp, m_ptr): FALSE, 
				 (k == 0) ? parent_range(m_ptr): 0,
				 &start_pos) == FALSE) {
	    invalid_flag = TRUE;
	    error_check[k] = TRUE;
	}
	else {
	    error_check[k] = FALSE; /* 初期化 */
	}
    }
    
    /* 依存構造解析に失敗した場合

       「彼は東京で八百屋を,彼女は大阪で主婦を,私は京都で学生をしている」
       のように述語の省略された含む並列構造を検出する．

       各部分の係り先のない文節の数が同じで,強並列であれば
       上記のタイプの並列構造と考える．
       (係り先のない文節のタイプ(ガ格など)は,厳密に対応するとは限らない
       ので制限しない)

       アルゴリズム : 
       先頭部分の各係り先のない文節について
	       	各部分に係先のないそれと同じタイプの文節があるかどうか調べる
       */

    if (invalid_flag == TRUE) {

	/* 要検討 */
        /* if (sp->para_data[m_ptr->para_data_num[0]].pure_score < PARA_INCOMP_TH) { */

	if (m_ptr->status != 's') {
	    if ((revised_p_num = check_error_state(sp, m_ptr, error_check)) 
		!= -1) {
		revise_para_kakari(sp, revised_p_num, D_found_array);
		return FALSE;
	    } else {
		goto cannnot_revise;
	    }
	}

	for (k = 0; k < m_ptr->part_num; k++)
	  error_pos[k] = m_ptr->end[k];
	while (1) {
	    no_more_error = FALSE;
	    no_more_error_here = FALSE;

	    for (i = error_pos[0] - 1; 
		 D_found_array[i] == TRUE && start_pos <= i; i--);
	    error_pos[0] = i;
	    if (i == start_pos - 1) no_more_error = TRUE;

	    for (k = 1; k < m_ptr->part_num; k++) {
		for (i = error_pos[k] - 1; 
		     D_found_array[i] == TRUE && m_ptr->start[k] <= i; i--);
		error_pos[k] = i;
		if (i == m_ptr->start[k] - 1) no_more_error_here = TRUE;

		/* エラーの対応がつかない(部分並列でない) */
		if (no_more_error != no_more_error_here) {
		    if ((revised_p_num = check_error_state(sp, m_ptr, error_check))
			!= -1) {
			revise_para_kakari(sp, revised_p_num, D_found_array);
			return FALSE;
		    } else {
			goto cannnot_revise;
		    }
		}
	    }
	    if (no_more_error == TRUE) break;
	    else continue;
	}	
    }

 cannnot_revise:
    /* チェック済みの印 */
    for (k = start_pos; k < m_ptr->end[m_ptr->part_num-1]; k++)
	D_check_array[k] = TRUE;

    if (Language != CHINESE) { 
	/* 先頭のconjunctのマスク */
	k = 0;
	for (i = 0; i < start_pos; i++) 	       /* < start_pos */
	    for (j = m_ptr->start[k]; j <= m_ptr->end[k]; j++)
		Mask_matrix[i][j] = 0;

	/* ★★ 実験 endの上のカバーしない
	   for (i = start_pos; i < m_ptr->start[k]; i++)       end の上
	   Mask_matrix[i][m_ptr->end[k]] = 0;
	*/
	for (i = m_ptr->start[k]; i <= m_ptr->end[k]; i++)
	    for (j = m_ptr->end[k] + 1; j < sp->Bnst_num; j++)
		Mask_matrix[i][j] = 0;

	if (sp->para_data[m_ptr->para_data_num[0]].status == 's') /* 強並列 ??? */
	    for (i = 0; i < m_ptr->start[0]; i++)
		Mask_matrix[i][m_ptr->end[0]] = 0;
    
	/* 内部のconjunctのマスク */
	for (k = 1; k < m_ptr->part_num - 1; k++) {
	    for (i = 0; i < m_ptr->start[k]; i++)
		for (j = m_ptr->start[k]; j <= m_ptr->end[k]; j++)
		    Mask_matrix[i][j] = 0;

	    for (i = m_ptr->start[k]; i <= m_ptr->end[k]; i++)
		for (j = m_ptr->end[k] + 1; j < sp->Bnst_num; j++)
		    Mask_matrix[i][j] = 0;
	}
    
	/* 末尾のconjunctのマスク */
	k = m_ptr->part_num - 1;
	for (i = 0; i < m_ptr->start[k]; i++)
	    for (j = m_ptr->start[k]; j < m_ptr->end[k]; j++) /* < end */
		Mask_matrix[i][j] = 0;

	for (i = m_ptr->start[k]; i < m_ptr->end[k]; i++)   /* < end */
	    for (j = m_ptr->end[k] + 1; j < sp->Bnst_num; j++)
		Mask_matrix[i][j] = 0;


	/* 並列の係り先 */
	for (k = 0; k < m_ptr->part_num - 1; k++) {
	    Mask_matrix[m_ptr->end[k]][m_ptr->end[k+1]] = 2;
	    /*
	      Mask_matrix[m_ptr->end[k]][m_ptr->end[m_ptr->part_num - 1]] = 2;
	    */
	}

	if (invalid_flag == TRUE)
	    for (k = 0; k < m_ptr->part_num; k++)
		for (i = m_ptr->start[k]; i <= m_ptr->end[k]; i++)
		    if (D_found_array[i] == FALSE) {
			Mask_matrix[i][m_ptr->end[k]] = 3;
			Mask_matrix[i][m_ptr->end[m_ptr->part_num - 1]] = 3;
		    }
    }

    /* 部分並列の場合,Mask_matrixは最初のheadと最後のheadを3にしておく．
       最初のheadはdpnd.headをつくるとき，最後のheadはtreeを作る時に使う */

    if (Language == CHINESE) {
	/* if the first word in a noun coordination is P, and the following word is not noun, then remove this coordination */
	if (sp->bnst_data[m_ptr->end[0]].para_key_type == PARA_KEY_N) {
	    if (check_feature((sp->bnst_data+m_ptr->start[0])->f, "P") &&
		!check_feature((sp->bnst_data+m_ptr->start[0]+1)->f, "NN") &&
		!check_feature((sp->bnst_data+m_ptr->start[0]+1)->f, "NR") &&
		!check_feature((sp->bnst_data+m_ptr->start[0]+1)->f, "NT") &&
		!check_feature((sp->bnst_data+m_ptr->start[0]+1)->f, "PN")) {
		return FALSE;
	    }
	}

	for (k = 0; k < m_ptr->part_num; k++) {
	    // check if the scope of coordination conflict with the scope of baseNP
	    if (((Chi_np_start_matrix[m_ptr->start[k]][m_ptr->start[k]] != -1 && Chi_np_start_matrix[m_ptr->start[k]][m_ptr->start[k]] < m_ptr->start[k]) &&
		 (Chi_np_end_matrix[m_ptr->start[k]][m_ptr->start[k]] != -1 && Chi_np_end_matrix[m_ptr->start[k]][m_ptr->start[k]] >= m_ptr->start[k] && Chi_np_end_matrix[m_ptr->start[k]][m_ptr->start[k]] <= m_ptr->end[k]))) {
		return FALSE;
	    }
	    else if ((Chi_np_start_matrix[m_ptr->end[k]][m_ptr->end[k]] != -1 && Chi_np_start_matrix[m_ptr->end[k]][m_ptr->end[k]] >= m_ptr->start[k]) &&
		 (Chi_np_end_matrix[m_ptr->end[k]][m_ptr->end[k]] != -1 && Chi_np_end_matrix[m_ptr->end[k]][m_ptr->end[k]] > m_ptr->end[k])) {
		return FALSE;
	    }

	    // check if the scope of coordination conflict with the scope of quote
	    if (((Chi_quote_start_matrix[m_ptr->start[k]][m_ptr->start[k]] != -1 && Chi_quote_start_matrix[m_ptr->start[k]][m_ptr->start[k]] < m_ptr->start[k]) &&
		 (Chi_quote_end_matrix[m_ptr->start[k]][m_ptr->start[k]] != -1 && Chi_quote_end_matrix[m_ptr->start[k]][m_ptr->start[k]] >= m_ptr->start[k] && Chi_quote_end_matrix[m_ptr->start[k]][m_ptr->start[k]] <= m_ptr->end[k]))) {
		return FALSE;
	    }
	    else if ((Chi_quote_start_matrix[m_ptr->end[k]][m_ptr->end[k]] != -1 && Chi_quote_start_matrix[m_ptr->end[k]][m_ptr->end[k]] >= m_ptr->start[k]) &&
		 (Chi_quote_end_matrix[m_ptr->end[k]][m_ptr->end[k]] != -1 && Chi_quote_end_matrix[m_ptr->end[k]][m_ptr->end[k]] > m_ptr->end[k])) {
		return FALSE;
	    }
	}

	if (sp->bnst_data[m_ptr->end[0] - 1].para_key_type == PARA_KEY_P) {
	    if (OptParaFix) {
		// for verb coordination, mask the outside area for the non-first conjunction
		for (i = 0; i < m_ptr->start[0]; i++) {
		    for (j = m_ptr->start[1] + 1; j <= m_ptr->end[m_ptr->part_num - 1]; j++) {
			Mask_matrix[i][j] = 0;
		    }
		}
		for (i = m_ptr->start[1] + 1; i <= m_ptr->end[m_ptr->part_num - 1]; i++) {
		    for (j = m_ptr->end[m_ptr->part_num - 1] + 1; j < sp->Bnst_num; j++) {
			Mask_matrix[i][j] = 0;
		    }
		}
	    }

	    for (k = 0; k < m_ptr->part_num; k++) {
		Mask_matrix[m_ptr->start[k]][m_ptr->end[k]] = 'V'; // verb coordination
		if (k < m_ptr->part_num - 1) {
		    for (i = m_ptr->start[k] + 1; i < m_ptr->end[m_ptr->part_num - 2]; i++) {
			Mask_matrix[i][m_ptr->end[k]] = 0;
		    }
		}
	    }
	}
	else if (sp->bnst_data[m_ptr->end[0]].para_key_type == PARA_KEY_N) {
	    if (OptParaFix) {
		// for noun coordination, mask the outside area for the non-last conjunction
		for (i = 0; i < m_ptr->start[0]; i++) {
		    for (j = m_ptr->start[0]; j < m_ptr->start[m_ptr->part_num - 1]; j++) {
			Mask_matrix[i][j] = 0;
		    }
		}
		for (i = m_ptr->start[0]; i < m_ptr->start[m_ptr->part_num - 1]; i++) {
		    for (j = m_ptr->end[m_ptr->part_num - 1] + 1; j < sp->Bnst_num; j++) {
			Mask_matrix[i][j] = 0;
		    }
		}
	    }

	    for (k = 0; k < m_ptr->part_num; k++) {
		Mask_matrix[m_ptr->start[k]][m_ptr->end[k]] = 'N'; // noun coordination
		if (k > 0) {
		    for (i = m_ptr->start[k] + 1; i < m_ptr->end[m_ptr->part_num - 1]; i++) {
			Mask_matrix[m_ptr->start[k]][i] = 0;
		    }
		}
	    }
	}
    }

    return TRUE;
}

/*==================================================================*/
	       void init_mask_matrix(SENTENCE_DATA *sp)
/*==================================================================*/
{
    int i, j;

    for (i = 0; i < sp->Bnst_num; i++)
	for (j = 0; j < sp->Bnst_num; j++)
	    Mask_matrix[i][j] = 1;
}

/*==================================================================*/
	       void init_para_matrix(SENTENCE_DATA *sp)
/*==================================================================*/
{
    int i, j, k;

    for (k = 0; k < sp->Para_num; k++) {
	for (i = 0; i < sp->Bnst_num; i++) {
	    for (j = 0; j < sp->Bnst_num; j++) {
		Para_matrix[k][i][j] = -1;
	    }
	}
    }
}

/*==================================================================*/
	      int check_dpnd_in_para(SENTENCE_DATA *sp)
/*==================================================================*/
{
    int i;

    /* 初期化 */

    init_mask_matrix(sp);
    for (i = 0; i < sp->Bnst_num; i++)
	D_check_array[i] = FALSE;

    /* 並列構造内の係受けチェック，マスク */
    
    for (i = 0; i < sp->Para_M_num; i++) {
	if (sp->para_manager[i].parent == NULL) {
	    if (check_para_d_struct(sp, &sp->para_manager[i]) == FALSE) {
		if (Language == CHINESE) {
		    if (i == sp->Para_M_num - 1) {
			return FALSE;
		    }
		}
		else {
		    return FALSE;
		}
	    }
	}
    }

    return TRUE;
}

/*====================================================================
                               END
====================================================================*/
