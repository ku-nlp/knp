/*====================================================================

				 CKY

    $Id$
====================================================================*/

#include "knp.h"

typedef struct _CKY *CKYptr;
typedef struct _CKY {
    char	cp;
    int		score;
    int		direction; /* direction of dependency */
    BNST_DATA	*b_ptr;
    int 	scase_check[SCASE_CODE_SIZE];
    int		un_count;
    CKYptr	left;	/* pointer to the left child */
    CKYptr	right;	/* pointer to the right child */
    CKYptr	next;	/* pointer to the next CKY data at this point */
} CKY;

#define	CKY_TABLE_MAX	50000
CKY *cky_matrix[BNST_MAX][BNST_MAX];/* CKY行列の各位置の最初のCKYデータへのポインタ */
CKY cky_table[CKY_TABLE_MAX];	  /* CKYデータの配列 */


int convert_to_dpnd(TOTAL_MGR *Best_mgr, CKY *cky_ptr, int space, int flag) {
    /* flag == 1 : 右の子も書く
       flag == 0 : 右の子はもう書かない */
    int i;
    char *cp;

    if (OptDisplay == OPT_DEBUG) {
	if (flag == 1) {
	    for (i = 0; i < space; i++) 
		printf(" ");
	    printf("%c\n", cky_ptr->cp);
	}
    }

    if (cky_ptr->right) {
	if (Mask_matrix[cky_ptr->left->b_ptr->num][cky_ptr->b_ptr->num] == 2) {
	    Best_mgr->dpnd.head[cky_ptr->left->b_ptr->num] = cky_ptr->b_ptr->num;
	    Best_mgr->dpnd.type[cky_ptr->left->b_ptr->num] = 'P';
	}
	else if (Mask_matrix[cky_ptr->left->b_ptr->num][cky_ptr->b_ptr->num] == 3) {
	    Best_mgr->dpnd.head[cky_ptr->left->b_ptr->num] = cky_ptr->b_ptr->num;
	    Best_mgr->dpnd.type[cky_ptr->left->b_ptr->num] = 'I';
	}
	else {
	    if ((cp = check_feature(cky_ptr->left->b_ptr->f, "係:無格従属")) != NULL) {
		sscanf(cp, "%*[^:]:%*[^:]:%d", &(Best_mgr->dpnd.head[cky_ptr->left->b_ptr->num]));
		Best_mgr->dpnd.type[cky_ptr->left->b_ptr->num] = 
		    Dpnd_matrix[cky_ptr->left->b_ptr->num][cky_ptr->b_ptr->num];
	    }
	    else {
		if (cky_ptr->direction == RtoL) { /* <- */
		    Best_mgr->dpnd.head[cky_ptr->b_ptr->num] = cky_ptr->left->b_ptr->num;
		    Best_mgr->dpnd.type[cky_ptr->b_ptr->num] = 
			Dpnd_matrix[cky_ptr->left->b_ptr->num][cky_ptr->b_ptr->num];
		}
		else { /* -> */
		    Best_mgr->dpnd.head[cky_ptr->left->b_ptr->num] = cky_ptr->b_ptr->num;
		    Best_mgr->dpnd.type[cky_ptr->left->b_ptr->num] = 
			Dpnd_matrix[cky_ptr->left->b_ptr->num][cky_ptr->b_ptr->num];
		}
	    }
	}

	convert_to_dpnd(Best_mgr, cky_ptr->right, space + 2, 0);
	convert_to_dpnd(Best_mgr, cky_ptr->left, space + 2, 1);
    }
}

int check_scase (BNST_DATA *g_ptr, int *scase_check, int rentai, int un_count) {
    /* 空いているガ格,ヲ格,ニ格,ガ２ */
    int vacant_slot_num = 0;

    if ((g_ptr->SCASE_code[case2num("ガ格")]
	 - scase_check[case2num("ガ格")]) == 1) {
	vacant_slot_num++;
    }
    if ((g_ptr->SCASE_code[case2num("ヲ格")]
	 - scase_check[case2num("ヲ格")]) == 1) {
	vacant_slot_num++;
    }
    if ((g_ptr->SCASE_code[case2num("ニ格")]
	 - scase_check[case2num("ニ格")]) == 1 &&
	rentai == 1 &&
	check_feature(g_ptr->f, "用言:動")) {
	vacant_slot_num++;
	/* ニ格は動詞で連体修飾の場合だけ考慮，つまり連体
	   修飾に割り当てるだけで，未格のスロットとはしない */
    }
    if ((g_ptr->SCASE_code[case2num("ガ２")]
	 - scase_check[case2num("ガ２")]) == 1) {
	vacant_slot_num++;
    }

    /* 空きスロット分だけ連体修飾，未格にスコアを与える */
    if ((rentai + un_count) <= vacant_slot_num) {
	return (rentai + un_count) * 10;
    }
    else {
	return vacant_slot_num * 10;
    }
}

