/*====================================================================

       並列構造内部の依存構造のチェック，依存可能性行列のマスク

                                               S.Kurohashi 93. 5.25
                                               S.Kurohashi 93. 5.31

    $Id$
====================================================================*/
#include "knp.h"

int D_check_array[BNST_MAX];
int D_found_array[BNST_MAX];

extern FILE  *Infp;
extern FILE  *Outfp;

/*==================================================================*/
		    int check_stop_extend(int num)
/*==================================================================*/
{
    if (check_feature(bnst_data[num].f, "読点") ||
	check_feature(bnst_data[num].f, "提題") ||
	(check_feature(bnst_data[num].f, "係:デ格") &&
	 check_feature(bnst_data[num].f, "ハ")))
      return TRUE;
    else
      return FALSE;
}

/*==================================================================*/
	       int para_extend_p(PARA_MANAGER *m_ptr)
/*==================================================================*/
{
    /* 並列構造前部を延長する基準 : 強並列 or 用言を含む */

    int i;

    if (para_data[m_ptr->para_data_num[0]].status == 's') 
      return TRUE;

    for (i = m_ptr->start[0]; i <= m_ptr->end[0]; i++)
      if (check_feature(bnst_data[i].f, "用言:強"))
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
   int _check_para_d_struct(int str, int end, 
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
		    fprintf(Outfp, ";; Cannot find a head for bunsetsu <");
		    print_bnst(bnst_data + i, NULL);
		    fprintf(Outfp, ">.\n");
		}
		success_p = FALSE;
		for (k = i + 1; k <= end; k++) hikousa_array[k] = 0;
	    }
	}
    } 

    /* 並列構造前部の延長可能範囲を調べる */
    
    if (extend_p == TRUE && success_p == TRUE) {
	for (i = str - 1;; i--) {
	    if (i < limit || check_stop_extend(i) == TRUE) {
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
       int check_error_state(PARA_MANAGER *m_ptr, int error[])
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
	    fprintf(Outfp, 
		    ";; Cannot revise invalid kakari struct in para!!\n");
	    return -1;
	  }
	return m_ptr->para_data_num[0];
    } 
    else if (error[m_ptr->part_num - 1] == TRUE) {
	for (i = 0; i < m_ptr->part_num - 1; i++)
	  if (error[i] == TRUE) {
	    fprintf(Outfp, 
		    ";; Cannot revise invalid kakari struct in para!!\n");
	    return -1;
	  }
	return m_ptr->para_data_num[m_ptr->para_num-1];
    }	    
    else
      return -1;
}

/*==================================================================*/
	     int check_para_d_struct(PARA_MANAGER *m_ptr)