int check_dpnd_possibility (int dep, int gov, int relax_flag) {
    if ((Dpnd_matrix[dep][gov] && 
	 Quote_matrix[dep][gov] && 
	 Mask_matrix[dep][gov] == 1) || /* 並列ではない場合、並列マスクを無視する場合を考慮するなら、この条件をはずす */
	Mask_matrix[dep][gov] == 2 || /* 並列P */
	Mask_matrix[dep][gov] == 3) { /* 並列I */
	return TRUE;
    }
    else if (relax_flag && Language != CHINESE) { /* relax */
	if (!Dpnd_matrix[dep][gov]) {
	    Dpnd_matrix[dep][gov] = 'R';
	}
	return TRUE;
    }
    return FALSE;
}

int calc_score(SENTENCE_DATA *sp, CKY *cky_ptr) {
    CKY *right_ptr = cky_ptr->right, *tmp_cky_ptr = cky_ptr;
    BNST_DATA *g_ptr = cky_ptr->b_ptr, *d_ptr;
    int i, k, one_score = 0, pred_p = 0, topic_score = 0;
    int ha_check = 0, *un_count;
    int rentai, vacant_slot_num, *scase_check;
    int count, pos, default_pos;
    char *cp, *cp2;

    /* 対象の用言以外のスコアを集める (rightをたどりながらleftのスコアを足す) */
    while (tmp_cky_ptr) {
	if (tmp_cky_ptr->left) {
	    one_score += tmp_cky_ptr->left->score;
	}
	tmp_cky_ptr = tmp_cky_ptr->right;
    }
    if (OptDisplay == OPT_DEBUG) {
	printf("%d=>", one_score);
    }

    if (check_feature(g_ptr->f, "用言") ||
	check_feature(g_ptr->f, "準用言")) {
	pred_p = 1;
	for (k = 0; k < SCASE_CODE_SIZE; k++) cky_ptr->scase_check[k] = 0;
	scase_check = &(cky_ptr->scase_check[0]);
	cky_ptr->un_count = 0;
	un_count = &(cky_ptr->un_count);
    }

    /* すべての子供について */
    while (cky_ptr) {
	if (cky_ptr->left) {
	    d_ptr = cky_ptr->left->b_ptr;

	    if (Mask_matrix[d_ptr->num][g_ptr->num] == 2 || /* 並列P */
		Mask_matrix[d_ptr->num][g_ptr->num] == 3) { /* 並列I */
		;
	    }
	    else {
		/* 距離コストを計算するために、まず係り先の候補を調べる */
		count = 0;
		for (i = d_ptr->num + 1; i < sp->Bnst_num; i++) {
		    if (check_dpnd_possibility(d_ptr->num, i, ((i == sp->Bnst_num - 1) && count == 0) ? TRUE : FALSE)) {
			if (i == g_ptr->num) {
			    pos = count;
			}
			count++;
		    }
		}

		default_pos = (d_ptr->dpnd_rule->preference == -1) ?
		    count : d_ptr->dpnd_rule->preference;

		/* 係り先のDEFAULTの位置との差をペナルティに
		   ※ 提題はC,B'を求めて遠くに係ることがあるが，それが
		   他の係り先に影響しないよう,ペナルティに差をつける */
		if (check_feature(d_ptr->f, "提題")) {
		    one_score -= abs(default_pos - 1 - pos);
		}
		else {
		    one_score -= abs(default_pos - 1 - pos) * 2;
		}

		/* 読点をもつものが隣にかかることを防ぐ */
		if (d_ptr->num + 1 == g_ptr->num && 
		    abs(default_pos - 1 - pos) > 0 && 
		    check_feature(d_ptr->f, "読点")) {
		    one_score -= 5;
		}
	    }
	    
	    if (pred_p && (cp = check_feature(d_ptr->f, "係")) != NULL) {
		    
		/* 未格 提題(「〜は」)の扱い */
		if (check_feature(d_ptr->f, "提題") && !strcmp(cp, "係:未格")) {

		    /* 文末, 「〜が」など, 並列末, C, B'に係ることを優先 */
		    if ((cp2 = check_feature(g_ptr->f, "提題受")) != NULL) {
			sscanf(cp2, "%*[^:]:%d", &topic_score);
			one_score += topic_score;
		    }

		    /* 一つめの提題にだけ点を与える (時間,数量は別)
		       → 複数の提題が同一述語に係ることを防ぐ */
		    if (check_feature(d_ptr->f, "時間") ||
			check_feature(d_ptr->f, "数量")) {
			one_score += 10;
		    }
		    else if (ha_check == 0){
			one_score += 10;
			ha_check = 1;
		    }
		}

		k = case2num(cp + 3);

		/* 格要素一般の扱い */

		/* 未格 : 数えておき，後で空スロットを調べる (時間,数量は別) */
		if (!strcmp(cp, "係:未格")) {
		    if (check_feature(d_ptr->f, "時間") ||
			check_feature(d_ptr->f, "数量")) {
			one_score += 10;
		    }
		    else {
			(*un_count)++;
		    }
		}

		/* ノ格 : 体言以外なら break 
		   → それより前の格要素には点を与えない．
		   → ノ格がかかればそれより前の格はかからない

		   ※ 「体言」というのは判定詞のこと，ただし
		   文末などでは用言:動となっていることも
		   あるので，「体言」でチェック */
		else if (!strcmp(cp, "係:ノ格")) {
		    if (!check_feature(g_ptr->f, "体言")) {
			/* one_score += 10;
			   break; */
			if (g_ptr->SCASE_code[case2num("ガ格")] &&
			    scase_check[case2num("ガ格")] == 0) {
			    one_score += 10;
			    scase_check[case2num("ガ格")] = 1;
			}
		    }
		} 

		/* ガ格 : ガガ構文があるので少し複雑 */
		else if (!strcmp(cp, "係:ガ格")) {
		    if (g_ptr->SCASE_code[case2num("ガ格")] &&
			scase_check[case2num("ガ格")] == 0) {
			one_score += 10;
			scase_check[case2num("ガ格")] = 1;
		    }
		    else if (g_ptr->SCASE_code[case2num("ガ２")] &&
			     scase_check[case2num("ガ２")] == 0) {
			one_score += 10;
			scase_check[case2num("ガ格")] = 1;
		    }
		}

		/* 他の格 : 各格1つは点数をあたえる
		   ※ ニ格の場合，時間とそれ以外は区別する方がいいかも？ */
		else if (k != -1) {
		    if (scase_check[k] == 0) {
			scase_check[k] = 1;
			one_score += 10;
		    } 
		}

		/* 「〜するのは〜だ」にボーナス 01/01/11
		   ほとんどの場合改善．

		   改善例)
		   「抗議したのも 任官を 拒否される 理由の 一つらしい」

		   「使うのは 恐ろしい ことだ。」
		   「円満決着に なるかどうかは 微妙な ところだ。」
		   ※ これらの例は「こと/ところだ」に係ると扱う

		   「他人に 教えるのが 好きになる やり方です」
		   ※ この例は曖昧だが，文脈上正しい

		   副作用例)
		   「だれが ＭＶＰか 分からない 試合でしょう」
		   「〜 殴るなど した 疑い。」
		   「ビザを 取るのも 大変な 時代。」
		   「波が 高まるのは 避けられそうにない 雲行きだ。」
		   「あまり 役立つとは 思われない 論理だ。」
		   「どう 折り合うかが 問題視されてきた 法だ。」
		   「認められるかどうかが 争われた 裁判で」

		   ※問題※
		   「あの戦争」が〜 のような場合も用言とみなされるのが問題
		*/

		if (check_feature(d_ptr->f, "用言") &&
		    (check_feature(d_ptr->f, "係:未格") ||
		     check_feature(d_ptr->f, "係:ガ格")) &&
		    check_feature(g_ptr->f, "用言:判")) {
		    one_score += 3;
		}
	    }

	    /* 連体修飾の場合，係先が
	       ・形式名詞,副詞的名詞
	       ・「予定」,「見込み」など
	       でなければ一つの格要素と考える */
	    if (check_feature(d_ptr->f, "係:連格")) {
		if (check_feature(g_ptr->f, "外の関係") || 
		    check_feature(g_ptr->f, "ルール外の関係")) {
		    one_score += 10;	/* 外の関係ならここで加点 */
		}
		else {
		    /* それ以外なら空きスロットをチェック (連体修飾を付けたときのスコアの差分) */
		    one_score += check_scase(d_ptr, &(cky_ptr->left->scase_check[0]), 1, cky_ptr->left->un_count) - 
			check_scase(d_ptr, &(cky_ptr->left->scase_check[0]), 0, cky_ptr->left->un_count);
		}
	    }
	}

	cky_ptr = cky_ptr->right;
    }

    /* 用言の場合，最終的に未格,ガ格,ヲ格,ニ格,連体修飾に対して
       ガ格,ヲ格,ニ格のスロット分だけ点数を与える */
    if (pred_p) {
	one_score += check_scase(g_ptr, scase_check, 0, *un_count);
    }

    return one_score;
}

int cky (SENTENCE_DATA *sp, TOTAL_MGR *Best_mgr) {
    int i, j, k, l, sen_len, cky_table_num, best_score, dep_check[BNST_MAX];
    CKY *cky_ptr, *left_ptr, *right_ptr, *best_ptr, *pre_ptr, *best_pre_ptr, *start_ptr;
    CKY **next_pp, **next_pp_for_ij;

    cky_table_num = 0;

    /* initialize */
    for (i = 0; i < sp->Bnst_num; i++) {
	dep_check[i] = -1;
	Best_mgr->dpnd.head[i] = -1;
	Best_mgr->dpnd.type[i] = 'D';
    }

    /* ループは左から右,下から上
       iからjまでの要素をまとめる処理 */
    for (j = 0; j < sp->Bnst_num; j++) { /* left to right (左から右) */
	for (i = j; i >= 0; i--) { /* bottom to top (下から上) */
	    if (OptDisplay == OPT_DEBUG) {
		printf("(%d,%d)\n", i, j);
	    }
	    cky_matrix[i][j] = NULL;
	    if (i == j) {
		cky_ptr = &(cky_table[cky_table_num]);
		cky_table_num++;
		if (cky_table_num >= CKY_TABLE_MAX) {
		    fprintf(stderr, ";;; cky_table_num exceeded maximum\n");
		    exit(1);
		}
		cky_matrix[i][j] = cky_ptr;
		cky_ptr->left = NULL;
		cky_ptr->right = NULL;
		cky_ptr->next = NULL;
		cky_ptr->cp = 'a' + j;
		cky_ptr->score = 0;
		cky_ptr->b_ptr = sp->bnst_data + j;
		for (k = 0; k < SCASE_CODE_SIZE; k++) cky_ptr->scase_check[k] = 0;
		cky_ptr->un_count = 0;
	    } 
	    else {
		next_pp_for_ij = NULL;	/* その位置に一つも句ができてない印 */

		/* iからi+kまでと,i+k+1からjまでをまとめる */
		for (k = 0; k < j - i; k++) {
		    /* 条件が合う場合に句を作る */
		    if ((dep_check[i + k] <= 0 || /* バリアがないとき */
			 dep_check[i + k] >= j) && /* バリアがあるときはそこまで */
			check_dpnd_possibility(i + k, j, (j == sp->Bnst_num - 1) && dep_check[i + k] == -1 ? TRUE : FALSE)) {
			if (OptDisplay == OPT_DEBUG) {
			    printf("   (%d,%d), (%d,%d) [%s->%s], score=", i, i + k, i + k + 1, j, 
				   (sp->bnst_data + i + k)->head_ptr->Goi, 
				   (sp->bnst_data + j)->head_ptr->Goi);
			}
			left_ptr = cky_matrix[i][i + k];
			next_pp = NULL;
			while (left_ptr) {
			    right_ptr = cky_matrix[i + k + 1][j];
			    while (right_ptr) {
				cky_ptr = &(cky_table[cky_table_num]);
				cky_table_num++;
				if (cky_table_num >= CKY_TABLE_MAX) {
				    fprintf(stderr, ";;; cky_table_num exceeded maximum\n");
				    exit(1);
				}
				if (next_pp == NULL) {
				    start_ptr = cky_ptr;
				}
				else {
				    *next_pp = cky_ptr;
				}
				cky_ptr->next = NULL;
				cky_ptr->left = left_ptr;
				cky_ptr->right = right_ptr;
				cky_ptr->direction = Dpnd_matrix[i + k][j] == 'L' ? RtoL : LtoR;
				cky_ptr->cp = 'a' + j;
				cky_ptr->b_ptr = cky_ptr->right->b_ptr;
				next_pp = &(cky_ptr->next);
				cky_ptr->un_count = 0;
				for (l = 0; l < SCASE_CODE_SIZE; l++) cky_ptr->scase_check[l] = 0;
				cky_ptr->score = calc_score(sp, cky_ptr);
				if (OptDisplay == OPT_DEBUG) {
				    printf("%d,", cky_ptr->score);
				}

				right_ptr = right_ptr->next;
			    }
			    left_ptr = left_ptr->next;
			}

			if (next_pp) {
			    /* choose the best one */
			    cky_ptr = start_ptr;
			    best_score = -INT_MAX;
			    while (cky_ptr) {
				if (cky_ptr->score > best_score) {
				    best_score = cky_ptr->score;
				    best_ptr = cky_ptr;
				}
				cky_ptr = cky_ptr->next;
			    }
			    start_ptr = best_ptr;
			    start_ptr->next = NULL;

			    if (next_pp_for_ij == NULL) {
				cky_matrix[i][j] = start_ptr;
			    }
			    else {
				*next_pp_for_ij = start_ptr;
			    }
			    next_pp_for_ij = &(start_ptr->next);

			    /* バリアのチェック */
			    if (j != sp->Bnst_num - 1) { /* relax時はチェックしない */
				if ((sp->bnst_data + i + k)->dpnd_rule->barrier.fp[0] && 
				    feature_pattern_match(&((sp->bnst_data + i + k)->dpnd_rule->barrier), 
							  (sp->bnst_data + j)->f, 
							  sp->bnst_data + i + k, sp->bnst_data + j) == TRUE) {
				    dep_check[i + k] = j; /* バリアの位置をセット */
				}
				else if (dep_check[i + k] == -1) {
				    dep_check[i + k] = 0; /* 係り受けがすくなくとも1つは成立したことを示す */
				}
			    }
			}

			if (OptDisplay == OPT_DEBUG) {
			    printf("\n");
			}
		    }
		}
	    }
	}
    }

    if (OptDisplay == OPT_DEBUG) {
	printf(">>> n=%d\n", cky_table_num);
    }

    /* choose the best one */
    cky_ptr = cky_matrix[0][sp->Bnst_num - 1];
    if (cky_ptr->next) { /* if there are more than one possibility */
	best_score = -INT_MAX;
	pre_ptr = NULL;
	while (cky_ptr) {
	    if (cky_ptr->score > best_score) {
		best_score = cky_ptr->score;
		best_ptr = cky_ptr;
		best_pre_ptr = pre_ptr;
	    }
	    pre_ptr = cky_ptr;
	    cky_ptr = cky_ptr->next;
	}
	if (pre_ptr != best_ptr) {
	    if (best_pre_ptr) {
		best_pre_ptr->next = best_ptr->next;
	    }
	    else {
		cky_matrix[0][sp->Bnst_num - 1] = cky_matrix[0][sp->Bnst_num - 1]->next;
	    }
	    pre_ptr->next = best_ptr; /* move the best one to the end of the list */
	    best_ptr->next = NULL;
	}

	/* cky_ptr = cky_matrix[0][sp->Bnst_num - 1]; * when print all possible structures */
	cky_ptr = best_ptr;
    }

    while (cky_ptr) {
	if (OptDisplay == OPT_DEBUG) {
	    printf("---------------------\n");
	    printf("score=%d\n", cky_ptr->score);
	}

	Best_mgr->dpnd.head[cky_ptr->b_ptr->num] = -1;
	convert_to_dpnd(Best_mgr, cky_ptr, 0, 1);
	cky_ptr = cky_ptr->next;
    }

    /* 無格従属: 前の文節の係り受けに従う場合 */
    for (i = 0; i < sp->Bnst_num - 1; i++) {
	if (Best_mgr->dpnd.head[i] < 0) {
	    /* ありえない係り受け */
	    if (i >= Best_mgr->dpnd.head[i + Best_mgr->dpnd.head[i]]) {
		continue;
	    }
	    Best_mgr->dpnd.head[i] = Best_mgr->dpnd.head[i + Best_mgr->dpnd.head[i]];
	    /* Best_mgr->dpnd.check[i].pos[0] = Best_mgr->dpnd.head[i]; */
	}
    }
}