/*==================================================================*/
{
    int i, j, k;
    int noun_flag;
    int start_pos;
    int invalid_flag = FALSE;
    int no_more_error, no_more_error_here;
    int error_type = -1, error_type_here = -1;
    int error_check[PARA_PART_MAX], error_pos[PARA_PART_MAX];
    int revised_p_num;
    BNST_DATA *k_ptr, *u_ptr;
    
    for (i = 0; i < m_ptr->child_num; i++)	/* 子供の再帰処理 */
      if (check_para_d_struct(m_ptr->child[i]) == FALSE)
	return FALSE;
    
    /* 体言文節の働きが曖昧なものが，並列構造解析で明確になる場合の処理
       ----------------------------------------------------------------
       例) 「風間さんは雑誌の編集者、恵美子さんは車メーカーのコンパニオン。」
       		「編集者」が判定詞省略であることがわかる

       例) 「発電能力で約四十倍、電力量では約六十倍も増やし、…」
           「今年は四月に統一地方選挙、七月に参院選挙があります。」
       		「約四十倍」,「統一地方選挙」がサ変省略でないことがわかる
       
       → 最後のconjunctのheadとの係り受けを他のconjunctのheadとの係り受け
       にコピーする．

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
	    if (!check_feature(bnst_data[m_ptr->end[k]].f, "体言"))
		noun_flag = 0;
	if (noun_flag) {
	    for (k = 0; k < m_ptr->part_num - 1; k++)
		for (i = m_ptr->start[k]; i < m_ptr->end[k]; i ++) 
		    Dpnd_matrix[i][m_ptr->end[k]] =     
			Dpnd_matrix[i][m_ptr->end[m_ptr->part_num - 1]];
	}
    }

    /* 依存構造解析可能性のチェック */

    start_pos = m_ptr->start[0];
    for (k = 0; k < m_ptr->part_num; k++)
      if (_check_para_d_struct(m_ptr->start[k], m_ptr->end[k],
			       (k == 0) ? para_extend_p(m_ptr): FALSE, 
			       (k == 0) ? parent_range(m_ptr): 0,
			       &start_pos) == FALSE) {
	  invalid_flag = TRUE;
	  error_check[k] = TRUE;
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
	
	if (m_ptr->status != 's') {
	    if ((revised_p_num = check_error_state(m_ptr, error_check)) 
		!= -1) {
		revise_para_kakari(revised_p_num, D_found_array);
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
		    if ((revised_p_num = check_error_state(m_ptr, error_check))
			!= -1) {
			revise_para_kakari(revised_p_num, D_found_array);
			return FALSE;
		    } else {
			goto cannnot_revise;
		    }
		}
	    }
	    if (no_more_error == TRUE) break;
	    else continue;
	}	
      cannnot_revise:
    }

    /* チェック済みの印 */
    for (k = start_pos; k < m_ptr->end[m_ptr->part_num-1]; k++)
      D_check_array[k] = TRUE;

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
      for (j = m_ptr->end[k] + 1; j < Bnst_num; j++)
	Mask_matrix[i][j] = 0;

    if (para_data[m_ptr->para_data_num[0]].status == 's') /* 強並列 ??? */
      for (i = 0; i < m_ptr->start[0]; i++)
	Mask_matrix[i][m_ptr->end[0]] = 0;
    
    /* 内部のconjunctのマスク */
    for (k = 1; k < m_ptr->part_num - 1; k++) {
	for (i = 0; i < m_ptr->start[k]; i++)
	  for (j = m_ptr->start[k]; j <= m_ptr->end[k]; j++)
	    Mask_matrix[i][j] = 0;
	for (i = m_ptr->start[k]; i <= m_ptr->end[k]; i++)
	  for (j = m_ptr->end[k] + 1; j < Bnst_num; j++)
	    Mask_matrix[i][j] = 0;
    }
    
    /* 末尾のconjunctのマスク */
    k = m_ptr->part_num - 1;
    for (i = 0; i < m_ptr->start[k]; i++)
      for (j = m_ptr->start[k]; j < m_ptr->end[k]; j++) /* < end */
	Mask_matrix[i][j] = 0;
    for (i = m_ptr->start[k]; i < m_ptr->end[k]; i++)   /* < end */
      for (j = m_ptr->end[k] + 1; j < Bnst_num; j++)
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

    /* 部分並列の場合,Mask_matrixは最初のheadと最後のheadを3にしておく．
       最初のheadはdpnd.headをつくるとき，最後のheadはtreeを作る時に使う */

    return TRUE;
}

/*==================================================================*/
		       void init_mask_matrix()
/*==================================================================*/
{
    int i, j;

    for (i = 0; i < Bnst_num; i++)
      for (j = 0; j < Bnst_num; j++)
	Mask_matrix[i][j] = 1;
}

/*==================================================================*/
			 int check_dpnd_in_para()
/*==================================================================*/
{
    int i;

    /* 初期化 */

    init_mask_matrix();
    for (i = 0; i < Bnst_num; i++)
      D_check_array[i] = FALSE;

    /* 並列構造内の係受けチェック，マスク */
    
    for (i = 0; i < Para_M_num; i++)
      if (para_manager[i].parent == NULL)
	if (check_para_d_struct(&para_manager[i]) == FALSE)
	  return FALSE;

    return TRUE;
}

/*====================================================================
                               END
====================================================================*/
